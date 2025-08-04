/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file Config.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 31.05.2023
 **/

#ifndef CBM_RECO_OFFLINE_STEER_CONFIG_H
#define CBM_RECO_OFFLINE_STEER_CONFIG_H 1

#include "CbmAlgoBuildRawEvents.h"
#include "CbmDefs.h"

#include <array>
#include <fstream>
#include <sstream>
#include <string>

#include <yaml-cpp/yaml.h>

namespace cbm::reco::offline
{

  enum class ECbmEvbuildType
  {
    Ideal,
    Real,
    Undefined
  };


  /** @class Config
   ** @author Volker Friese <v.friese@gsi.de>
   ** @date 31.05.2023
   **
   ** Configuration for the offline reconstruction chain, replacing run_reco.C.
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
    ECbmEvbuildType ToCbmEvbuildType(std::string tag);
    ECbmRecoMode ToCbmRecoMode(std::string tag);
    EOverlapModeRaw ToOverlapModeRaw(std::string tag);
    std::string ToString(ECbmEvbuildType type);
    std::string ToString(ECbmRecoMode mode);
    std::string ToString(EOverlapModeRaw mode);

    /** @brief Save to YAML node **/
    YAML::Node ToYaml();


   public:
    // --- Global settings
    std::string f_glb_logLevel   = "INFO";
    std::string f_glb_logVerbose = "LOW";
    std::string f_glb_logColor   = "true";
    ECbmRecoMode f_glb_mode      = ECbmRecoMode::Undefined;
    Int_t f_glb_numTs            = -1;
    Int_t f_glb_firstTs          = 0;

    // --- Event builder
    ECbmEvbuildType f_evbuild_type      = ECbmEvbuildType::Undefined;
    EOverlapModeRaw f_evbuild_ovlapmode = EOverlapModeRaw::Undefined;
    ECbmModuleId f_evbuild_trigDet      = ECbmModuleId::kNotExist;
    int64_t f_evbuild_trigNumMin        = 0;
    int64_t f_evbuild_trigNumMax        = 0;
    float f_evbuild_trigWinMin          = std::nanf("undefined");
    float f_evbuild_trigWinMax          = std::nanf("undefined");

    // --- STS
    bool f_sts_usegpu = false;

    // --- TRD
    float f_trd_trigThresh = std::nanf("undefined");

    // --- littrack
    std::string f_lit_trackType = "";
    std::string f_lit_mergeType = "";
  };

}  // namespace cbm::reco::offline


#endif /* CBM_RECO_OFFLINE_STEER_CONFIG_H */
