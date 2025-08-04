/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: David Gutierrez [committer], Dario Ramirez */

#include "Definitions.h"
#include "detectors/sts/StsRecoUtils.h"

#include <array>
#include <optional>

#include <gtest.h>
#include <gtest/gtest-death-test.h>
#include <gtest/gtest.h>

#define EXPECT_OPT(maybe_val1, val2)                                                                                   \
  ASSERT_TRUE(maybe_val1.has_value());                                                                                 \
  EXPECT_EQ(*maybe_val1, val2)

using namespace cbm::algo;
using namespace cbm::algo::sts;

TEST(_ChannelMapping, FailOnValueLimits)
{
  EXPECT_EQ(Module::ChannelInModule(Module::CHANNELS_PER_ASIC + 1, 0), std::nullopt);
  EXPECT_EQ(Module::ChannelInModule(0, Module::ASICS_PER_MODULE + 1), std::nullopt);
}

TEST(_ChannelMapping, ReverseFailOnValueLimits)
{
  EXPECT_EQ(Module::ChannelInAsic(Module::CHANNELS_PER_MODULE + 1), std::nullopt);
}

TEST(_ChannelMapping, ModuleEdgesMapping)
{
  EXPECT_OPT(Module::ChannelInModule(0, 0), 0);
  EXPECT_OPT(Module::ChannelInModule(127, 7), 1023);
  EXPECT_OPT(Module::ChannelInModule(127, 8), 2046);
  EXPECT_OPT(Module::ChannelInModule(125, 8), 1024);
  EXPECT_OPT(Module::ChannelInModule(0, 15), 2047);
}

TEST(_ChannelMapping, ReverseModuleEdgesMapping)
{
  {
    const auto expect = std::make_pair<u16, u16>(0, 0);
    EXPECT_OPT(Module::ChannelInAsic(0), expect);
  }
  {
    const auto expect = std::make_pair<u16, u16>(127, 7);
    EXPECT_OPT(Module::ChannelInAsic(1023), expect);
  }
  {
    const auto expect = std::make_pair<u16, u16>(127, 8);
    EXPECT_OPT(Module::ChannelInAsic(2046), expect);
  }
  {
    const auto expect = std::make_pair<u16, u16>(125, 8);
    EXPECT_OPT(Module::ChannelInAsic(1024), expect);
  }
  {
    const auto expect = std::make_pair<u16, u16>(0, 15);
    EXPECT_OPT(Module::ChannelInAsic(2047), expect);
  }
}

TEST(_ChannelMapping, ExaustiveMapping)
{
  for (u16 channel = 0; channel < Module::CHANNELS_PER_MODULE; channel++) {
    const auto maybe_hw = Module::ChannelInAsic(channel);
    ASSERT_TRUE(maybe_hw.has_value());

    const auto maybe_sh = Module::ChannelInModule(maybe_hw->first, maybe_hw->second);
    EXPECT_OPT(maybe_sh, channel);
  }
}

TEST(_ChannelMapping, Compatibility)
{
  for (u16 asic = 0; asic < Module::ASICS_PER_MODULE / 2; asic++) {  // n side
    for (u16 chn = 0; chn < Module::CHANNELS_PER_ASIC; chn++) {
      const auto legacy   = asic * Module::CHANNELS_PER_ASIC + chn;
      const auto maybe_sw = Module::ChannelInModule(chn, asic);
      EXPECT_OPT(maybe_sw, legacy);
    }
  }

  for (u16 asic = Module::ASICS_PER_MODULE / 2; asic < Module::ASICS_PER_MODULE; asic++) {  // p side
    for (u16 chn = 0; chn < Module::CHANNELS_PER_ASIC; chn += 2) {                          // even channels
      const auto legacy   = (asic + 1) * Module::CHANNELS_PER_ASIC - chn - 1;
      const auto maybe_sw = Module::ChannelInModule(chn, asic);
      EXPECT_OPT(maybe_sw, legacy);
    }

    for (u16 chn = 1; chn < Module::CHANNELS_PER_ASIC; chn += 2) {  // odd channels
      const auto legacy   = (asic + 1) * Module::CHANNELS_PER_ASIC - chn - 1;
      const auto maybe_sw = Module::ChannelInModule(chn, asic);

      if ((asic == 8) && (chn == 127)) {  // z-strip
        EXPECT_OPT(maybe_sw, 2046);
        EXPECT_EQ(1024, legacy);
      }
      else {
        EXPECT_OPT(maybe_sw, legacy - 2);
      }
    }
  }
}

TEST(_ChannelMapping, Completness)
{
  std::array<bool, Module::CHANNELS_PER_MODULE> visited = {false};

  for (u16 asic = 0; asic < Module::ASICS_PER_MODULE; asic++) {
    for (u16 chn = 0; chn < Module::CHANNELS_PER_ASIC; chn++) {
      const auto maybe_sw = Module::ChannelInModule(chn, asic);
      ASSERT_TRUE(maybe_sw.has_value());
      ASSERT_LT(*maybe_sw, visited.size());
      visited[*maybe_sw] = true;
    }
  }

  for (const auto chn : visited) {
    EXPECT_TRUE(chn);
  }
}
