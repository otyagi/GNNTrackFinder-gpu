/* Copyright (C) 2019-2021 Institute for Nuclear Research, Moscow
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Nikolay Karpushkin [committer]*/

#ifndef PSD_GBT_DATA_FORMAT_V100_H_
#define PSD_GBT_DATA_FORMAT_V100_H_

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace PsdDataV100
{

  struct PsdMsHeader {
    enum bitFieldSizes
    {
      MWs = 8,  //! MagicWord size in bits
      E0s = 8,  //! Empty bits size in bits
      MSs = 64  //! MicroSlice size in bits
    };

    uint8_t uMagicWord : MWs;     //! MagicWord
    uint8_t uEmpty0 : E0s;        //! Empty bits
    uint64_t ulMicroSlice : MSs;  //! Epoch

    void printout()
    {
      printf("MS magic word: %x; microslice: %llu\n", uMagicWord, static_cast<long long unsigned int>(ulMicroSlice));
    }

    void clear()
    {
      uMagicWord   = 0;
      uEmpty0      = 0;
      ulMicroSlice = 0;
    }

    PsdMsHeader() { clear(); }

  };  //PsdMsHeader;


  struct PsdPackHeader {
    enum bitFieldSizes
    {
      MWs = 4,   //! MagicWord size in bits
      LIs = 4,   //! Link index size in bits
      E0s = 24,  //! Empty bits size in bits
      HNs = 8,   //! Hits number size in bits
      TWs = 8,   //! Words in pack size in bits
      TMs = 32   //! ADC Time size in bits
    };

    uint8_t uMagicWord : MWs;   //! MagicWord
    uint8_t uLinkIndex : LIs;   //! Link index
    uint32_t uEmpty0 : E0s;     //! Empty bits
    uint8_t uHitsNumber : HNs;  //! Hits number
    uint8_t uTotalWords : TWs;  //! Words in data pack
    uint32_t uAdcTime : TMs;    //! ADC Time of threshold cross from the begining of MS

    void printout()
    {
      printf("Pack magic word: %x; link: %u; total hits: %u; total gbt words: %u; ADC time in microslice: %u\n",
             uMagicWord, uLinkIndex, uHitsNumber, uTotalWords, uAdcTime);
    }

    void clear()
    {
      uMagicWord  = 0;
      uLinkIndex  = 0;
      uEmpty0     = 0;
      uHitsNumber = 0;
      uTotalWords = 0;
      uAdcTime    = 0;
    }

    PsdPackHeader() { clear(); }

  };  //PsdPackHeader;


  struct PsdHitHeader {
    enum bitFieldSizes
    {
      HCs = 8,   //! Hit channel size in bits
      WWs = 8,   //! Waveform points size in bits
      FAs = 16,  //! FEE accumulator bits size in bits
      E0s = 12,  //! Empty bits size in bits
      SCs = 20,  //! Signal charge size in bits
      ZLs = 16   //! ZeroLevel size in bits
    };

    uint8_t uHitChannel : HCs;     //! Hit channel
    uint8_t uWfmWords : WWs;       //! Total waveform points per hit
    uint32_t uFeeAccum : FAs;      //! FEE accumulator
    uint32_t uEmpty0 : E0s;        //! Empty bits
    uint32_t uSignalCharge : SCs;  //! Waveform integral above ZeroLevel
    uint16_t uZeroLevel : ZLs;     //! Waveform ZeroLevel

    void printout()
    {
      printf("hit channel: %u; waveform words: %u; fee accumulator: %u; signal charge: %u; zero level: %u\n",
             uHitChannel, uWfmWords, uFeeAccum, uSignalCharge, uZeroLevel);
    }

    void clear()
    {
      uHitChannel   = 0;
      uWfmWords     = 0;
      uFeeAccum     = 0;
      uEmpty0       = 0;
      uSignalCharge = 0;
      uZeroLevel    = 0;
    }

    PsdHitHeader() { clear(); }

  };  //PsdHitHeader;


  struct PsdHitData {
    enum bitFieldSizes
    {
      E0s = 16,  //! Empty bits size in bits
      WPs = 16   //! Waveform point size in bits
    };

    uint16_t uEmpty0 : E0s;      //! Empty bits
    std::vector<uint16_t> uWfm;  //! Waveform vector

    void printout()
    {
      printf("waveform: ");
      for (uint8_t iter = 0; iter < uWfm.size(); iter++)
        printf("%u ", uWfm.at(iter));
      printf("\n");
    }

    void clear()
    {
      uEmpty0 = 0;
      uWfm.clear();
    }

    PsdHitData() { clear(); }

  };  //PsdHitData;


  struct PsdMsTrailer {
    enum bitFieldSizes
    {
      E0s = 64,  //! Empty bits size in bits
    };

    uint64_t uEmpty0 : E0s;  //! Empty bits

    void printout() { printf("trailer: %llu\n", static_cast<long long unsigned int>(uEmpty0)); }

    void clear() { uEmpty0 = 0; }

    PsdMsTrailer() { clear(); }

  };  //PsdMsTrailer;


}  // namespace PsdDataV100


#endif /* PSD_GBT_DATA_FORMAT_V100_H_ */
