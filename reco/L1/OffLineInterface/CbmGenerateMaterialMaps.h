/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmGenerateMaterialMaps.h
/// \brief  A FAIR-task to generate material maps (header)
/// \since  18.02.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "FairTask.h"
#include "KfMaterialMap.h"
#include "KfMaterialMapFactory.h"
#include "yaml/Yaml.h"

#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

/// \class CbmGenerateMaterialMaps
/// \brief Steer class for executing the material budget maps generator independently from tracking
class CbmGenerateMaterialMaps : public FairTask {
  using MaterialMap = cbm::algo::kf::MaterialMap;

  /// \struct MaterialSlice
  /// \brief  Input parameters for the material map generation
  struct MaterialSlice {
    std::string fName;
    double fRefZ;   ///< Reference z-coordinate [cm]
    double fMinZ;   ///< Lower bound along z-axis [cm]
    double fMaxZ;   ///< Upper bound along z-axis [cm]
    double fMaxXY;  ///< Size in the transverse plane [cm]

    bool operator<(const MaterialSlice& r) const { return (fRefZ < r.fRefZ); }

    CBM_YAML_PROPERTIES(cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::MaterialSlice::fName, "name", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::MaterialSlice::fRefZ, "ref_z", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::MaterialSlice::fMinZ, "min_z", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::MaterialSlice::fMaxZ, "max_z", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::MaterialSlice::fMaxXY, "max_xy", ""));
  };

  struct Config {
    std::vector<MaterialSlice> fvUserSlices;  ///< Material slices defined by user

    double fPitch           = 0.1;    ///< Minimal bin size (cm)
    int fMaxNofBins         = 100;    ///< Number of bins in material budged map (x and y axes)
    int fNofRays            = 3;      ///< Number of rays per dimension in each bin
    bool fbParallelRays     = false;  ///< Rays mode (false - radial, true - parallel to z-axis)
    bool fbTrackingStations = false;  ///< Generates material maps for the actual geometry stations
    bool fbSafeMaterialInit = true;   ///< Safe material initialization (takes extra computational time)

    CBM_YAML_PROPERTIES(cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::Config::fvUserSlices, "user_slices", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::Config::fPitch, "pitch", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::Config::fMaxNofBins, "max_nof_bins", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::Config::fNofRays, "nof_rays", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::Config::fbParallelRays, "parallel_rays", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::Config::fbTrackingStations,
                                               "tracking_stations", ""),
                   cbm::algo::yaml::Property(&CbmGenerateMaterialMaps::Config::fbSafeMaterialInit,
                                               "safe_material_init", ""));
  };

 public:
  /// \brief Default constructor
  CbmGenerateMaterialMaps();

  /// \brief Constructor from parameters
  /// \param name    Name of the task
  /// \param verbose Verbosity level
  CbmGenerateMaterialMaps(const char* name, int verbose);

  /// \brief Destructor
  virtual ~CbmGenerateMaterialMaps() = default;

  /// \brief Copy constructor
  CbmGenerateMaterialMaps(const CbmGenerateMaterialMaps&) = delete;

  /// \brief Move constructor
  CbmGenerateMaterialMaps(CbmGenerateMaterialMaps&&) = delete;

  /// \brief Copy assignment operator
  CbmGenerateMaterialMaps& operator=(const CbmGenerateMaterialMaps&) = delete;

  /// \brief Move assignment operator
  CbmGenerateMaterialMaps& operator=(CbmGenerateMaterialMaps&&) = delete;

  /// \brief Initializer
  InitStatus Init();

  /// \brief Re-initializer
  InitStatus ReInit();

  /// \brief Finish function
  void Finish();

  /// \brief Sets output file name
  void SetOutputName(const char* fileName) { fsOutputFile = fileName; }

  /// \brief Sets user configuration file
  void SetUserConfig(const char* userConfig) { fsUserConfig = userConfig; }

  /// \brief Gets material map
  const std::map<std::string, MaterialMap>& GetMaterial() const { return fmMaterial; }

 private:
  /// \brief Writes material budget maps to file
  ///
  /// The material maps are represented with ROOT histograms. The value of the material thickness
  /// is represented in units of radiational length
  void WriteMaterialMaps();

  static constexpr double kXYoffset     = 1.3;  /// Offset from station boarders [scale]
  static constexpr double kTargetOffset = 1.;   /// Offset from target to start generating mat. maps [cm]

  Config fConfig;                                                               ///< Configuration for the task
  std::string fsUserConfig = "";                                                ///< User configuration file
  std::string fsOutputFile = "matBudget.root";                                  ///< Output file name
  std::map<std::string, MaterialMap> fmMaterial;                                ///< Material budget maps
  std::unique_ptr<::kf::tools::MaterialMapFactory> fpMaterialFactory{nullptr};  ///< Material factory instance

  double fTargetZ;  ///< z-coordinate of the target center

  ClassDef(CbmGenerateMaterialMaps, 0);
};
