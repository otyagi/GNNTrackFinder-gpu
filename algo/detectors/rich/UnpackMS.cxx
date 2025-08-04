/* Copyright (C) 2021 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig, Dominik Smith [committer] */

#include "UnpackMS.h"

#include "AlgoFairloggerCompat.h"

#include <cstdint>

namespace cbm::algo::rich
{
  UnpackMS::UnpackMS(const UnpackPar& pars) : fParams(pars) {}
  UnpackMS::~UnpackMS() = default;

  // ----   Algorithm execution   ---------------------------------------------
  UnpackMS::Result_t UnpackMS::operator()(const uint8_t* msContent, const fles::MicrosliceDescriptor& msDescr,
                                          const uint64_t tTimeslice) const
  {
    MSContext ctx;

    // Clear CbmTime of MS. Used to get time offset of subtriggers to MS start
    ctx.cbmTimeMS = 0;
    ctx.digis.reserve(msDescr.size / sizeof(u32));

    rich::MicrosliceReader reader;
    reader.SetData(msContent, msDescr.size);

    const auto mstime = msDescr.idx;
    ctx.refTime       = mstime - tTimeslice;

    // There are a lot of MS  with 8 bytes size
    // Does one need these MS?
    if (reader.GetSize() <= 8) return {};

    while (true) {
      ProcessTrbPacket(reader, ctx);
      ProcessHubBlock(reader, ctx);

      // -4*2 for 2 last words which contain microslice index
      if (reader.GetOffset() >= reader.GetSize() - 8) break;
      // -4*3 for 0xffffffff padding and 2 last words which contain microslice index
      if (reader.IsNextPadding() && reader.GetOffset() >= reader.GetSize() - 12) break;
    }
    [[maybe_unused]] uint32_t msIndexWord1 = reader.NextWord();
    [[maybe_unused]] uint32_t msIndexWord2 = reader.NextWord();

    return std::make_tuple(std::move(ctx.digis), std::move(ctx.monitor), UnpackAuxData());
  }


  void UnpackMS::ProcessTrbPacket(rich::MicrosliceReader& reader, MSContext& ctx) const
  {
    //process CBM time
    const uint32_t word_MSB = reader.NextWord();  // CBM 63:32
    const uint32_t word_LSB = reader.NextWord();  // CBM 31: 0
    ctx.cbmTimePacket       = (uint64_t) word_MSB << 32 | word_LSB;
    if (ctx.cbmTimeMS == 0) ctx.cbmTimeMS = ctx.cbmTimePacket;

    //discard unused words
    for (auto l = 0; l < 10; ++l) {
      reader.NextWord();
    }

    //process remaining words
    for (auto l = 0; l < 14; ++l) {
      const uint32_t wordEpoch = reader.NextWord();
      const uint32_t epoch     = ProcessEpoch(wordEpoch);
      const uint32_t wordTime  = reader.NextWord();
      const TdcTimeData td     = ProcessTimeData(wordTime);
      const double fullTime    = CalculateTime(epoch, td.fCoarse, td.fFine);

      if (l == 0) ctx.prevLastCh0ReTime[12] = fullTime;
      if (l == 1) ctx.mbsCorr = fullTime - ctx.prevLastCh0ReTime[12];  // = MbsPrevTimeCh1 - MbsPrevTimeCh0
      if (l > 1) ctx.prevLastCh0ReTime[l - 2] = fullTime;
    }

    [[maybe_unused]] const uint32_t trbNum = reader.NextWord();  // TRB trigger number
    //if (isLog()) L_(debug4) << getLogHeader(reader) << "TRB Num:" << reader.GetWordAsHexString(trbNum);
  }


