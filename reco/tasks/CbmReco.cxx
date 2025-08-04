/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "CbmReco.h"

#include "CbmSourceTs.h"
#include "CbmTaskBuildEvents.h"
#include "CbmTaskDigiEventQa.h"
#include "CbmTaskTriggerDigi.h"
#include "CbmTaskUnpack.h"
#include "CbmTsEventHeader.h"
#include "DigiEventSelector.h"
#include "DigiEventSelectorConfig.h"

#include <FairFileSource.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRunOnline.h>
#include <Logger.h>

#include <THttpServer.h>
#include <TRootSniffer.h>

#include <memory>
#include <string>

#include <yaml-cpp/yaml.h>

using std::make_unique;
using std::string;


// -----   Constructor from single source   -----------------------------------
CbmReco::CbmReco(Config config, string source, string outFile, string configFile, int32_t numTs, uint16_t port,
                 cbm::Monitor* monitor)
  : fConfig(config)
  , fSourceNames{source}
  , fOutputFileName(outFile)
  , fConfigFileName(configFile)
  , fNumTs(numTs)
  , fHttpServerPort(port)
  , fMonitor(monitor)
{
}
// ----------------------------------------------------------------------------


// -----   Constructor from multiple sources   --------------------------------
CbmReco::CbmReco(Config config, std::vector<string> sources, string outFile, string configFile, int32_t numTs,
                 uint16_t port, cbm::Monitor* monitor)
  : fConfig(config)
  , fSourceNames(sources)
  , fOutputFileName(outFile)
  , fConfigFileName(configFile)
  , fNumTs(numTs)
  , fHttpServerPort(port)
  , fMonitor(monitor)
{
}
// ----------------------------------------------------------------------------


// -----   List of source names   ---------------------------------------------
std::string CbmReco::ListSources() const
{
  std::string result = "{";
  for (auto& source : fSourceNames) {
    result += source + ", ";
  }
  result += "}";
  return result;
}
// ----------------------------------------------------------------------------


// -----   Configure and execute run   ----------------------------------------
int32_t CbmReco::Run()
{

  // --- Timing
  TStopwatch timer;
  timer.Start();

  // --- Check if the input is a ROOT file. In that case, digis are already present and
  // --- the unpacking stage must be skipped. The digis are in direct branches of the ROOT
  // --- tree when coming from simulation, or in form of CbmDigiTimeslice if produced
  // --- by a previous unpacking run. This variety is caught by the tasks and need not be
  // --- considered here.
  bool isRootInput =
    fSourceNames.size() == 1 && fSourceNames.at(0).compare(fSourceNames.at(0).size() - 5, 5, ".root") == 0;

  // --- Run instance
  FairRunOnline run;

  // --- Input source
  if (isRootInput) {
    auto source = make_unique<FairFileSource>(fSourceNames.at(0));
    LOG(info) << "Reco: Using ROOT input " << fSourceNames.at(0);
    run.SetSource(source.release());
  }
  else {
    auto source = make_unique<CbmSourceTs>(fSourceNames);
    if (source)
      LOG(info) << "Reco: Using sources " << ListSources();
    else {
      LOG(error) << "Reco: Could not open sources " << ListSources() << "; aborting.";
      return -1;
    }
    run.SetSource(source.release());
  }

  // --- Output file
  auto sink = make_unique<FairRootFileSink>(fOutputFileName);
  if (sink)
    LOG(info) << "Reco: Using output file " << fOutputFileName;
  else {
    LOG(error) << "Reco: Could not open output " << fOutputFileName << "; aborting.";
    return -1;
  }
  run.SetSink(sink.release());

  // --- Event header
  auto header = make_unique<CbmTsEventHeader>();
  run.SetEventHeader(header.release());

  // --- Unpacking
  if (!isRootInput) {
    CbmTaskUnpack::Config unpackConfig;
    unpackConfig.dumpSetup = fConfig.dumpSetup;
    //auto unpack            = make_unique<CbmTaskUnpack>(unpackConfig);
    // TO DO: This call no longer works. Parameters directory and run ID is needed!
    auto unpack = make_unique<CbmTaskUnpack>();
    unpack->SetMonitor(fMonitor);
    unpack->SetOutputBranchPersistent("DigiTimeslice.", false);
    run.AddTask(unpack.release());
  }

  // --- Event building configuration
  cbm::algo::evbuild::Config evbuildConfig(YAML::LoadFile(fConfigFileName));

  // --- Digi trigger
  auto trigger = make_unique<CbmTaskTriggerDigi>();
  trigger->AddSystem(evbuildConfig.fDigiTrigger.Detector());
  trigger->SetConfig(evbuildConfig.fDigiTrigger);
  trigger->SetOutputBranchPersistent("Trigger", false);
  run.AddTask(trigger.release());

  // --- Event selector parameters
  cbm::algo::evbuild::DigiEventSelectorConfig selectConfig = evbuildConfig.fSelector;

  // --- Event building
  auto evtBuild = make_unique<CbmTaskBuildEvents>();
  evtBuild->SetConfig(evbuildConfig.fBuilder);
  evtBuild->SetOutputBranchPersistent("DigiEvent", true);
  evtBuild->SetDigiEventSelector(selectConfig);
  run.AddTask(evtBuild.release());

  // --- Event QA
  auto evtQa = make_unique<CbmTaskDigiEventQa>();
  evtQa->Config(evbuildConfig);
  run.AddTask(evtQa.release());

  // ----- HttpServer for online monitoring
  if (fHttpServerPort) {
    run.ActivateHttpServer(100, fHttpServerPort);
    run.GetHttpServer()->GetSniffer()->SetScanGlobalDir(kFALSE);
  }

  // --- Initialise and start run
  timer.Stop();
  double timeSetup = timer.RealTime();
  timer.Start();
  LOG(info) << "Reco: Initialising...";
  run.Init();
  timer.Stop();
  double timeInit = timer.RealTime();

  // --- Start run
  timer.Start();
  run.Run(0, fNumTs);
  timer.Stop();
  double timeRun = timer.RealTime();

  // --- Run log
  size_t numTs = 1;
  if (!isRootInput) {
    auto src = dynamic_cast<CbmSourceTs*>(run.GetSource());
    assert(src);
    numTs = src->GetNumTs();
  }
  // TODO: Don't know how to get the number of processed timeslices for ROOT input.
  double timeTotal = timeSetup + timeInit + timeRun;
  LOG(info) << "=====================================";
  LOG(info) << "Reco: Run summary";
  LOG(info) << "Timeslices  : " << numTs;
  LOG(info) << "Time setup  : " << timeSetup << " s";
  LOG(info) << "Time init   : " << timeInit << " s";
  LOG(info) << "Time run    : " << timeRun << " s";
  LOG(info) << "Time total  : " << timeTotal << " s"
            << " (" << timeTotal / numTs << " s/ts)";
  LOG(info) << "Output file : " << fOutputFileName;
  LOG(info) << "=====================================";

  return numTs;
}
// ----------------------------------------------------------------------------

ClassImp(CbmReco)
