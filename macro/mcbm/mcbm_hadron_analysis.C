/* Copyright (C) 2020-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

void mcbm_hadron_analysis(Int_t nEvents = 10, TString setupName = "mcbm_beam_2020_03", const char* output = "data/test")
{
  TString dataset(output);
  TString InputFile = dataset + ".tra.root";
  TString DigiFile  = dataset + ".event.raw.root";
  TString RecoFile  = dataset + ".rec.root";
  TString ParFile   = dataset + ".par.root";
  TString OutFile   = dataset + ".ana.root";
  TString srcDir    = gSystem->Getenv("VMCWORKDIR");

  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("VERYHIGH");

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // -----   Reconstruction run   -------------------------------------------
  FairFileSource* inputFiles = new FairFileSource(InputFile.Data());
  inputFiles->AddFriend(RecoFile.Data());
  inputFiles->AddFriend(DigiFile.Data());

  FairRootFileSink* sink = new FairRootFileSink(OutFile.Data());

  FairRunAna* fRun = new FairRunAna();
  fRun->SetSource(inputFiles);
  fRun->SetSink(sink);

  // -----  Parameter database   --------------------------------------------
  FairRuntimeDb* rtdb          = fRun->GetRuntimeDb();
  FairParRootFileIo* parInput1 = new FairParRootFileIo();
  parInput1->open(ParFile.Data());
  rtdb->setFirstInput(parInput1);

  // -----   Load the geometry setup   -------------------------------------
  std::cout << std::endl;
  //  std::cout << "-I- " << myName << ": Loading setup " << setup << std::endl;
  CbmSetup* setup = CbmSetup::Instance();
  setup->LoadSetup(setupName);
  TString geoTag;
  // ------------------------------------------------------------------------

  // ----- MuCh digi parameters initialization --------------------------------------

  if (setup->IsActive(ECbmModuleId::kMuch)) {
    // Parameter file name
    setup->GetGeoTag(ECbmModuleId::kMuch, geoTag);
    Int_t muchFlag  = (geoTag.Contains("mcbm") ? 1 : 0);
    TString parFile = gSystem->Getenv("VMCWORKDIR");

    if (muchFlag) {
      std::cout << geoTag << std::endl;
      parFile = parFile + "/parameters/much/much_" + geoTag + "_digi_sector.root";
      std::cout << "Using parameter file " << parFile << std::endl;
    }
    else {
      std::cout << geoTag(0, 4) << std::endl;
      parFile = parFile + "/parameters/much/much_" + geoTag(0, 4) + "_digi_sector.root";
      std::cout << "Using parameter file " << parFile << std::endl;
    }

    // Initialization of the geometry scheme
    auto muchGeoScheme = CbmMuchGeoScheme::Instance();
    if (!muchGeoScheme->IsInitialized()) {
      muchGeoScheme->Init(parFile, muchFlag);
    }
  }

  /*
  CbmMCDataManager* mcManager = new CbmMCDataManager("MCDataManager", 1);
  mcManager->AddFile(InputFile);
  fRun->AddTask(mcManager);

  CbmMatchRecoToMC* match = new CbmMatchRecoToMC();
  fRun->AddTask(match);
*/

  // Kalman filter
  fRun->AddTask(new CbmTrackingDetectorInterfaceInit());
  fRun->AddTask(new CbmKF());

  CbmHadronAnalysis* HadronAna = new CbmHadronAnalysis();  // in hadron
  HadronAna->SetBeamMomentum(1.65);                        // beam momentum
  HadronAna->SetDY(0.5);                                   // flow analysis exclusion window
  HadronAna->SetRecSec(kTRUE);                             // enable lambda reconstruction
  Int_t parSet = 1;
  switch (parSet) {
    case 0:                             // with background
      HadronAna->SetDistPrimLim(1.2);   // Max Tof-Sts trans distance for primaries
      HadronAna->SetDistPrimLim2(0.3);  // Max Sts-Sts trans distance for primaries
      HadronAna->SetDistSecLim2(0.3);   // Max Sts-Sts trans distance from TOF direction for secondaries
      HadronAna->SetD0ProtLim(0.5);     // Min impact parameter for secondary proton
      HadronAna->SetOpAngMin(0.1);      // Min opening angle for accepting pair
      HadronAna->SetDCALim(0.1);        // Max DCA for accepting pair
      HadronAna->SetVLenMin(5.);        // Min Lambda flight path length for accepting pair
      HadronAna->SetVLenMax(25.);       // Max Lambda flight path length for accepting pair
      HadronAna->SetNMixedEvents(10);   // Number of events to be mixed with
      break;
    case 1:                             // signal only, debugging
      HadronAna->SetDistPrimLim(0.5);   // Max Tof-Sts trans distance for primaries
      HadronAna->SetDistPrimLim2(0.3);  // Max Sts-Sts trans distance for primaries
      HadronAna->SetDistSecLim2(0.3);   // Max Sts-Sts trans distance from TOF direction for secondaries
      HadronAna->SetD0ProtLim(0.4);     // Min impact parameter for secondary proton
      HadronAna->SetOpAngMin(0.1);      // Min opening angle for accepting pair
      HadronAna->SetDCALim(0.2);        // Max DCA for accepting pair
      HadronAna->SetVLenMin(5.);        // Min Lambda flight path length for accepting pair
      HadronAna->SetVLenMax(25.);       // Max Lambda flight path length for accepting pair
      HadronAna->SetNMixedEvents(10);   // Number of events to be mixed with
      break;
  }
  fRun->AddTask(HadronAna);

  // -----   Intialise and run   --------------------------------------------
  fRun->Init();
  cout << "Starting run" << endl;
  fRun->Run(0, nEvents);
  // ------------------------------------------------------------------------

  // save histos to file
  TFile* fHist = fRun->GetOutputFile();
  fHist->Write();

  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << std::endl << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "Output file is " << OutFile << std::endl;
  std::cout << "Parameter file is " << ParFile << std::endl;
  std::cout << "Real time " << rtime << " s, CPU time " << ctime << "s" << std::endl << std::endl;
  // ------------------------------------------------------------------------

  // -----   Resource monitoring   ------------------------------------------
  FairSystemInfo sysInfo;
  Float_t maxMemory = sysInfo.GetMaxMemory();
  std::cout << "<DartMeasurement name=\"MaxMemory\" type=\"numeric/double\">";
  std::cout << maxMemory;
  std::cout << "</DartMeasurement>" << std::endl;

  Float_t cpuUsage = ctime / rtime;
  std::cout << "<DartMeasurement name=\"CpuLoad\" type=\"numeric/double\">";
  std::cout << cpuUsage;
  std::cout << "</DartMeasurement>" << std::endl;


  std::cout << " Test passed" << std::endl;
  std::cout << " All ok " << std::endl;
  // ------------------------------------------------------------------------

  RemoveGeoManager();
}
