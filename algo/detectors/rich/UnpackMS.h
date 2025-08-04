/* Copyright (C) 2021 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig, Dominik Smith [committer] */
#pragma once

#include "CbmRichDigi.h"
#include "Definitions.h"
#include "Timeslice.hpp"
#include "UnpackMSBase.h"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>

namespace cbm::algo::rich
{
  class MicrosliceReader;

  enum class TdcWordType
  {
    TimeData,
    Header,
    Epoch,
    Trailer,
    Debug,
    Error
  };


  struct TdcTimeData {
    uint32_t fCoarse       = 0;  // 11 bits
    uint32_t fIsRisingEdge = 0;  // 1 bit
    uint32_t fFine         = 0;  // 10 bits
    uint32_t fChannel      = 0;  // 7 bits
  };


  /** @struct UnpackElinkPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 26 May 2023
   ** @brief RICH Unpacking parameters for one eLink / ASIC
   **/
  struct UnpackElinkPar {
    std::vector<double> fToTshift;  ///< TOT shift for different channels
    uint64_t fTimeOffset = 0.;      ///< Time calibration parameter
  };


  /** @struct UnpackPar
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 26 May 2023
   ** @brief Parameters required for the RICH unpacking (specific to one component)
   **/
  struct UnpackPar {
    std::map<uint32_t, UnpackElinkPar> fElinkParams = {};  ///< Map TRB address to elink params
  };


  /** @struct UnpackMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 26 May 2023
   ** @brief Monitoring data for RICH unpacking
   **/
  struct UnpackMonitorData {
    uint32_t fNumDebugMessage            = 0;  ///< Received debug messages
    uint32_t fNumCtsAndUnmappedDirich    = 0;  ///< Dirich address is unknown (or is of a CTS)
    uint32_t fNumErrInvalidFirstMessage  = 0;  ///< First message is not HEADER
    uint32_t fNumErrInvalidSecondMessage = 0;  ///< Second message is not EPOCH
    uint32_t fNumErrInvalidLastMessage   = 0;  ///< Last message is not TRAILER
    uint32_t fNumErrWildHeaderMessage    = 0;  ///< TDC header in invalid position
    uint32_t fNumErrWildTrailerMessage   = 0;  ///< TDC trailer in invalid position
    uint32_t fNumErrWildEpoch            = 0;  ///< TDC epoch in invalid position
    uint32_t fNumErrWildTdcTime          = 0;  ///< TDC time (digi) in invalid position
    uint32_t fNumErrTdcErrorWord         = 0;  ///< TDC word of error type
    uint32_t fNumErrChannelOutOfBounds   = 0;  ///< TDC channel out of bounds
    uint32_t fNumErrOrphanRecovTimeData  = 0;  ///< TimeData in last position (not TRAILER) but not creating a Digi
    uint32_t fNumWarnRecoveredLastDigi   = 0;  ///< TimeData in last position (not TRAILER) led to recovered Digi
    uint32_t fNumErrInvalidHubId         = 0;  ///< "SubSubEvent" has invalid ID
    uint32_t fNumErrInvalidHubSize       = 0;  ///< Premature end of hub block
    uint32_t fNumErrExcessLastWords      = 0;  ///< More than expected trailing words
    uint32_t fNumSkippedSubsubevent      = 0;  ///< Whenever subsubevent skipped due to error (but not mask)

