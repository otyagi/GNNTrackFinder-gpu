/* Copyright (C) 2019-2023 UGiessen, JINR-LIT
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Martin Beyer */

#if !defined(__CLING__)
#include "CbmL1RichRingQa.h"
#include "CbmLitTrackingQa.h"
#include "CbmMCDataManager.h"
#include "CbmMatchRecoToMC.h"
#include "CbmRichGeoTest.h"
#include "CbmRichMatchRings.h"
#include "CbmRichRecoQa.h"
#include "CbmRichUrqmdTest.h"
// #include "CbmRichRingFinderQa.h"
// #include "CbmRichTrackProjQa.h"
#include "CbmSetup.h"

#include <FairFileSource.h>
#include <FairLogger.h>
#include <FairMonitor.h>
#include <FairParAsciiFileIo.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>

#include <TGeoManager.h>
#include <TObjString.h>
#include <TROOT.h>
#include <TStopwatch.h>
#include <TTree.h>

#include <iostream>
#include <string>
#endif

void run_qa(TString traFile = "", TString parFile = "", TString digiFile = "", TString recoFile = "",
            TString qaFile      = "",  // i/o files
            Int_t nofTimeSlices = -1, TString geoSetup = "sis100_electron", TString resultDir = "",
            Bool_t monitor = true)
{
  TTree::SetMaxTreeSize(90000000000);

  // -----   Files   --------------------------------------------------------
  TString macroPath = __FILE__;
  TString macroDir  = macroPath(0, macroPath.Last('/') + 1);
  if (traFile.IsNull()) traFile = macroDir + "data/test.tra.root";
  if (parFile.IsNull()) parFile = macroDir + "data/test.par.root";
  if (digiFile.IsNull()) digiFile = macroDir + "data/test.digi.root";
  if (recoFile.IsNull()) recoFile = macroDir + "data/test.reco.root";
  if (qaFile.IsNull()) qaFile = macroDir + "data/test.qa.root";

  remove(qaFile);
  // ------------------------------------------------------------------------

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("LOW");
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  TString srcDir = gSystem->Getenv("VMCWORKDIR");
  // ------------------------------------------------------------------------

  // -----   Load the geometry setup   -------------------------------------
  CbmSetup* geo = CbmSetup::Instance();
  geo->LoadSetup(geoSetup);
  // ------------------------------------------------------------------------

  // -----   Parameter files as input to the runtime database   -------------
  TList* parFileList = new TList();
  TString geoTag;

  // - TRD digitisation parameters
  if (CbmSetup::Instance()->GetGeoTag(ECbmModuleId::kTrd, geoTag)) {
    const Char_t* npar[4] = {"asic", "digi", "gas", "gain"};
    TObjString* trdParFile(NULL);
    for (Int_t i(0); i < 4; i++) {
      trdParFile = new TObjString(srcDir + "/parameters/trd/trd_" + geoTag + "." + npar[i] + ".par");
      parFileList->Add(trdParFile);
    }
  }

  // - TOF digitisation parameters
  if (CbmSetup::Instance()->GetGeoTag(ECbmModuleId::kTof, geoTag)) {
    TObjString* tofBdfFile = new TObjString(srcDir + "/parameters/tof/tof_" + geoTag + ".digibdf.par");
    parFileList->Add(tofBdfFile);
  }
  // ------------------------------------------------------------------------

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // -----   FairRunAna   ---------------------------------------------------
  FairFileSource* inputSource = new FairFileSource(digiFile);
  inputSource->AddFriend(traFile);
  inputSource->AddFriend(recoFile);

  FairRunAna* run = new FairRunAna();
  run->SetSource(inputSource);
  run->SetGenerateRunInfo(kFALSE);

  FairRootFileSink* sink = new FairRootFileSink(qaFile);
  run->SetSink(sink);

  FairMonitor::GetMonitor()->EnableMonitor(monitor);
  // ------------------------------------------------------------------------

  // -----   MCDataManager   ------------------------------------------------
  CbmMCDataManager* mcManager = new CbmMCDataManager("MCManager", 0);
  mcManager->AddFile(traFile);
  run->AddTask(mcManager);
  // ------------------------------------------------------------------------

  // -----   Match reco to MC   ---------------------------------------------
  // CbmMatchRecoToMC* match = new CbmMatchRecoToMC();
  // run->AddTask(match);
  // ------------------------------------------------------------------------

  // -----   RichRingFinderQa   ---------------------------------------------
  // CbmRichRingFinderQa* finder = new CbmRichRingFinderQa();
  // run->AddTask(finder);
  // ------------------------------------------------------------------------

  // -----   CbmRichTrackProjQa   -------------------------------------------
  // CbmRichTrackProjQa* proj = new CbmRichTrackProjQa();
  // run->AddTask(proj);
  // ------------------------------------------------------------------------

  // -----   RICH Reco QA   -------------------------------------------------
  CbmRichRecoQa* richRecoQa = new CbmRichRecoQa();
  richRecoQa->SetOutputDir(resultDir.Data());
  run->AddTask(richRecoQa);
  // ------------------------------------------------------------------------

  // -----   RICH GeoTest   -------------------------------------------------
  // CbmRichGeoTest* geoTest = new CbmRichGeoTest();
  // geoTest->SetOutputDir(resultDir.Data());
  // run->AddTask(geoTest);
  // ------------------------------------------------------------------------

  // -----   RICH UrqmdTest   -----------------------------------------------
  // CbmRichUrqmdTest* urqmdTest = new CbmRichUrqmdTest();
  // urqmdTest->SetOutputDir(resultDir.Data());
  // run->AddTask(urqmdTest);
  // ------------------------------------------------------------------------

  // -----   Lit Tracking Qa   ----------------------------------------------
  // CbmLitTrackingQa* trackingQa = new CbmLitTrackingQa();
  // trackingQa->SetMinNofPointsSts(4);
  // trackingQa->SetUseConsecutivePointsInSts(true);
  // trackingQa->SetMinNofPointsTrd(2);
  // trackingQa->SetMinNofPointsMuch(10);
  // trackingQa->SetMinNofPointsTof(1);
  // trackingQa->SetQuota(0.7);
  // trackingQa->SetMinNofHitsTrd(2);
  // trackingQa->SetMinNofHitsMuch(10);
  // trackingQa->SetVerbose(0);
  // trackingQa->SetMinNofHitsRich(7);
  // trackingQa->SetQuotaRich(0.6);
  // trackingQa->SetPRange(12, 0., 6.);
  // // trackingQa->SetTrdAnnCut(trdAnnCut);
  // std::vector<std::string> trackCat, richCat;
  // trackCat.push_back("All");
  // trackCat.push_back("Electron");
  // richCat.push_back("Electron");
  // richCat.push_back("ElectronReference");
  // trackingQa->SetTrackCategories(trackCat);
  // trackingQa->SetRingCategories(richCat);
  // trackingQa->SetOutputDir(resultDir.Data());
  // run->AddTask(trackingQa);
  // ------------------------------------------------------------------------

  // -----  Parameter database   --------------------------------------------
  FairRuntimeDb* rtdb        = run->GetRuntimeDb();
  FairParRootFileIo* parIo1  = new FairParRootFileIo();
  FairParAsciiFileIo* parIo2 = new FairParAsciiFileIo();
  parIo1->open(parFile, "UPDATE");
  rtdb->setFirstInput(parIo1);
  if (!parFileList->IsEmpty()) {
    parIo2->open(parFileList, "in");
    rtdb->setSecondInput(parIo2);
  }
  // ------------------------------------------------------------------------

  // -----   Run initialisation   -------------------------------------------
  run->Init();
  rtdb->setOutput(parIo1);
  rtdb->saveOutput();
  rtdb->print();
  // ------------------------------------------------------------------------

  // -----   Start run   ----------------------------------------------------
  run->Run(0, nofTimeSlices);
  // ------------------------------------------------------------------------

  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  std::cout << std::endl;
  FairMonitor::GetMonitor()->Print();
  std::cout << std::endl;
  std::cout << "Macro finished succesfully." << std::endl;
  std::cout << "Output file is " << qaFile << std::endl;
  std::cout << "Parameter file is " << parFile << std::endl;
  std::cout << "Real time " << timer.RealTime() << " s, CPU time " << timer.CpuTime() << " s" << std::endl;
  if (gROOT->GetVersionInt() >= 60602) {
    gGeoManager->GetListOfVolumes()->Delete();
    gGeoManager->GetListOfShapes()->Delete();
    delete gGeoManager;
  }
  std::cout << std::endl << "Test passed" << std::endl << "All ok" << std::endl;
  // ------------------------------------------------------------------------
}
