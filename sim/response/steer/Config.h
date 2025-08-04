/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file Config.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 09.01.2024
 **/

#ifndef CBMSIM_DIGITIZATION_STEER_CONFIG_H
#define CBMSIM_DIGITIZATION_STEER_CONFIG_H 1

#include "CbmDefs.h"
#include "CbmDigitization.h"

#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace cbm::sim::digitization
{

  /** @class Config
   ** @author Frederic Linz <f.linz@gsi.de>
   ** @date 09.01.2024
   **
   ** Configuration for the analysis tree converter, replacing run_digi_json_config.C.
   ** With interfaces to YAML.
   **/
  class Config {
   public:
    /** @brief Constructor **/
    Config() = default;


    /** @brief Destructor **/
    virtual ~Config() = default;


    /** @brief Load from YAML file
     ** @param filename  Name of input YAML file
     **/
    void LoadYaml(const std::string& filename);


    /** @brief String output (YAML format) **/
    std::string ToString()
    {
      std::stringstream out;
      out << ToYaml();
      return out.str();
    }


    /** @brief Save to YAML file
     ** @param filename  Name of output YAML file
     **/
    void SaveYaml(const std::string& filename)
    {
      std::ofstream fout(filename);
      fout << ToYaml();
    }


   private:
    cbm::sim::Mode ToCbmDigiMode(std::string tag);
    std::string ToString(cbm::sim::Mode mode);

    cbm::sim::TimeDist ToCbmSimTimeDist(std::string tag);
    std::string ToString(cbm::sim::TimeDist dist);

    ECbmModuleId ToECbmModuleId(std::string moduleString);
    std::string ToString(ECbmModuleId moduleId);

    ECbmTreeAccess ToECbmTreeAccess(std::string treeAccessString);
    std::string ToString(ECbmTreeAccess treeAccessString);

    /** @brief Save to YAML node **/
    YAML::Node ToYaml();


   public:
    // --- Global settings
    std::string f_glb_logLevel   = "INFO";
    std::string f_glb_logVerbose = "LOW";
    std::string f_glb_logColor   = "true";
    cbm::sim::Mode f_glb_mode    = cbm::sim::Mode::Undefined;
    int f_glb_numTs              = -1;
    int f_glb_firstTs            = 0;

    // --- Detector selection
    ECbmModuleId f_det_deactivateAllBut = ECbmModuleId::kNotExist;
    std::vector<ECbmModuleId> f_det_deactivate;

    // --- Timeslice settings
    float f_ts_tslength  = 1.e5;
    float f_ts_startTime = 1000.;
    bool f_ts_storeAllTS = false;
    cbm::sim::TimeDist f_ts_timeDist =
      cbm::sim::TimeDist::Undefined;  // Event time distribution model: poisson or uniform

    // --- Background settings
    bool f_bg_produceNoise = false;

    // --- Input sources settings
    std::vector<int> f_src_id;
    std::vector<float> f_src_rate;
    std::vector<ECbmTreeAccess> f_src_treeAccessMode;
    std::vector<int> f_src_embedToId;
  };

}  // namespace cbm::sim::digitization


#endif /* CBMSIM_DIGITIZATION_STEER_CONFIG_H */
