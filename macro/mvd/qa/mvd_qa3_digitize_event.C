/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Volker Friese */

// --------------------------------------------------------------------------
//
// Macro for local MVD reconstruction from MC data
//
// Tasks:  CbmMvdHitProducer
//
//
// V. Friese   06/02/2007
//
// --------------------------------------------------------------------------
void mvd_qa3_digitize_event(const char* setup = "sis100_electron")
{

  // ========================================================================
  //          Adjust this part according to your requirements

  TString inDir = gSystem->Getenv("VMCWORKDIR");

  TString outDir = "data/";
  // Input file (MC events)
  TString inFile = outDir + "mvd.mcQA.root";

  // Parameter file name
  TString parFile = outDir + "params.root";

  // Output file
  TString outFile = outDir + "mvd.ev.rawQA.root";

  // Background file (MC events, for pile-up)
  TString bgFile = inFile;

  // Delta file (Au Ions)
  TString deltaFile = outDir + "mvd.mcDelta.root";

  // Number of events to process
  Int_t nEvents = 5;

  // Verbosity level (0=quiet, 1=event level, 2=track level, 3=debug)
  Int_t iVerbose = 3;

  FairLogger* logger = FairLogger::GetLogger();
  logger->SetLogScreenLevel("INFO");
  logger->SetLogVerbosityLevel("LOW");

  TString setupFile  = inDir + "/geometry/setup/setup_" + setup + ".C";
  TString setupFunct = "setup_";
  setupFunct         = setupFunct + setup + "()";

  gROOT->LoadMacro(setupFile);
  gInterpreter->ProcessLine(setupFunct);

  // In general, the following parts need not be touched
  // ========================================================================


  // ----    Debug option   -------------------------------------------------
  gDebug = 0;
  // ------------------------------------------------------------------------


  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------


  // -------   MVD Digitiser   ----------------------------------------------
  CbmMvdDigitizer* digi = new CbmMvdDigitizer("MVDDigitiser", 0, iVerbose);
  std::cout << "Adding Task:  CbmMvdDigitiser... " << std::endl;
  //  digi->ShowDebugHistograms();

  // -----   Digitization run   ---------------------------------------------
  CbmDigitization run;


  run.SetOutputFile(outFile, kTRUE);
  run.SetParameterRootFile(parFile);
  run.SetMode(cbm::sim::Mode::EventByEvent);
  cbm::sim::TimeDist timeDist = cbm::sim::TimeDist::Uniform;
  run.SetDigitizer(ECbmModuleId::kMvd, digi);

  run.AddInput(0, inFile, timeDist, -1);

  run.Run(nEvents);
  // ------------------------------------------------------------------------


  // -----   Finish   ----------------------------------------------------------
  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << std::endl << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "Output file is " << outFile << std::endl;
  std::cout << "Parameter file is " << parFile << std::endl;
  std::cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << std::endl;
  std::cout << std::endl;
  // ---------------------------------------------------------------------------

  std::cout << " Test passed" << std::endl;
  std::cout << " All ok " << std::endl;
  //digi->Finish();

  /*digi->CollectHistograms();
  TObjArray* digiHisto=digi->GetHistograms();
  std::cout << "Received HistoArray with Address " << digiHisto << std::endl;
  std::cout << "The number of entries is  " << digiHisto->GetEntriesFast() << std::endl;
  ((TH1*)digiHisto->At(0))->Draw();*/
}
