/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Florian Uhlig */

/** @file Run.cxx
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 18.05.2018
 **/

#include "Run.h"

#include "CbmSetup.h"
#include "TaskFactory.h"

#include <FairEventHeader.h>
#include <FairFileSource.h>
#include <FairMonitor.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <FairTask.h>
#include <Logger.h>

#include <TFile.h>
#include <TGeoManager.h>
#include <TStopwatch.h>
#include <TString.h>
#include <TTree.h>

#include <cassert>
#include <iostream>

#include <sys/stat.h>

namespace cbm::reco::offline
{


  // -----   Constructor   ----------------------------------------------------
  Run::Run() : TNamed("Run", "CBM Reconstruction Run") {}
  // --------------------------------------------------------------------------


  // -----   Destructor   -----------------------------------------------------
  Run::~Run() { LOG(debug) << "Destructing " << fName; }
  // --------------------------------------------------------------------------


  // -----   Add a reconstruction task   --------------------------------------
  void Run::AddTask(FairTask* task)
  {
    fRun.AddTask(task);
    LOG(info) << GetName() << ": Added task " << task->GetName();
  }
  // --------------------------------------------------------------------------


  // -----   Check a digi branch   --------------------------------------------
  void Run::CheckDigiBranch(TTree* tree, ECbmModuleId detector)
  {
    TString branchName = ToString(detector);
    branchName += "Digi";
    if (tree->GetBranchStatus(branchName.Data())) {
      LOG(info) << GetName() << ": Found branch " << branchName;
      fDataPresent.insert(detector);
    }
  }
  // --------------------------------------------------------------------------


  // -----   Check existence of a file   --------------------------------------
  bool Run::CheckFile(const char* fileName)
  {
    struct stat buffer;
    return (stat(fileName, &buffer) == 0);
  }
  // --------------------------------------------------------------------------


  // -----   Check which input digi branches are present   --------------------
  void Run::CheckInputBranches(FairFileSource* source)
  {

    TFile* inFile = source->GetInFile();
    if (!inFile) throw std::runtime_error("No input file");
    auto* inTree = inFile->Get<TTree>("cbmsim");
    if (!inTree) throw std::runtime_error("No input tree");

    for (ECbmModuleId detector = ECbmModuleId::kMvd; detector != ECbmModuleId::kNofSystems; ++detector)
      CheckDigiBranch(inTree, detector);
  }
  // --------------------------------------------------------------------------


  // -----   Create the topology   --------------------------------------------
  void Run::CreateTopology()
  {

    TaskFactory fact(this);

    // --- Timeslice processing
    if (fConfig.f_glb_mode == ECbmRecoMode::Timeslice) {
      fact.RegisterStsReco();            // Local reconstruction in STS
      fact.RegisterRichHitFinder();      // Hit finding in RICH
      fact.RegisterMuchReco();           // Local reconstruction in MUCH
      fact.RegisterTrdReco();            // Local reconstruction in TRD
      fact.RegisterTofReco();            // Local reconstruction in TOF
      fact.RegisterPsdReco();            // Local reconstruction in PSD
      fact.RegisterFsdReco();            // Local reconstruction in FSD
      fact.RegisterCaTracking();         // CA track finder in STS and MVD
      fact.RegisterTrackEventBuilder();  // Event building from STS tracks
    }

    // --- Event-by-event processing
    else if (fConfig.f_glb_mode == ECbmRecoMode::EventByEvent) {
      fact.RegisterDigiEventBuilder();  // Event building from digis
      fact.RegisterMvdReco();           // Local reconstruction in MVD
      fact.RegisterStsReco();           // Local reconstruction in STS
      fact.RegisterRichHitFinder();     // Hit finding in RICH
      fact.RegisterMuchReco();          // Local reconstruction in MUCH
      fact.RegisterTrdReco();           // Local reconstruction in TRD
      fact.RegisterTofReco();           // Local reconstruction in TOF
      fact.RegisterPsdReco();           // Local reconstruction in PSD
      fact.RegisterFsdReco();           // Local reconstruction in FSD
      fact.RegisterCaTracking();        // CA track finder in STS and MVD
      fact.RegisterPvFinder();          // Primary vertex finding
      fact.RegisterGlobalTracking();    // Global tracking
      fact.RegisterTrdPid();            // PID in TRD
      fact.RegisterRichReco();          // Local RICH reconstruction
      fact.RegisterBmonReco();          // Reconstruction of Bmon from BMON
    }

    // --- Mode not defined
    else
      throw std::out_of_range("Reconstruction mode not defined");
  }
  // --------------------------------------------------------------------------


