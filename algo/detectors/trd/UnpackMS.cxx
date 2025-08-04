/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig, Dominik Smith [committer], David Schledt */

#include "UnpackMS.h"

#include "AlgoFairloggerCompat.h"

#include <algorithm>
#include <cassert>
#include <vector>

using std::unique_ptr;

namespace cbm::algo::trd
{

  // ---- digestBufInfoFlags ----
  template<uint8_t sys_ver>
  Spadic::MsInfoType UnpackMS<sys_ver>::digestBufInfoFlags(const std::uint32_t frame) const
  {
    auto flag = (frame >> 15) & 0x3;
    Spadic::MsInfoType infotype;
    if (flag == 1) infotype = Spadic::MsInfoType::kChannelBuf;
    if (flag == 2) infotype = Spadic::MsInfoType::kOrdFifoBuf;
    if (flag == 3) infotype = Spadic::MsInfoType::kChannelBufM;
    return infotype;
  }

  template Spadic::MsInfoType UnpackMS<0x01>::digestBufInfoFlags(const std::uint32_t frame) const;

  // ---- digestInfoMsg ----
  template<uint8_t sys_ver>
  void UnpackMS<sys_ver>::digestInfoMsg(const std::uint32_t frame) const
  {
    /// Save info message if needed.
    //if (fOptOutBVec) { fOptOutBVec->emplace_back(std::make_pair(ctx.fLastFulltime, frame)); }
    [[maybe_unused]] Spadic::MsInfoType infotype = getInfoType(frame);
    // "Spadic_Info_Types";
  }

  template void UnpackMS<0x01>::digestInfoMsg(const std::uint32_t frame) const;

  // ---- digestInfoMsg ----
  template<uint8_t sys_ver>
  void UnpackMS<sys_ver>::digestMsFlags(const std::uint16_t flags, UnpackMonitorData& monitor) const
  {
    if (flags & static_cast<std::uint16_t>(fles::MicrosliceFlags::CrcValid)) {
      monitor.fNumCrcValidFlags++;
    }
    if (flags & static_cast<std::uint16_t>(fles::MicrosliceFlags::OverflowFlim)) {
      monitor.fNumOverflowFlimFlags++;
    }
    if (flags & static_cast<std::uint16_t>(fles::MicrosliceFlags::OverflowUser)) {
      monitor.fNumOverflowUserFlags++;
    }
    if (flags & static_cast<std::uint16_t>(fles::MicrosliceFlags::DataError)) {
      monitor.fNumDataErrorFlags++;
    }
  }

  template void UnpackMS<0x01>::digestMsFlags(const std::uint16_t flags, UnpackMonitorData& monitor) const;
  template void UnpackMS<0x10>::digestMsFlags(const std::uint16_t flags, UnpackMonitorData& monitor) const;

  // ---- extractSample ----
  template<uint8_t sys_ver>
  std::float_t UnpackMS<sys_ver>::extractAvgSample(size_t* adcbuffer, size_t* nadcbits) const
  {
    // can not extract samples from a buffer with less than 9 bits
    assert(*nadcbits >= 9);
    *nadcbits -= 9;

    // The decoding of the average sample is kind of interesting:
    // We get 9 bits in total iiiiiiiff. The 7 "i" bits refer to std integer bits, hence,
    // covering a range of 0..128.
    // The 2 "f" bits refer to values after a fix point refering to [0,0.25,0.5,0.75].
    // The sign we have to assume to be negative (bit-9 = 1) and also bit 8 of our std
    // interger range we have to assume to be 0, such that the returned number is in
    // between -256..-128.

    // Activate the 7 "i" bits
    std::int16_t sample = 0x07f;

    // Write the content of the 7 "i" bits to temp
    sample &= (*adcbuffer >> (*nadcbits + 2));

    // Switch on the negative sign
    sample |= 0xff00;

    return sample;
  }

  template std::float_t UnpackMS<0x01>::extractAvgSample(size_t* adcbuffer, size_t* nadcbits) const;

