/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */
#pragma once

#include "CbmTofDigi.h"
#include "CriGet4Mess001.h"
#include "Definitions.h"
#include "MicrosliceDescriptor.hpp"
#include "Timeslice.hpp"
#include "UnpackMSBase.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

namespace cbm::algo::tof
{

  /** @struct UnpackElinkPar
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 25 November 2021
   ** @brief TOF Unpacking parameters for one eLink / ASIC
   **/
  struct UnpackElinkPar {
    std::vector<uint32_t> fChannelUId;  ///< CbmTofAddress for different channels
    int32_t fTimeOffset = 0.;           ///< Time calibration parameter
  };

  /** @struct UnpackPar
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 25 November 2021
   ** @brief Parameters required for the STS unpacking (specific to one component)
   **/
  struct UnpackPar {
    std::vector<UnpackElinkPar> fElinkParams = {};  ///< Parameters for each eLink
  };

  /** @struct UnpackMoni
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 2 December 2021
   ** @brief Monitoring data for STS unpacking
   **/
  struct UnpackMonitorData {
    uint32_t fNumNonHitOrTsbMessage     = 0;
    uint32_t fNumErrElinkOutOfRange     = 0;  ///< Elink not contained in parameters
    uint32_t fNumErrInvalidFirstMessage = 0;  ///< First message is not EPOCH
    uint32_t fNumErrInvalidLastMessage  = 0;  ///< Last message is not EndOfMs
    uint32_t fNumErrInvalidMsSize       = 0;  ///< Microslice size is not multiple of message size
    uint32_t fNumErrTimestampOverflow   = 0;  ///< Overflow in 64 bit time stamp
    uint32_t fNumErrInvalidStartEpoch   = 0;  ///< Microslice index doesn't match first epoch
    uint32_t fNumErrInvalidAsicEpochs   = 0;  ///< Epoch did not match merged one for an asic
    bool HasErrors()
    {
      uint32_t numErrors = fNumNonHitOrTsbMessage + fNumErrElinkOutOfRange + fNumErrInvalidFirstMessage
                           + fNumErrInvalidLastMessage + fNumErrInvalidMsSize + fNumErrTimestampOverflow
                           + fNumErrInvalidStartEpoch + fNumErrInvalidAsicEpochs;
      return (numErrors > 0 ? true : false);
    }
    std::string print()
    {
      std::stringstream ss;
      ss << "errors " << fNumNonHitOrTsbMessage << " | " << fNumErrElinkOutOfRange << " | "
         << fNumErrInvalidFirstMessage << " | " << fNumErrInvalidLastMessage << " | " << fNumErrInvalidMsSize << " | "
         << fNumErrTimestampOverflow << " | " << fNumErrInvalidStartEpoch << " | " << fNumErrInvalidAsicEpochs << " | ";
      return ss.str();
    }
  };

  /** @struct UnpackAux
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 24 May 2024
   ** @brief Auxiliary data for BMON unpacking
   **/
  struct UnpackAuxData {
    ///// TO BE FILLED
  };

  /** @class UnpackMS
   ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 25 November 2021
   ** @brief Unpack algorithm for STS
   **/
  class UnpackMS : public UnpackMSBase<CbmTofDigi, UnpackMonitorData, UnpackAuxData> {

   public:
    /** @brief Default constructor **/
    UnpackMS(const UnpackPar& pars);

    /** @brief Destructor **/
    ~UnpackMS() override;

    /** @brief Algorithm execution
     ** @param  msContent  Microslice payload
     ** @param  msDescr    Microslice descriptor
     ** @param  tTimeslice Unix start time of timeslice [ns]
     ** @return STS digi data
     **/
    Result_t operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                        const uint64_t tTimeslice) const override;

    /** @brief Set the parameter container
     ** @param params Pointer to parameter container
     **/
    void SetParams(std::unique_ptr<UnpackPar> params) { fParams = *(std::move(params)); }

   private:  // types
    struct TimeSpec {
      u64 currentTsTime    = 0;  ///< Unix time of timeslice in units of epoch length
      u32 currentEpochInTs = 0;  ///< Current epoch number relative to timeslice start epoch
    };

   private:  // methods
    /** @brief Process a hit message
     ** @param message SMX message (32-bit word)
     ** @param digiVec Vector to append the created digi to
     ** @param monitor Reference to monitor object
     **/
    void ProcessHitMessage(const critof001::Message& message, std::vector<CbmTofDigi>& digiVec,
                           UnpackMonitorData& monitor, TimeSpec& time) const;

    /** @brief Process an epoch message
     ** @param message SMX message (32-bit word)
     **/
    void ProcessEpochMessage(const critof001::Message& message, TimeSpec& time) const;

   private:                  // members
    UnpackPar fParams = {};  ///< Parameter container
  };

}  // namespace cbm::algo::tof
