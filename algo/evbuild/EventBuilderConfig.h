/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBM_ALGO_EVBUILD_EVENTBUILDERCONFIG_H
#define CBM_ALGO_EVBUILD_EVENTBUILDERCONFIG_H 1

#include "CbmDefs.h"

#include <map>

#include <yaml-cpp/yaml.h>


namespace cbm::algo::evbuild
{

  /** @class EventBuilderConfig
   ** @brief Configuration of the EventBuilder class
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 12.11.2021
   **
   ** Holds time windows around the trigger time in which digis are associated to an event.
   **/
  class EventBuilderConfig {

   public:
    /** @brief Constructor from YAML **/
    EventBuilderConfig(YAML::Node config);

    /** @brief Save to YAML **/
    YAML::Node ToYaml() const;


   public:
    std::map<ECbmModuleId, std::pair<double, double>> fWindows;  ///< Key: detector; value: [tmin, tmax]
  };


}  // namespace cbm::algo::evbuild

#endif /* CBM_ALGO_EVBUILD_EVENTBUILDERCONFIG_H */
