/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig, Alexandru Bercuci, Dominik Smith [committer] */

#include "UnpackMS.h"

#include "AlgoFairloggerCompat.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using std::unique_ptr;

namespace cbm::algo::trd2d
{
  // ----   Fasp message constructor  ----------------------------------------
  FaspMessage::FaspMessage(uint8_t c, uint8_t typ, uint8_t t, uint16_t d, uint8_t asic)
    : ch(c)
    , type(eMessageType::kNone)
    , tlab(t)
    , data(d)
    , fasp(asic)
  {
    if (typ == uint8_t(eMessageType::kData))
      type = eMessageType::kData;
    else if (typ == uint8_t(eMessageType::kEpoch))
      type = eMessageType::kEpoch;
  }

  std::string FaspMessage::print() const
  {
    std::stringstream ss;
    switch (type) {
      case eMessageType::kData:
        ss << "    DATA : fasp=" << std::setw(2) << (int) fasp << " ch=" << std::setw(2) << (int) ch
           << " t=" << std::setw(3) << (int) tlab << " data=" << std::setw(4) << (int) data << std::endl;
        break;
      case eMessageType::kEpoch: ss << "    EPOCH: ch=" << (int) ch << " epoch=" << (int) epoch << std::endl; break;
      default: ss << "    MTYPE: unknown" << std::endl; break;
    }

    return ss.str();
  }

  // ----   Data type descriptor   ---------------------------------------------
  template<std::uint8_t mess_ver>
  eMessageType FaspMessage::getType(uint32_t)
  {
    return eMessageType::kNone;
  }

  template<>
  eMessageType FaspMessage::getType<uint8_t(eMessageVersion::kMess24)>(uint32_t w)
  {
    /** Search the data type descriptor in a FASP word. Starting with message version kMess24*/

    if ((w >> 31) & 0x1) return eMessageType::kEpoch;
    return eMessageType::kData;
  }

  // ----   Unpacking a DATA WORD   ---------------------------------------------
  template<std::uint8_t mess_ver>
  void FaspMessage::readDW(uint32_t)
  {
    return;
  }

  template<>
  void FaspMessage::readDW<uint8_t(eMessageVersion::kMess24)>(uint32_t w)
  {
    /** Data Word unpacking starting with message version kMess24*/

    uint8_t shift(0);
    uint16_t adc_data = (w >> shift) & 0x3fff;
    // TODO This data format version delivers the ADC value as bit_sgn + 13 significant bits
    // TODO The CbmTrdDigi supports digi data with only 12bits unsigned. The first tests will
    // TODO convert the measurement to the old format leaving the implementation of the new storage to // TODO later time.  (AB 14.06.2024)
    uint16_t sign = adc_data >> 13;  // sign
    int value_i;
    if (!sign)
      value_i = adc_data;
    else
      value_i = (-1) * ((adc_data ^ 0xffff) & 0x1fff);
    // convert to 12bit unsigned
    data = (value_i + 0x1fff) >> 2;
    shift += uint8_t(eMessageLength::kMessData);
    tlab = (w >> shift) & 0x7f;
    shift += uint8_t(eMessageLength::kMessTlab);
    ch = (w >> shift) & 0xf;
    shift += uint8_t(eMessageLength::kMessCh);
    fasp = ((w >> shift) & 0x3f);
    shift += uint8_t(eMessageLength::kMessFasp);
    type = eMessageType((w >> shift) & 0x1);
    shift += uint8_t(eMessageLength::kMessType);

    // if (VERBOSE >= 2) {
    //   printf("v06.24Mess_readDW[%x] signed charge = %+d\n", w, value_i);
    //   print();
    // }
    return;
  }

  // ----   Unpacking an EPOCH WORD   ---------------------------------------------
  template<std::uint8_t mess_ver>
  void FaspMessage::readEW(uint32_t)
  {
    return;
  }

