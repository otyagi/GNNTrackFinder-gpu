/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */


#ifndef ALGO_BASE_MAINCONFIG_H_
#define ALGO_BASE_MAINCONFIG_H_

#include "CbmDefs.h"

#include <map>

namespace cbm::algo
{

  /** @class MainConfig
   ** @brief Configuration of online data processing
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 10 July 2023
   **/
  class MainConfig {
   public:  // methods
    /** @brief Constructor **/
    MainConfig() = default;

    /** @brief Constructor **/
    ~MainConfig() = default;

    /** @brief Load from YAML file **/
    void LoadYaml(const std::string& filename);

    /** @brief Save to YAML file **/
    void SaveYaml(const std::string& filename);

   public:  // data members
    // --- Digi trigger
    ECbmModuleId fTriggerDet = ECbmModuleId::kNotExist;  // Trigger detector
    double fTriggerWin       = 0.;                       // Trigger window size [ns]
    size_t fTriggerThreshold = 0;                        // Minimum number if digis in trigger window
    double fTriggerDeadTime  = 0.;                       // Minimal time between two trigger [ns]

    // --- Event builder: (detector -> (tMin, tMax))
    std::map<ECbmModuleId, std::pair<double, double>> fEvtbuildWindows = {};

    // --- Event selector
    size_t fSelectMinStationsSts = 0;
    size_t fSelectMinStationsTof = 0;
    size_t fSelectMinDigisBmon   = 0;

    // --- Branch persistence in output file
    bool fStoreTimeslice = false;
    bool fStoreTrigger   = false;
    bool fStoreEvents    = false;

    // --- QA publishing
    int32_t fHttpServerRefreshRate = 100;
  };

} /* namespace cbm::algo */

#endif /* ALGO_BASE_MAINCONFIG_H_ */
