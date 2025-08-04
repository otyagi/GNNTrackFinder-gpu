/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

/**
 * @file CbmOnlineParWrite.h
 * @brief This file contains the declaration of the CbmOnlineParWrite class.
**/

#include "Definitions.h"

#include <TString.h>

class FairRunAna;
class TList;
class CbmSetup;

/**
 * @class CbmOnlineParWrite
 * @brief This class is responsible for writing the online parameters to configuration files.
 * @note Currently covers TRD, TOF and STS setup.
**/
class CbmOnlineParWrite {

 public:
  struct Config {
    cbm::algo::Setup setupType = cbm::algo::Setup::mCBM2022;
    bool doAlignment           = false;
  };

  void Run(const Config& config);

 private:
  Config fConfig;
  TString fSrcDir  = "";       // CbmRoot Source directory
  TString fGeoSetupTag = "";       // Geometry setup tag
  CbmSetup* fSetup = nullptr;  // Global Geometry setup
  FairRunAna* fRun = nullptr;  // FairRunAna object
  TList* fParList  = nullptr;  // List of parameter files, opened with FairRuntimeDb

  void AddDetectors();

  void AddTrd();
  void AddTof();
  void AddSts();
  void AddCa();
};
