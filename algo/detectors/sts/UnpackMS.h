/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Volker Friese [committer] */
#pragma once

#include "CbmStsDigi.h"
#include "Definitions.h"
#include "MicrosliceDescriptor.hpp"
#include "StsXyterMessage.h"
#include "UnpackMSBase.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace cbm::algo::sts
{

  /** @struct UnpackStsElinkPar
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 25 November 2021
   ** @brief STS Unpacking parameters for one eLink / ASIC
   **/
  struct UnpackElinkPar {
    int32_t fAddress     = 0;     ///< CbmStsAddress for the connected module
    uint32_t fAsicNr     = 0;     ///< Number of connected ASIC within the module
    uint64_t fTimeOffset = 0.;    ///< Time calibration parameter
    double fAdcOffset    = 0.;    ///< Charge calibration parameter
    double fAdcGain      = 0.;    ///< Charge calibration parameter
    uint32_t fAdcMinCut  = 0.;    ///< Minimum Acd cut
    std::vector<double> fWalk;    ///< Walk correction coefficients
    std::vector<bool> fChanMask;  ///< Channel masking flags
  };


  /** @struct UnpackStsPar
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 25 November 2021
   ** @brief Parameters required for the STS unpacking (specific to one component)
   **/
  struct UnpackPar {
    uint32_t fNumChansPerAsic                = 0;      ///< Number of channels per ASIC
    uint32_t fNumAsicsPerModule              = 0;      ///< Number of ASICS per module
    std::vector<UnpackElinkPar> fElinkParams = {};     ///< Parameters for each eLink
    bool fWriteAux                           = false;  ///< Write auxiliary data for module
  };


  /** @struct UnpackStsMoni
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

  /** @struct QaDigi
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 05 Jun 2024
   ** @brief Extended digi which contains auxiliary QA information
   **/
  struct QaDigi {

    QaDigi(bool missed, int32_t addr, int32_t chan, uint32_t t, uint16_t c, uint16_t e)
      : missedEvent(missed)
      , address(addr)
      , channel(chan)
      , time(t)
      , charge(c)
      , elink(e)
    {
    }

    bool missedEvent;
    int32_t address;
    int32_t channel;
    uint32_t time;
    uint16_t charge;
    uint16_t elink;
  };

  /** @struct UnpackStsAux
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 24 May 2024
   ** @brief Auxiliary data for STS unpacking
   **/
  struct UnpackAuxData {
    std::vector<QaDigi> fQaDigis;
  };

  /** @class UnpackMS
   ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 25 November 2021
   ** @brief Unpack algorithm for STS
   **/
  class UnpackMS : public UnpackMSBase<CbmStsDigi, UnpackMonitorData, UnpackAuxData> {

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
    /**
     * @brief Structure to hold the current time information for the current microslice
     */
    struct TimeSpec {
      u64 currentTsTime    = 0;  ///< Unix time of timeslice in units of epoch length
      u64 currentCycle     = 0;  ///< Current epoch cycle
      u32 currentEpoch     = 0;  ///< Current epoch number within epoch cycle
      u64 currentEpochTime = 0;  ///< Current epoch time relative to timeslice in clock cycles
    };

   private:  // methods
    /** @brief Process a hit message
     ** @param message SMX message (32-bit word)
     ** @param digiVec Vector to append the created digi to
     ** @param monitor Reference to monitor object
     ** @param aux Reference to auxiliary data object
     **/
    void ProcessHitMessage(const stsxyter::Message& message, const TimeSpec& time, std::vector<CbmStsDigi>& digiVec,
                           UnpackMonitorData& monitor, UnpackAuxData& aux) const;

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

} /* namespace cbm::algo::sts */
