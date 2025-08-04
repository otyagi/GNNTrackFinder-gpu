/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "CbmTaskDigiEventQa.h"

#include "CbmReco.h"  // for CbmRecoConfig
#include "algo/qa/Histo1D.h"

#include <FairRunOnline.h>
#include <Logger.h>

#include <TH1D.h>
#include <THttpServer.h>
#include <TStopwatch.h>

#include <cassert>
#include <iomanip>

using namespace std;
using cbm::algo::evbuild::DigiEventQaConfig;
using cbm::algo::evbuild::DigiEventQaData;
using cbm::algo::evbuild::DigiEventQaDetConfig;

#define NUM_BINS 100
#define BORDER 10.


// -----   Constructor   -----------------------------------------------------
CbmTaskDigiEventQa::CbmTaskDigiEventQa() : FairTask("DigiEventQa") {}
// ---------------------------------------------------------------------------


// -----   Destructor   ------------------------------------------------------
CbmTaskDigiEventQa::~CbmTaskDigiEventQa() {}
// ---------------------------------------------------------------------------


// -----   Configuration   ---------------------------------------------------
void CbmTaskDigiEventQa::Config(const cbm::algo::evbuild::Config& config)
{

  // The histogram ranges are defined by the event building windows. The number of bins
  // is hard-coded. To be changed on request.

  for (const auto& entry : config.fBuilder.fWindows) {
    ECbmModuleId system = entry.first;

    // --- Create histogram
    string name  = "hDigiTime" + ToString(entry.first);
    string title = ToString(entry.first) + " digi time in event";
    double lower = entry.second.first - BORDER;   // Lower edge of histogram
    double upper = entry.second.second + BORDER;  // Upper edge of histogram
    assert(fDigiTimeHistos.count(system) == 0);
    fDigiTimeHistos[system] = new TH1D(name.c_str(), title.c_str(), NUM_BINS, lower, upper);

    // --- Set algo configuration
    assert(fConfig.fData.count(system) == 0);
    fConfig.fData[system] = {NUM_BINS, lower, upper};
  }
}
// ---------------------------------------------------------------------------


// -----   Execution   -------------------------------------------------------
void CbmTaskDigiEventQa::Exec(Option_t*)
{

  // --- Timer and counters
  TStopwatch timer;
  timer.Start();

  // --- Algo execution
  DigiEventQaConfig config;
  DigiEventQaData result = (*fAlgo)(cbm::algo::DigiEvent::FromCbmDigiEvents(*fEvents));

  // --- Copy QA results (Histo1D) into ROOT output histograms
  // TODO: Probably not the most efficient implementation. Creates first a ROOT histogram from a CBM one (data copy),
  // which is then added to the member histogram (another data copy). Should implement a method for direct addition TH1D + Histo1D.
  for (const auto& entry : result.fDigiTimeHistos) {
    ECbmModuleId subsystem = entry.first;
    fDigiTimeHistos[subsystem]->Add(ToTH1D(*entry.second));
  }

  // --- Timeslice log
  timer.Stop();
  fExecTime += timer.RealTime();
  size_t numEvents = result.fNumEvents;
  stringstream logOut;
  logOut << setw(15) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << timer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNumTs << ", events " << numEvents;
  LOG(info) << logOut.str();

  // --- Run statistics
  fNumTs++;
  fNumEvents += numEvents;
}
// ----------------------------------------------------------------------------


// -----   End-of-timeslice action   ------------------------------------------
void CbmTaskDigiEventQa::Finish()
{
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Timeslices : " << fNumTs;
  LOG(info) << "Events     : " << fNumEvents;
  LOG(info) << "Exec time  : " << fixed << setprecision(2) << 1000. * fExecTime / double(fNumTs) << " ms / TS";
  for (const auto& entry : fDigiTimeHistos) {
    ECbmModuleId subsystem = entry.first;
    TH1D* histo            = entry.second;
    LOG(info) << ToString(subsystem) << " digi times: entries " << histo->GetEntries() << ", mean " << histo->GetMean()
              << ", stddev " << histo->GetStdDev();
    histo->Write();
  }
  LOG(info) << "=====================================";
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
InitStatus CbmTaskDigiEventQa::Init()
{
  // --- Get FairRootManager instance
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  LOG(info) << "==================================================";
  LOG(info) << GetName() << ": Initialising...";

  // --- Input data
  fEvents = ioman->InitObjectAs<const std::vector<CbmDigiEvent>*>("DigiEvent");
  if (!fEvents) {
    LOG(error) << GetName() << ": No input branch DigiEvent!";
    return kFATAL;
  }
  LOG(info) << "--- Found branch DigiEvent";

  // --- Register histograms
  THttpServer* server = FairRunOnline::Instance()->GetHttpServer();
  if (server) {
    LOG(info) << "--- Http server present; registering histograms";
    for (const auto& entry : fDigiTimeHistos)
      server->Register("DigiEvent", entry.second);
  }
  else
    LOG(info) << "--- No Http server present";

  // --- Configure algorithm
  fAlgo = std::make_unique<cbm::algo::evbuild::DigiEventQa>(fConfig);
  LOG(info) << fAlgo->ToString();

  LOG(info) << "==================================================";
  return kSUCCESS;
}
// ----------------------------------------------------------------------------


// -----   Convert CBM histogram to ROOT histogram   --------------------------
TH1D* CbmTaskDigiEventQa::ToTH1D(const cbm::algo::qa::H1D& source)
{
  bool add = TH1::AddDirectoryStatus();
  TH1::AddDirectory(false);  // Needed to prevent ROOT from adding histogram to its internal registry
  TH1D* result = new TH1D(source.GetName().c_str(), source.GetName().c_str(), source.GetNbinsX(), source.GetMinX(),
                          source.GetMaxX());
  TH1::AddDirectory(add);  // Needed to prevent ROOT from adding histogram to its internal registry
  for (uint32_t bin = 0; bin <= source.GetNbinsX() + 1; bin++) {
    result->SetBinContent(bin, source.GetBinContent(bin));
  }
  result->SetEntries(source.GetEntries());
  return result;
}
// ----------------------------------------------------------------------------

ClassImp(CbmTaskDigiEventQa)