  // ---- extractSample ----
  template<uint8_t sys_ver>
  std::int16_t UnpackMS<sys_ver>::extractSample(size_t* adcbuffer, size_t* nadcbits) const
  {
    // can not extract samples from a buffer with less than 9 bits
    assert(*nadcbits >= 9);

    // We want to access the bits stored at the positions between nadcbits and nadcbits - 9, so we can already here
    // reduce nadcbits by 9 and than shift the adcbuffer by this value to the right and compare it with temp which has the 9 lsbs set to 1
    *nadcbits -= 9;

    std::int16_t temp = 0x1ff;
    temp &= (*adcbuffer >> (*nadcbits));

    // Now we have our 9 bits stored in temp, but for a std::int16_t this does not match in terms of the sign handling.
    // So we check on bit 9 for the sign (temp & 0x0100) and if we have a negative value we manipulate bit 16-10 to 1
    // to get the correct negative number
    std::int16_t sample = (temp & 0x0100) ? (temp | 0xff00) : temp;

    return sample;
  }

  template std::int16_t UnpackMS<0x01>::extractSample(size_t* adcbuffer, size_t* nadcbits) const;

  // ---- getInfoType ----
  template<uint8_t sys_ver>
  Spadic::MsInfoType UnpackMS<sys_ver>::getInfoType(const std::uint32_t frame) const
  {
    // Set first 20 bits to 1 for the mask
    size_t mask = 0x000FFFFF;

    // 000011.................. : BOM word
    // 0000010................. : MSB word
    // 0000011................. : BUF word
    // 0000100................. : UNU word
    // 0000101................. : MIS word

    if (((frame & mask) >> 18) == 3)  // BOM
    {
      return Spadic::MsInfoType::kBOM;
    }
    if (((frame & mask) >> 17) == 2)  // MSB
    {
      return Spadic::MsInfoType::kMSB;
    }
    if (((frame & mask) >> 17) == 3)  // BUF
    {
      digestBufInfoFlags(frame);
      return Spadic::MsInfoType::kBUF;
    }
    if (((frame & mask) >> 17) == 4)  // UNU
    {
      return Spadic::MsInfoType::kUNU;
    }
    if (((frame & mask) >> 17) == 5)  // MIS
    {
      return Spadic::MsInfoType::kMIS;
    }
    else {
      // TODO: Track this error!
      L_(error) << "UnpackMS::GetInfoType] unknown type!";
      return Spadic::MsInfoType::kMSB;
    }
  }

  template Spadic::MsInfoType UnpackMS<0x01>::getInfoType(const std::uint32_t frame) const;

  // ---- getMessageType ----
  template<uint8_t sys_ver>
  Spadic::MsMessageType UnpackMS<sys_ver>::getMessageType(const std::uint32_t frame) const
  {
    std::uint32_t checkframe = frame;
    checkframe &= 0xffffff;
    if ((checkframe >> 21) == 1)  // SOM  001. ....
    {
      return Spadic::MsMessageType::kSOM;
    }
    else if ((checkframe >> 22) == 1)  // RDA  01.. ....
    {
      return Spadic::MsMessageType::kRDA;
    }
    else if ((checkframe >> 20) == 1)  // EOM  0001 ....
    {
      return Spadic::MsMessageType::kEOM;
    }
    else if ((checkframe >> 22) == 3)  // TS_MSB 11.. ....
    {
      return Spadic::MsMessageType::kEPO;
    }
    else if (0 < (checkframe >> 18) && (checkframe >> 18) <= 3) {
      return Spadic::MsMessageType::kINF;
    }
    else if (checkframe == 0)  // Last Word in a Microslice is 0
    {
      return Spadic::MsMessageType::kNUL;
    }
    else  // not a spadic message
    {
      return Spadic::MsMessageType::kUNK;
    }
  }

  template Spadic::MsMessageType UnpackMS<0x01>::getMessageType(const std::uint32_t frame) const;

