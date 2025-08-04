/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   mcbm_hadron_kfp_ana.C
/// \brief  Macro for hadron analysis with the KfParticleFinder in mCBM
/// \since  08.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>


/// \brief  Main function of the macro
/// \param  uRunId          Run identifier
/// \param  firstTimeslice  First timeslice
/// \param  nTimeslices     Number of timeslices to proceed
/// \param  sRecoFile       Input with reconstructed data tree
/// \param  sCollTraFile    Input with transported collision MC data tree [optional]
/// \param  sSignTraFile    Input with transported signal MC data tree [optional]
/// \param  sBeamTraFile    Input with transported beam MC data tree [optional]
/// \param  sSinkFile       Output file
/// \param  sGeoFile        Geometry file
/// \param  sParInFile      Input parameters file
/// \param  bUseAlignment   Flag: use alignment
/// \param  bMixEvent       Flag: perform mixed-event analysis
/// \param  bUseMc          Flag: use MC-information (if available)
///
/// \note File extention conventions used:
///        1. Input:
///         - geometry:         <geoSetupTag>.geo.root
///         - reco:             <dataLabel>.rec.root
///         - transport:
///             - collisions:   <genLabel>.tra.root
///             - signal:       <genLabel>.tra.root
///             - beam:         <genLabel>.tra.root
///        2. Output:
///         - sink:             <dataLabel>.kfp.ana.root
///         - monitor:          <dataLabel>.kfp.mon.root
///         - parameters:       <dataLabel>.kfp.par.root
///
///        <geoSetupTag> -- geometry setup tag provided by cbm::mcbm::GetSetupFromRunId(uRunId)
///        <dataLabel>   -- label of data files reflecting the name prefix of the original TSA file
///                           E.g.:  <dataLabel> = 2391_node8_0_0055
///        <genLabel>    -- label of the simulated files reflecting the name prefix of the original generated file:
///                           E.g.:  <genLabel>  = urqmd.auau.1.24gev.mbias.00109
///
///  TODO: Probably, use a combined label for output: <dataLabel>.<genLabel>.kfp.ana.root
///
/* clang-format off */
Bool_t mcbm_hadron_kfp_ana(UInt_t uRunId         = 2391,
                           Int_t firstTimeslice  = 0,
                           Int_t nTimeslices     = 0,
                           TString sRecoFile     = "./data/2391_first20Ts.rec.root",
                           TString sCollTraFile  = "",
                           TString sSignTraFile  = "",
                           TString sBeamTraFile  = "",
                           TString sSinkFile     = "./data/2391_first20Ts.kfp.ana.root",
                           TString sGeoFile      = "./data/mcbm_beam_2022_05_23_nickel.geo.root",
                           TString sParInFile    = "./data/2391_first20Ts.par.root",
                           Bool_t bUseAlignment  = true,
                           Bool_t bMixEvent      = false,
                           Bool_t bUseMc         = false
)
/* clang-format on */
{
  // -- Logger settings ------------------------------------------------------------------------------------------------
  const TString logLevel{"INFO"};               // Logger level
  const TString logVerbosity{"LOW"};            // Logger verbosity
  const bool logColored{true};                  // Colored logger
  const TString myName{"mcbm_hadron_kfp_ana"};  // Macro name
  // -------------------------------------------------------------------------------------------------------------------


  // -- Environment settings -------------------------------------------------------------------------------------------
  TString sOutDir{};
  if (auto lastSlashPos = sSinkFile.Last('/'); lastSlashPos != -1) {
    sOutDir = sSinkFile(0, lastSlashPos);
  }
  else {
    sOutDir = "./";
  }
  const TString srcDir{gSystem->Getenv("VMCWORKDIR")};  // Top source directory
  gSystem->Exec("mkdir " + sOutDir);                    // Output directory
  //gSystem->Exec("cp $VMCWORKDIR/macro/run/.rootrc .");  // Loading environment
  // -------------------------------------------------------------------------------------------------------------------


  // -- Auxilary common variables --------------------------------------------------------------------------------------
  const TString sRunId{TString::Format("%04u", uRunId)};  // String representation of the run id
  // -------------------------------------------------------------------------------------------------------------------


  // -- Geometry setup tag ---------------------------------------------------------------------------------------------
  cbm::mcbm::ToForceLibLoad dummy;  /// Needed to trigger loading of the library as no fct dict in ROOT6 and CLING
  TString geoSetupTag{""};
  try {
    geoSetupTag = cbm::mcbm::GetSetupFromRunId(uRunId);
  }
  catch (const std::invalid_argument& err) {
    std::cout << "-E- " << myName << ": mapping from runID to geometry setup tag failed: " << err.what() << '\n';
    return false;
  }
  auto* geoSetup = CbmSetup::Instance();
  geoSetup->LoadSetup(geoSetupTag);
  // -------------------------------------------------------------------------------------------------------------------


  // -- Start timer ----------------------------------------------------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // -------------------------------------------------------------------------------------------------------------------


  // -- FairRun initialisation -----------------------------------------------------------------------------------------
  FairRunAna* run = new FairRunAna();
  run->SetGeomFile(sGeoFile);
  FairFileSource* inputSource = new FairFileSource(sRecoFile);
  if (bUseMc) {
    int nTraFiles{0};
    if (!sCollTraFile.IsNull()) {
      inputSource->AddFriend(sCollTraFile);
      nTraFiles++;
    }
    if (!sSignTraFile.IsNull()) {
      inputSource->AddFriend(sSignTraFile);
      nTraFiles++;
    }
    if (!sBeamTraFile.IsNull()) {
      inputSource->AddFriend(sBeamTraFile);
      nTraFiles++;
    }
    if (nTraFiles == 0) {
      std::cout << "-E- " << myName << ": MC information was required, but no transport files were provided";
      return false;
    }
  }
  run->SetSource(inputSource);

  FairRootFileSink* outputSink = new FairRootFileSink(sSinkFile);
  run->SetSink(outputSink);

  TString sMonitorFile{sSinkFile};
  sMonitorFile.ReplaceAll(".ana.", ".mon.");
  FairMonitor::GetMonitor()->EnableMonitor(true, sMonitorFile);
  // -------------------------------------------------------------------------------------------------------------------


  // -- Logger settings ------------------------------------------------------------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());
  FairLogger::GetLogger()->SetColoredLog(logColored);
  //FairLogger::GetLogger()->SetLogScreenLevel("DEBUG");
  // -------------------------------------------------------------------------------------------------------------------


  // -- Alignment corrections ------------------------------------------------------------------------------------------
  if (bUseAlignment) {
    std::map<std::string, TGeoHMatrix>* matrices{nullptr};
    TString sAlignmentMatrixFileName = srcDir + "/parameters/mcbm/AlignmentMatrices_" + geoSetupTag + ".root";
    std::cout << "-I- " << myName << ": loading alignment matrices from file " << sAlignmentMatrixFileName << " ... ";
    TFile* misalignmentMatrixRootfile = TFile::Open(sAlignmentMatrixFileName, "READ");
    if (misalignmentMatrixRootfile && misalignmentMatrixRootfile->IsOpen()) {
      gDirectory->GetObject("MisalignMatrices", matrices);
      misalignmentMatrixRootfile->Close();
      std::cout << "done\n";
    }
    else {
      std::cout << "failed: running with misaligned geometry\n";
    }
    if (matrices) run->AddAlignmentMatrices(*matrices);
  }
  // -------------------------------------------------------------------------------------------------------------------


  // -- KF initialization ----------------------------------------------------------------------------------------------
  run->AddTask(new CbmTrackingDetectorInterfaceInit());
  run->AddTask(new CbmKF());
  // -------------------------------------------------------------------------------------------------------------------


  // -- V0 Finder ------------------------------------------------------------------------------------------------------
  auto* pV0 = new cbm::kfp::V0FinderTask(/*verbose = */ 3);
  pV0->SetMixedEventMode(bMixEvent);
  pV0->SetProcessingMode(cbm::kfp::V0FinderTask::EProcessingMode::TimeBased);
  pV0->SetPidApproach(cbm::kfp::V0FinderTask::EPidApproach::Topo);
  pV0->SetPvFindingMode(cbm::kfp::V0FinderTask::EPvUsageMode::Target);
  pV0->SetConfigName(srcDir + "/macro/KF/configs/mcbm_kfpf_lambda.yaml");
  // QA output setting
  {
    TString sQaFile{sSinkFile};
    sQaFile.ReplaceAll(".kfp.ana.", ".qa.kfp.ana.");
    pV0->SetQaOutputFileName(sQaFile);
    pV0->SetRunQa(true);
  }
  run->AddTask(pV0);
  // -------------------------------------------------------------------------------------------------------------------


  // -- Parameter database ---------------------------------------------------------------------------------------------
  std::cout << "\n\n";
  std::cout << "-I- " << myName << ": setting runtime DB\n";
  FairRuntimeDb* rtdb       = run->GetRuntimeDb();
  FairParRootFileIo* parInp = new FairParRootFileIo();
  FairParRootFileIo* parOut = new FairParRootFileIo();
  parInp->open(sParInFile.Data(), "READ");
  rtdb->setFirstInput(parInp);
  TString sParOutFile{sSinkFile};
  sParOutFile.ReplaceAll(".ana.", ".par.");
  parOut->open(sParOutFile.Data(), "RECREATE");
  rtdb->setOutput(parOut);
  // ------------------------------------------------------------------------------------------------------------------


  // -- Run initialisation --------------------------------------------------------------------------------------------
  std::cout << "\n\n";
  std::cout << "-I- " << myName << ": initialising run\n";
  run->Init();
  rtdb->print();
  // ------------------------------------------------------------------------------------------------------------------


  // -- Run execution -------------------------------------------------------------------------------------------------
  std::cout << "\n\n";
  std::cout << "-I- " << myName << ": starting run\n";
  run->Run(firstTimeslice, nTimeslices);
  // ------------------------------------------------------------------------------------------------------------------


  // -- Finish --------------------------------------------------------------------------------------------------------
  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << "-I- " << myName << "Macro finished successfully.\n";
  std::cout << "-I- " << myName << "Output file is " << sSinkFile << '\n';
  std::cout << "-I- " << myName << "Parameter file is " << sParOutFile << '\n';
  std::cout << "-I- " << myName << "Real time " << rtime << " s, CPU time " << ctime << " s\n\n";
  // ------------------------------------------------------------------------------------------------------------------


  // -- Resourse monitoring -------------------------------------------------------------------------------------------
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
  // ------------------------------------------------------------------------------------------------------------------


  // -- Memory clean-up -----------------------------------------------------------------------------------------------
  RemoveGeoManager();
  // ------------------------------------------------------------------------------------------------------------------


  // -- Screen output for automatic tests -----------------------------------------------------------------------------
  std::cout << " Test passed" << std::endl;
  std::cout << " All ok " << std::endl;
  // ------------------------------------------------------------------------------------------------------------------


  return true;
}
