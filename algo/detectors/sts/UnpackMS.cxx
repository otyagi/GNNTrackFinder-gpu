/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Volker Friese [committer] */

#include "UnpackMS.h"

#include "AlgoFairloggerCompat.h"
#include "StsRecoUtils.h"
#include "StsXyterMessage.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

using std::unique_ptr;
using std::vector;

namespace cbm::algo::sts
{

  UnpackMS::UnpackMS(const UnpackPar& pars) : fParams(pars) {}
  UnpackMS::~UnpackMS() = default;

  // ----   Algorithm execution   ---------------------------------------------
  UnpackMS::Result_t UnpackMS::operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                                          const uint64_t tTimeslice) const
  {
    // --- Output data
    Result_t result = {};

    // --- Current Timeslice start time in epoch units. Note that it is always a multiple of epochs
    // --- and the epoch is a multiple of ns.
    TimeSpec time;
    const uint64_t epochLengthInNs = fkEpochLength * fkClockCycleNom / fkClockCycleDen;
    time.currentTsTime             = tTimeslice / epochLengthInNs;

    // --- Current TS_MSB epoch cycle
    auto const msTime = msDescr.idx;  // Unix time of MS in ns
    time.currentCycle = std::ldiv(msTime, fkCycleLength).quot;

    // --- Number of messages in microslice
    auto msSize = msDescr.size;
    if (msSize % sizeof(stsxyter::Message) != 0) {
      L_(error) << "Invalid microslice size: " << msSize;
      std::get<1>(result).fNumErrInvalidMsSize++;
      return result;
    }
    const uint32_t numMessages = msSize / sizeof(stsxyter::Message);
    if (numMessages < 2) {
      L_(error) << "Microslice too small: " << numMessages;
      std::get<1>(result).fNumErrInvalidMsSize++;
      return result;
    }

    const u32 maxDigis = numMessages - 2;  // -2 for the TS_MSB and EPOCH messages
    std::get<0>(result).reserve(maxDigis);

    // --- Interpret MS content as sequence of SMX messages
    auto message = reinterpret_cast<const stsxyter::Message*>(msContent);

    // --- The first message in the MS is expected to be of type EPOCH and can be ignored.
    if (message[0].GetMessType() != stsxyter::MessType::Epoch) {
      L_(error) << "First message in microslice is not of type EPOCH";
      std::get<1>(result).fNumErrInvalidFirstMessage++;
      return result;
    }

    // --- The second message must be of type ts_msb.
    if (message[1].GetMessType() != stsxyter::MessType::TsMsb) {
      L_(error) << "Second message in microslice is not of type TS_MSB";
      std::get<1>(result).fNumErrInvalidFirstMessage++;
      return result;
    }
    ProcessTsmsbMessage(message[1], time);

    // --- Message loop
    for (uint32_t messageNr = 2; messageNr < numMessages; messageNr++) {

      // --- Action depending on message type
      switch (message[messageNr].GetMessType()) {

        case stsxyter::MessType::Hit: {
          ProcessHitMessage(message[messageNr], time, std::get<0>(result), std::get<1>(result), std::get<2>(result));
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
                                          vector<CbmStsDigi>& digiVec, UnpackMonitorData& monitor,
                                          UnpackAuxData& aux) const
  {

    // --- Check eLink and get parameters
    uint16_t elink = message.GetLinkIndexHitBinning();
    if (elink >= fParams.fElinkParams.size()) {
      monitor.fNumErrElinkOutOfRange++;
      return;
    }
    const UnpackElinkPar& elinkPar = fParams.fElinkParams.at(elink);
    uint32_t asicNr                = elinkPar.fAsicNr;

    // --- Check minimum adc cut
    if (message.GetHitAdc() <= elinkPar.fAdcMinCut) {
      return;
    }

    // --- Check for masked channel
    const uint16_t msg_channel = message.GetHitChannel();
    if (!elinkPar.fChanMask.empty() && elinkPar.fChanMask[msg_channel] == true) {
      return;
    }

    // --- Hardware-to-software address
    const auto maybe_channel = Module::ChannelInModule(msg_channel, asicNr);
    if (!maybe_channel.has_value()) return;

    // --- Expand time stamp to time within timeslice (in clock cycle)
    uint64_t messageTime = message.GetHitTimeBinning() + time.currentEpochTime;

    // --- Convert time stamp from clock cycles to ns. Round to nearest full ns.
    messageTime = (messageTime * fkClockCycleNom + fkClockCycleDen / 2) / fkClockCycleDen;

    // --- Correct ASIC-wise offsets
    messageTime -= elinkPar.fTimeOffset;

    // --- Apply walk correction if applicable
    if (message.GetHitAdc() <= elinkPar.fWalk.size()) {
      double walk = elinkPar.fWalk[message.GetHitAdc() - 1];
      messageTime += walk;
    }

    // --- Charge
    double charge = elinkPar.fAdcOffset + (message.GetHitAdc() - 1) * elinkPar.fAdcGain;

    if (messageTime > CbmStsDigi::kMaxTimestamp) {
      monitor.fNumErrTimestampOverflow++;
      return;
    }

    // --- Create output digi
    digiVec.emplace_back(elinkPar.fAddress, *maybe_channel, messageTime, charge);

    if (fParams.fWriteAux) {
      aux.fQaDigis.emplace_back(message.IsHitMissedEvts(), elinkPar.fAddress, *maybe_channel, messageTime, charge,
                                elink);
    }
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


}  // namespace cbm::algo::sts
