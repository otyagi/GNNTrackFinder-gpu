/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig, Dominik Smith [committer], David Schledt */

#pragma once

#include "CbmTrdDigi.h"
#include "CbmTrdFexMessageSpadic.h"
#include "CbmTrdRawMessageSpadic.h"
#include "MicrosliceDescriptor.hpp"
#include "Timeslice.hpp"
#include "UnpackMSBase.h"

#include <cmath>
#include <memory>
#include <sstream>

namespace cbm::algo::trd
{
  /** @struct UnpackElinkPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief TRD Unpacking parameters for one eLink
   **/
  struct UnpackElinkPar {
    std::vector<uint32_t> fChanAddress;  ///< CbmTrdAddress for different channels
    uint32_t fAddress;                   ///< Asic address
    uint64_t fTimeOffset = 0.;           ///< Time calibration parameter
  };

  /** @struct UnpackCrobPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief TRD Unpacking parameters for one CROB
   **/
  struct UnpackCrobPar {
    std::vector<UnpackElinkPar> fElinkParams = {};  ///< Parameters for each eLink
  };

  /** @struct UnpackPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief Parameters required for the TRD unpacking (specific to one component)
   **/
  struct UnpackPar {
    bool fUseBaselineAvg                   = true;  ///< Is baseline average function of Spadic activated
    float_t fMaxAdcToEnergyCal             = 1.0;   ///< max adc to energy in keV
    uint32_t fNumChansPerAsic              = 0;     ///< Number of channels per ASIC
    uint32_t fNumAsicsPerModule            = 0;     ///< Number of ASICS per module
    std::vector<UnpackCrobPar> fCrobParams = {};    ///< Parameters for each CROB
  };


  /** @struct UnpackMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief Monitoring data for TRD unpacking
   **/
  struct UnpackMonitorData {
    size_t fNumNonMajorTsMsb     = 0;  ///< Counter for the ts_msb used to reconstruct the time
    size_t fNumCreatedRawMsgs    = 0;  ///< counter of created raw messages
    size_t fNumEpochMsgs         = 0;  ///< counter of created raw messages
    size_t fNumCreatedInfoMsgs   = 0;  ///< counter of created info messages
    size_t fNumElinkMis          = 0;  ///< Number of SOM to RDA/EOM mismatches
    size_t fNumCorruptEom        = 0;  ///< Number of corrupted EOM frames
    size_t fNumWildRda           = 0;  ///< Number of rda frames outside of a SOM frame range
    size_t fNumWildEom           = 0;  ///< Number of eom frames outside of a SOM frame range
    size_t fNumUnknownWords      = 0;  ///< Number of unknown words
    size_t fNumMissingEom        = 0;  ///< Number of missing EOM frames to finish a SOM frame
    size_t fNumWildNul           = 0;  ///< Number of wild null words, should only appear at the end of a µSlice
    size_t fNumCrcValidFlags     = 0;  ///< counter for inf/error flags from the µSlices
    size_t fNumOverflowFlimFlags = 0;  ///< counter for inf/error flags from the µSlices
    size_t fNumOverflowUserFlags = 0;  ///< counter for inf/error flags from the µSlices
    size_t fNumDataErrorFlags    = 0;  ///< counter for inf/error flags from the µSlices

