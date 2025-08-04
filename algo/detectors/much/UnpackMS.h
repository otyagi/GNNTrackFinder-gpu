/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Volker Friese [committer] */
#pragma once

#include "CbmMuchDigi.h"
#include "MicrosliceDescriptor.hpp"
#include "StsXyterMessage.h"
#include "Timeslice.hpp"
#include "UnpackMSBase.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

namespace cbm::algo::much
{


  /** @struct UnpackElinkPar
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 25 November 2021
   ** @brief STS Unpacking parameters for one eLink / ASIC
   **/
  struct UnpackElinkPar {
    std::vector<uint32_t> fAddress;  ///< CbmMuchAddress for different channels
    std::vector<bool> fChanMask;     ///< Channel masking flags
    uint64_t fTimeOffset = 0.;       ///< Time calibration parameter
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
    uint32_t fNumErrInvalidFirstMessage = 0;  ///< First message is not TS_MSB or second is not EPOCH
    uint32_t fNumErrInvalidMsSize       = 0;  ///< Microslice size is not multiple of message size
    uint32_t fNumErrTimestampOverflow   = 0;  ///< Overflow in 64 bit time stamp
    bool HasErrors()
    {
      uint32_t numErrors = fNumNonHitOrTsbMessage + fNumErrElinkOutOfRange + fNumErrInvalidFirstMessage
                           + fNumErrInvalidMsSize + fNumErrTimestampOverflow;
      return (numErrors > 0 ? true : false);
    }
    std::string print()
    {
      std::stringstream ss;
      ss << "errors " << fNumNonHitOrTsbMessage << " | " << fNumErrElinkOutOfRange << " | "
         << fNumErrInvalidFirstMessage << " | " << fNumErrInvalidMsSize << " | " << fNumErrTimestampOverflow << " | ";
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
  class UnpackMS : public UnpackMSBase<CbmMuchDigi, UnpackMonitorData, UnpackAuxData> {

   public:
    /** @brief Construct with parameters **/
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

   private:  // datatypes
    struct TimeSpec {
      uint64_t currentTsTime    = 0;  ///< Unix time of timeslice in units of epoch length
      uint64_t currentCycle     = 0;  ///< Current epoch cycle
      uint32_t currentEpoch     = 0;  ///< Current epoch number within epoch cycle
      uint64_t currentEpochTime = 0;  ///< Current epoch time relative to timeslice in clock cycles
    };


   private:  // methods
    /** @brief Process a hit message
     ** @param message SMX message (32-bit word)
     ** @param digiVec Vector to append the created digi to
     ** @param monitor Reference to monitor object
     **/
    void ProcessHitMessage(const stsxyter::Message& message, const TimeSpec& time, std::vector<CbmMuchDigi>& digiVec,
                           UnpackMonitorData& monitor) const;

    /** @brief Process an epoch message (TS_MSB)
     ** @param message SMX message (32-bit word)
     **/
    void ProcessTsmsbMessage(const stsxyter::Message& message, TimeSpec& time) const;


   private:                  // members
    UnpackPar fParams = {};  ///< Parameter container

    /** Number of TS_MSB epochs per cycle **/
    static constexpr uint64_t fkEpochsPerCycle = stsxyter::kuTsMsbNbTsBinsBinning;

    /** Length of TS_MSB epoch in clock cycles **/
    static constexpr uint64_t fkEpochLength = stsxyter::kuHitNbTsBinsBinning;

    /** Clock cycle nominator [ns] and denominator. The clock cycle in ns is nominator / denominator. **/
    static constexpr uint32_t fkClockCycleNom = stsxyter::kulClockCycleNom;
    static constexpr uint32_t fkClockCycleDen = stsxyter::kulClockCycleDen;

    /** Epoch cycle length in ns **/
    static constexpr uint64_t fkCycleLength = (fkEpochsPerCycle * fkEpochLength * fkClockCycleNom) / fkClockCycleDen;
  };


}  // namespace cbm::algo::much
