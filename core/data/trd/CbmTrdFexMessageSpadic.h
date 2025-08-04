/* Copyright (C) 2024 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: David Schledt [committer] */

#ifndef CbmTrdFexMessageSpadic_H
#define CbmTrdFexMessageSpadic_H

#include "fmt/format.h"

#include <array>
#include <stdexcept>


namespace Spadic
{

  template<std::uint8_t sys_ver>
  constexpr std::uint8_t BytesPerWord()
  {
    switch (sys_ver) {
      case 0x01: return 8;
      case 0x10: return 8;
      default: return 0;
    }
  }

  template<size_t bytes>
  struct NByteContainer {
    std::uint8_t b[bytes];

    operator std::uint64_t()
    {
      std::uint64_t r;
      memcpy(&r, b, sizeof(long));
      return r;
    }

    size_t range(std::uint8_t h, std::uint8_t l)
    {

      if (l > h) {
        throw std::invalid_argument(fmt::format("Reverse ranges not supported : low={:#x} > high={:#x}", l, h));
      }
      // get high and low byte positions
      std::uint16_t bh = h >> 3;
      std::uint16_t bl = l >> 3;
      // get number of bytes that need to be accessed
      std::uint16_t rbytes = bh - bl;
      if ((bh + 1) > bytes) {
        throw std::invalid_argument(fmt::format("High value: out-of-range, got={:#x}, size={:#x}", h, (bytes * 8 - 1)));
      }
      if ((bl + 1) > bytes) {
        throw std::invalid_argument(fmt::format("Low value: out-of-range, got={:#x}, size={:#x}", l, (bytes * 8 - 1)));
      }
      // calculate bit positions for high and low byte
      std::uint16_t bits_in_h = h - (8 * bh - 1);
      std::uint16_t bits_in_l = l - (8 * bl);
      std::uint16_t mask_h    = (1 << bits_in_h) - 1;

      // assemble return value
      size_t r = b[bl] >> bits_in_l;
      if (bh != bl) r |= (b[bh] & mask_h) << (rbytes * 8);
      for (int i = 1; i < rbytes; ++i)
        r |= b[bl + i] << (i * 8);
      return r;
    }
  };

  // Define template specialisations for each system version
  template<std::uint8_t sys_ver>
  struct FexWord {
  };


  // sys_ver 0x10 (c.f.git.cbm.gsi.de/trd/reports/technical-notes/integrated-data-format)
  template<>
  struct FexWord<0x10> {
    std::uint8_t elink;
    std::uint8_t channel;
    std::uint64_t timestamp;
    float prec_time;
    std::uint16_t maxAdc;
    std::uint16_t timesample;
    std::uint8_t iMA;
    std::uint8_t ht;
    std::uint8_t mh;

    FexWord(NByteContainer<BytesPerWord<0x10>()> word)
    {
      constexpr int hts  = 62;
      constexpr int els  = hts - 8;
      constexpr int chs  = els - 4;
      constexpr int maxs = chs - 9;
      constexpr int imas = maxs - 3;
      constexpr int trs  = imas - 9;
      constexpr int mhs  = trs - 3;
      constexpr int pts  = mhs - 5;

      std::uint64_t w = word;

      ht = (word >> 62) & 0x3;  // TODO ext trigger
      if (ht != 0) {
        elink      = (w >> els) & 0xff;
        channel    = (w >> chs) & 0xf;
        maxAdc     = (w >> maxs) & 0x1ff;
        iMA        = (w >> imas) & 0x3;
        timesample = (w >> trs) & 0x1ff;
        mh         = (w >> mhs) & 0x3;
        prec_time  = (float) ((w >> pts) & 0x1f) / float(1 << 4);
        timestamp  = w & 0x1fffff;
      }
    }
  };

}  // namespace Spadic

#endif  // CbmTrdFexMessageSpadic_H