  template<>
  void FaspMessage::readEW<uint8_t(eMessageVersion::kMess24)>(uint32_t w)
  {
    /** Epoch Word unpacking starting with message version kMess24*/

    uint8_t shift(0);
    epoch = (w >> shift) & 0x1fffff;
    shift += uint8_t(eMessageLength::kMessEpoch);
    ch = (w >> shift) & 0xf;
    shift += uint8_t(eMessageLength::kMessCh);
    fasp = (w >> shift) & 0x3f;
    shift += uint8_t(eMessageLength::kMessFasp);
    type = eMessageType((w >> shift) & 0x1);
    shift += uint8_t(eMessageLength::kMessType);

    // if (VERBOSE >= 2) {
    //   printf("v06.24Mess_readEW[%x]\n", w);
    //   print();
    // }
    return;
  }

  void UnpackPar::dump() const
  {
    L_(debug) << "UnpackPar::dump() mod=" << fModId << " SysOff=" << fSystemTimeOffset << " sRef=" << fRefSignal;
    L_(debug) << " elink[" << (int) fEqId << "]=0x" << std::hex << (int) fEqAdd << " nAsics=" << std::dec
              << fAsicParams.size();
    for (const auto& [fasp, par] : fAsicParams) {
      L_(debug) << "  fasp=" << int(fasp) << " par=" << std::hex << &par;
    }
    for (int ipad(0); ipad < NFASPMOD * NFASPCH; ipad++) {
      float gn(fCalibParams[ipad].fGainFee), bl(fCalibParams[ipad].fBaseline);
      if (gn < 0) continue;
      L_(debug) << "  pad=" << ipad << " bl=" << bl << " gn=" << gn;
    }
    L_(debug) << "UnpackPar::dump(-----------------------)";
  }

  template<>
  uint8_t UnpackPar::mapFaspId2Mod<uint8_t(eMessageVersion::kMessLegacy)>(uint8_t fasp_id) const
  {
    /** Use the mapping 36 fasp -> 1 optical fiber (equipment id)
   * Applies to FASPRO/FW v1 (e.g. mCBM22)
   */
    L_(debug) << "<vLegacy> Eq[" << (int) fEqId << "] = 0x" << std::hex << fEqAdd;
    return fEqId * NFASPROB + fasp_id;
  }

  template<>
  uint8_t UnpackPar::mapFaspId2Mod<uint8_t(eMessageVersion::kMess24)>(uint8_t fasp_id) const
  {
    /** Use the mapping 36 fasp -> 2 optical fiber (equipment id)
   * Applies to FASPRO/FW v2 (e.g. mCBM25)
   */

    int rob = fEqId / 2;  // ROB on the current chamber
    //L_(debug) << "<v24> ROB=" << rob << " Eq[" << (int)fEqId << "] = 0x" << std::hex << fEqAdd;

    return rob * NFASPROB + fasp_id;
  }

