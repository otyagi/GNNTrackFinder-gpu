/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMSOURCE_H_
#define CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMSOURCE_H_

#include <RtypesCore.h>
#include <TChain.h>
#include <TFile.h>
#include <TString.h>

#include <vector>

#include <Hal/IOManager.h>
#include <Hal/RootIOManager.h>
#include <Hal/Source.h>

/**
 * ugly work-around for AT that doesn't contain TObject based object
 */
class CbmAnaTreeRecoSourceContainer;
class CbmAnaTreeMcSourceContainer;

class HalCbmATIOManager : public Hal::RootIOManager {
 protected:
  virtual void FillBranches();
  CbmAnaTreeRecoSourceContainer* fRecoContainer = {nullptr};
  CbmAnaTreeMcSourceContainer* fSimContainer    = {nullptr};

 public:
  HalCbmATIOManager(TString name = "") : Hal::RootIOManager(name){};
  virtual ~HalCbmATIOManager();
  ClassDef(HalCbmATIOManager, 1)
};

class HalCbmSource : public Hal::Source {
 protected:
  HalCbmATIOManager* fManager;

 public:
  HalCbmSource(TString filename = "");
  virtual void AddFile(TString fileName = "");
  virtual void AddFriend(TString friendName = "", Int_t level = 0);
  Hal::IOManager* GetIOManager() const { return fManager; };
  virtual ~HalCbmSource();
  ClassDef(HalCbmSource, 1)
};

#endif /* CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMSOURCE_H_ */
