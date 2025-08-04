/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */

/**
 ** Start "timeslice + event" compatible event display with hits and reco tracks drawing. Need ROOT call w/o batch or
 ** auto-quit CLI options (no "-b", no "-q"). For more details see HowTo page.
 ** Parameters:
 ** - uRunId = Index of the mCBM run, used to auto-detected the corresponding setup tag
 ** - sRecoFile = full path to ".reco.root" file holding the reconstructed data (Hits and CbmRecoTracks)
 ** - sGeoFile = full path to ".geo.root" file holding the full geometry
 ** - sAlignFile = full path to geometry alignment file (FairRoot format)
 ** - sXmlGeoConfig = full path (no `~`) to an xml config file, allowing for example to set the visibility,
 **   transparency or color of any geometry node
 ** - sUnpFile = optional path to file holding the Digi data
 **/
void event_display_l1reco(uint32_t uRunId, std::string sRecoFile, std::string sGeoFile, std::string sAlignFile = "",
                          std::string sXmlGeoConfig = "", std::string sUnpFile = "")
{
  // -----   Logger settings   -------------------------------------------------------------------------------------- //
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  fair::Logger::DefineVerbosity(
    "user1", fair::VerbositySpec::Make(fair::VerbositySpec::Info::severity, fair::VerbositySpec::Info::file_line));
  FairLogger::GetLogger()->SetLogVerbosityLevel("user1");
  // ---------------------------------------------------------------------------------------------------------------- //


  // -----   CbmSetup   ----------------------------------------------------------------------------------------------------------------- //
  /// Do automatic mapping only if not overridden by user or empty
  cbm::mcbm::ToForceLibLoad dummy;  /// Needed to trigger loading of the library as no fct dict in ROOT6 and CLING
  TString geoSetupTag = "";
  try {
    geoSetupTag = cbm::mcbm::GetSetupFromRunId(uRunId);
  }
  catch (const std::invalid_argument& e) {
    std::cout << "Error in mapping from runID to setup name: " << e.what() << std::endl;
    return kFALSE;
  }

  auto geoSetup = CbmSetup::Instance();
  geoSetup->LoadSetup(geoSetupTag);

  // You can modify the pre-defined setup by using
  geoSetup->SetActive(ECbmModuleId::kMvd, kFALSE);
  geoSetup->SetActive(ECbmModuleId::kSts, kTRUE);
  geoSetup->SetActive(ECbmModuleId::kMuch, kFALSE);
  geoSetup->SetActive(ECbmModuleId::kRich, kTRUE);
  geoSetup->SetActive(ECbmModuleId::kTrd, kTRUE);
  geoSetup->SetActive(ECbmModuleId::kTrd2d, kTRUE);
  geoSetup->SetActive(ECbmModuleId::kTof, kTRUE);
  geoSetup->SetActive(ECbmModuleId::kPsd, kFALSE);
  // ---------------------------------------------------------------------------------------------------------------- //

  // -----   Reconstruction run   ----------------------------------------------------------------------------------- //
  FairRunAna* fRun = new FairRunAna();

  FairFileSource* inputSource = new FairFileSource(sRecoFile);
  if ("" != sUnpFile) inputSource->AddFriend(sUnpFile);
  fRun->SetSource(inputSource);

  FairRootFileSink* outputSink = new FairRootFileSink("temp_event_display.root");
  fRun->SetSink(outputSink);

  fRun->SetGeomFile(sGeoFile.data());

  if ("" != sAlignFile) {
    std::cout << "Applying alignment for file " << sAlignFile << std::endl;

    // Define the basic structure which needs to be filled with information
    // This structure is stored in the output file and later passed to the
    // FairRoot framework to do the (miss)alignment
    std::map<std::string, TGeoHMatrix>* matrices{nullptr};

    // read matrices from disk
    TFile* misalignmentMatrixRootfile = new TFile(sAlignFile.data(), "READ");
    if (misalignmentMatrixRootfile->IsOpen()) {
      gDirectory->GetObject("MisalignMatrices", matrices);
      misalignmentMatrixRootfile->Close();
    }
    else {
      std::cout << "Could not open file " << sAlignFile << "\n Exiting";
      return kFALSE;
    }

    if (matrices) {
      fRun->AddAlignmentMatrices(*matrices);
    }
    else {
      LOG(error) << "Alignment required but no matrices found."
                 << "\n Exiting";
      return kFALSE;
    }
  }
  // ---------------------------------------------------------------------------------------------------------------- //

  // -----   Event display   ---------------------------------------------------------------------------------------- //
  CbmTimesliceManager* fMan = new CbmTimesliceManager();
  fMan->SetXMLConfig(sXmlGeoConfig);
  fMan->SetDisplayMcbm();
  // ---------------------------------------------------------------------------------------------------------------- //

  fMan->Init(1, 7);  // Make all sensors visible, finer tuning needs to be done in XML file
  // fMan->Init(1, 4);
  // fMan->Init(1, 5);  //make TPF and TRD visible by default
  // fMan->Init(1, 6);  // make STS sensors visible by default
  // fMan->Init(1, 7);  // make RICH sensors visible by default

  //-------------- NH display macro --------------------------------------------------------------------------------- //
  cout << "customize TEveManager gEve " << gEve << endl;
  gEve->GetDefaultGLViewer()->SetClearColor(kYellow - 10);
  TGLViewer* v       = gEve->GetDefaultGLViewer();
  TGLAnnotation* ann = new TGLAnnotation(v, Form("%u", uRunId), 0.01, 0.98);
  ann->SetTextSize(0.03);  // % of window diagonal
  ann->SetTextColor(4);
  // ---------------------------------------------------------------------------------------------------------------- //
}
