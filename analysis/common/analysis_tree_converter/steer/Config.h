/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file Config.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 30.10.2023
 **/

#ifndef CBM_ATCONVERTER_STEER_CONFIG_H
#define CBM_ATCONVERTER_STEER_CONFIG_H 1

#include "CbmDefs.h"

#include <array>
#include <fstream>
#include <sstream>
#include <string>

#include <yaml-cpp/yaml.h>

namespace cbm::atconverter
{

  enum class ECbmEvbuildType
  {
    Ideal,
    Real,
    Undefined
  };


  /** @class Config
   ** @author Frederic Linz <f.linz@gsi.de>
   ** @date 30.10.2023
   **
   ** Configuration for the analysis tree converter, replacing run_analysis_tree_maker_json_config.C.
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
    ECbmRecoMode ToCbmRecoMode(std::string tag);
    std::string ToString(ECbmRecoMode mode);

    /** @brief Save to YAML node **/
    YAML::Node ToYaml();


  public:
    // --- Global settings
    std::string f_glb_logLevel   = "INFO";
    std::string f_glb_logVerbose = "LOW";
    std::string f_glb_logColor   = "true";
    ECbmRecoMode f_glb_mode      = ECbmRecoMode::Undefined;
    int f_glb_numTs              = -1;
    int f_glb_firstTs            = 0;
    float f_glb_tslength         = 1.e5;

    std::string f_glb_system = "";
    float f_glb_beamMom      = 12.;

    bool f_glb_trackMatching = true;

    // --- Event builder
    ECbmEvbuildType f_evbuild_type = ECbmEvbuildType::Undefined;

    // --- Fsd hits converter settings
    float f_fsd_minChi2match = -1.;
    float f_fsd_maxChi2match = 10000.;
  };

}  // namespace cbm::atconverter


#endif /* CBM_ATCONVERTER_STEER_CONFIG_H */
