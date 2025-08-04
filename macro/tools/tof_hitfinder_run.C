/* Copyright (C) 2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Adrian Weber [committer], Dominik Smith */


#include <math.h>
#include <stdio.h>
#include <string.h>

template<typename TofTaskType>
void CreateTask(TofTaskType* tofCluster, FairRunAna* run, TString& myName, TString& cFname, Int_t& calMode,
                Double_t& dDeadtime)
{
  tofCluster->SetCalParFileName(cFname);
  tofCluster->SetCalMode(calMode);
  tofCluster->SetTotMax(20.);                 // Tot upper limit for walk corection
  tofCluster->SetTotMin(0.);                  //(12000.);  // Tot lower limit for walk correction
  tofCluster->SetTotMean(5.);                 // Tot calibration target value in ns
  tofCluster->SetMaxTimeDist(1.0);            // default cluster range in ns
  tofCluster->SetChannelDeadtime(dDeadtime);  // artificial deadtime in ns
  tofCluster->PosYMaxScal(0.75);              //in % of length

  run->AddTask(tofCluster);
  std::cout << "-I- " << myName << ": Added task " << tofCluster->GetName() << std::endl;
}

/// FIXME: Disable clang formatting to keep easy parameters overview
/* clang-format off */
Bool_t tof_hitfinder_run(UInt_t uRunId                   = 2391,
                         Int_t nTimeslices               = 1,
                         TString sInpDir                 = "../beamtime/mcbm2022/data/",
                         TString sOutDir                 = "rec/",
                         Int_t iUnpFileIndex             = -1,
                         TString TofFileFolder           = "", // Your folder with the Tof Calibration files;
                         TString calibFile               = "",
                         TString alignmentMatrixFileName = "",
                         bool bGenerateYamlFiles         = false)
{
  /// FIXME: Re-enable clang formatting after parameters initial values setting
  /* clang-format on */

  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------


  // -----   Environment   --------------------------------------------------
  TString myName = "tof_hitfinder_run";            // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------


  // -----   In- and output file names   ------------------------------------
  /// Standardized RUN ID
  TString sRunId = TString::Format("%04u", uRunId);

  /// Initial pattern
  TString inFile = sInpDir + "/" + sRunId + ".digi";

  TString parFileOut = sOutDir + "/reco_event_mcbm_params_" + sRunId;
  TString outFile    = sOutDir + "/reco_event_mcbm_" + sRunId;

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

  // -----   TOF defaults ------------------------
  Int_t calMode      = 93;
  Int_t calSel       = 1;
  Int_t calSm        = 0;
  Int_t RefSel       = 0;
  Double_t dDeadtime = 50.;
  Int_t iSel2        = 20;  //500;
  // ------------------------------------------------------------------------

  // -----   TOF Calibration Settings ---------------------------------------
  TString cCalId = "490.100.5.0";
  if (uRunId >= 759) cCalId = "759.100.4.0";
  if (uRunId >= 812) cCalId = "831.100.4.0";
  if (uRunId >= 1588) cCalId = "1588.50.6.0";
  if (uRunId >= 2160) cCalId = "2160.50.4.0";
  if (uRunId >= 2352) cCalId = "2365.5.lxbk0600";
  if (uRunId >= 2391) cCalId = "2391.5.lxbk0598";
  Int_t iCalSet = 30040500;  // calibration settings
  if (uRunId >= 759) iCalSet = 10020500;
  if (uRunId >= 812) iCalSet = 10020500;
  if (uRunId >= 1588) iCalSet = 12002002;
  if (uRunId >= 2160) iCalSet = 700900500;
  if (uRunId >= 2352) iCalSet = 42032500;
  if (uRunId >= 2391) iCalSet = 22002500;

  const Int_t iTofCluMode = 1;

  if (TofFileFolder.Length() != 0) {
    // Use user provided path
    TofFileFolder = Form("%s/%s", TofFileFolder.Data(), cCalId.Data());
  }
  else {
    // use default path
    TofFileFolder = srcDir + "parameter/mcbm/";
    // TofFileFolder = "../run/data/" + cCalId;
  }
  // ------------------------------------------------------------------------

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

  TString geoFile    = sInpDir + "/" + geoSetupTag + ".geo.root";
  CbmSetup* geoSetup = CbmSetup::Instance();
  geoSetup->LoadSetup(geoSetupTag);

  // You can modify the pre-defined setup by using
  geoSetup->SetActive(ECbmModuleId::kTof, kTRUE);

  //-----  Load Parameters --------------------------------------------------
  TList* parFileList = new TList();

  // ----- TOF digitisation parameters -------------------------------------
  TString geoTag;
  if (geoSetup->IsActive(ECbmModuleId::kTof)) {
    geoSetup->GetGeoTag(ECbmModuleId::kTof, geoTag);
    TObjString* tofBdfFile = new TObjString(srcDir + "/parameters/tof/tof_" + geoTag + ".digibdf.par");
    parFileList->Add(tofBdfFile);
    std::cout << "-I- " << myName << ": Using parameter file " << tofBdfFile->GetString() << std::endl;
  }
  // ------------------------------------------------------------------------

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
  TString monitorFile {outFile};
  monitorFile.ReplaceAll("reco", "reco.monitor");
  FairMonitor::GetMonitor()->EnableMonitor(kTRUE, monitorFile);
  // ------------------------------------------------------------------------

  // =========================================================================
  // ===                        GEO Alignment                              ===
  // =========================================================================
  if (alignmentMatrixFileName.Length() != 0) {
    std::cout << "-I- " << myName << ": Applying alignment for file " << alignmentMatrixFileName << std::endl;

    // Define the basic structure which needs to be filled with information
    // This structure is stored in the output file and later passed to the
    // FairRoot framework to do the (miss)alignment
    std::map<std::string, TGeoHMatrix>* matrices{nullptr};

    // read matrices from disk
    LOG(info) << "Filename: " << alignmentMatrixFileName;
    TFile* misalignmentMatrixRootfile = new TFile(alignmentMatrixFileName, "READ");
    if (misalignmentMatrixRootfile->IsOpen()) {
      gDirectory->GetObject("MisalignMatrices", matrices);
      misalignmentMatrixRootfile->Close();
    }
    else {
      LOG(error) << "Could not open file " << alignmentMatrixFileName << "\n Exiting";
      return kFALSE;
    }

    if (matrices) {
      run->AddAlignmentMatrices(*matrices);
    }
    else {
      LOG(error) << "Alignment required but no matrices found."
                 << "\n Exiting";
      return kFALSE;
    }
  }
  // -------------------------------------------------------------------------

  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());

  // =========================================================================
  // ===                        TOF Hitfinding                             ===
  // =========================================================================

  if (geoSetup->IsActive(ECbmModuleId::kTof)) {
    TString cFname;
    switch (iTofCluMode) {
      case 1: {
        cFname = Form("%s/%s_set%09d_%02d_%01dtofClust.hst.root", TofFileFolder.Data(), cCalId.Data(), iCalSet, calMode,
                      calSel);
        if (calibFile.Length() != 0) {
          // Use user provided calib file, overriding the automatic determination based on run ID
          cFname = calibFile;
        }

        if (bGenerateYamlFiles) {
          // For production of YAML files
          CbmTaskTofClusterizerParWrite* tofCluster = new CbmTaskTofClusterizerParWrite("Task TOF Clusterizer", 0, 1);
          CreateTask<CbmTaskTofClusterizerParWrite>(tofCluster, run, myName, cFname, calMode, dDeadtime);
        }
        else {
          // For actual processing
          CbmTaskTofClusterizer* tofCluster = new CbmTaskTofClusterizer("Task TOF Clusterizer", 0, 1);
          CreateTask<CbmTaskTofClusterizer>(tofCluster, run, myName, cFname, calMode, dDeadtime);
        }
        /*
        tofCluster->SetCalParFileName(cFname);
        tofCluster->SetCalMode(calMode);
        tofCluster->SetTotMax(20.);                 // Tot upper limit for walk corection
        tofCluster->SetTotMin(0.);                  //(12000.);  // Tot lower limit for walk correction
        tofCluster->SetTotMean(5.);                 // Tot calibration target value in ns
        tofCluster->SetMaxTimeDist(1.0);            // default cluster range in ns
        tofCluster->SetChannelDeadtime(dDeadtime);  // artificial deadtime in ns
        tofCluster->PosYMaxScal(0.75);              //in % of length

        run->AddTask(tofCluster);
        std::cout << "-I- " << myName << ": Added task " << tofCluster->GetName() << std::endl;
        */
      } break;

      default: {
        ;
      }
    }
    // -------------------------------------------------------------------------
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
