/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#pragma once

#include "CbmDefs.h"

#include <map>

#include <yaml-cpp/yaml.h>


namespace cbm::algo::evbuild
{

  /** @class V0TriggerConfig
   ** @brief Configuration of the V0 trigger class (trigger on displaced vertices)
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 19.04.2024
   **/
  class V0TriggerConfig {

   public:
    /** @brief Default constructor **/
    V0TriggerConfig(){};

    /** @brief Constructor with parameters
     ** @param cutTime  Maximum time difference of tracks [ns]
     ** @param cutZ     Minimum z position at closest approach
     ** @param cutDist  Maximum distance at closest approach
     **/
    /*
    V0TriggerConfig(double cutTime, double cutZ, double cutDist)
      : fCutTime(cutTime)
      , fCutZ(cutZ)
      , fCutDist(cutDist)
      , fIsSet(true)
    {
    }
    */

    /** @brief Constructor from YAML **/
    V0TriggerConfig(YAML::Node config);

    /** @brief Destructor **/
    ~V0TriggerConfig() = default;

    /** @brief Minimum z at first track measurement **/
    double TrackStartZ_min() const { return fTrackStartZ_min; }

    /** @brief Maximum z at first track measurement **/
    double TrackStartZ_max() const { return fTrackStartZ_max; }

    /** @brief Minimum z at last track measurement **/
    double TrackEndZ_min() const { return fTrackEndZ_min; }

    /** @brief Minimum x of track impact in target plane **/
    double TrackImpactX_min() const { return fTrackImpactX_min; }

    /** @brief Maximum x of track impact in target plane **/
    double TrackImpactX_max() const { return fTrackImpactX_max; }

    /** @brief Minimum y of track impact in target plane **/
    double TrackImpactY_min() const { return fTrackImpactY_min; }

    /** @brief Maximum y of track impact in target plane **/
    double TrackImpactY_max() const { return fTrackImpactY_max; }

    /** @brief Maximum time difference of tracks **/
    double PairDeltaT_max() const { return fPairDeltaT_max; }

    /** @brief Maximum distance at closest approach **/
    double PairDist_max() const { return fPairDist_max; }

    /** @brief Minimum z of PCA **/
    double PairZ_min() const { return fPairZ_min; }

    /** @brief Maximum z of PCA **/
    double PairZ_max() const { return fPairZ_max; }

    /** @brief Check whether config was set **/
    bool IsSet() const { return fIsSet; }

    /** @brief Save to YAML **/
    YAML::Node ToYaml() const;


   private:
    // Track cuts
    double fTrackStartZ_min{0.};   /// Minimum z at first track measurement
    double fTrackStartZ_max{0.};   /// Maximum z at first track measurement
    double fTrackEndZ_min{0.};     /// Minimum z at last track measurement
    double fTrackImpactX_min{0.};  /// Minimum x of excluded track impact in target plane
    double fTrackImpactX_max{0.};  /// Maximum x of excluded track impact in target plane
    double fTrackImpactY_min{0.};  /// Minimum y of excluded track impact in target plane
    double fTrackImpactY_max{0.};  /// Maximum y of excluded track impact in target plane

    // Track pair cuts
    double fPairDeltaT_max{0.};  /// Maximum time difference of tracks
    double fPairDist_max{0.};    /// Maximum distance at closest approach
    double fPairZ_min{0.};       /// Minimum z of PCA
    double fPairZ_max{0.};       /// Maximum z of PCA

    bool fIsSet{false};  /// Flag whether a configuration was set

    double GetDoubleRequired(const YAML::Node& config, const char* key);
  };


}  // namespace cbm::algo::evbuild