  // ----   Algorithm execution   ---------------------------------------------
  template<>
  typename UnpackMS<uint8_t(eMessageVersion::kMessLegacy)>::Result_t
  UnpackMS<uint8_t(eMessageVersion::kMessLegacy)>::operator()(const uint8_t* msContent,
                                                              const fles::MicrosliceDescriptor& msDescr,
                                                              const uint64_t tTimeslice) const
  {
    /** Implementation of TRD2D unpacking for the 2021 - 2022 mCBM data taking
     * The algorithm implements digi buffering due to a "feature" on the ADC read-out
     */
    // --- Output data
    Result_t result = {};

    MsContext ctx = {};

    // define time wrt start of time slice in TRD/FASP clks [80 MHz].
    uint64_t time = uint64_t((msDescr.idx - tTimeslice) / 25);  // guaranteed by CBM-DAQ
    time <<= 1;                                                 // time expressed in 12.5 ns clks

    // Get the number of complete words in the input MS buffer.
    const uint32_t nwords = msDescr.size / 4;
    L_(debug) << "UnpackMS<kMessLegacy>::op() param.olink[" << (int) fParams.fEqId << "]=0x" << std::hex
              << (int) fParams.fEqAdd << " data.rob=0x" << int(msDescr.eq_id) << " words=" << std::dec << nwords;
    // We have 32 bit FASP frames in this readout version
    const uint32_t* wd = reinterpret_cast<const uint32_t*>(msContent);

    unsigned char lFaspOld(0xff);
    std::vector<FaspMessage> vMess;
    for (uint64_t j = 0; j < nwords; j++, wd++) {
      uint32_t w      = *wd;
      uint8_t ch_id   = w & 0xf;
      uint8_t isaux   = (w >> 4) & 0x1;
      uint8_t slice   = (w >> 5) & 0x7f;
      uint16_t data   = (w >> 12) & 0x3fff;
      uint8_t fasp_id = ((w >> 26) & 0x3f);

      if (isaux) {
        if (ch_id == 0) {
          // clear buffer
          if (vMess.size()) {
            pushDigis(vMess, time, ctx);
          }
          vMess.clear();

          lFaspOld = 0xff;
          time += FASP_EPOCH_LENGTH;
        }
        continue;
      }
      if (lFaspOld != fasp_id) {
        if (vMess.size()) {
          pushDigis(vMess, time, ctx);
        }
        vMess.clear();
        lFaspOld = fasp_id;
      }
      if (data & 0x1) {
        ctx.fMonitor.fNumErrEndBitSet++;
        continue;
      }
      if (data & 0x2000) {
        ctx.fMonitor.fNumSelfTriggeredData++;
        data &= 0x1fff;
      }
      vMess.emplace_back(ch_id, (uint8_t) eMessageType::kData, slice, data >> 1, lFaspOld);
    }
    std::get<0>(result) = FinalizeComponent(ctx);  //TO DO: Original (non-algo) version calls this after MS loop!!
    std::get<1>(result) = ctx.fMonitor;

    return result;
  }
  //_________________________________________________________________________________
  template<uint8_t sys_ver>
  bool UnpackMS<sys_ver>::pushDigis(std::vector<FaspMessage> messes, const uint64_t time, MsContext& ctx) const
  {
    constexpr uint8_t mLegacy =
      uint8_t(eMessageVersion::kMessLegacy);  // message versions compatible with the current algo specialization
    const uint16_t mod_id        = fParams.fModId;
    const uint8_t fasp_mod_id    = fParams.mapFaspId2Mod<mLegacy>(messes[0].fasp);
    const UnpackAsicPar& asicPar = fParams.fAsicParams.at(fasp_mod_id);

    for (auto imess : messes) {
      const UnpackChannelPar& chPar = asicPar.fChanParams[imess.ch];
      //std::cout << imess.print();
      const int32_t pad                   = std::abs(chPar.fPadAddress) / 2;
      const bool hasPairingR              = bool(chPar.fPadAddress > 0);
      const uint8_t tdaqOffset            = chPar.fDaqOffset;
      const uint64_t lTime                = time + tdaqOffset + imess.tlab;
      const uint16_t lchR                 = hasPairingR ? imess.data : 0;
      const uint16_t lchT                 = hasPairingR ? 0 : imess.data;
      std::vector<CbmTrdDigi>& digiBuffer = ctx.fDigiBuffer[pad];

      if (digiBuffer.size() == 0) {  // init pad position in map and build digi for message
        digiBuffer.emplace_back(pad, lchT, lchR, lTime);
        digiBuffer.back().SetAddressModule(mod_id);
        continue;
      }

      // check if last digi has both R/T message components. Update if not and is within time window
      auto id = digiBuffer.rbegin();  // Should always be valid here.
                                      // No need to extra check
      double r, t;
      int32_t dt;
      const int32_t dtime = (*id).GetTime() - lTime;
      bool use(false);

      if (abs(dtime) < 5) {  // test message part of (last) digi
        r = (*id).GetCharge(t, dt);
        if (lchR && r < 0.1) {  // set R charge on an empty slot
          (*id).SetCharge(t, lchR, -dtime);
          use = true;
        }
        else if (lchT && t < 0.1) {  // set T charge on an empty slot
          (*id).SetCharge(lchT, r, +dtime);
          (*id).SetTime(lTime);
          use = true;
        }
      }

      // build digi for message when update failed
      if (!use) {
        digiBuffer.emplace_back(pad, lchT, lchR, lTime);
        digiBuffer.back().SetAddressModule(mod_id);
        id = digiBuffer.rbegin();
      }

      // update charge for previously allocated digis to account for FASPRO ADC buffering and read-out feature
      for (++id; id != digiBuffer.rend(); ++id) {
        r = (*id).GetCharge(t, dt);
        if (lchR && int(r)) {  // update R charge and mark on digi
          (*id).SetCharge(t, lchR, dt);
          (*id).SetFlag(1);
          break;
        }
        else if (lchT && int(t)) {  // update T charge and mark on digi
          (*id).SetCharge(lchT, r, dt);
          (*id).SetFlag(0);
          break;
        }
      }
    }
    messes.clear();

    return true;
  }