  void UnpackMS::ProcessHubBlock(rich::MicrosliceReader& reader, MSContext& ctx) const
  {
    uint32_t word = reader.NextWord();

    [[maybe_unused]] const uint32_t hubId = word & 0xffff;          // 16 bits
    const uint32_t hubSize                = (word >> 16) & 0xffff;  // 16 bits

    bool isLast            = false;  // if true then it is CTS sub-sub-event
    size_t totalSize       = 0;
    ctx.currentSubSubEvent = 0;

    uint32_t subSubEventId = 0, subSubEventSize = 0;

    //loop over events in hub block
    while (totalSize < hubSize) {
      word            = reader.NextWord();
      subSubEventId   = word & 0xffff;                              // 16 bits
      subSubEventSize = (word >> 16) & 0xffff;                      // 16 bits
      isLast          = reader.IsLastSubSubEvent(subSubEventSize);  // if true then it is CTS sub-sub-event
      totalSize += (1 + subSubEventSize);

      if (!isLast) {                                 // all except last are DiRICH events
        uint16_t sSubSubEvtIdMsb = (subSubEventId >> 12) & 0xF;
        if ((sSubSubEvtIdMsb != 0x7) && (sSubSubEvtIdMsb != 0x9)) {  // catch invalid ids: mRICH, mFSD, PASTA/MUST
          ctx.monitor.fNumErrInvalidHubId++;
        }
        if (totalSize == hubSize) {
          ctx.monitor.fNumErrInvalidHubSize++;
        }
        ProcessSubSubEvent(reader, subSubEventSize, subSubEventId, ctx);
        ctx.currentSubSubEvent++;
      }
    }

    //last event ist expected to be CTS
    if (totalSize != hubSize || !isLast) {
      ctx.monitor.fNumErrInvalidHubSize++;
    }
    subSubEventSize = ProcessCtsHeader(reader, subSubEventSize, subSubEventId);
    ProcessSubSubEvent(reader, subSubEventSize, subSubEventId, ctx);

    // read last words
    int lastWordsCounter = 0;
    while (true) {
      lastWordsCounter++;
      word = reader.NextWord();
      if (word == 0x600dda7a) {
        if (reader.IsNextPadding()) word = reader.NextWord();
        break;
      }
      if (lastWordsCounter >= 7) {
        ctx.monitor.fNumErrExcessLastWords++;
      }
    }
  }

  int UnpackMS::ProcessCtsHeader(rich::MicrosliceReader& reader, uint32_t subSubEventSize,
                                 uint32_t /*subSubEventId*/) const
  {
    const uint32_t word = reader.NextWord();

    [[maybe_unused]] const uint32_t ctsState = word & 0xffff;  // 16 bits

    const uint32_t nofInputs    = (word >> 16) & 0xf;   // 4 bits
    const uint32_t nofTrigCh    = (word >> 20) & 0x1f;  // 5 bits
    const uint32_t inclLastIdle = (word >> 25) & 0x1;   // 1 bit
    const uint32_t inclTrigInfo = (word >> 26) & 0x1;   // 1 bit
    const uint32_t inclTime     = (word >> 27) & 0x1;   // 1 bit
    const uint32_t ETM          = (word >> 28) & 0x3;   // 2 bits

    uint32_t ctsInfoSize = 2 * nofInputs + 2 * nofTrigCh + 2 * inclLastIdle + 3 * inclTrigInfo + inclTime;  // in words
    switch (ETM) {
      case 0: break;
      case 1: ctsInfoSize += 1; break;
      case 2: ctsInfoSize += 4; break;
      case 3: break;
    }
    for (uint32_t i = 0; i < ctsInfoSize; i++) {
      reader.NextWord();  // do nothing?
    }
    const int nofTimeWords = subSubEventSize - ctsInfoSize - 1;
    return nofTimeWords;
  }

