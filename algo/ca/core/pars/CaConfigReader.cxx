/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   ConfigReader.h
/// \brief  Configuration parameter file reader/writer for L1 tracking algorithm (implementation)
/// \author S.Zharko <s.zharko@gsi.de>
/// \since  18.07.2022

#include "CaConfigReader.h"

#include "CaDefs.h"
#include "CaInitManager.h"

#include <boost/algorithm/string.hpp>

#include <iostream>
#include <numeric>
#include <sstream>
#include <unordered_map>

#include <yaml-cpp/yaml.h>


using cbm::algo::ca::ConfigReader;
using cbm::algo::ca::EDetectorID;
using cbm::algo::ca::InitManager;
using cbm::algo::ca::Iteration;

// ---------------------------------------------------------------------------------------------------------------------
//
ConfigReader::ConfigReader(InitManager* pInitManager, int verbose) : fpInitManager(pInitManager), fVerbose(verbose) {}

// ---------------------------------------------------------------------------------------------------------------------
//
// TODO: switch to the Yaml library by Felix
YAML::Node ConfigReader::GetNode(std::function<YAML::Node(YAML::Node)> fn, bool optional) const
{
  auto node = YAML::Node(YAML::NodeType::Undefined);
  if (fUserConfigNode) {
    node = fn(fUserConfigNode);
  }
  if (!node) {
    node = fn(fMainConfigNode);
  }
  if (!node && !optional) {
    std::stringstream msg;
    msg << "requested node was not found ";
    if (fUserConfigNode) {
      msg << "either in user config (path: " << fsUserConfigPath << ") or ";
    }
    msg << "in main config (path: " << fsMainConfigPath << ")";
    throw std::runtime_error(msg.str());
  }
  return node;
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::vector<std::string> ConfigReader::GetNodeKeys(const YAML::Node& node) const
{
  std::vector<std::string> res;
  res.reserve(node.size());
  try {
    for (const auto& item : node) {
      res.push_back(item.first.as<std::string>());
    }
  }
  catch (const YAML::InvalidNode& exc) {
    LOG(warn)
      << "L1 config: attempt to call ConfigReader::GetNodeKeys for node, keys of which could not be represented "
      << "with strings. An empty vector will be returned";
    std::vector<std::string>().swap(res);
  }
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ConfigReader::Read()
{
  // TODO: Remove fbGeometryLock as soon as the legacy geometry variables are removed from the ca::Parameters class
  if (!fbGeometryLock) {  // Unset inactive tracking stations
    if (fVerbose >= 1) {
      LOG(info) << "- disabling inactive tracking stations";
    }
    auto inactiveMap = this->ReadInactiveStationMap();

    if (std::any_of(inactiveMap.begin(), inactiveMap.end(), [](const auto& s) { return (s.size() != 0); })) {
      for (auto& station : fpInitManager->GetStationInfo()) {
        int iDet   = static_cast<int>(station.GetDetectorID());
        int iStLoc = station.GetStationID();
        if (inactiveMap[iDet].find(iStLoc) != inactiveMap[iDet].end()) {
          station.SetTrackingStatus(false);
        }
      }
      // Since we disabled some stations, we have to rerun the layout initialization again, thus the station scheme is
      // kept consistent
      fpInitManager->InitStationLayout();
    }
  }

  {  // Init CA iterations in L1InitManager
    if (fVerbose >= 1) {
      LOG(info) << "- reading track finder iterations";
    }
    auto iters = this->ReadCAIterationVector();
    assert(iters.size());
    fpInitManager->ClearCAIterations();
    fpInitManager->SetCAIterationsNumberCrosscheck(iters.size());
    std::for_each(iters.begin(), iters.end(), [&](auto& iter) { fpInitManager->PushBackCAIteration(iter); });
  }


  // Init parameters, independnent from the tracking iteration

  if (fVerbose >= 1) {
    LOG(info) << "- reading miscellaneous parameters";
  }

  fpInitManager->SetRandomSeed(
    GetNode([](YAML::Node n) { return n["core"]["common"]["random_seed"]; }).as<unsigned int>());
  fpInitManager->SetGhostSuppression(
    GetNode([](YAML::Node n) { return n["core"]["track_finder"]["is_ghost_suppression"]; }).as<bool>());
  fpInitManager->SetMaxDoubletsPerSinglet(
    GetNode([](YAML::Node n) { return n["core"]["track_finder"]["max_doublets_per_singlet"]; }).as<unsigned int>());
  fpInitManager->SetMaxTripletPerDoublets(
    GetNode([](YAML::Node n) { return n["core"]["track_finder"]["max_triplets_per_doublet"]; }).as<unsigned int>());

  ReadMisalignmentTolerance();

  if (fVerbose >= 1) {
    LOG(info) << "- reading developement parameters";
  }
  // Dev flags
  fpInitManager->DevSetIgnoreHitSearchAreas(
    GetNode([](YAML::Node n) { return n["core"]["dev"]["ignore_hit_search_areas"]; }).as<bool>());
  fpInitManager->DevSetUseOfOriginalField(
    GetNode([](YAML::Node n) { return n["core"]["dev"]["use_of_original_field"]; }).as<bool>());
  fpInitManager->DevSetIsMatchDoubletsViaMc(
    GetNode([](YAML::Node n) { return n["core"]["dev"]["match_doublets_via_mc"]; }).as<bool>());
  fpInitManager->DevSetIsMatchTripletsViaMc(
    GetNode([](YAML::Node n) { return n["core"]["dev"]["match_triplets_via_mc"]; }).as<bool>());
  fpInitManager->DevSetIsExtendTracksViaMc(
    GetNode([](YAML::Node n) { return n["core"]["dev"]["extend_tracks_via_mc"]; }).as<bool>());
  fpInitManager->DevSetIsSuppressOverlapHitsViaMc(
    GetNode([](YAML::Node n) { return n["core"]["dev"]["suppress_overlap_hits_via_mc"]; }).as<bool>());
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::vector<Iteration> ConfigReader::ReadCAIterationVector()
{
  std::vector<Iteration> res;

  // Read iterations store
  std::unordered_map<std::string, Iteration> mPossibleIterations;
  {
    auto currentNode = fMainConfigNode["possible_iterations"];
    assert(currentNode);
    for (const auto& iterNode : currentNode) {
      std::string thisIterName = iterNode["name"].as<std::string>("");
      std::string baseIterName = iterNode["base_iteration"].as<std::string>("");

      if (baseIterName.size()) {  // Create iteration from previously defined one
        if (mPossibleIterations.find(baseIterName) == mPossibleIterations.end()) {
          std::stringstream msg;
          msg << "A CA iteration \"" << thisIterName << "\" requires a base iteration with name \"" << baseIterName
              << "\", which was not registered yet. Please, place an entry with the requested base iteration above "
              << "in the possible_iterations node";
          throw std::runtime_error(std::move(msg.str()));
        }
        mPossibleIterations[thisIterName] = ReadSingleCAIteration(iterNode, mPossibleIterations.at(baseIterName));
      }
      else {
        mPossibleIterations[thisIterName] = ReadSingleCAIteration(iterNode, Iteration());
      }
    }
  }

  // Read actual iteration sequence
  //
  if (fUserConfigNode) {
    if (fVerbose >= 1) {
      LOG(info) << "- Reading user iterations ";
    }

    auto currentNode = fUserConfigNode["core"]["track_finder"]["iterations"];
    if (currentNode) {
      for (const auto& iterNode : currentNode) {
        std::string thisIterName = iterNode["name"].as<std::string>("");
        std::string baseIterName = iterNode["base_iteration"].as<std::string>("");
        if (fVerbose >= 2) {
          LOG(info) << "- Reading user iteration " << thisIterName << "(" << baseIterName << ")";
        }
        if (mPossibleIterations.find(thisIterName) != mPossibleIterations.end()) {
          // This is an update of existing possible iteration
          if (fVerbose >= 2) {
            LOG(info) << "- Select A";
          }
          res.push_back(ReadSingleCAIteration(iterNode, mPossibleIterations.at(thisIterName)));
        }
        else if (mPossibleIterations.find(baseIterName) != mPossibleIterations.end()) {
          // This is a user iteration based on the existing possible iteration
          if (fVerbose >= 2) {
            LOG(info) << "- Select B";
          }
          res.push_back(ReadSingleCAIteration(iterNode, mPossibleIterations.at(baseIterName)));
        }
        else {
          // Try to find a base iteration from user-defined
          auto itFound = std::find_if(res.begin(), res.end(), [&](auto& i) { return i.GetName() == baseIterName; });
          if (itFound != res.end()) {
            if (fVerbose >= 2) {
              LOG(info) << "- Select C";
            }
            res.push_back(ReadSingleCAIteration(iterNode, *itFound));
          }
          else {
            if (fVerbose >= 2) {
              LOG(info) << "- Select D";
            }
            res.push_back(ReadSingleCAIteration(iterNode, Iteration()));
          }
        }
      }
    }
  }

  if (res.size() == 0) {
    auto currentNode = fMainConfigNode["core"]["track_finder"]["iteration_sequence"];
    assert(currentNode);
    assert(currentNode.size());
    for (const auto& iterNode : currentNode) {
      std::string thisIterName = iterNode.as<std::string>();
      if (mPossibleIterations.find(thisIterName) == mPossibleIterations.end()) {
        std::stringstream msg;
        msg << "Unknow iteration in the iteration sequence, defined in main config file: " << thisIterName;
        throw std::runtime_error(std::move(msg.str()));
      }
      res.push_back(mPossibleIterations.at(thisIterName));
    }
  }

  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::vector<std::set<int>> ConfigReader::ReadInactiveStationMap()
{
  // Check, if the "skip_stations" branch exists either in the user config or in the main config
  std::string sNodePath = "ca/core/common/inactive_stations";
  auto node             = GetNode([](YAML::Node n) { return n["core"]["common"]["inactive_stations"]; }, true);
  if (!node) {
    return std::vector<std::set<int>>{};
  }

  // Fill map of inactive stations
  std::vector<std::set<int>> vGeoIdToTrackingStatus(constants::size::MaxNdetectors);

  if (node && node.size()) {
    std::unordered_map<std::string, int> mDetNameToID;
    for (int iDet = 0; iDet < constants::size::MaxNdetectors; ++iDet) {
      auto detName = boost::algorithm::to_lower_copy(fpInitManager->GetDetectorName(static_cast<EDetectorID>(iDet)));
      if (!detName.size()) {
        continue;
      }
      mDetNameToID[detName] = iDet;
    }
    for (const auto& item : node) {
      std::string stName = item.as<std::string>();
      if (!stName.size()) {
        continue;
      }
      //
      // Check name for spaces
      if (std::any_of(stName.begin(), stName.end(), [](auto c) { return c == ' ' || c == '\t' || c == '\n'; })) {
        std::stringstream msg;
        msg << "Illegal station name in the configuration branch \"" << sNodePath << "\": \"" << stName
            << "\" contains illegal characters";
        throw std::runtime_error(msg.str());
      }
      //
      // Split stName into a detector name and a station number
      std::vector<std::string> vNames;
      boost::algorithm::split(vNames, stName, boost::is_any_of(":"));
      if (vNames.size() > 2) {
        std::stringstream msg;
        msg << "Illegal station name in the configuration branch \"" << sNodePath << "\": \"" << stName
            << "\" contains more then one colon character";
        throw std::runtime_error(msg.str());
      }
      //
      // Parse detector name
      const auto& detName = boost::algorithm::to_lower_copy(vNames.front());
      auto it             = mDetNameToID.find(detName);
      if (it == mDetNameToID.end()) {
        std::stringstream msg;
        msg << "ConfigReader: legal but undefined name of the detector \"" << detName << "\" (originally \"" << stName
            << "\") was passed to the " << sNodePath << " config "
            << "branch";
        throw std::runtime_error(msg.str());
      }

      int nStations = fpInitManager->GetNstationsGeometry(static_cast<EDetectorID>(it->second));
      if (vNames.size() == 2) {  // Disable one particular station
        try {
          int iStLoc = std::stoi(vNames.back());
          if (iStLoc < 0 || iStLoc >= nStations) {
            throw std::runtime_error("illegal local station index");
          }
          vGeoIdToTrackingStatus[it->second].insert(iStLoc);
        }
        catch (const std::exception&) {
          std::stringstream msg;
          msg << "Illegal station name in the configuration branch \"" << sNodePath << "\": \"" << stName
              << "\" contains expression after the colon symbol, which cannot be translated "
              << "to a local number of station";
          throw std::runtime_error(msg.str());
        }
      }
      else {  // Disable all stations for this detector
        for (int iStLoc = 0; iStLoc < nStations; ++iStLoc) {
          vGeoIdToTrackingStatus[it->second].insert(iStLoc);
        }
      }
    }

    std::stringstream msg;
    msg << "\033[1;31m ConfigReader: the next tracking stations will be disabled from the configuration file:\n";
    for (int iDet = 0; iDet < static_cast<int>(mDetNameToID.size()); ++iDet) {
      const auto& detName = fpInitManager->GetDetectorName(static_cast<EDetectorID>(iDet));
      int nStations       = fpInitManager->GetNstationsGeometry(static_cast<EDetectorID>(iDet));
      for (int iStLoc = 0; iStLoc < nStations; ++iStLoc) {
        if (vGeoIdToTrackingStatus[iDet].find(iStLoc) != vGeoIdToTrackingStatus[iDet].end()) {
          msg << "\t- " << detName << iStLoc << '\n';
        }
      }
    }
    msg << "\033[0m";
    LOG(warn) << msg.str();
  }

  return vGeoIdToTrackingStatus;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ConfigReader::ReadMisalignmentTolerance()
{
  // Check, if the "misalignment_tolerance" branch exists either in the user config or in the main config

  std::string sNodePath = "ca/core/common/misalignment_tolerance";

  YAML::Node node(YAML::NodeType::Undefined);

  try {
    node = this->GetNode([](YAML::Node n) { return n["core"]["common"]["misalignment_tolerance"]; });
  }
  catch (const std::exception&) {
  }

  if (!node) {
    std::stringstream msg;
    msg << "Ca ConfigReader: misalignment_tolerance node was not found\n ";
    if (fUserConfigNode) {
      msg << "     either in the user config (path: " << fsUserConfigPath;
      msg << "     or ";
    }
    else {
      msg << "     ";
    }

    msg << "in the main config (path: " << fsMainConfigPath << ")";
    LOG(info) << msg.str();
    LOG(info) << "Ca ConfigReader: run with zero misalignment tolerance";
    return;
  }


  std::unordered_map<std::string, int> mDetNameToID;
  for (int iDet = 0; iDet < constants::size::MaxNdetectors; ++iDet) {
    auto detName = boost::algorithm::to_lower_copy(fpInitManager->GetDetectorName(static_cast<EDetectorID>(iDet)));
    if (!detName.empty()) {
      mDetNameToID[detName] = iDet;
    }
  }

  for (const auto& item : node) {
    std::string stName = boost::algorithm::to_lower_copy(item.first.as<std::string>());

    auto it = mDetNameToID.find(stName);
    if (it == mDetNameToID.end()) {
      std::stringstream msg;
      msg << "Illegal station name in the configuration branch \"" << sNodePath << "\": \"" << stName << "\"";
      throw std::runtime_error(msg.str());
    }
    int iDetSystem = it->second;
    auto v         = item.second.as<std::vector<double>>();
    if (v.size() != 3) {
      std::stringstream msg;
      msg << "Illegal number of misalignbment tolerances in configuration branch \"" << sNodePath << "\": \"" << stName
          << "\": " << v.size() << " values were passed, while 3 values are expected";
      throw std::runtime_error(msg.str());
    }
    fpInitManager->SetMisalignmentTolerance(static_cast<EDetectorID>(iDetSystem), v[0], v[1], v[2]);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
Iteration ConfigReader::ReadSingleCAIteration(const YAML::Node& node, const Iteration& defaultIter) const
{
  auto iter = Iteration();
  try {
    iter.SetName(node["name"].as<std::string>());
    iter.SetTrackChi2Cut(node["track_chi2_cut"].as<float>(defaultIter.GetTrackChi2Cut()));
    iter.SetTripletChi2Cut(node["triplet_chi2_cut"].as<float>(defaultIter.GetTripletChi2Cut()));
    iter.SetTripletFinalChi2Cut(node["triplet_final_chi2_cut"].as<float>(defaultIter.GetTripletFinalChi2Cut()));
    iter.SetDoubletChi2Cut(node["doublet_chi2_cut"].as<float>(defaultIter.GetDoubletChi2Cut()));
    iter.SetPickGather(node["pick_gather"].as<float>(defaultIter.GetPickGather()));
    iter.SetTripletLinkChi2(node["triplet_link_chi2"].as<float>(defaultIter.GetTripletLinkChi2()));
    iter.SetMaxQp(node["max_qp"].as<float>(defaultIter.GetMaxQp()));
    iter.SetMaxSlopePV(node["max_slope_pv"].as<float>(defaultIter.GetMaxSlopePV()));
    iter.SetMaxSlope(node["max_slope"].as<float>(defaultIter.GetMaxSlope()));
    iter.SetMaxDZ(node["max_dz"].as<float>(defaultIter.GetMaxDZ()));
    iter.SetTargetPosSigmaXY(node["target_pos_sigma_x"].as<float>(defaultIter.GetTargetPosSigmaX()),
                             node["target_pos_sigma_y"].as<float>(defaultIter.GetTargetPosSigmaY()));
    iter.SetFirstStationIndex(node["first_station_index"].as<int>(defaultIter.GetFirstStationIndex()));
    iter.SetPrimaryFlag(node["is_primary"].as<bool>(defaultIter.GetPrimaryFlag()));
    iter.SetElectronFlag(node["is_electron"].as<bool>(defaultIter.GetElectronFlag()));
    iter.SetTrackFromTripletsFlag(node["is_track_from_triplets"].as<bool>(defaultIter.GetTrackFromTripletsFlag()));
    iter.SetExtendTracksFlag(node["is_extend_tracks"].as<bool>(defaultIter.GetExtendTracksFlag()));
    iter.SetMaxStationGap(node["max_station_gap"].as<int>(defaultIter.GetMaxStationGap()));
    iter.SetMinNhits(node["min_n_hits"].as<int>(defaultIter.GetMinNhits()));
    iter.SetMinNhitsStation0(node["min_n_hits_station_0"].as<int>(defaultIter.GetMinNhitsStation0()));
  }
  catch (const YAML::InvalidNode& exc) {
    const auto nodeKeys = this->GetNodeKeys(node);
    const auto nodeKeysStr =
      std::accumulate(nodeKeys.cbegin(), nodeKeys.cend(), std::string(""),
                      [](std::string lhs, std::string rhs) { return std::move(lhs) + "\n\t" + std::move(rhs); });
    LOG(fatal) << "L1 config: attempt to access key which does not exist in the configuration file (message from "
               << "YAML exception: " << exc.what() << "). Defined keys: " << nodeKeysStr;
  }
  return iter;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ConfigReader::SetMainConfigPath(const std::string& path)
{
  if (path.size()) {
    try {
      fsMainConfigPath = path;
      fMainConfigNode  = YAML::LoadFile(fsMainConfigPath)["ca"];
      if (fVerbose >= 1) {
        LOG(info) << "ConfigReader: Registering main configuraiton file: \"\033[1;32m" << path << "\033[0m\"";
      }
    }
    catch (const std::exception& err) {
      LOG(error) << "ERROR: " << err.what();
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ConfigReader::SetUserConfigPath(const std::string& path)
{
  if (path.size()) {
    try {
      fsUserConfigPath = path;
      fUserConfigNode  = YAML::LoadFile(fsUserConfigPath)["ca"];
      if (fVerbose >= 1) {
        LOG(info) << "ConfigReader: Registering user configuraiton file: \"\033[1;32m" << path << "\033[0m\"";
      }
    }
    catch (const std::exception& err) {
      LOG(error) << "ERROR: " << err.what();
    }
  }
}
