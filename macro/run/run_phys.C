/* Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: M. Zyzak, Anna Senger, Volker Friese [committer] */

//---------------------------------------------------------------------------------
// @author  M. Zyzak
// @version 1.0
// @since   15.08.14
//
// Anna Senger: update for APR20 release
// 23.05.2020
//
// pID: 0 - without pID
//      1 - MC pID
//      2 - reconstructed pID (TOF, RICH, TRD, MUCH)
//
// macro to reconstruct particles from signal events by KFParticleFinder
//_________________________________________________________________________________

void run_phys(Int_t nEvents = 2, TString dataset = "test", TString setupName = "sis100_electron", Int_t pID = 2,
              Bool_t useMC = true)
{
  TStopwatch timer;
  timer.Start();

  const int firstEventToRun = 0;
  const int lastEventToRun  = firstEventToRun + nEvents - 1;

  TString parFile  = dataset + ".par.root";
  TString traFile  = dataset + ".tra.root";
  TString digiFile = dataset + ".raw.root";
  TString recFile  = dataset + ".reco.root";

  TString sinkFile  = dataset + ".phys.root";                        // dummy file
  TString effFile   = dataset + ".Efficiency_KFParticleFinder.txt";  // efficiency for all particles
  TString histoFile = dataset + ".KFParticleFinder.root";            // output histograms

  CbmSetup* setup = CbmSetup::Instance();
  setup->LoadSetup(setupName);

  TList* parFileList = new TList();

  // -----   Reconstruction run   -------------------------------------------
  FairRunAna* run             = new FairRunAna();
  FairFileSource* inputSource = new FairFileSource(recFile);
  inputSource->AddFriend(digiFile);
  if (useMC) {
    inputSource->AddFriend(traFile);
  }

  run->SetSource(inputSource);

  FairRootFileSink* sink = new FairRootFileSink(sinkFile);
  run->SetSink(sink);
  run->SetGenerateRunInfo(kFALSE);

  // ------------------------------------------------------------------------

  // ----- MC Data Manager   ------------------------------------------------
  if (useMC) {
    CbmMCDataManager* mcManager = new CbmMCDataManager("MCManager");
    mcManager->AddFile(traFile);
    run->AddTask(mcManager);

    CbmMatchRecoToMC* matchTask = new CbmMatchRecoToMC();
    // NOTE: Matching of hits and clusters is suppressed when the matches are already present in the tree.
    matchTask->SuppressHitReMatching();
    run->AddTask(matchTask);
  }

  // detector interfaces

  run->AddTask(new CbmTrackingDetectorInterfaceInit());

  //          Adjust this part according to your requirements

  CbmL1* l1 = new CbmL1("CbmL1");
  run->AddTask(l1);

  CbmKF* KF = new CbmKF();
  run->AddTask(KF);

  // ----- PID for KF Particle Finder --------------------------------------------
  CbmKFParticleFinderPID* kfParticleFinderPID = new CbmKFParticleFinderPID();
  kfParticleFinderPID->SetPIDMode(pID);
  // kfParticleFinderPID->SetSIS100();
  //  kfParticleFinderPID->UseMuch();
  //  kfParticleFinderPID->UseTRDANNPID();
  //  kfParticleFinderPID->UseRICHRvspPID();
  run->AddTask(kfParticleFinderPID);

  // ----- KF Particle Finder --------------------------------------------
  CbmKFParticleFinder* kfParticleFinder = new CbmKFParticleFinder();
  kfParticleFinder->SetPIDInformation(kfParticleFinderPID);
  kfParticleFinder->ReconstructSinglePV();
  //  kfParticleFinder->UseMCPV();
  //  kfParticleFinder->SetSuperEventAnalysis(); // SuperEvent
  run->AddTask(kfParticleFinder);

  // ----- KF Particle Finder QA --------------------------------------------
  CbmKFParticleFinderQa* kfParticleFinderQA =
    new CbmKFParticleFinderQa("CbmKFParticleFinderQa", 0, kfParticleFinder->GetTopoReconstructor(), histoFile.Data());
  kfParticleFinderQA->SetPrintEffFrequency(nEvents);
  //  kfParticleFinderQA->SetSuperEventAnalysis(); // SuperEvent
  kfParticleFinderQA->SetEffFileName(effFile.Data());
  run->AddTask(kfParticleFinderQA);

  // -----  Parameter database   --------------------------------------------
  FairRuntimeDb* rtdb        = run->GetRuntimeDb();
  FairParRootFileIo* parIo1  = new FairParRootFileIo();
  FairParAsciiFileIo* parIo2 = new FairParAsciiFileIo();
  parIo1->open(parFile.Data(), "UPDATE");
  rtdb->setFirstInput(parIo1);
  if (!parFileList->IsEmpty()) {
    parIo2->open(parFileList, "in");
    rtdb->setSecondInput(parIo2);
  }
  // ------------------------------------------------------------------------

  // -----   Intialise and run   --------------------------------------------
  run->Init();

  rtdb->setOutput(parIo1);
  rtdb->saveOutput();
  rtdb->print();

  KFPartEfficiencies eff;
  for (int jParticle = eff.fFirstStableParticleIndex + 10; jParticle <= eff.fLastStableParticleIndex; jParticle++) {
    TDatabasePDG* pdgDB = TDatabasePDG::Instance();

    if (!pdgDB->GetParticle(eff.partPDG[jParticle])) {
      pdgDB->AddParticle(eff.partTitle[jParticle].data(), eff.partTitle[jParticle].data(), eff.partMass[jParticle],
                         kTRUE, 0, eff.partCharge[jParticle] * 3, "Ion", eff.partPDG[jParticle]);
    }
  }

  cout << "Starting run" << endl;
  run->Run(firstEventToRun, lastEventToRun + 1);
  // ------------------------------------------------------------------------

  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  cout << "Macro finished succesfully." << endl;
  cout << "Output file is " << sinkFile << endl;
  cout << "Parameter file is " << parFile << endl;
  printf("RealTime=%f seconds, CpuTime=%f seconds\n", rtime, ctime);

  RemoveGeoManager();
}
