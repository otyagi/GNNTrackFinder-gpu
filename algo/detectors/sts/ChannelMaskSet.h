/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#pragma once

#include "Definitions.h"
#include "compat/Filesystem.h"
#include "yaml/Property.h"
#include "yaml/Yaml.h"

#include <map>
#include <set>

namespace cbm::algo::sts
{

  struct ChannelMaskSet {

    struct MaskedChannels {
      std::set<u16> channels;

      bool Contains(u16 channel) const { return channels.count(channel) > 0; }

      MaskedChannels() = default;

      MaskedChannels(std::initializer_list<u16> chans) : channels(chans) {}

      CBM_YAML_FORMAT(YAML::Flow);
      CBM_YAML_MERGE_PROPERTY();
      CBM_YAML_PROPERTIES(yaml::Property(&MaskedChannels::channels, "channels", "Channel mask", YAML::Flow));
    };

    // febId -> set of channel numbers
    std::map<size_t, MaskedChannels> values;

    /**
      * @brief Construct emtpy mapping
      */
    ChannelMaskSet()  = default;
    ~ChannelMaskSet() = default;

    CBM_YAML_MERGE_PROPERTY();
    CBM_YAML_PROPERTIES(yaml::Property(&ChannelMaskSet::values, "maskSet", "Channel mask set"));
  };

}  // namespace cbm::algo::sts

CBM_YAML_EXTERN_DECL(cbm::algo::sts::ChannelMaskSet);
