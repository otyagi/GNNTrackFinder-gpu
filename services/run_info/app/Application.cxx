/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Application.cxx
/// \brief  The application class for the run_info service (implementation)
/// \since  24.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "Application.h"

#include "CbmEnumArray.h"
#include "CbmMcbmUtils.h"

#include <boost/program_options.hpp>

#include <exception>
#include <iostream>
#include <sstream>

using cbm::services::run_info::Application;
using cbm::services::run_info::EInfo;

namespace po = boost::program_options;


// ---------------------------------------------------------------------------------------------------------------------
//
std::optional<EInfo> Application::ParseOptions(int argc, char* argv[])
{
  cbm::core::EnumArray<EInfo, bool> vbSelected{{0}};

  // ----- Define options
  po::options_description opts("  Options");
  auto optsAdd = opts.add_options();
  // Default options
  optsAdd("help,h", "display this help and exit");

  // Required options
  optsAdd("r,run", po::value<uint32_t>(&fRunId)->required(), "standard CBM run identifier (required)");

  // Info selection options
  optsAdd("g,geotag", po::bool_switch(&vbSelected[EInfo::GeoTag])->default_value(false), "prints the geometry tag");
  optsAdd("p,print", po::bool_switch(&vbSelected[EInfo::Print])->default_value(false),
          "prints available information on the run ID");

  po::variables_map vars;
  po::store(po::parse_command_line(argc, argv, opts), vars);
  po::notify(vars);

  if (vars.count("help")) {
    std::cout << opts << std::endl;
    return std::nullopt;
  }

  int nSelectedInfos{0};
  EInfo selectedInfo{EInfo::Print};
  for (size_t iInfo = 0; iInfo < vbSelected.size(); ++iInfo) {
    if (vbSelected[iInfo]) {
      selectedInfo = static_cast<EInfo>(iInfo);
      ++nSelectedInfos;
    }
  }
  if (nSelectedInfos > 1) {
    throw std::logic_error("More then one information items were requested");
  }

  return std::make_optional(selectedInfo);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Application::Print(EInfo info) const
{
  switch (info) {
    case EInfo::GeoTag: std::cout << cbm::mcbm::GetSetupFromRunId(fRunId) << std::endl; break;
    case EInfo::Print: std::cout << GetRunInfo() << std::endl;
    case EInfo::END: break;
  };
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string Application::GetRunInfo() const
{
  std::stringstream msg;
  msg << "------ Run #" << fRunId << '\n';
  msg << "geometry tag: " << cbm::mcbm::GetSetupFromRunId(fRunId);
  return msg.str();
}
