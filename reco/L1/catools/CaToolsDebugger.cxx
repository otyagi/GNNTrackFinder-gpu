/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaToolsDebugger.h
/// \brief  Tracking Debugger class (implementation)
/// \since  23.09.2022
/// \author S.Zharko <s.zharko@gsi.de>

/*
root -l CAdebug.root
TNtuple *n = (TNtuple*) gDirectory->FindObjectAny("triplets")
n->Draw("chi2")
*/

#include "CaToolsDebugger.h"

#include "TDirectory.h"
#include "TFile.h"
#include "TNtuple.h"

#include <cstdarg>
#include <iostream>
#include <memory>
#include <string>

using cbm::ca::tools::Debugger;

Debugger& Debugger::Instance()
{
  static std::unique_ptr<Debugger> instance;
  if (!instance) {
    instance.reset(new Debugger);
  }
  return *instance.get();
}


Debugger::Debugger(const char* fileName)
{
  /// Default constructor
  fFileName = fileName;
}

void Debugger::Write()
{
  /// Write data to file
  fIsWritten          = true;
  TDirectory* currDir = gDirectory;
  if (fFile) {
    fFile->cd();
    for (unsigned int i = 0; i < fNtuples.size(); i++) {
      fNtuples[i]->Write();
    }
  }
  gDirectory = currDir;
}

Debugger::~Debugger()
{
  /// Destructor
  if (!fIsWritten && fFile) {
    std::cerr << "CaDebugger: you forgot to Write()!!" << std::endl;
  }
}

void Debugger::AddNtuple(const char* name, const char* varlist)
{
  /// add ntuple

  if (!fFile) {
    fFile = new TFile(fFileName.c_str(), "RECREATE");
  }
  assert(fFile);
  if (GetNtupleIndex(name) >= 0) return;
  TDirectory* currDir = gDirectory;
  fFile->cd();
  fNtuples.push_back(new TNtuple(name, name, varlist));
  gDirectory = currDir;
}

int Debugger::GetNtupleIndex(const char* name)
{
  int ind = -1;
  for (unsigned int i = 0; i < fNtuples.size(); i++) {
    if (strcmp(name, fNtuples[i]->GetName()) == 0) {
      ind = i;
      break;
    }
  }
  return ind;
}

void Debugger::FillNtuple(const char* name, float v[])
{
  int iNtuple = GetNtupleIndex(name);
  if (iNtuple < 0) {
    std::cerr << "CaDebugger: Ntuple (" << name << ") doesn't exist" << std::endl;
  }
  fNtuples[iNtuple]->Fill(v);
}