  void UnpackMS::ProcessSubSubEvent(rich::MicrosliceReader& reader, int nofTimeWords, uint32_t subSubEventId,
                                    MSContext& ctx) const
  {
    bool wasHeader  = false;
    bool wasEpoch   = false;
    bool wasTime    = false;
    bool wasTrailer = false;

    uint32_t epoch = 0;  // store last epoch obtained in sub-sub-event

    // Store last raising edge time for each channel or -1. if no time
    // this array is used to match raising and falling edges
    std::vector<double> raisingTime(33, -1.);

    // check if DiRICH (SubSubEvId) is masked and skip to end if so
    if (CheckMaskedDiRICH(subSubEventId)) {
      for (int i = 0; i < nofTimeWords; i++) {
        reader.NextWord();
      }
      return;
    }
    // Skip SubSubEvent for CTS and unknown DiRICH addresses
    // TODO: properly handle/skip CTS subsubevents
    if (fParams.fElinkParams.end() == fParams.fElinkParams.find(subSubEventId)) {
      if (0x8000 != (subSubEventId & 0xF000)) {
        // No log for CTS subsubevents
        L_(debug) << "Unknown DiRICH ID 0x" << std::hex << subSubEventId << std::dec
                  << ": Stopping processing of this subsubevent!!!";
      }
      ctx.monitor.fNumCtsAndUnmappedDirich++;
      ctx.monitor.fNumSkippedSubsubevent++;
      for (int i = 0; i < nofTimeWords; i++) {
        reader.NextWord();
      }
      return;
    }

    // Catch Subsubevents where some of the 3 "mandatory words" is missing
    // but this lack was properly accounted/detected by the TRBnet/CTS
    // => Otherwise the final readout of the Trailer word offsets the next
    //    subsubevent
    if (nofTimeWords < 3) {
      ctx.monitor.fNumSkippedSubsubevent++;
      for (int i = 0; i < nofTimeWords; i++) {
        reader.NextWord();
      }
      return;
    }


    // First word is expected to be of type "header"
    if (GetTdcWordType(reader.NextWord()) != TdcWordType::Header) {
      L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                << ": 1st message is not a TDC Header, skipping subsubevent";
      ctx.monitor.fNumErrInvalidFirstMessage++;
      ctx.monitor.fNumSkippedSubsubevent++;
      for (int i = 1; i < nofTimeWords; i++) {
        reader.NextWord();
      }
      return;
    }
    else {
      wasHeader = true;
    }

    // Second word is expected to be of type "epoch"
    uint32_t word = reader.NextWord();
    if (GetTdcWordType(word) != TdcWordType::Epoch) {
      L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                << ": 2nd message is not an epoch, skipping subsubevent";
      ctx.monitor.fNumErrInvalidSecondMessage++;
      ctx.monitor.fNumSkippedSubsubevent++;
      // Currently skip the subsubevent
      // TODO: Check if possible to only skip to next Epoch and continue from there
      for (int i = 2; i < nofTimeWords; i++) {
        reader.NextWord();
      }
      return;
    }
    else {
      epoch    = ProcessEpoch(word);
      wasEpoch = true;
    }

    // Loop over words
    for (int i = 2; i < nofTimeWords - 1; i++) {
      word = reader.NextWord();
      switch (GetTdcWordType(word)) {
        case TdcWordType::TimeData: {
          if (!wasHeader || !wasEpoch || wasTrailer) {
            L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                      << ": illegal position of TDC Time (before header/epoch or after trailer)";
            ctx.monitor.fNumErrWildTdcTime++;
            ctx.monitor.fNumSkippedSubsubevent++;
            // Currently skip the subsubevent
            // TODO: Check if possible to only skip to next Epoch and continue from there
            for (; i < nofTimeWords - 1; i++) {
              reader.NextWord();
            }
            return;
          }
          wasTime = true;
          ProcessTimeDataWord(epoch, word, subSubEventId, raisingTime, ctx);
          break;
        }
        case TdcWordType::Epoch: {
          if (!wasHeader || wasTrailer) {
            L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                      << ": illegal position of TDC Epoch (before header or after trailer)";
            ctx.monitor.fNumErrWildEpoch++;
            ctx.monitor.fNumSkippedSubsubevent++;
            for (; i < nofTimeWords - 1; i++) {
              reader.NextWord();
            }
            return;
          }
          wasEpoch = true;
          epoch    = ProcessEpoch(word);
          break;
        }
        case TdcWordType::Header: {
          if (wasEpoch || wasTime || wasTrailer) {
            L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                      << ": illegal position of TDC Header (after time/epoch/trailer)";
            ctx.monitor.fNumErrWildHeaderMessage++;
            ctx.monitor.fNumSkippedSubsubevent++;
            for (; i < nofTimeWords - 1; i++) {
              reader.NextWord();
            }
            return;
          }
          ctx.monitor.fNumErrWildHeaderMessage++;
          break;
        }
        case TdcWordType::Trailer: {
          if (!wasEpoch || !wasTime || !wasHeader) {
            L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                      << ": illegal position of TDC Trailer (before time/epoch/header)";
            ctx.monitor.fNumErrWildTrailerMessage++;
            ctx.monitor.fNumSkippedSubsubevent++;
            for (; i < nofTimeWords - 1; i++) {
              reader.NextWord();
            }
            return;
          }
          L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec << ": TDC Trailer too early, "
                    << (nofTimeWords - 1 - i) << " word(s) before last";
          ctx.monitor.fNumErrWildTrailerMessage++;
          wasTrailer = true;
          break;
        }
        case TdcWordType::Debug: {
          // for the moment do nothing
          ctx.monitor.fNumDebugMessage++;
          break;
        }
        case TdcWordType::Error: {
          ctx.monitor.fNumErrTdcErrorWord++;
          break;
        }
      }
    }

    // Last word is expected to be of type "trailer"
    word = reader.NextWord();
    if (GetTdcWordType(word) != TdcWordType::Trailer) {
      L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec << ": Last word not a TDC trailer";
      ctx.monitor.fNumErrInvalidLastMessage++;
      if (GetTdcWordType(word) == TdcWordType::TimeData && !wasTrailer) {
        if (ProcessTimeDataWord(epoch, word, subSubEventId, raisingTime, ctx)) {
          ctx.monitor.fNumWarnRecoveredLastDigi++;
          L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                    << ": Rescuing TimeData in subsubevent with missing Trailer leading to saved Digi";
        }
        else {
          ctx.monitor.fNumErrOrphanRecovTimeData++;
          L_(debug) << "DiRICH 0x" << std::hex << subSubEventId << std::dec
                    << ": Rescuing TimeData in subsubevent with missing Trailer w/o extra digi";
        }
      }
    }
  }

