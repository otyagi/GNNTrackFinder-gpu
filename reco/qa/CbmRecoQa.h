/* Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Tim Fauerbach, Florian Uhlig [committer] */

/** @file CbmRecoQa.h
 ** @author Tim Fauerbach
 ** @since Jun 2019
 **/

#ifndef CBMRECOQA
#define CBMRECOQA 1

#include "CbmMCDataManager.h"
#include "CbmMCEventList.h"
#include "FairRootManager.h"
#include "FairTask.h"
#include "TFile.h"
#include "TH1.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

class CbmRecoQa : public FairTask {
 private:
  void record(std::string decName, int i);
  TFile* pullresfile;
  int verbosity;
  std::vector<std::pair<std::string, std::array<int, 4>>> detectors;
  std::vector<std::vector<TH1F*>> hists;
  CbmMCEventList* eventList;
  FairRootManager* fManager;
  CbmMCDataManager* mcManager;
  std::string outname;

 public:
  CbmRecoQa(std::vector<std::pair<std::string, std::array<int, 4>>> decNames, std::string outName = "test",
            int verbose_l = 0);
  ~CbmRecoQa();
  static CbmRecoQa* instance;
  virtual InitStatus ReInit();
  virtual InitStatus Init();
  virtual void FinishEvent();
  virtual void FinishTask();

  ClassDef(CbmRecoQa, 1);
};

#endif
