/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "UnpackMS.h"

#include <cassert>
#include <cmath>
#include <utility>
#include <vector>

using std::unique_ptr;
using std::vector;

namespace cbm::algo::tof
{

  UnpackMS::UnpackMS(const UnpackPar& pars) : fParams(pars) {}
  UnpackMS::~UnpackMS() = default;

  // ----   Algorithm execution   ---------------------------------------------
  UnpackMS::Result_t UnpackMS::operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                                          const uint64_t tTimeslice) const
  {

    // --- Output data
    Result_t result = {};
    TimeSpec time   = {};

    // --- Current Timeslice start time in epoch units. Note that it is always a multiple of epochs
    // --- and the epoch is a multiple of ns.
    time.currentTsTime = static_cast<uint64_t>(tTimeslice / critof001::kuEpochInNs) % critof001::kulEpochCycleEp;

    // --- Number of messages in microslice
    auto msSize = msDescr.size;
    if (msSize % sizeof(critof001::Message) != 0) {
      std::get<1>(result).fNumErrInvalidMsSize++;
      return result;
    }
    const uint32_t numMessages = msSize / sizeof(critof001::Message);
    if (numMessages < 2) {
      std::get<1>(result).fNumErrInvalidMsSize++;
      return result;
    }

    // --- Interpret MS content as sequence of Critof messages
    auto message = reinterpret_cast<const critof001::Message*>(msContent);

    const uint32_t maxDigis = numMessages - 2;  // -2 for the TS_MSB and EPOCH messages
    std::get<0>(result).reserve(maxDigis);

    // --- The first message in the MS is expected to be of type EPOCH.
    if (message[0].getMessageType() != critof001::MSG_EPOCH) {
      std::get<1>(result).fNumErrInvalidFirstMessage++;
      return result;
    }

    {  // --- Check that first epoch matches with the microslice index
      const uint64_t msStartEpoch =
        static_cast<uint64_t>(msDescr.idx / critof001::kuEpochInNs) % critof001::kulEpochCycleEp;
      if (message[0].getGdpbEpEpochNb() != msStartEpoch) {
        std::get<1>(result).fNumErrInvalidStartEpoch++;
        return result;
      }
    }

    // --- The last message in the MS is expected to be EndOfMs.
    if (!message[numMessages - 1].isEndOfMs()) {
      std::get<1>(result).fNumErrInvalidLastMessage++;
      return result;
    }
    // Check if last message is "EndOfMs"!! Maybe loop to messageNr < numMessages - 1

    // --- Message loop
    for (uint32_t messageNr = 0; messageNr < numMessages; messageNr++) {

      // --- Action depending on message type
      switch (message[messageNr].getMessageType()) {

        case critof001::MSG_HIT: {
          ProcessHitMessage(message[messageNr], std::get<0>(result), std::get<1>(result), time);
          break;
        }
        case critof001::MSG_EPOCH: {
          if (critof001::kuChipIdMergedEpoch == message[messageNr].getGet4Idx()) {
            ProcessEpochMessage(message[messageNr], time);
          }  // if this epoch message is a merged one valid for all chips
          else {
            /// FIXME: Should be checked in/forwarded to <some> monitor <task?>, here we just jump it
            std::get<1>(result).fNumErrInvalidAsicEpochs++;
            continue;
          }  // if single chip epoch message
          break;
        }
        case critof001::MSG_SLOWC: {
          std::get<1>(result).fNumNonHitOrTsbMessage++;
          break;
        }
        case critof001::MSG_SYST: {
          std::get<1>(result).fNumNonHitOrTsbMessage++;
          break;
        }
        default: {
          std::get<1>(result).fNumNonHitOrTsbMessage++;
          break;
        }
      }  //? Message type
    }    //# Messages

    return result;
  }
  // --------------------------------------------------------------------------


  // -----   Process hit message   --------------------------------------------
  inline void UnpackMS::ProcessHitMessage(const critof001::Message& message, vector<CbmTofDigi>& digiVec,
                                          UnpackMonitorData& monitor, TimeSpec& time) const
  {
    // --- Check eLink and get parameters
    const uint32_t elink = message.getGet4Idx();
    if (elink >= fParams.fElinkParams.size()) {
      monitor.fNumErrElinkOutOfRange++;
      return;
    }
    const UnpackElinkPar& elinkPar = fParams.fElinkParams.at(elink);

    const uint32_t channel    = message.getGdpbHitChanId();
    const uint32_t channelUId = (elinkPar.fChannelUId)[channel];

    double messageTime  = message.getMsgFullTimeD(time.currentEpochInTs) - elinkPar.fTimeOffset;
    const double charge = (double) message.getGdpbHit32Tot();  //cast from uint32_t

    // --- Create output digi
    digiVec.emplace_back(channelUId, messageTime, charge);
  }
  // --------------------------------------------------------------------------


  // -----   Process an epoch message   ---------------------------------------
  inline void UnpackMS::ProcessEpochMessage(const critof001::Message& message, TimeSpec& time) const
  {
    const uint64_t epoch = message.getGdpbEpEpochNb();

    // --- Calculate epoch relative to timeslice start time; correct for epoch cycles
    if (time.currentTsTime <= epoch) {
      time.currentEpochInTs = epoch - time.currentTsTime;
    }
    else {
      time.currentEpochInTs = epoch + critof001::kulEpochCycleEp - time.currentTsTime;
    }
    //Problem if MS spans multiple epoch cycles?
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::tof