  // ---- ProcessTimeDataWord ----
  bool UnpackMS::ProcessTimeDataWord(uint32_t epoch, uint32_t tdcWord, uint32_t subSubEventId,
                                     std::vector<double>& raisingTime, MSContext& ctx) const
  {
    const TdcTimeData td  = ProcessTimeData(tdcWord);
    const double fullTime = CalculateTime(epoch, td.fCoarse, td.fFine);

    bool madeDigi = false;
    if (td.fChannel != 0) {
      const double dT = fullTime - ctx.prevLastCh0ReTime[ctx.currentSubSubEvent];
      const double subtrigOffset =
        (ctx.cbmTimePacket - ctx.cbmTimeMS) * 25.0;  // offset of SubTrigger to MS start in ns
      const double fullTimeCorr = dT - ctx.mbsCorr + subtrigOffset;

      if (td.fChannel < 1 || td.fChannel >= raisingTime.size()) {
        ctx.monitor.fNumErrChannelOutOfBounds++;
        return false;
      }
      if (td.fIsRisingEdge) {
        // always store the latest raising edge. It means that in case RRFF situation only middle RF will be matched.
        raisingTime[td.fChannel] = fullTimeCorr;
      }
      else {
        if (raisingTime[td.fChannel] != -1.) {
          // Matching was found, calculate ToT, if tot is in a good range -> create digi
          const double ToT = fullTimeCorr - raisingTime[td.fChannel];
          if (ToT >= fToTMin && ToT <= fToTMax) {
            if (fullTimeCorr >= 0.0) {
              WriteOutputDigi(subSubEventId, td.fChannel, raisingTime[td.fChannel], ToT, ctx);
              madeDigi = true;
            }
          }
          // pair was created, set raising edge to -1.
          raisingTime[td.fChannel] = -1.;
        }
      }
    }
    else {
      ctx.monitor.fNumErrChannelOutOfBounds++;
    }
    return madeDigi;
  }

