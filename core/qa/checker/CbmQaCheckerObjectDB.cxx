/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerObjectDB.h
/// @brief  Database for processed objects in the QA checker framework (implementation)
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  15.02.2023

#include "CbmQaCheckerObjectDB.h"

#include "Logger.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TH1.h"
#include "TString.h"

#include <algorithm>
#include <regex>
#include <sstream>

#include <yaml-cpp/yaml.h>

using cbm::qa::checker::ObjectDB;

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::Clear()
{
  fvDatasets.clear();
  fvFiles.clear();
  fvFileLabels.clear();
  fvObjects.clear();
  fvVersionLabels.clear();
  fvVersionPaths.clear();
  fvObjectFirstGlobIndex.clear();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::AddVersion(const char* label, const char* path)
{
  fvVersionLabels.push_back(label);
  fvVersionPaths.push_back(path);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::AddDataset(const char* dataset) { fvDatasets.push_back(dataset); }

// ---------------------------------------------------------------------------------------------------------------------
//
std::string ObjectDB::GetInputFileName(int iVersion, int iFile, int iDataset) const
{
  std::string res = fvFiles[iFile];
  res             = std::regex_replace(res, std::regex("\\%v"), fvVersionPaths[iVersion]);
  res             = std::regex_replace(res, std::regex("\\%d"), fvDatasets[iDataset]);
  return res;
}


// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::Init()
{
  // ----- Check consistency of input values
  LOG_IF(fatal, !GetNofObjects()) << "ObjectDB: No objects were passed to the checker";
  LOG_IF(fatal, GetNofDatasets() < 1) << "ObjectDB: No datasets were found, at least one dataset should be provided";
  LOG_IF(fatal, GetNofVersions() < 2) << "ObjectDB: File handler should have at least two versions to compare ("
                                      << GetNofVersions() << " were provided)";


  // ----- Define output file
  if (fsOutputPath.empty()) {
    fsOutputPath = "QaCheckerOutput.root";
  }

  // ----- Define default version index
  if (fsDefaultLabel.size()) {
    auto it = std::find(fvVersionLabels.cbegin(), fvVersionLabels.cend(), fsDefaultLabel);
    if (it == fvVersionLabels.cend()) {
      std::stringstream msg;
      msg << "ObjectDB: registered default label \"" << fsDefaultLabel << "\" is not found among the version labels:\n";
      for (const auto& label : fvVersionLabels) {
        msg << "\t- " << label << '\n';
      }
      LOG(fatal) << msg.str();
    }
    fDefVersionID = it - fvVersionLabels.cbegin();
  }
  else {
    fDefVersionID = 0;
    LOG(warn) << "ObjectDB: default version was not registered. Using the first version as the default one (\""
              << fvVersionLabels[fDefVersionID] << "\")";
  }

  // ----- Read object list from file
  for (size_t iFile = 0; iFile < fvObjects.size(); ++iFile) {
    if (fvObjects[iFile].size() == 0) {
      this->ReadObjectList(iFile);
    }
  }

  LOG(info) << this->ToString();

  // ----- Init the object index vector
  fvObjectFirstGlobIndex.clear();
  fvObjectFirstGlobIndex.resize(fvObjects.size() + 1, 0);
  for (size_t iFile = 1; iFile <= fvObjects.size(); ++iFile) {
    fvObjectFirstGlobIndex[iFile] = fvObjectFirstGlobIndex[iFile - 1] + fvObjects[iFile - 1].size();
  }

  // ----- Add root path of input, if it were defined
  auto regexSlashes = std::regex("(/+)");  // regular expression for a sequence of consecutive slashes
  for (auto& path : fvVersionPaths) {
    if (fsInputRootPath.size()) {
      path = fsInputRootPath + "/" + path;
    }
    path = std::regex_replace(path, regexSlashes, "/");  // replace all consecutive slashes with a single one
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::ReadFromYAML(const char* configName)
{
  // ----- Open input file
  YAML::Node config;
  try {
    config = YAML::LoadFile(configName)["checker"];
  }
  catch (const YAML::BadFile& exc) {
    LOG(fatal) << "ObjectDB: configuration file " << configName << " does not exist";
  }
  catch (const YAML::ParserException& exc) {
    LOG(fatal) << "ObjectDB: configuration file " << configName << " is badly formatted";
  }

  // ----- Define file-object map
  if (const auto& node = config["files"]) {
    if (fvObjectFirstGlobIndex.size()) {
      LOG(warn) << "ObjectDB: file-object map was defined before. Redefining it from the config file " << configName;
      fvFiles.clear();
      fvFileLabels.clear();
      fvObjects.clear();
    }
    try {
      // Calculate total number of objects and files
      size_t nFiles = node.size();
      fvFiles.reserve(nFiles);
      fvFileLabels.reserve(nFiles);
      fvObjects.reserve(nFiles);

      // Fill vectors
      for (const auto& fileNode : node) {
        const auto& objectsNode = fileNode["objects"];
        int nObjects            = objectsNode ? objectsNode.size() : 0;
        auto& objectsInFile     = fvObjects.emplace_back();
        objectsInFile.reserve(nObjects);
        if (nObjects > 0) {
          for (const auto& objectNode : objectsNode) {
            objectsInFile.push_back(objectNode.as<std::string>());
          }
        }
        fvFiles.push_back(fileNode["name"].as<std::string>());
        fvFileLabels.push_back(fileNode["label"].as<std::string>());
      }
    }
    catch (const YAML::InvalidNode& exc) {
      LOG(fatal) << "ObjectDB: error while reading checker/files node from the config " << configName;
    }
  }
  else {
    LOG(warn) << "ObjectDB: node checker/inputformat is not defined in the config " << configName;
  }

  // ----- Define dataset names
  if (const auto& node = config["datasets"]) {
    LOG_IF(fatal, fvDatasets.size())
      << "ObjectDB: dataset names were defined before. Please, use only one initialisation method:"
      << " either configuration file, or setters of the checker::Core class";
    try {
      fvDatasets.reserve(node.size());
      for (const auto& datasetNode : node) {
        fvDatasets.push_back(datasetNode.as<std::string>());
      }
    }
    catch (const YAML::InvalidNode& exc) {
      LOG(fatal) << "ObjectDB:: error while reading checker/datasets node from the config " << configName;
    }
  }
  else {
    LOG(warn) << "ObjectDB: node checker/datasets is not defined in the config " << configName;
  }

  // ----- Define version names
  if (const auto& node = config["versions"]) {
    LOG_IF(fatal, fvVersionLabels.size())
      << "ObjectDB: dataset names were defined before. Attempt to redefine dataset names from config " << configName;
    try {
      fvVersionLabels.reserve(node.size());
      fvVersionPaths.reserve(node.size());
      for (const auto& versionNode : node) {
        fvVersionLabels.push_back(versionNode["label"].as<std::string>());
        fvVersionPaths.push_back(versionNode["path"].as<std::string>());
      }
    }
    catch (const YAML::InvalidNode& exc) {
      LOG(fatal) << "ObjectDB:: error while reading checker/versions node from the config " << configName;
    }
  }
  else {
    LOG(warn) << "ObjectDB: node checker/versions is not defined in the config " << configName;
  }

  // ----- Define default version
  if (const auto& node = config["default_label"]) {
    try {
      SetDefaultLabel(node.as<std::string>().c_str());
    }
    catch (const YAML::InvalidNode& exc) {
      LOG(fatal) << "ObjectDB:: error while reading checker/default_label node from the config " << configName;
    }
  }

  // ----- Define the comparison parameters
  if (const auto& node = config["settings"]) {
    try {
      double ratioMin = node["ratio_min"].as<double>(fRatioMin);
      double ratioMax = node["ratio_max"].as<double>(fRatioMax);
      SetRatioRange(ratioMin, ratioMax);

      double pValThresh = node["pval_threshold"].as<double>(fPvalThresh);
      SetPvalThreshold(pValThresh);
    }
    catch (const YAML::InvalidNode& exc) {
      LOG(fatal) << "ObjectDB:: error while reading checker/versions node from the config " << configName;
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::CollectObjectPaths(TDirectory* pDir, const TString& parentPath, std::set<std::string>& paths)
{
  for (auto&& pKey : *(pDir->GetListOfKeys())) {
    TString sName = parentPath + pKey->GetName();
    if (gFile->Get<TH1>(sName)) {
      paths.insert(sName.Data());
    }
    else if (auto* pSubDir = gFile->Get<TDirectory>(sName)) {
      CollectObjectPaths(pSubDir, sName + "/", paths);
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::ReadObjectList(int iFile)
{
  // TODO: test performance, probably unordered_set will fit better
  std::set<std::string> objectPaths;
  LOG(info) << "Reading object list from files: ...";
  for (int iDs = 0; iDs < static_cast<int>(fvDatasets.size()); ++iDs) {
    TString fileName = this->GetInputFileName(fDefVersionID, iFile, iDs);
    LOG(info) << "- file: " << fileName;
    TFile fileIn{fileName, "READONLY"};
    fileIn.cd();
    CollectObjectPaths(&fileIn, "", objectPaths);
    fileIn.Close();
  }
  fvObjects[iFile].clear();
  fvObjects[iFile].reserve(objectPaths.size());
  fvObjects[iFile].insert(fvObjects[iFile].begin(), objectPaths.begin(), objectPaths.end());
  LOG(info) << "Reading object list from files: done";
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::SetPvalThreshold(double pVal)
{
  if (pVal <= 0 || pVal >= 1) {
    LOG(fatal) << "ObjectDB::SetPvalThreshold(): p-value threshold runs out the range (0, 1): " << pVal;
  }
  fPvalThresh = pVal;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void ObjectDB::SetRatioRange(double min, double max)
{
  if (min > max || min < 0) {
    LOG(fatal) << "ObjectDB::SetPvalThreshold(): min and max for ratio run out the range: min = " << min
               << ", max = " << max;
  }
  fRatioMin = min;
  fRatioMax = max;
}


// ---------------------------------------------------------------------------------------------------------------------
//
std::string ObjectDB::ToString(int verbose) const
{
  std::stringstream msg;
  if (verbose > 0) {
    msg << '\n';
    msg << "  ********************\n";
    msg << "  ** CBM QA-Checker **\n";
    msg << "  ********************\n\n";

    msg << "\e[1mVersions\e[0m:\n";
    for (size_t iV = 0; iV < fvVersionLabels.size(); ++iV) {
      if (iV == (size_t) fDefVersionID) {
        msg << "\t- " << fvVersionLabels[iV] << " (path: " << fvVersionPaths[iV] << ") -> \e[1;33mDEFAULT\e[0m\n";
      }
      else {
        msg << "\t- " << fvVersionLabels[iV] << " (path: " << fvVersionPaths[iV] << ")\n";
      }
    }
    msg << "\e[1mDatasets\e[0m:\n";
    for (const auto& dataset : fvDatasets) {
      msg << "\t- " << dataset << "\n";
    }
    msg << "\e[1mFiles\e[0m:\n";
    for (size_t iF = 0; iF < fvFiles.size(); ++iF) {
      msg << "\t- " << fvFiles[iF];
      if (verbose > 1) {
        msg << " with objects:\n";
        for (const auto& object : fvObjects[iF]) {
          msg << "\t\t- " << object << '\n';
        }
      }
      else {
        msg << '\n';
      }
    }
  }
  return msg.str();
}
