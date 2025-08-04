/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Volker Friese [committer] */

#include "UnpackMS.h"

#include "StsXyterMessage.h"

#include <cassert>
#include <cmath>
#include <utility>
#include <vector>

using std::unique_ptr;
using std::vector;

namespace cbm::algo::much
{

  UnpackMS::UnpackMS(const UnpackPar& pars) : fParams(pars) {}
  UnpackMS::~UnpackMS() = default;

  // ----   Algorithm execution   ---------------------------------------------
  UnpackMS::Result_t UnpackMS::operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                                          const uint64_t tTimeslice) const
  {

    // --- Output data
    Result_t result = {};

    TimeSpec time;

    // --- Current Timeslice start time in epoch units. Note that it is always a multiple of epochs
    // --- and the epoch is a multiple of ns.
    const uint64_t epochLengthInNs = fkEpochLength * fkClockCycleNom / fkClockCycleDen;
    time.currentTsTime             = tTimeslice / epochLengthInNs;

    // --- Current TS_MSB epoch cycle
    auto const msTime = msDescr.idx;  // Unix time of MS in ns
    time.currentCycle = std::ldiv(msTime, fkCycleLength).quot;
    time.currentEpoch = 0;  // Needed to make each MS independent of the previous! Will be updated in message 1 if MS OK
    time.currentEpochTime =
      0;  // Needed to make each MS independent of the previous! Will be updated in message 1 if MS OK

    // --- Number of messages in microslice
    auto msSize = msDescr.size;
    if (msSize % sizeof(stsxyter::Message) != 0) {
      std::get<1>(result).fNumErrInvalidMsSize++;
      return result;
    }
    const uint32_t numMessages = msSize / sizeof(stsxyter::Message);
    if (numMessages < 2) {
      std::get<1>(result).fNumErrInvalidMsSize++;
      return result;
    }

    const uint32_t maxDigis = numMessages - 2;  // -2 for the TS_MSB and EPOCH messages
    std::get<0>(result).reserve(maxDigis);

    // --- Interpret MS content as sequence of SMX messages
    auto message = reinterpret_cast<const stsxyter::Message*>(msContent);

    // --- The first message in the MS is expected to be of type EPOCH and can be ignored.
    if (message[0].GetMessType() != stsxyter::MessType::Epoch) {
      std::get<1>(result).fNumErrInvalidFirstMessage++;
      return result;
    }

    // --- The second message must be of type ts_msb.
    if (message[1].GetMessType() != stsxyter::MessType::TsMsb) {
      std::get<1>(result).fNumErrInvalidFirstMessage++;
      return result;
    }
    ProcessTsmsbMessage(message[1], time);

    // --- Message loop
    for (uint32_t messageNr = 2; messageNr < numMessages; messageNr++) {

      // --- Action depending on message type
      switch (message[messageNr].GetMessType()) {

        case stsxyter::MessType::Hit: {
          ProcessHitMessage(message[messageNr], time, std::get<0>(result), std::get<1>(result));
          break;
        }
        case stsxyter::MessType::TsMsb: {
          ProcessTsmsbMessage(message[messageNr], time);
          break;
        }
        default: {
          std::get<1>(result).fNumNonHitOrTsbMessage++;
          break;
        }

      }  //? Message type

    }  //# Messages

    return result;
  }
  // --------------------------------------------------------------------------


  // -----   Process hit message   --------------------------------------------
  inline void UnpackMS::ProcessHitMessage(const stsxyter::Message& message, const TimeSpec& time,
                                          vector<CbmMuchDigi>& digiVec, UnpackMonitorData& monitor) const
  {
    // --- Check eLink and get parameters
    uint16_t elink = message.GetLinkIndexHitBinning();
    if (elink >= fParams.fElinkParams.size()) {
      monitor.fNumErrElinkOutOfRange++;
      return;
    }
    const UnpackElinkPar& elinkPar = fParams.fElinkParams.at(elink);
    uint16_t channel               = message.GetHitChannel();

    // --- Check for masked channel
    if (!elinkPar.fChanMask.empty() && elinkPar.fChanMask[channel] == true) {
      return;
    }

    uint32_t address = (elinkPar.fAddress)[channel];

    // --- Expand time stamp to time within timeslice (in clock cycle)
    uint64_t messageTime = message.GetHitTimeBinning() + time.currentEpochTime;

    // --- Convert time stamp from clock cycles to ns. Round to nearest full ns.
    messageTime = (messageTime * fkClockCycleNom + fkClockCycleDen / 2) / fkClockCycleDen;

    // --- Correct ASIC-wise offsets
    messageTime -= elinkPar.fTimeOffset;

    // --- Charge
    double charge = message.GetHitAdc();

    // --- Create output digi
    digiVec.emplace_back(address, charge, messageTime);
  }
  // --------------------------------------------------------------------------


  // -----   Process an epoch (TS_MSB) message   ------------------------------
  inline void UnpackMS::ProcessTsmsbMessage(const stsxyter::Message& message, TimeSpec& time) const
  {
    // The compression of time is based on the hierarchy epoch cycle - epoch - message time.
    // Cycles are counted from the start of Unix time and are multiples of an epoch (ts_msb).
    // The epoch number is counted within each cycle. The time in the hit message is expressed
    // in units of the readout clock cycle relative to the current epoch.
    // The ts_msb message indicates the start of a new epoch. Its basic information is the epoch
    // number within the current cycle. A cycle wrap resets the epoch number to zero, so it is
    // indicated by the epoch number being smaller than the previous one (epoch messages are
    // seemingly not consecutively in the data stream, but only if there are hit messages in between).
    auto epoch = message.GetTsMsbValBinning();

    // --- Cycle wrap
    if (epoch < time.currentEpoch) time.currentCycle++;

    // --- Update current epoch counter
    time.currentEpoch = epoch;

    // --- Calculate epoch time in clocks cycles relative to timeslice start time
    time.currentEpochTime = (time.currentCycle * fkEpochsPerCycle + epoch - time.currentTsTime) * fkEpochLength;
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::much