    bool HasErrors() const
    {
      uint32_t numErrors = fNumDebugMessage + fNumErrInvalidFirstMessage + fNumErrInvalidSecondMessage
                           + fNumErrInvalidLastMessage + fNumErrWildHeaderMessage + fNumErrWildTrailerMessage
                           + fNumErrWildEpoch + fNumErrWildTdcTime + fNumErrTdcErrorWord + fNumErrChannelOutOfBounds
                           + fNumErrOrphanRecovTimeData + fNumErrInvalidHubId + fNumErrInvalidHubSize
                           + fNumErrExcessLastWords;
      return (numErrors > 0 ? true : false);
    }
    std::string print() const
    {
      std::stringstream ss;
      ss << "errors " << fNumDebugMessage << " | " << fNumCtsAndUnmappedDirich << " | " << fNumErrInvalidFirstMessage
         << " | " << fNumErrInvalidSecondMessage << " | " << fNumErrInvalidLastMessage << " | "
         << fNumErrWildHeaderMessage << " | " << fNumErrWildTrailerMessage << " | " << fNumErrWildEpoch << " | "
         << fNumErrWildTdcTime << " | " << fNumErrTdcErrorWord << " | " << fNumErrChannelOutOfBounds << " | "
         << fNumErrOrphanRecovTimeData << " | " << fNumWarnRecoveredLastDigi << " | " << fNumErrInvalidHubId << " | "
         << fNumErrInvalidHubSize << " | " << fNumErrExcessLastWords << " | " << fNumSkippedSubsubevent;
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

  class UnpackMS : public UnpackMSBase<CbmRichDigi, UnpackMonitorData, UnpackAuxData> {

   public:
    /** @brief Construct from parameters **/
    UnpackMS(const UnpackPar& pars);


    /** @brief Destructor **/
    ~UnpackMS() override;


    /** @brief Algorithm execution
     ** @param  msContent  Microslice payload
     ** @param  msDescr    Microslice descriptor
     ** @param  tTimeslice Unix start time of timeslice [ns]
     ** @return RICH digi data
     **/
    Result_t operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                        const uint64_t tTimeslice) const override;

    /** @brief Set the parameter container
     ** @param params Pointer to parameter container
     **/
    void SetParams(std::unique_ptr<UnpackPar> params) { fParams = *(std::move(params)); }

   private:
    struct MSContext {
      u64 cbmTimeMS     = 0;  // CbmTime of MS. Used to get time offset of subtriggers to MS start
      u64 cbmTimePacket = 0;
      u64 refTime       = 0;

      // FIXME: Magic number, should be named.
      //
      // Comment from RICH expert (Martin Beyer):
      //  > This number will also be valid for the Cbm Rich, 1 Rich backplane contains max. 12 DiRICHes.
      //  > For the Cbm Rich the number of backplanes will be scaled
      //  > up. Nevertheless it is very likely that fewer DiRiches(maybe not constant)
      //  > per backplane are used in the day1 setup.
      double prevLastCh0ReTime[13];  // 12 DiRICHes chnl0 + 1 CTS chnl0
      double mbsCorr = 0.;

      uint16_t currentSubSubEvent = 0;

      std::vector<CbmRichDigi> digis;
      UnpackMonitorData monitor;
    };

   private:
    void ProcessTrbPacket(MicrosliceReader& reader, MSContext& ctx) const;

    void ProcessHubBlock(MicrosliceReader& reader, MSContext& ctx) const;

    int ProcessCtsHeader(MicrosliceReader& reader, uint32_t subSubEventSize, uint32_t subSubEventId) const;

    void ProcessSubSubEvent(MicrosliceReader& reader, int nofTimeWords, uint32_t subSubEventId, MSContext& ctx) const;

    bool ProcessTimeDataWord(uint32_t epoch, uint32_t tdcWord, uint32_t subSubEventId, std::vector<double>& raisingTime,
                             MSContext& ctx) const;

    TdcWordType GetTdcWordType(uint32_t tdcWord) const;

    TdcTimeData ProcessTimeData(uint32_t tdcWord) const;

    uint32_t ProcessEpoch(uint32_t tdcWord) const;

    uint16_t ProcessHeader(uint32_t tdcWord) const;

    uint16_t ProcessTrailer(uint32_t tdcWord) const;

    void WriteOutputDigi(int32_t fpgaID, int32_t channel, double time, double tot, MSContext& ctx) const;

    double CalculateTime(uint32_t epoch, uint32_t coarse, uint32_t fine) const;

    int32_t GetPixelUID(int32_t fpgaID, int32_t ch) const;

    bool CheckMaskedDiRICH(int32_t subSubEventId) const;

   private:
    UnpackPar fParams = {};  ///< Parameter container

    std::vector<int32_t> fMaskedDiRICHes;

    bool fbDoToTCorr = true;  //activates ToT correction from parameter file

    // uint64_t fCbmTimeMS;
    // uint64_t fCbmTimePacket;

    // double fMbsCorr = 0.;
    // double fPrevLastCh0ReTime[13];  // 12 DiRICHes chnl0 + 1 CTS chnl0
    // uint16_t fCurrentSubSubEvent = 0;

    double fToTMin = -20.;
    double fToTMax = 100.;
  };


  class MicrosliceReader {
   private:
    const uint8_t* fData = nullptr;
    size_t fSize         = 0;
    size_t fOffset       = 0;  // offset in bytes
    size_t fWordCounter  = 0;
    uint32_t fCurWord    = 0;

   public:
    void SetData(const uint8_t* data, size_t size)
    {
      fData        = data;
      fSize        = size;
      fOffset      = 0;
      fWordCounter = 0;
      fCurWord     = 0;
    }

    const uint8_t* GetData() { return fData; }
    size_t GetSize() { return fSize; }
    size_t GetOffset() { return fOffset; }
    size_t GetWordCounter() { return fWordCounter; }
    uint32_t GetCurWord() { return fCurWord; }

    std::string GetWordAsHexString(uint32_t word)
    {
      std::stringstream stream;
      stream << "0x" << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << std::hex << word;
      return stream.str();
    }

    uint32_t NextWord()
    {
      uint32_t i = ((uint32_t*) (fData + fOffset))[0];
      //swap bytes
      i = (i >> 24) | ((i << 8) & 0x00FF0000) | ((i >> 8) & 0x0000FF00) | (i << 24);
      fOffset += 4;
      fWordCounter++;
      fCurWord = i;
      return i;
    }

    bool IsNextPadding()
    {
      uint32_t nextWord = ((uint32_t*) (fData + fOffset))[0];
      if (nextWord == 0xffffffff) return true;
      return false;
    }

    bool IsLastSubSubEvent(uint32_t subSubEventSize)
    {
      uint32_t i = ((uint32_t*) (fData + fOffset + 4 * subSubEventSize))[0];
      i          = (i >> 24) | ((i << 8) & 0x00ff0000) | ((i >> 8) & 0x0000ff00) | (i << 24);
      if (i == 0x00015555) return true;
      return false;
    }
  };

}  // namespace cbm::algo::rich