    bool HasErrors()
    {
      uint32_t numErrors = fNumElinkMis + fNumCorruptEom + fNumWildRda + fNumWildEom + fNumUnknownWords + fNumMissingEom
                           + fNumWildNul + fNumCrcValidFlags + fNumOverflowFlimFlags + fNumOverflowUserFlags
                           + fNumDataErrorFlags;
      return (numErrors > 0 ? true : false);
    }
    std::string print()
    {
      std::stringstream ss;
      ss << " stats " << fNumNonMajorTsMsb << " | " << fNumCreatedRawMsgs << " | " << fNumEpochMsgs << " | "
         << fNumCreatedInfoMsgs << " errors " << fNumElinkMis << " | " << fNumCorruptEom << " | " << fNumWildRda
         << " | " << fNumWildEom << " | " << fNumUnknownWords << " | " << fNumMissingEom << " | " << fNumWildNul
         << " | " << fNumCrcValidFlags << " | " << fNumOverflowFlimFlags << " | " << fNumOverflowUserFlags << " | "
         << fNumDataErrorFlags << " | ";
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

  /** @class Unpack
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 31 January 2023
   ** @brief Unpack algorithm for TRD
   **/
  template<uint8_t sys_ver>
  class UnpackMS : public UnpackMSBase<CbmTrdDigi, UnpackMonitorData, UnpackAuxData> {

   public:
    /** @brief Construct from parameters **/
    UnpackMS(const UnpackPar& pars) : fParams(pars) {}


    /** @brief Destructor **/
    ~UnpackMS() override = default;


    /** @brief Algorithm execution
     ** @param  msContent  Microslice payload
     ** @param  msDescr    Microslice descriptor
     ** @param  tTimeslice Unix start time of timeslice [ns]
     ** @return TRD digi data
     **/
    Result_t operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                        const uint64_t tTimeslice) const override;

    /** @brief Set the parameter container
     ** @param params Pointer to parameter container
     **/
    void SetParams(std::unique_ptr<UnpackPar> params) { fParams = *(std::move(params)); }

   private:  // Types
    struct MsContext {
      /** @brief Start time of the current µSlice relative to the Timeslice start time in Spadic CC. */
      size_t fMsStartTimeRelCC = 0;

      /** @brief Time of the last succesful digest hit message */
      size_t fLastFulltime = 0;

      /** @brief Counter for the ts_msb used to reconstruct the time */
      std::vector<std::uint8_t> fNrTsMsbVec = {};
    };

   private:                  // members
    UnpackPar fParams = {};  ///< Parameter container

    /**
     ** @brief Handle the output created by the explicit algorithms. E.g. write to output vectors.
     ** @param digi
     ** @param raw
     **/
    void digestOutput(std::unique_ptr<CbmTrdDigi> digi, CbmTrdRawMessageSpadic raw);

    /**
     ** @brief Digest the aditional flags stored in the 4 "cccc" bits of the EPO messages.
     ** @param frame
     ** @return Spadic::MsInfoType
     **/
    Spadic::MsInfoType digestBufInfoFlags(const std::uint32_t frame) const;

    /**
     ** @brief Digest the flags of the currently unpacked µSlice.
     ** @param flags flags stored in the µSlice descriptor
     ** @param storage of monitoring data
     **/
    void digestMsFlags(const std::uint16_t flags, UnpackMonitorData& monitor) const;

    /**
     ** @brief Digest a info message run all default information forwarding from the msg.
     ** @param frame
     **/
    void digestInfoMsg(const std::uint32_t frame) const;

    /**
     ** @brief Extract one adc sample from a given adcbuffer
     ** @param[in] adcbuffer
     ** @param[in,out] nadcbits
     ** @return std::int16_t
     **/
    std::int16_t extractSample(size_t* adcbuffer, size_t* nadcbits) const;

    /**
     ** @brief Extract the baseline average sample from a given adcbuffer.
     ** Depending on the Spadic settings sample-0 is a plain sample or the averaged
     ** baseline calculation. The latter is not a 9 bit signed integer, but a 9 bit
     ** floating point number 7 digits before the point and 2 afterwards.
     ** @param[in] adcbuffer
     ** @param[in,out] nadcbits
     ** @return std::float_t
     **/
    std::float_t extractAvgSample(size_t* adcbuffer, size_t* nadcbits) const;

    /** @brief Identify the InfoType of a 64bit InfoMessage word inside a Microslice */
    Spadic::MsInfoType getInfoType(const std::uint32_t frame) const;

    /**
     ** @brief Get the ts_msb information from the TS_MSB(kEPO) frame. We take the first of the 3
     ** The 3 redundant TS_MSB sets are already compared at the FPGA level.
     ** @param frame
     ** @param storage of monitoring data
     ** @return ts_msb value
     **/
    std::uint8_t getTsMsb(const std::uint32_t frame, UnpackMonitorData& monitor) const;

    /**
     ** @brief Create a CbmTrdRawMessageSpadic from the hit message input.
     ** @param word
     ** @param criId id of the cri that send the µSlice
     ** @param criobId id of the crob that send the µSlice (currently not used set to 0 062021 PR)
     ** @param istream
     ** @return CbmTrdRawMessageSpadic
     ** @todo Check if we can get rid of the future obsolete microslice stuff.
     **/
    CbmTrdRawMessageSpadic makeRaw(const std::uint32_t frame, std::uint16_t criId, std::uint8_t crobId,
                                   std::uint16_t elinkId, std::uint8_t istream, MsContext& ctx) const;

    /**
     ** @brief Get the Digi Trigger Type from the raw message triggertype
     ** @param tt
     ** @return CbmTrdDigi::eTriggerType
     **/
    static CbmTrdDigi::eTriggerType GetDigiTriggerType(Spadic::eTriggerType tt);

    /**
     ** @brief Create an actual digi from the raw message
     ** @param raw
     **/
    CbmTrdDigi makeDigi(CbmTrdRawMessageSpadic raw, MsContext& ctx) const;

    /**
     ** @brief Create an actual digi from the fex message
     ** @param fw
     **/
    CbmTrdDigi makeDigi(Spadic::FexWord<sys_ver> fw, size_t fMsStartTimeRel) const;

    /**
     ** @brief Get the Bin Time Shift value
     ** @param samples
     ** @return ULong64_t
     **/
    uint64_t GetBinTimeShift(const std::vector<std::int16_t>* /*samples*/) const { return 0.; };

    /**
     ** @brief Get the MaxAdc value
     ** @param samples
     ** @return Float_t
     **/
    std::float_t GetMaxAdcValue(const std::vector<std::int16_t>* samples) const;

    /**
     ** @brief Get the Baseline value
     ** The digi charge is an unsigned. Hence, we need to get the baseline to 0
     ** @param samples
     ** @return float_t
     **/
    float_t GetBaseline(const std::vector<std::int16_t>* samples) const;

    /** @brief Identify the message type of a given 32bit frame inside a Microslice */
    Spadic::MsMessageType getMessageType(const std::uint32_t frame) const;

    /** @brief Bytes per spadic frame stored in the microslices */
    static constexpr std::uint8_t fBytesPerWord = 8;

    /** For the msg format used from 2021 ongoing we have 2 parallel streams per word. *
     ** All data from eLinks 0..20 go to one stream and 21..41 to the other            */
    /** @brief Number of streams per word **/
    static constexpr std::uint8_t fStreamsPerWord = 2;

    /** @brief Number of samples not considered for max adc */
    static constexpr size_t fNrOfPresamples = 1;

    /** @brief Clock length of Spadic in ns. */
    static constexpr float_t fAsicClockCycle = 62.5;

    /** @brief length of one ts_msb in [ns] */
    static constexpr std::uint16_t fTsMsbLength = 16000;

    /** @brief length of one ts_msb in [cc] */
    static constexpr size_t fTsMsbLengthCC = fTsMsbLength / fAsicClockCycle;

    /** @brief First sample to look for the max adc */
    static constexpr size_t fPeakingBinMin = fNrOfPresamples;

    /**
     ** @brief Last sample to look for the max adc
     ** Default value is set based on the Shaping time + 5 samples safety margin.
     ** @remark the peaking time strongly depends on the input signal. Effective range of
     ** the shaping time is between 120 and 240 ns.
     **/
    static constexpr size_t fPeakingBinMax = static_cast<size_t>(120.0 / fAsicClockCycle + fNrOfPresamples + 5);
  };

}  // namespace cbm::algo::trd