  template<uint8_t sys_ver>
  std::vector<CbmTrdDigi> UnpackMS<sys_ver>::FinalizeComponent(MsContext& ctx) const
  {
    std::vector<CbmTrdDigi> outputDigis;

    for (uint16_t ipad(0); ipad < NFASPMOD * NFASPPAD; ipad++) {
      if (!ctx.fDigiBuffer[ipad].size()) continue;
      uint nIncomplete(0);
      for (auto id = ctx.fDigiBuffer[ipad].begin(); id != ctx.fDigiBuffer[ipad].end(); id++) {
        double r, t;
        int32_t dt;
        r = (*id).GetCharge(t, dt);
        // check if digi has all signals CORRECTED
        if (((t > 0) != (*id).IsFlagged(0)) || ((r > 0) != (*id).IsFlagged(1))) {
          nIncomplete++;
          continue;
        }
        // reset flags as they were used only to mark the correctly setting of the charge/digi
        (*id).SetFlag(0, false);
        (*id).SetFlag(1, false);
        /** Convert global time from clk temporary representation to the finale version in [ns]
        * Correct the time with the system time offset which is derived in calibration.*/
        uint64_t gtime = (*id).GetTime() * fAsicClockPeriod;
        gtime >>= 1;
        if (gtime >= uint64_t(fParams.fSystemTimeOffset)) gtime -= fParams.fSystemTimeOffset;
        (*id).SetTime(gtime);
        outputDigis.emplace_back(std::move((*id)));
      }
      // clear digi buffer wrt the digi which was forwarded to higher structures
      ctx.fDigiBuffer[ipad].clear();
      if (nIncomplete > 2) {
        ctx.fMonitor.fNumIncompleteDigis++;  //TO DO: This must be moved if finalization is done after MS loop
      }
    }
    return outputDigis;
  }

