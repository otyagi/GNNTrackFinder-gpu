/* Copyright (C) 2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Adrian Weber [committer], Dominik Smith */


#include <math.h>
#include <stdio.h>
#include <string.h>

/// FIXME: Disable clang formatting to keep easy parameters overview
/* clang-format off */
Bool_t trd_hitfinder_run(UInt_t uRunId                   = 2457,
                       Int_t nTimeslices               = 5,
 //                      Int_t nTimeslices               = 1,
                       TString sInpDir                 = "/home/dsmith/cbmroot/macro/beamtime/mcbm2022/data/",
                       TString sOutDir                 = "rec/",
                       Int_t iUnpFileIndex             = -1)
{
  /// FIXME: Re-enable clang formatting after parameters initial values setting
  /* clang-format on */

  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------


  // -----   Environment   --------------------------------------------------
  TString myName = "trd_hitfinder_run";            // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------


  // -----   In- and output file names   ------------------------------------
  /// Standardized RUN ID
  TString sRunId = TString::Format("%04u", uRunId);

  /// Initial pattern
  //TString inFile = sInpDir + "/" + sRunId + "_faspMixed_10ts.digi";
  TString inFile = sInpDir + "/" + sRunId + ".digi";

  TString parFileOut = sOutDir + "/reco_event_mcbm_params_" + sRunId;
  TString outFile    = sOutDir + "/reco_event_mcbm_" + sRunId;

  // Your folder with the Tof Calibration files;
  TString TofFileFolder = "/home/dsmith/cbmroot/macro/run/data/";

  /// Add index of splitting at unpacking level if needed
  if (0 <= iUnpFileIndex) {
    inFile += TString::Format("_%02u", iUnpFileIndex);
    // the input par file is not split during unpacking!
    parFileOut += TString::Format("_%02u", iUnpFileIndex);
    outFile += TString::Format("_%02u", iUnpFileIndex);
  }  // if ( 0 <= uUnpFileIndex )
  /// Add ROOT file suffix
  inFile += ".root";
  parFileOut += ".root";
  outFile += ".root";
  // ---------------------------------------------


  // --- Load the geometry setup ----
  // This is currently only required by the TRD (parameters)
  cbm::mcbm::ToForceLibLoad dummy;  /// Needed to trigger loading of the library as no fct dict in ROOT6 and CLING
  TString geoSetupTag = "";
  try {
    geoSetupTag = cbm::mcbm::GetSetupFromRunId(uRunId);
  }
  catch (const std::invalid_argument& e) {
    std::cout << "Error in mapping from runID to setup name: " << e.what() << std::endl;
    return kFALSE;
  }

  TString geoFile    = srcDir + "/macro/mcbm/data/" + geoSetupTag + ".geo.root";
  CbmSetup* geoSetup = CbmSetup::Instance();
  geoSetup->LoadSetup(geoSetupTag);

  // You can modify the pre-defined setup by using
  geoSetup->SetActive(ECbmModuleId::kTof, kTRUE);

  //-----  Load Parameters --------------------------------------------------
  TList* parFileList = new TList();

  // ----- TRD digitisation parameters -------------------------------------
  TString geoTagTrd;
  if (geoSetup->IsActive(ECbmModuleId::kTrd)) {
    if (geoSetup->GetGeoTag(ECbmModuleId::kTrd, geoTagTrd)) {
      TString paramFilesTrd(Form("%s/parameters/trd/trd_%s", srcDir.Data(), geoTagTrd.Data()));
      std::vector<TString> paramFilesVecTrd = {"asic", "digi", "gas", "gain"};
      for (auto parIt : paramFilesVecTrd) {
        parFileList->Add(new TObjString(Form("%s.%s.par", paramFilesTrd.Data(), parIt.Data())));
      }
    }
    for (auto parFileVecIt : *parFileList) {
      std::cout << Form("TrdParams - %s - added to parameter file list", parFileVecIt->GetName()) << std::endl;
    }
  }

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------


  // -----   FairRunAna   ---------------------------------------------------
  FairRunAna* run             = new FairRunAna();
  FairFileSource* inputSource = new FairFileSource(inFile);
  run->SetSource(inputSource);

  FairRootFileSink* outputSink = new FairRootFileSink(outFile);
  run->SetSink(outputSink);
  run->SetGeomFile(geoFile);

  // Define output file for FairMonitor histograms
  TString monitorFile{outFile};
  monitorFile.ReplaceAll("reco", "reco.monitor");
  FairMonitor::GetMonitor()->EnableMonitor(kTRUE, monitorFile);
  // ------------------------------------------------------------------------


  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());


  // =========================================================================
  // ===                 local TRD Reconstruction                          ===
  // =========================================================================

  bool bTRD   = true;
  bool bTRD2d = true;
  if ((bTRD && geoSetup->IsActive(ECbmModuleId::kTrd))
      || (bTRD2d && (geoSetup->IsActive(ECbmModuleId::kTrd2d) || geoSetup->IsActive(ECbmModuleId::kTrd)))) {
    //Double_t triggerThreshold = 0.5e-6;  // SIS100

    CbmTaskTrdHitFinderParWrite* trdHitfinderPar = new CbmTaskTrdHitFinderParWrite();
    run->AddTask(trdHitfinderPar);

    CbmTaskTrdHitFinder* trdHitfinder = new CbmTaskTrdHitFinder();
    run->AddTask(trdHitfinder);

    // CbmTrdClusterFinder* trdCluster;
    // trdCluster = new CbmTrdClusterFinder();
    // trdCluster->SetNeighbourEnable(true, false);
    // trdCluster->SetMinimumChargeTH(triggerThreshold);
    // trdCluster->SetRowMerger(true);
    // run->AddTask(trdCluster);
    //  std::cout << "-I- : Added task " << trdCluster->GetName() << std::endl;

    // CbmTrdHitProducer* trdHit = new CbmTrdHitProducer();
    // run->AddTask(trdHit);
    // std::cout << "-I- : Added task " << trdHit->GetName() << std::endl;
  }

  // -----  Parameter database   --------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Set runtime DB" << std::endl;
  FairRuntimeDb* rtdb        = run->GetRuntimeDb();
  FairParRootFileIo* parIo1  = new FairParRootFileIo();
  FairParAsciiFileIo* parIo2 = new FairParAsciiFileIo();
  FairParRootFileIo* parIo3  = new FairParRootFileIo();
  parIo2->open(parFileList, "in");
  rtdb->setSecondInput(parIo2);
  parIo3->open(parFileOut.Data(), "RECREATE");
  // ------------------------------------------------------------------------
  rtdb->setOutput(parIo3);
  rtdb->saveOutput();
  rtdb->print();

  // -----   Run initialisation   -------------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Initialise run" << std::endl;
  run->Init();


  // -----   Start run   ----------------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Starting run" << std::endl;
  run->Run(0, nTimeslices);
  // ------------------------------------------------------------------------

  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  FairMonitor::GetMonitor()->Print();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << std::endl << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "Output file is " << outFile << std::endl;
  std::cout << "Parameter file is " << parFileOut << std::endl;
  std::cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << std::endl;
  std::cout << std::endl;
  // ------------------------------------------------------------------------


  // -----   Resource monitoring   ------------------------------------------
  // Extract the maximal used memory an add is as Dart measurement
  // This line is filtered by CTest and the value send to CDash
  FairSystemInfo sysInfo;
  Float_t maxMemory = sysInfo.GetMaxMemory();
  std::cout << "<DartMeasurement name=\"MaxMemory\" type=\"numeric/double\">";
  std::cout << maxMemory;
  std::cout << "</DartMeasurement>" << std::endl;

  Float_t cpuUsage = ctime / rtime;
  std::cout << "<DartMeasurement name=\"CpuLoad\" type=\"numeric/double\">";
  std::cout << cpuUsage;
  std::cout << "</DartMeasurement>" << std::endl;
  // ------------------------------------------------------------------------

  /// --- Screen output for automatic tests
  std::cout << " Test passed" << std::endl;
  std::cout << " All ok " << std::endl;

  return kTRUE;
}
