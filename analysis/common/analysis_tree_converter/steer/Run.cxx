/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese, Florian Uhlig */

/** @file Run.cxx
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 27.10.2023
 **/

#include "Run.h"

#include "CbmSetup.h"

#include <FairEventHeader.h>
#include <FairFileSource.h>
#include <FairMonitor.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>
#include <FairTask.h>
#include <Logger.h>

#include "TaskFactory.h"
#include <TFile.h>
#include <TGeoManager.h>
#include <TStopwatch.h>
#include <TString.h>
#include <TTree.h>

#include <cassert>
#include <iostream>

#include <sys/stat.h>

namespace cbm::atconverter
{


  // -----   Constructor   ----------------------------------------------------
  Run::Run() : TNamed("Run", "CBM AnalysisTree Converter Run") {}
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


  // -----   Check existence of a file   --------------------------------------
  bool Run::CheckFile(const char* fileName)
  {
    struct stat buffer;
    return (stat(fileName, &buffer) == 0);
  }
  // --------------------------------------------------------------------------


  // -----   Check existence of reco branch   ---------------------------------
  void Run::CheckRecoBranch(TTree* tree, ECbmModuleId detector)
  {
    TString branchName = ToString(detector);
    branchName += "Hit";
    if (tree->GetBranchStatus(branchName.Data())) {
      LOG(info) << GetName() << ": Found branch " << branchName;
      fDataPresent.insert(detector);
    }
  }
  // --------------------------------------------------------------------------


  // -----   Check which input reco branches are present   --------------------
  void Run::CheckInputBranches(FairFileSource* source)
  {
    TFile* inFile = source->GetInFile();
    if (!inFile) throw std::runtime_error("No input file");
    auto* inTree = inFile->Get<TTree>("cbmsim");
    if (!inTree) throw std::runtime_error("No input tree");

    for (ECbmModuleId detector = ECbmModuleId::kMvd; detector != ECbmModuleId::kNofSystems; ++detector)
      CheckRecoBranch(inTree, detector);
  }
  // --------------------------------------------------------------------------


  // -----   Create the topology   --------------------------------------------
  void Run::CreateTopology()
  {

    TaskFactory fact(this);

    fact.RegisterMCDataManager(fTra);
    if (fConfig.f_glb_trackMatching) fact.RegisterTrackMatching();
    fact.RegisterCaTracking();
    fact.RegisterTrdPid();
    fact.RegisterConverterManager(fOutput);
  }
  // --------------------------------------------------------------------------


  // -----   Execute reconstruction run   -------------------------------------
  void Run::Exec()
  {

    // --- Mirror options and configuration
    LOG(info) << GetName() << ": Output file is         " << fOutput;
    for (auto traFile : fTra) {
      LOG(info) << GetName() << ": Transport file is      " << traFile;
      if (!CheckFile(traFile.Data())) throw std::runtime_error("Transport file does not exist");
    }
    LOG(info) << GetName() << ": Digitization file is   " << fRaw;
    LOG(info) << GetName() << ": Parameter file is      " << fPar;
    LOG(info) << GetName() << ": Reconstruction file is " << fReco;
    LOG(info) << GetName() << ": Geometry setup is      " << fSetupTag;
    LOG(info) << "Configuration: \n" << fConfig.ToString() << "\n";

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
    if (CheckFile(fOutput.Data()) && !fOverwrite) throw std::runtime_error("Output file already exists");
    if (!CheckFile(fRaw.Data())) throw std::runtime_error("Digitizazion (raw) file does not exist");
    if (!CheckFile(fPar.Data())) throw std::runtime_error("Parameter file does not exist");
    if (!CheckFile(fReco.Data())) throw std::runtime_error("Reconstruction file does not exist");

    // --- Input and output
    FairFileSource* source = new FairFileSource(fReco);
    for (auto traFile : fTra)
      source->AddFriend(traFile);
    source->AddFriend(fRaw);
    fRun.SetSource(source);
    fRun.SetSink(new FairRootFileSink(fOutput));

    // --- Check presence of input (reco) branches
    LOG(info) << GetName() << ": Checking reco input...";
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
    for (auto traFile : fTra)
      LOG(info) << GetName() << ": Transport file was      " << traFile;
    LOG(info) << GetName() << ": Digitization file was   " << fRaw;
    LOG(info) << GetName() << ": Parameter file was      " << fPar;
    LOG(info) << GetName() << ": Reconstruction file was " << fReco;
    LOG(info) << GetName() << ": Output file is          " << fOutput;
    LOG(info) << GetName() << ": Execution time: Init    " << timeInit << " s, Exec " << timeExec << "s";
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


  // -----  Set transport input sources   -------------------------------------
  void Run::SetTraFiles(const std::vector<std::string> files)
  {
    for (auto file : files) {
      const char* filename = file.c_str();
      TString traFile      = filename;
      fTra.push_back(traFile);
    }
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::atconverter

ClassImp(cbm::atconverter::Run)