  // -----   Execute reconstruction run   -------------------------------------
  void Run::Exec()
  {

    // --- Mirror options and configuration
    LOG(info) << GetName() << ": Output file is       " << fOutput;
    LOG(info) << GetName() << ": Digitization file is " << fRaw;
    LOG(info) << GetName() << ": Parameter file is    " << fPar;
    LOG(info) << GetName() << ": Geometry setup is    " << fSetupTag;
    LOG(info) << "Configuration: \n" << fConfig.ToString();

    // --- Timer
    TStopwatch timer;
    timer.Start();

    // --- Run info
    fRun.SetGenerateRunInfo(true);

    // --- Logger settings
    FairLogger::GetLogger()->SetLogScreenLevel(fConfig.f_glb_logLevel.data());
    FairLogger::GetLogger()->SetLogVerbosityLevel(fConfig.f_glb_logVerbose.data());
    FairLogger::GetLogger()->SetColoredLog(fConfig.f_glb_logColor.data());

    // --- Check input, output and parameter files
    if (!CheckFile(fRaw.Data())) throw std::runtime_error("Digitization (raw) file does not exist");
    if (!CheckFile(fPar.Data())) throw std::runtime_error("Parameter file does not exist");
    if (CheckFile(fOutput.Data()) && !fOverwrite) throw std::runtime_error("Output file already exists");

    // --- Input and output
    FairFileSource* source = new FairFileSource(fRaw);
    fRun.SetSource(source);
    fRun.SetSink(new FairRootFileSink(fOutput));

    // --- Check presence of input (digi) branches
    LOG(info) << GetName() << ": Checking digi input...";
    CheckInputBranches(source);

    // --- Geometry setup
    LOG(info) << GetName() << ": Loading setup " << fSetupTag;
    fSetup = CbmSetup::Instance();
    fSetup->LoadSetup(fSetupTag);
    // TODO: This CbmSetup business with singleton is not nice. Should be replaced eventually.

    // --- Topology
    LOG(info) << GetName() << ": Creating topology...";
    CreateTopology();

    // --- Parameter database
    LOG(info) << GetName() << ": Set runtime DB...";
    FairRuntimeDb* rtdb       = fRun.GetRuntimeDb();
    FairParRootFileIo* parIo1 = new FairParRootFileIo();
    parIo1->open(fPar.Data(), "UPDATE");
    rtdb->setFirstInput(parIo1);

    // --- Initialisation
    LOG(info) << GetName() << ": Initialise FairRun..." << std::endl;
    fRun.Init();
    rtdb->setOutput(parIo1);
    rtdb->saveOutput();
    rtdb->print();
    timer.Stop();
    double timeInit = timer.RealTime();
    timer.Start();

    // TODO: Register light ions (see run_reco.C). But what for?

    // --- Execute run
    std::cout << std::endl << std::endl;
    LOG(info) << GetName() << ": Starting run" << std::endl;
    fRun.Run(fConfig.f_glb_firstTs, fConfig.f_glb_numTs);

    // --- Finish
    timer.Stop();
    double timeExec = timer.RealTime();
    FairMonitor::GetMonitor()->Print();
    std::cout << std::endl << std::endl;
    LOG(info) << GetName() << ": Execution successful";
    LOG(info) << GetName() << ": Digitization file was " << fRaw;
    LOG(info) << GetName() << ": Parameter file was    " << fPar;
    LOG(info) << GetName() << ": Output file is        " << fOutput;
    LOG(info) << GetName() << ": Execution time: Init " << timeInit << " s, Exec " << timeExec << "s";
  }
  // --------------------------------------------------------------------------


  // -----  Read configuration from YAML file   -------------------------------
  void Run::LoadConfig(const char* fileName)
  {

    TString file(fileName);
    if (file.IsNull()) {
      file = std::getenv("VMCWORKDIR");
      file += "/reco/offline/config/RecoConfig_event_ideal.yaml";
    }
    LOG(info) << GetName() << ": Loading configuration from " << file;
    fConfig.LoadYaml(file.Data());
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::reco::offline

ClassImp(cbm::reco::offline::Run)