  // ---- getTsMsb ----
  template<uint8_t sys_ver>
  std::uint8_t UnpackMS<sys_ver>::getTsMsb(const std::uint32_t frame, UnpackMonitorData& monitor) const
  {
    if ((frame & 0xf) > 0)
      return -2;  // if a 'error' ts_msb is received the tsmsb value is not correct. To not mess up the counting -2 is returned.
    // The epoch in form of the TS_MSBs is written 3 times into the frame to allow to check for bit flips and catch errors.
    // It has the length of 6 bits and an offset of 4
    std::uint8_t tsmsb[3];
    for (uint iepoch = 0; iepoch < 3; ++iepoch) {
      tsmsb[iepoch] = static_cast<std::uint8_t>((frame >> (4 + 6 * iepoch) & 0x3f));
    }

    // Check if the epoch at position 0 is at least compatible with one of the others.
    // Since, we only have 3 that value is automatically the majority value.
    if (tsmsb[0] == tsmsb[1] || tsmsb[0] == tsmsb[2]) return tsmsb[0];

    // If we arrive here the epoch at position 0 is not compatible with the other two.
    // So let's check if they are compatible with each other. If so we have again a majority epoch
    if (tsmsb[1] == tsmsb[2]) return tsmsb[1];

    monitor.fNumNonMajorTsMsb++;

    return tsmsb[0];
  }

  template std::uint8_t UnpackMS<0x01>::getTsMsb(const std::uint32_t frame, UnpackMonitorData& monitor) const;


  // method definition for specialization
  template<uint8_t sys_ver>
  CbmTrdDigi UnpackMS<sys_ver>::makeDigi(Spadic::FexWord<sys_ver> fw, size_t fMsStartTimeRel) const
  {
    return CbmTrdDigi();
  }

  // ---- makeDigi sys_ver <0x10> ----
  template<>
  CbmTrdDigi UnpackMS<0x10>::makeDigi(Spadic::FexWord<0x10> fw, size_t fMsStartTimeRel) const
  {
    auto rawTriggerType = static_cast<Spadic::eTriggerType>(fw.ht);
    auto triggerType    = GetDigiTriggerType(rawTriggerType);

    int32_t errClass = 0;

    // Get the address of the originating spadic
    const UnpackCrobPar& crobPar   = fParams.fCrobParams.at(0);
    const UnpackElinkPar& elinkPar = crobPar.fElinkParams.at(fw.elink);
    const uint32_t asicAddress     = elinkPar.fAddress;

    // Get the channel id on the module
    int32_t padChNr = elinkPar.fChanAddress.at(fw.channel);

    // Get the time information and apply the necessary correction
    uint64_t time = (fw.timestamp - fw.prec_time) * fAsicClockCycle + fMsStartTimeRel;
    time -= elinkPar.fTimeOffset;

    auto energy = fw.maxAdc * fParams.fMaxAdcToEnergyCal;

    // Get the unique module id from the asic address
    int32_t uniqueModuleId = asicAddress / 1000;

    CbmTrdDigi digi = CbmTrdDigi(padChNr, uniqueModuleId, energy, time, triggerType, errClass);

    // If the message was flagged as multi hit, forward this info to the digi
    if (fw.mh != 0) digi.SetTriggerType(CbmTrdDigi::eTriggerType::kMulti);

    return digi;
  }

