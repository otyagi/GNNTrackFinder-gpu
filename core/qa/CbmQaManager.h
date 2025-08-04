/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaManager.h
/// \brief  Manager task for other QA taska (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  17.10.2023

#pragma once

#include "FairTask.h"
#include "TString.h"

#include <TFile.h>

#include <map>
#include <string>

class CbmQaManager : public FairTask {
 public:
  /// \brief  Constructor from parameters
  /// \param  verbose  Verbose level
  CbmQaManager(int verbose = 1);

  /// \brief  Destructor
  virtual ~CbmQaManager() = default;

  /// \brief Copy constructor
  CbmQaManager(const CbmQaManager&) = delete;

  /// \brief Move constructor
  CbmQaManager(CbmQaManager&&) = delete;

  /// \brief Copy assignment operator
  CbmQaManager& operator=(const CbmQaManager&) = delete;

  /// \brief Move assignment operator
  CbmQaManager& operator=(CbmQaManager&&) = delete;

  /// \brief Adds sub-task
  void AddTask(FairTask* task) { this->Add(task); }

  /// \brief Action of the task in the time-slice
  void Exec(Option_t*){};

  /// \brief Action of the task in the end of the run
  void Finish();

  /// \brief Gets status flag
  bool GetStatus() const { return fStatus; }

  /// \brief Gets YAML config name
  const TString& GetConfigName() const { return fsConfigName; }

  /// \brief Gets default tag
  const TString& GetDefaultTag() const { return fsDefaultTag; }

  /// \brief Gets version tag
  const TString& GetVersionTag() const { return fsVersionTag; }

  /// \brief Task initialization
  InitStatus Init();

  /// \brief Task re-initialization
  InitStatus ReInit() { return Init(); }

  ClassDef(CbmQaManager, 0);

  /// \brief Open cross-check file
  /// \param path  Path to the cross-check file
  ///
  /// Opens the benchmark input ROOT-file with the QA-chain output, obtained under the default code base.
  void OpenBenchmarkInput(const TString& path);

  /// \brief Open benchmark output file
  /// \param path  Path to the comparison output
  void OpenBenchmarkOutput(const TString& path);

  /// \brief Sets YAML config name
  void SetConfigName(const TString& name) { fsConfigName = name; }

  /// \brief Sets version tag
  void SetVersionTag(const TString& tag) { fsVersionTag = tag; }

  /// \brief Sets default tag
  void SetDefaultTag(const TString& tag) { fsDefaultTag = tag; }

 private:
  bool fStatus         = true;  ///< Status of QA: true - all tasks passed, false - at least one of the task failed
  TString fsConfigName = "";    ///< Name of the configuration YAML file (passed to underlying QA tasks)
  TString fsVersionTag = "";    ///< Version tag (git SHA etc.)
  TString fsDefaultTag = "";    ///< Default tag (git SHA etc.)

  std::shared_ptr<TFile> fpBenchmarkInput =
    nullptr;  ///< A benchmark file with default ROOT objects used for the cross-check
  std::shared_ptr<TFile> fpBenchmarkOutput = nullptr;  ///< An output file for histograms cross-check
};