  double UnpackMS::CalculateTime(uint32_t epoch, uint32_t coarse, uint32_t fine) const
  {
    return ((double) epoch) * 2048. * 5. + ((double) coarse) * 5. - ((double) fine) * 0.005;
  }

  void UnpackMS::WriteOutputDigi(int32_t fpgaID, int32_t channel, double time, double tot, MSContext& ctx) const
  {
    double ToTcorr   = fbDoToTCorr ? fParams.fElinkParams.at(fpgaID).fToTshift[channel] : 0.;
    int32_t pixelUID = GetPixelUID(fpgaID, channel);
    //check ordering
    double finalTime = time + (double) ctx.refTime - fParams.fElinkParams.at(fpgaID).fTimeOffset;

    // Do not accept digis, where the MS und TS differs by more than 6 sec (mainly TS0)
    if (6e9 < finalTime) return;

    ctx.digis.emplace_back(pixelUID, finalTime, tot - ToTcorr);
  }

  bool UnpackMS::CheckMaskedDiRICH(int32_t subSubEventId) const
  {
    for (unsigned int i = 0; i < fMaskedDiRICHes.size(); ++i) {
      if (fMaskedDiRICHes.at(i) == subSubEventId) return true;
    }
    return false;
  }

  TdcWordType UnpackMS::GetTdcWordType(uint32_t tdcWord) const
  {
    uint32_t tdcTimeDataMarker = (tdcWord >> 31) & 0x1;  // 1 bit
    uint32_t tdcMarker         = (tdcWord >> 29) & 0x7;  // 3 bits

    if (tdcTimeDataMarker == 0x1) {
      // TODO: I also include tdcMarker == 0x5, some tdc time data words have this marker. Is it correct?
      if (tdcMarker == 0x4 || tdcMarker == 0x5) {
        return TdcWordType::TimeData;
      }
      else {
        return TdcWordType::Error;
      }
    }
    if (tdcMarker == 0x0) return TdcWordType::Trailer;
    if (tdcMarker == 0x1) return TdcWordType::Header;
    if (tdcMarker == 0x2) return TdcWordType::Debug;
    if (tdcMarker == 0x3) return TdcWordType::Epoch;
    return TdcWordType::Error;
  }

  int32_t UnpackMS::GetPixelUID(int32_t fpgaID, int32_t ch) const
  {
    // First 16 bits are used for the FPGA ID, then
    // 8 bits unused and then 8 bits are used for the channel
    return ((fpgaID << 16) | (ch & 0x00FF));
  }

  TdcTimeData UnpackMS::ProcessTimeData(uint32_t tdcWord) const
  {
    TdcTimeData out;
    out.fCoarse       = static_cast<uint32_t>(tdcWord & 0x7ff);          // 11 bits
    out.fIsRisingEdge = static_cast<uint32_t>((tdcWord >> 11) & 0x1);    // 1 bit
    out.fFine         = static_cast<uint32_t>((tdcWord >> 12) & 0x3ff);  // 10 bits
    out.fChannel      = static_cast<uint32_t>((tdcWord >> 22) & 0x7f);   // 7 bits
    return out;
  }

  uint32_t UnpackMS::ProcessEpoch(uint32_t tdcWord) const { return static_cast<uint32_t>(tdcWord & 0xfffffff); }

  uint16_t UnpackMS::ProcessHeader(uint32_t tdcWord) const
  {
    return static_cast<uint16_t>(tdcWord & 0xff);  //8 bits
  }

  uint16_t UnpackMS::ProcessTrailer(uint32_t tdcWord) const { return static_cast<uint16_t>(tdcWord & 0xffff); }

}  // namespace cbm::algo::rich