  // ---- makeDigi from raw ----
  template<uint8_t sys_ver>
  CbmTrdDigi UnpackMS<sys_ver>::makeDigi(CbmTrdRawMessageSpadic raw, MsContext& ctx) const
  {
    // Extract the trigger type and translate it to the digi enum
    auto rawTriggerType = static_cast<Spadic::eTriggerType>(raw.GetHitType());
    auto triggerType    = GetDigiTriggerType(rawTriggerType);

    // Get the digi error class (dummy for the time being)
    int32_t errClass = 0;

    // Get the address of the originating spadic
    const UnpackCrobPar& crobPar   = fParams.fCrobParams.at(raw.GetCrobId());
    const UnpackElinkPar& elinkPar = crobPar.fElinkParams.at(raw.GetElinkId());
    const uint32_t asicAddress     = elinkPar.fAddress;

    // Get the channel id on the module
    int32_t padChNr = elinkPar.fChanAddress.at(raw.GetChannelId());

    // Store the full time information to last full-time member for error message handling
    ctx.fLastFulltime = raw.GetFullTime();

    // Get the time information and apply the necessary correction
    uint64_t time = raw.GetTime() - elinkPar.fTimeOffset;

    // Get the timeshift and set the member, which is required for some of the rtd methods
    uint64_t currentTimeshift = GetBinTimeShift(raw.GetSamples());

    // In simulation since we often start at time = 0 there is a non negligible chance that a time < 0 is extracted.
    //Since, this is not allowed in the code we set it to 0 for these cases
    time = time > currentTimeshift ? time - currentTimeshift : 0;

    // In this case of CbmTrdRawToDigi GetCharge calls GetBinTimeshift, since the information is needed.
    // The shift is stored in fCurrentTimeshift
    // Hence, the order of charge and time assignement here makes a difference!
    auto maxadc = GetMaxAdcValue(raw.GetSamples());

    // Get energy from maxadc value
    auto energy = maxadc * fParams.fMaxAdcToEnergyCal;

    // Get the unique module id from the asic address
    int32_t uniqueModuleId = asicAddress / 1000;

    CbmTrdDigi digi = CbmTrdDigi(padChNr, uniqueModuleId, energy, time, triggerType, errClass);

    // If the raw message was flagged as multi hit, forward this info to the digi
    if (raw.GetMultiHit()) digi.SetTriggerType(CbmTrdDigi::eTriggerType::kMulti);

    return digi;
  }

  template CbmTrdDigi UnpackMS<0x01>::makeDigi(CbmTrdRawMessageSpadic raw, MsContext& ctx) const;

  // ---- makeRaw ----
  template<uint8_t sys_ver>
  CbmTrdRawMessageSpadic UnpackMS<sys_ver>::makeRaw(const std::uint32_t frame, std::uint16_t criId, std::uint8_t crobId,
                                                    std::uint16_t elinkId, std::uint8_t istream, MsContext& ctx) const
  {
    auto chId             = static_cast<std::uint8_t>(((frame >> 17) & 0xf));
    auto timestamp        = static_cast<std::uint8_t>((frame >> 9) & 0xff);
    bool multihit         = ((frame >> 8) & 0x1);
    auto hitType          = static_cast<std::uint8_t>((frame >> 6) & 0x3);
    std::uint8_t nsamples = 0;
    // We directly start with the largest possible samples vector to only init it once
    std::vector<std::int16_t> samples = std::vector<std::int16_t>(0);

    uint64_t fulltime = ctx.fMsStartTimeRelCC + (ctx.fNrTsMsbVec.at(istream) * fTsMsbLengthCC) + timestamp;

    // Create message
    return CbmTrdRawMessageSpadic(chId, elinkId, crobId, criId, hitType, nsamples, multihit, fulltime, samples);
  }

  template CbmTrdRawMessageSpadic UnpackMS<0x01>::makeRaw(const std::uint32_t frame, std::uint16_t criId,
                                                          std::uint8_t crobId, std::uint16_t elinkId,
                                                          std::uint8_t istream, MsContext& ctx) const;


  // ---- GetDigiTriggerType ----
  template<uint8_t sys_ver>
  CbmTrdDigi::eTriggerType UnpackMS<sys_ver>::GetDigiTriggerType(Spadic::eTriggerType tt)
  {
    // Shift self trigger to digi selftrigger
    // Shift neighbour trigger to digi neighbour
    // Hide spadic kSandN in Self
    switch (tt) {
      case Spadic::eTriggerType::kGlobal: return CbmTrdDigi::eTriggerType::kNTrg;
      case Spadic::eTriggerType::kSelf: return CbmTrdDigi::eTriggerType::kSelf;
      case Spadic::eTriggerType::kNeigh: return CbmTrdDigi::eTriggerType::kNeighbor;
      case Spadic::eTriggerType::kSandN: return CbmTrdDigi::eTriggerType::kSelf;
      default: return CbmTrdDigi::eTriggerType::kNTrg;
    }
  }

