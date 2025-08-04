/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBM_ALGO_EVBUILD_DIGITRIGGERCONFIG_H
#define CBM_ALGO_EVBUILD_DIGITRIGGERCONFIG_H 1

#include "CbmDefs.h"

#include <map>

#include <yaml-cpp/yaml.h>


namespace cbm::algo::evbuild
{

  /** @class DigiTriggerConfig
   ** @brief Configuration of the digi trigger class (TimeClusterTrigger)
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 21.11.2021
   **/
  class DigiTriggerConfig {

   public:
    /** @brief Constructor with parameters
     ** @param detector  Detector to be used for triggering
     ** @param window    Time interval to look for clusters in [ns]
     ** @param threshold Minimal required number of data within the time window
     ** @param deadTime  Minimal time between two subsequent triggers
     **/
    DigiTriggerConfig(ECbmModuleId detector, double window, size_t threshold, double deadTime)
      : fDetector(detector)
      , fWindow(window)
      , fThreshold(threshold)
      , fDeadTime(deadTime)
      , fIsSet(true)
    {
    }

    /** @brief Constructor from YAML **/
    DigiTriggerConfig(YAML::Node config);

    /** @brief Destructor **/
    ~DigiTriggerConfig() = default;

    /** @brief Trigger dead time **/
    double DeadTime() const { return fDeadTime; }

    /** @brief Trigger detector **/
    ECbmModuleId Detector() const { return fDetector; }

    /** @brief Check whether config was set **/
    bool IsSet() const { return fIsSet; }

    /** @brief Trigger threshold **/
    size_t Threshold() const { return fThreshold; }

    /** @brief Save to YAML **/
    YAML::Node ToYaml() const;

    /** @brief Trigger window **/
    double Window() const { return fWindow; }


   private:
    ECbmModuleId fDetector;  ///< Trigger detector
    double fWindow;          ///< Trigger window size [ns]
    size_t fThreshold;       ///< Minimum number if digis in trigger window
    double fDeadTime;        ///< Minimal time between two trigger [ns]
    bool fIsSet{false};      ///< Flag config being properly set
  };


}  // namespace cbm::algo::evbuild

#endif /* CBM_ALGO_EVBUILD_DIGITRIGGERCONFIG_H */