  // ------------- Specialization kMess24 --------------------------------
  typename UnpackMS<uint8_t(eMessageVersion::kMess24)>::Result_t
  UnpackMS<uint8_t(eMessageVersion::kMess24)>::operator()(const uint8_t* msContent,
                                                          const fles::MicrosliceDescriptor& msDescr,
                                                          const uint64_t tTimeslice) const
  {
    /** Implementation of TRD2D unpacking for the 2024 - PRESENT mCBM data taking
     * The algorithm implements the new message format starting with version kMess24
     */

    constexpr uint8_t m24 =
      uint8_t(eMessageVersion::kMess24);  // message versions compatible with the current algo specialization
    Result_t result = {};
    MsContext ctx   = {};

    // // define time wrt start of time slice in TRD/FASP clks [80 MHz]. Contains:
    // //  - relative offset of the MS wrt the TS
    // //  - FASP epoch offset for current CROB
    // //  - TRD2D system offset wrt to experiment time
    // uint64_t time = uint64_t((msDescr.idx - tTimeslice - fParams.fSystemTimeOffset) * fAsicClockFreq);

    //define the time in run as function of CBM 40MHz clk
    uint64_t timeClk = (msDescr.idx - tTimeslice) / 25;
    //convert to TRD2D 80MHz clk
    timeClk <<= 1;

    // Get the number of complete words in the input MS buffer.
    const uint32_t nwords = msDescr.size / 4;
    L_(debug) << "UnpackMS<kMess24>::op() param.olink[" << (int) fParams.fEqId << "]=0x" << std::hex
              << (int) fParams.fEqAdd << " data.rob=0x" << int(msDescr.eq_id) << " words=" << std::dec << nwords;

    // We have 32 bit FASP frames in this readout version
    const uint32_t* wd = reinterpret_cast<const uint32_t*>(msContent);

    unsigned char lFaspOld(0xff);
    std::vector<FaspMessage> vMess;
    for (uint32_t j = 0; j < nwords; j++, wd++) {
      uint32_t w = *wd;
      // Select the appropriate conversion type of the word according to
      // the current message version and type
      switch (FaspMessage::getType<m24>(w)) {
        case eMessageType::kData: ctx.fMess.readDW<m24>(w); break;
        case eMessageType::kEpoch: ctx.fMess.readEW<m24>(w); break;
        default: break;  // no way to reach this line
      }
      // PROCESS EPOCH MESSAGES
      if (ctx.fMess.type == eMessageType::kEpoch) {
        if (ctx.fMess.ch == 0) {  // check word integrity
          // clear buffer
          if (vMess.size()) {
            pushDigis(vMess, timeClk, ctx);
          }
          vMess.clear();

          lFaspOld = 0xff;
          timeClk += FASP_EPOCH_LENGTH;
        }
        else {
          L_(error) << "FASP message[Epoch] with wrong signature.";
          ctx.fMonitor.fNumErrEndBitSet++;
        }
        continue;
      }

      // PROCESS DATA MESSAGES
      // clear buffer when switching to other FASP
      if (ctx.fMess.fasp != lFaspOld) {
        if (vMess.size()) pushDigis(vMess, timeClk, ctx);
        vMess.clear();
        lFaspOld = ctx.fMess.fasp;
      }
      if (ctx.fMess.data & 0x2000) {  // kept for backward compatibility TODO
        ctx.fMonitor.fNumSelfTriggeredData++;
        ctx.fMess.data &= 0x1fff;
      }
      vMess.emplace_back(ctx.fMess);
    }

    // combine all digis from this ROB
    std::vector<CbmTrdDigi> outputDigis;
    for (uint16_t ipad(0); ipad < NFASPMOD * NFASPPAD; ipad++) {
      if (!ctx.fRobDigi[ipad].size()) continue;
      for (auto id : ctx.fRobDigi[ipad]) {
        /** Convert global time from clk temporary representation to the finale version in [ns]
        * Correct the time with the system time offset which is derived in calibration.*/
        uint64_t gtime = id.GetTime() * fAsicClockPeriod;
        gtime >>= 1;
        int toffGlobalCorrection = fParams.fSystemTimeOffset + fParams.toff[ipad];
        if (toffGlobalCorrection > 0 && gtime < uint64_t(toffGlobalCorrection))
          gtime = 0;
        else
          gtime -= (fParams.fSystemTimeOffset + fParams.toff[ipad]);
        id.SetTime(gtime);
        outputDigis.emplace_back(std::move(id));
      }
    }
    std::get<0>(result) = outputDigis;
    std::get<1>(result) = ctx.fMonitor;

    return result;
  }
  bool UnpackMS<uint8_t(eMessageVersion::kMess24)>::pushDigis(std::vector<FaspMessage> messes, const uint64_t time,
                                                              MsContext& ctx) const
  {
    constexpr uint8_t m24 =
      uint8_t(eMessageVersion::kMess24);  // message versions compatible with the current algo specialization
    const uint8_t fasp_mod_id = fParams.mapFaspId2Mod<m24>(messes[0].fasp);
    L_(debug) << "pushDigis<v24> fasp=" << (int) messes[0].fasp << "/" << (int) fasp_mod_id << " ns=" << messes.size();
    if (fParams.fAsicParams.find(fasp_mod_id) == fParams.fAsicParams.end()) {
      L_(error) << "pushDigis<v24> fasp=" << (int) messes[0].fasp << "/" << (int) fasp_mod_id
                << " not mapped to param.olink[" << (int) fParams.fEqId << "]=0x" << std::hex << (int) fParams.fEqAdd;
      return false;
    }
    const UnpackAsicPar& asicPar = fParams.fAsicParams.at(fasp_mod_id);

    for (auto imess : messes) {
      const UnpackChannelPar& chPar = asicPar.fChanParams[imess.ch];
      const uint32_t mch            = std::abs(chPar.fPadAddress);
      const CalibChannelPar& calPar = fParams.fCalibParams.at(mch);
      // skip message if threshold set and signal under
      if (calPar.noise.fSignalThres && imess.data <= calPar.noise.fSignalThres) continue;
      const uint32_t pad     = mch / 2;
      const bool hasPairingR = bool(chPar.fPadAddress > 0);
      const uint64_t lTime   = time + imess.tlab;
      const uint16_t sgn =
        uint16_t((imess.data - fParams.fRefSignal) * calPar.fGainFee - calPar.fBaseline + fParams.fRefSignal);
      const uint16_t lchR                 = hasPairingR ? sgn : 0;
      const uint16_t lchT                 = hasPairingR ? 0 : sgn;
      std::vector<CbmTrdDigi>& digiBuffer = ctx.fRobDigi[pad];

      // init pad position in array and build digi for message
      if (digiBuffer.size() == 0) {
        digiBuffer.emplace_back(pad, lchT, lchR, lTime);
        digiBuffer.back().SetAddressModule(fParams.fModId);
        continue;
      }

      // check if last digi has both R/T message components.
      // Update if not and is within time window
      auto& id = digiBuffer.back();  // Should always be valid here.
                                     // No need to extra check
      double r, t;
      int32_t dt;
      const int32_t dtime = id.GetTime() - lTime;
      bool use(false), noise(false);
      if (abs(dtime) < 5) {  // test message part of (last) digi
        r = id.GetCharge(t, dt);
        if (lchR && r < 0.1) {  // set R charge on an empty slot
          id.SetCharge(t, lchR, -dtime);
          use = true;
        }
        else if (lchT && t < 0.1) {  // set T charge on an empty slot
          id.SetCharge(lchT, r, +dtime);
          id.SetTime(lTime);
          use = true;
        }
      }

      // check if signal can be flagged as dynamic noise
      if (!use && calPar.noise.tWindow > 0) {
        if (lchR) {
          if (lchR < calPar.noise.lThreshold && std::abs(-dtime * 12.5 - calPar.noise.tDelay) < calPar.noise.tWindow)
            noise = true;
        }
        else {
          if (lchT < calPar.noise.lThreshold && std::abs(-dtime * 12.5 - calPar.noise.tDelay) < calPar.noise.tWindow)
            noise = true;
        }
      }

      // build digi for message when update failed
      if (!use && !noise) {
        digiBuffer.emplace_back(pad, lchT, lchR, lTime);
        digiBuffer.back().SetAddressModule(fParams.fModId);
      }
    }
    messes.clear();

    return true;
  }
}  // namespace cbm::algo::trd2d
