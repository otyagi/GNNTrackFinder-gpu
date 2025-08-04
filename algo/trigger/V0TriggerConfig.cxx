/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "V0TriggerConfig.h"


namespace cbm::algo::evbuild
{

  // -----   Constructor from YAML   ------------------------------------------
  V0TriggerConfig::V0TriggerConfig(YAML::Node config)
  {
    if (!config) {
      fIsSet = false;
      return;
    }
    fTrackStartZ_min  = GetDoubleRequired(config, "track_start_z_min");
    fTrackStartZ_max  = GetDoubleRequired(config, "track_start_z_max");
    fTrackEndZ_min    = GetDoubleRequired(config, "track_end_z_min");
    fTrackImpactX_min = GetDoubleRequired(config, "track_impact_x_min");
    fTrackImpactX_max = GetDoubleRequired(config, "track_impact_x_max");
    fTrackImpactY_min = GetDoubleRequired(config, "track_impact_y_min");
    fTrackImpactY_max = GetDoubleRequired(config, "track_impact_y_max");
    fPairDeltaT_max   = GetDoubleRequired(config, "pair_deltaT_max");
    fPairDist_max     = GetDoubleRequired(config, "pair_dist_max");
    fPairZ_min        = GetDoubleRequired(config, "pair_z_min");
    fPairZ_max        = GetDoubleRequired(config, "pair_z_max");
    fIsSet            = true;
  }
  // --------------------------------------------------------------------------


  // -----   Geta a required double value from YAML   -------------------------
  double V0TriggerConfig::GetDoubleRequired(const YAML::Node& config, const char* key)
  {
    auto value = config[key];
    if (!value) throw std::runtime_error(key + std::string(" required but not defined"));
    return value.as<double>();
  }
  // --------------------------------------------------------------------------


  // -----   Save to YAML   ---------------------------------------------------
  YAML::Node V0TriggerConfig::ToYaml() const
  {
    YAML::Node result;
    result["track_start_z_min"]  = fTrackStartZ_min;
    result["track_start_z_max"]  = fTrackStartZ_max;
    result["track_end_z_min"]    = fTrackEndZ_min;
    result["track_impact_x_min"] = fTrackImpactX_min;
    result["track_impact_x_max"] = fTrackImpactX_max;
    result["track_impact_y_min"] = fTrackImpactY_min;
    result["track_impact_y_max"] = fTrackImpactY_max;
    result["pair_deltaT_max"]    = fPairDeltaT_max;
    result["pair_dist_max"]      = fPairDist_max;
    result["pair_z_min"]         = fPairZ_min;
    result["pair_z_max"]         = fPairZ_max;
    return result;
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::evbuild
