/* Copyright (C) 2022-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaConfigReader.h
/// \brief  Configuration parameter file reader for the CA tracking algorithm (header)
/// \author S.Zharko <s.zharko@gsi.de>
/// \since  18.07.2022

#pragma once  // include this header only once per compilation unit

#include "CaIteration.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace YAML
{
  class Node;
}

namespace cbm::algo::ca
{
  class InitManager;
  enum class DetectorID;

  /// \class cbm::algo::ca::ConfigReader
  /// \brief A reader for the CA parameters from the YAML configuration files
  ///
  /// TODO: Describe configuration procedure here... (main config, user config etc...)
  ///
  class ConfigReader {
   public:
    /// \brief Constructor
    /// \param  pInitManager Pointer to the L1InitManager instance
    ConfigReader(InitManager* pInitManager, int verbose = 1);

    /// \brief Destructor
    ~ConfigReader() = default;

    /// \brief Reads configuration from files
    void Read();

    /// \brief  Reads CA track finder iterations from YAML node
    /// \return A vector of iterations
    std::vector<Iteration> ReadCAIterationVector();

    /// \brief Sets main config file
    /// \param path  Path to the file
    void SetMainConfigPath(const std::string& path);

    /// \brief Sets user config file
    /// \param path  Path to user config file
    void SetUserConfigPath(const std::string& path);

    /// \brief Sets verbosity level
    void SetVerbosity(int verbose) { fVerbose = verbose; }

    /// \brief Gets verbosity level
    int GetVerbosity() const { return fVerbose; }

    /// \brief Gets geometry lock status
    bool GetGeometryLock() const { return fbGeometryLock; }

    /// \brief Sets geometry lock status
    void SetGeometryLock(bool lock) { fbGeometryLock = lock; }

   private:
    /// \brief  Reads inactive tracking station map
    /// \return A vector of sets of disabled station local indexes vs. the the detector index
    std::vector<std::set<int>> ReadInactiveStationMap();

    /// \brief  Reads the misalignment tolerance
    void ReadMisalignmentTolerance();

    /// \brief Accesses a node either from user config or from main config
    /// \param fn        A function, which returns a YAML::Node reference object
    /// \param optional  true: node is not mandatory
    /// \note      If the node is not found in both configs
    /// \throw     std::runtime_error, if the path does not exist in the config
    ///
    /// This function is to be used, if the desired node should exist either in main or in user config. Firstly,
    /// the user config is checked, if it is provided. If the node is not found in user config, the main config
    /// is checked. If the node does not exist in the main config as well, an exception will be thrown.
    YAML::Node GetNode(std::function<YAML::Node(YAML::Node)> fn, bool optional = false) const;

    /// \brief Reads iteration from config file
    /// \param node         YAML node containing an iteration
    /// \param defaultIter  Default iteration
    /// \return             A CA-iteration object
    Iteration ReadSingleCAIteration(const YAML::Node& node, const Iteration& defaultIter) const;

    /// \brief   Gets parameters content of the node
    /// \param   node  YAML node
    /// \return  Vector of key names
    std::vector<std::string> GetNodeKeys(const YAML::Node& node) const;

    InitManager* fpInitManager = nullptr;  ///< Pointer to the L1InitManager instance
    int fVerbose               = 1;        ///< Verbosity level

    std::string fsMainConfigPath = "";  ///< Path to the main config file (mandatory)
    std::string fsUserConfigPath = "";  ///< Path to the user config file (optional)

    YAML::Node fMainConfigNode{YAML::NodeType::Undefined};  ///< Main configuration node
    YAML::Node fUserConfigNode{YAML::NodeType::Undefined};  ///< User configuration node

    bool fbMainConfigInitialized = false;  ///< Main configuration file is initialized (e.g. via parameters obj)
    bool fbGeometryLock          = false;  ///< Geometry initialization locked
  };
}  // namespace cbm::algo::ca