  template CbmTrdDigi::eTriggerType UnpackMS<0x01>::GetDigiTriggerType(Spadic::eTriggerType tt);
  template CbmTrdDigi::eTriggerType UnpackMS<0x10>::GetDigiTriggerType(Spadic::eTriggerType tt);

  // --- GetCharge ----
  template<uint8_t sys_ver>
  float_t UnpackMS<sys_ver>::GetMaxAdcValue(const std::vector<std::int16_t>* samples) const
  {
    // Safety for corrupted input samples
    assert(samples->size() >= fPeakingBinMin);

    // The signal should peak at the shaping time.
    // The corresponding sample is the peaking time divided by the sample length.
    auto itbegin = std::next(samples->begin(), fPeakingBinMin);

    // Check if the expected maximum position of the peaking bin exceeds the size of the vector
    auto nsamples      = samples->size();
    auto peakingBinMax = (nsamples - 1) > fPeakingBinMax ? fPeakingBinMax : nsamples;
    auto itend         = std::next(samples->begin(), peakingBinMax);

    // Get the maximum element
    auto itmax = std::max_element(itbegin, itend);

    // Get charge and correct for the baseline
    float_t charge = static_cast<float_t>(*itmax) - GetBaseline(samples);

    // Remark: Due to the fact, that we store the charge UInt_t in the Digi values below 0 are not allowed.
    // In this case the above only appears if the baseline fluctuated above all values in the applied peaking range.
    // This can only happen for forced neighbor triggers with a deposited charged that can not be separated from the baseline.
    return charge > 0 ? charge : 0;
  }

  template float_t UnpackMS<0x01>::GetMaxAdcValue(const std::vector<std::int16_t>* samples) const;

  // ---- GetBaseline ----
  template<uint8_t sys_ver>
  float_t UnpackMS<sys_ver>::GetBaseline(const std::vector<std::int16_t>* samples) const
  {
    // The spadic 2.2 has a functionality that an average baseline can be written to the first sample.
    // So first we have to check if this is active.
    if (fParams.fUseBaselineAvg)
      return samples->at(0);
    else {
      float_t baseline = 0.0;
      auto itend       = samples->begin() + fNrOfPresamples;
      if (itend > samples->end()) itend = samples->end();
      for (auto isample = samples->begin(); isample < itend; isample++) {
        baseline += *isample;
      }
      baseline /= fNrOfPresamples;

      return baseline;
    }
  }
  template float_t UnpackMS<0x01>::GetBaseline(const std::vector<std::int16_t>* samples) const;

  // ----   Algorithm execution sys_ver >= 0x10  ---------------------------------------------
  template<std::uint8_t sys_ver>
  typename UnpackMS<sys_ver>::Result_t UnpackMS<sys_ver>::operator()(const uint8_t* msContent,
                                                                     const fles::MicrosliceDescriptor& msDescr,
                                                                     const uint64_t tTimeslice) const
  {
    Result_t result = {};


    // Digest the flags from the µSlice
    digestMsFlags(msDescr.flags, std::get<1>(result));

    size_t fMsStartTimeRel = (msDescr.idx - tTimeslice);

    // Get bytes per word used for the given system version
    constexpr std::uint8_t bytes = Spadic::BytesPerWord<sys_ver>();

    auto mssize          = msDescr.size;
    std::uint32_t nwords = mssize / bytes;

    const Spadic::NByteContainer<bytes>* bp = reinterpret_cast<const Spadic::NByteContainer<bytes>*>(msContent);

    for (std::uint32_t iword = 0; iword < nwords; ++iword) {
      Spadic::NByteContainer<bytes> bCont = bp[iword];
      Spadic::FexWord<sys_ver> fw(bCont);
      if (fw.ht != 0) {
        std::get<0>(result).push_back(makeDigi(fw, fMsStartTimeRel));
      }
    }
    return result;
  }

  template typename UnpackMS<0x10>::Result_t UnpackMS<0x10>::operator()(const uint8_t* msContent,
                                                                        const fles::MicrosliceDescriptor& msDescr,
                                                                        const uint64_t tTimeslice) const;
  // --------------------------------------------------------------------------

