/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: David Gutierrez [committer], Dario Ramirez */

#pragma once

#include "Definitions.h"

#include <cassert>
#include <cstdlib>
#include <optional>

namespace cbm::algo::sts
{
  /** @brief STS Module parameters
  **/
  class Module {
   public:
    static constexpr u16 CHANNELS_PER_ASIC   = 128;
    static constexpr u16 ASICS_PER_MODULE    = 16;
    static constexpr u16 CHANNELS_PER_MODULE = CHANNELS_PER_ASIC * ASICS_PER_MODULE;

    static_assert(CHANNELS_PER_ASIC > 0, "ASICs must have at least one chanhel.");
    static_assert(ASICS_PER_MODULE > 0, "Modules must have at least one asic.");

    static constexpr u16 ZSTRIP_ASIC       = 8;
    static constexpr u16 ZSTRIP_HW_CHANNEL = 127;
    static constexpr u16 ZSTRIP_SW_CHANNEL = 2046;
    static constexpr u16 ZSTRIP_OFFSET     = ZSTRIP_SW_CHANNEL - 1022;

    /** @brief Channel HW to SW conversion
     ** @param  elink_chn       Channel number in elink (HW)
     ** @param  asic_idx        ASIC index in module
     ** @return channel         Channel number in SW
     **/
    static inline std::optional<u16> ChannelInModule(const u16 elink_chn, const u16 asic_idx)
    {
      if (elink_chn >= CHANNELS_PER_ASIC || asic_idx >= ASICS_PER_MODULE) return std::nullopt;

      const u16 channel =
        CHANNELS_PER_ASIC * asic_idx + elink_chn  // n side
        + (2 * asic_idx >= ASICS_PER_MODULE)
            * (                                      // p side
              CHANNELS_PER_ASIC - 1 - 2 * elink_chn  // revert channels in asic
              - (elink_chn & 0b1) * 2                // shift odd channels to the left
              + (elink_chn == ZSTRIP_HW_CHANNEL && asic_idx == ZSTRIP_ASIC) * ZSTRIP_OFFSET  // correct z-strip 1
            );

      return (channel < CHANNELS_PER_MODULE) ? std::make_optional(channel) : std::nullopt;
    }

    /** @brief Channel SW to HW conversion
     ** @param  channel             Channel number in module (SW)
     ** @param  chan_per_asic       Channels per ASIC
     ** @param  asic_per_module     ASICs per module
     ** @return elink_chn, asic_idx Channel number in elink (HW) and asic ASIC in module
     **/
    static inline std::optional<std::pair<u16, u16>> ChannelInAsic(const u16 channel)
    {
      if (channel >= CHANNELS_PER_MODULE) return std::nullopt;

      auto [asic_idx, elink_chn] =
        std::div(channel  // n side
                   + (2 * channel >= CHANNELS_PER_MODULE)
                       * (                                                // p side
                         -(channel == ZSTRIP_SW_CHANNEL) * ZSTRIP_OFFSET  // correct z-strip 1
                         + (!(channel & 0b1)) * 2                         // shift even channels to the right
                         ),
                 CHANNELS_PER_ASIC);

      elink_chn += (2 * asic_idx >= ASICS_PER_MODULE)
                   * (                                      // p side
                     CHANNELS_PER_ASIC - 1 - 2 * elink_chn  // revert channels in asic
                   );

      return (asic_idx < ASICS_PER_MODULE || elink_chn < CHANNELS_PER_ASIC)
               ? std::make_optional(std::make_pair(elink_chn, asic_idx))
               : std::nullopt;
    }
  };
}  // namespace cbm::algo::sts