  // ----   Algorithm execution sys_ver - 0x01  ---------------------------------------------
  template<>
  typename UnpackMS<0x01>::Result_t UnpackMS<0x01>::operator()(const uint8_t* msContent,
                                                               const fles::MicrosliceDescriptor& msDescr,
                                                               const uint64_t tTimeslice) const
  {
    // --- Output data
    Result_t result = {};

    MsContext ctx = {};

    // Get the µSlice starttime relative to the timeslice starttime (constant is clock length of Spadic in ns)
    ctx.fMsStartTimeRelCC = (msDescr.idx - tTimeslice) / fAsicClockCycle;

    // We only want to count on TS_MSB per Stream per TS_MSB package (each eLink sends its own TS_MSB frame)
    // so we store the current TS_MSB and compare it to the incoming.
    std::int8_t currTsMsb = 0;

    // Reset the TS_MSB counter for the new µSlice we unpack
    ctx.fNrTsMsbVec.resize(fStreamsPerWord);


    // Get the µslice size in bytes to calculate the number of completed words
    auto mssize = msDescr.size;

    // Get the hardware ids from which the current µSlice is coming
    std::uint8_t crobId = 0;
    auto criId          = msDescr.eq_id;

    // Digest the flags from the µSlice
    digestMsFlags(msDescr.flags, std::get<1>(result));

    // Get the number of complete words in the input MS buffer.
    std::uint32_t nwords = mssize / fBytesPerWord;

    // We have 32 bit spadic frames in this readout version
    const auto mscontent = reinterpret_cast<const size_t*>(msContent);

    std::get<0>(result).reserve(nwords);

    // Loop over all 64bit-Spadic-Words in the current µslice
    for (std::uint32_t istream = 0; istream < fStreamsPerWord; istream++) {
      currTsMsb = -1;
      for (std::uint32_t iword = 0; iword < nwords; ++iword) {
        // Access the actual word from the pointer
        size_t word = static_cast<size_t>(mscontent[iword]);

        // Access the actual frame[iframe] from the word. (see fStreamsPerWord)
        std::uint32_t frame = (word >> (32 * istream)) & 0xffffffff;

        // Get the type of the frame
        auto kWordtype = getMessageType(frame);

        // In case we saw any other word than an EPO(TS_MSB) reset the flag,
        // such that we again increase by one if an EPO frame arrives
        auto elinkId = (frame >> 24) & 0x3f;

        switch (kWordtype) {
          case Spadic::MsMessageType::kEPO: {
            auto tsmsb = getTsMsb(frame, std::get<1>(result));
            if (((tsmsb - currTsMsb) & 0x3f) == 1 || currTsMsb == -1) ctx.fNrTsMsbVec.at(istream)++;
            currTsMsb = tsmsb;
            std::get<1>(result).fNumEpochMsgs++;
            break;
            // FIXME in the kEPO msg we also have further flags that should be extracted
          }
          case Spadic::MsMessageType::kSOM: {
            // Create the raw message and fill it with all information we can get from the SOM msg
            CbmTrdRawMessageSpadic raw = makeRaw(frame, criId, crobId, elinkId, istream, ctx);

            // FIXME since we can not deduce the sample position from the messages we need in
            // future some parameter handling here to place the samples at the correct position
            // 6 adc bits are stored in the som message
            size_t nadcbits      = 6;
            size_t nadcbitstotal = 6;
            // Get the first bits from the adc signal
            size_t adcbuffer = frame & 0x3f;
            size_t isample   = 0;
            size_t irda      = 0;

            // Now lets check if we have rda words following our som
            iword++;
            word  = static_cast<size_t>(mscontent[iword]);
            frame = (word >> (32 * istream)) & 0xffffffff;

            // The maximum amount of samples (32) equals to 12 RDA messages
            while (getMessageType(frame) == Spadic::MsMessageType::kRDA && irda < 12) {
              // We have to count the number of rda frames for sample reconstruction in eom
              irda++;

              // Ensure that we are on the correct eLink
              elinkId = (frame >> 24) & 0x3f;
              if (elinkId != raw.GetElinkId()) {
                std::get<1>(result).fNumElinkMis++;
              }

              // We have 22 adc bits per RDA word lets add them to the buffer...
              adcbuffer <<= 22;
              adcbuffer |= static_cast<size_t>((frame & 0x3fffff));
              // and increase the adcbit counter by 22 bits
              nadcbits += 22;
              nadcbitstotal += 22;
              // If we have 9 or more samples stored we can extract n samples
              while (nadcbits >= 9) {
                raw.IncNrSamples();
                // In case the avg baseline feature was used we need to take special care of sample 0
                if (isample == 0 && fParams.fUseBaselineAvg)
                  raw.SetSample(extractAvgSample(&adcbuffer, &nadcbits), isample);
                else
                  raw.SetSample(extractSample(&adcbuffer, &nadcbits), isample);
                isample++;
              }
              iword++;
              word  = static_cast<size_t>(mscontent[iword]);
              frame = (word >> (32 * istream)) & 0xffffffff;
            }

            if (getMessageType(frame) == Spadic::MsMessageType::kEOM) {
              // Ensure that we are on the correct eLink
              elinkId = (frame >> 24) & 0x3f;
              if (elinkId != raw.GetElinkId()) {
                std::get<1>(result).fNumElinkMis++;
              }

              // Number of samples indicator = nsamples % 4
              std::uint8_t nsamplesindicator = (frame >> 18) & 0x3;
              // Number of required samples as indicated
              std::uint64_t nreqsamples = (nadcbitstotal + 18) / 9;
              std::uint8_t nn           = nreqsamples % 4;
              for (std::uint8_t itest = 0; itest < 3; itest++) {
                if (nn == nsamplesindicator || nreqsamples == 0) break;
                nreqsamples--;
                nn = nreqsamples % 4;
              }

              // There is a chance that the nsamplesindicator bits are corrupted,
              // here we check that we do not try to extract more adcbits than actually are streamed
              if (nreqsamples >= isample) {
                // Now extract from the above values the number of required adc bits from the eom
                std::int8_t nrequiredbits = (nreqsamples - isample) * 9 - nadcbits;
                adcbuffer <<= nrequiredbits;

                // The eom carries at maximum 18 adcbits
                adcbuffer |= static_cast<size_t>((frame & 0x3ffff) >> (18 - nrequiredbits));
                nadcbits += nrequiredbits;

                while (nadcbits >= 9) {
                  raw.IncNrSamples();
                  raw.SetSample(extractSample(&adcbuffer, &nadcbits), isample);
                  isample++;
                }
              }
              else {
                std::get<1>(result).fNumCorruptEom++;
              }
              std::get<1>(result).fNumCreatedRawMsgs++;

              // the message is done and the raw message container should contain everything we need.
              // So now we can call makeDigi(). Nevertheless there is a chance for a corrupted message,
              // which ends up with 0 samples so we have to check for it.
              if (isample > 0) std::get<0>(result).push_back(makeDigi(raw, ctx));
            }
            else {
              // We move the word counter backwards by one, such that the unexpected message can correctly be digested
              iword--;
              std::get<1>(result).fNumMissingEom++;
            }
            break;
          }
          case Spadic::MsMessageType::kRDA: {
            std::get<1>(result).fNumWildRda++;
            break;
          }
          case Spadic::MsMessageType::kEOM: {
            std::get<1>(result).fNumWildEom++;
            break;
          }
          case Spadic::MsMessageType::kINF: {
            std::get<1>(result).fNumCreatedInfoMsgs++;
            digestInfoMsg(frame);
            break;
          }
          case Spadic::MsMessageType::kNUL: {
            // last word in Microslice is 0.
            if (iword != (nwords - 1) || (istream != (fStreamsPerWord - 1))) {
              std::get<1>(result).fNumWildNul++;
            }
            break;
          }
          case Spadic::MsMessageType::kUNK: {
            std::get<1>(result).fNumUnknownWords++;
            return result;
            break;
          }
          default:
            // We have varying msg types for different versions of the message format.
            // Hence, to not produce compiler warnings we have a "default break;" here.
            break;
        }
      }
    }
    return result;
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::algo::trd
