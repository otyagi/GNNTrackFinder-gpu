/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "CbmOnlineParWrite.h"

#include "CbmKfTrackingSetupBuilder.h"
#include "CbmL1.h"
#include "CbmMcbmUtils.h"
#include "CbmSetup.h"
#include "CbmSinkDummy.h"
#include "CbmSourceDummy.h"
#include "CbmStsAddress.h"
#include "CbmStsParAsic.h"
#include "CbmStsParModule.h"
#include "CbmStsParSensor.h"
#include "CbmStsParSensorCond.h"
#include "CbmStsParSetSensor.h"
#include "CbmStsSetup.h"
#include "CbmTaskStsHitFinderParWrite.h"
#include "CbmTaskTofClusterizerParWrite.h"
#include "CbmTaskTrdHitFinderParWrite.h"
#include "CbmTaskTrdUnpackParWrite.h"
#include "CbmTrackingDetectorInterfaceInit.h"
#include "CbmTrdParSetAsic.h"
#include "CbmTrdParSetDigi.h"

#include <FairParAsciiFileIo.h>
#include <FairParRootFileIo.h>
#include <FairRootFileSink.h>
#include <FairRunAna.h>
#include <FairRuntimeDb.h>

#include <TGeoManager.h>
#include <TObjString.h>
#include <TStopwatch.h>

#include <iostream>

using namespace cbm::algo;

void CbmOnlineParWrite::AddDetectors()
{
  // Add detectors here
  AddTrd();
  AddTof();
  AddSts();
}

// ===========================================================================
// TRD setup
// ===========================================================================
void CbmOnlineParWrite::AddTrd()
{
  // Copied from macro/beamtime/mcbm2022/trd_hitfinder_run.C
  fSetup->SetActive(ECbmModuleId::kTrd, kTRUE);
  fSetup->SetActive(ECbmModuleId::kTrd2d, kTRUE);

  // ----- TRD digitisation parameters -------------------------------------
  TString geoTagTrd;
  if (!fSetup->IsActive(ECbmModuleId::kTrd)) {
    throw std::runtime_error{"TRD not enabled in current geometry!"};
  }

  if (!fSetup->GetGeoTag(ECbmModuleId::kTrd, geoTagTrd)) {
    throw std::runtime_error{"Failed to get TRD geo tag from CbmSetup!"};
  }

  TString paramFilesTrd(Form("%s/parameters/trd/trd_%s", fSrcDir.Data(), geoTagTrd.Data()));
  std::vector<TString> paramFilesVecTrd = {"asic", "digi", "gas", "gain"};
  for (auto parIt : paramFilesVecTrd) {
    fParList->Add(new TObjString(Form("%s.%s.par", paramFilesTrd.Data(), parIt.Data())));
  }

  for (auto parFileVecIt : *fParList) {
    std::cout << Form("TrdParams - %s - added to parameter file list", parFileVecIt->GetName()) << std::endl;
  }

  // ----- TRD task ---------------------------------------------------------
  auto* trdHitfinderPar = new CbmTaskTrdHitFinderParWrite{};
  fRun->AddTask(trdHitfinderPar);


  // Initialize input files
  FairParAsciiFileIo asciiInput;
  std::string digiparfile = Form("%s/parameters/trd/trd_%s.digi.par", fSrcDir.Data(), geoTagTrd.Data());
  std::string asicparfile = Form("%s/parameters/trd/trd_%s.asic.par", fSrcDir.Data(), geoTagTrd.Data());

  // Read the .digi file and store result
  auto* digiparset = new CbmTrdParSetDigi{};
  if (asciiInput.open(digiparfile.data())) {
    digiparset->init(&asciiInput);
  }
  asciiInput.close();

  // Read the .asic file and store result
  auto* asicparset = new CbmTrdParSetAsic{};
  if (asciiInput.open(asicparfile.data())) {
    asicparset->init(&asciiInput);
  }
  asciiInput.close();

  // Unpack
  CbmTaskTrdUnpackParWrite::Pars parFilesUnpack{.asic = asicparset, .digi = digiparset, .setup = fConfig.setupType};
  auto* trdUnpackPar = new CbmTaskTrdUnpackParWrite{parFilesUnpack};
  fRun->AddTask(trdUnpackPar);
}

// ===========================================================================
// TOF setup
// ===========================================================================
void CbmOnlineParWrite::AddTof()
{
  // Copied from macro/tools/tof_hitfinder_run.C
  fSetup->SetActive(ECbmModuleId::kTof, kTRUE);

  TString geoTag;
  if (fSetup->IsActive(ECbmModuleId::kTof)) {
    fSetup->GetGeoTag(ECbmModuleId::kTof, geoTag);
    TObjString* tofBdfFile = new TObjString(fSrcDir + "/parameters/tof/tof_" + geoTag + ".digibdf.par");
    fParList->Add(tofBdfFile);
    std::cout << "-I- TOF: Using parameter file " << tofBdfFile->GetString() << std::endl;
  }

  // -----   TOF defaults ------------------------
  Int_t calMode      = 93;
  Int_t calSel       = 1;
  Double_t dDeadtime = 50.;

  TString TofFileFolder = fSrcDir + "/parameters/mcbm/";
  bool doCalibration    = false;
  TString cCalId        = "490.100.5.0";
  Int_t iCalSet         = 30040500;  // calibration settings
  TString cFname        = "";

  switch (fConfig.setupType) {
    case Setup::mCBM2022:
      doCalibration = true;
      cCalId        = "2391.5.000";
      iCalSet       = 22002500;
      break;
    case Setup::mCBM2024_03:
      doCalibration = true;
      cCalId        = "2912.1";
      iCalSet       = 012032500;
      break;
    case Setup::mCBM2024_05:
      doCalibration = true;
      cFname        = Form("%s/3026_1_TofCal.hst.root", TofFileFolder.Data());
      break;
    case Setup::mCBM2025_02:
      doCalibration = true;
      cFname        = Form("%s/3310_1_TofCal.hst.root", TofFileFolder.Data());
      break;
    default: throw std::runtime_error("TOF: Unknown setup type");
  }

  if (cFname.IsNull() && doCalibration) {
    cFname =
      Form("%s/%s_set%09d_%02d_%01dtofClust.hst.root", TofFileFolder.Data(), cCalId.Data(), iCalSet, calMode, calSel);
  }

  auto* tofCluster = new CbmTaskTofClusterizerParWrite("Task TOF Clusterizer", 0, 1);
  tofCluster->SetCalParFileName(cFname);
  tofCluster->SetCalMode(calMode);
  tofCluster->SetTotMax(20.);                 // Tot upper limit for walk corection
  tofCluster->SetTotMin(0.);                  //(12000.);  // Tot lower limit for walk correction
  tofCluster->SetTotMean(5.);                 // Tot calibration target value in ns
  tofCluster->SetMaxTimeDist(1.0);            // default cluster range in ns
  tofCluster->SetChannelDeadtime(dDeadtime);  // artificial deadtime in ns
  tofCluster->PosYMaxScal(0.75);              //in % of length
  fRun->AddTask(tofCluster);
}

// ===========================================================================
// STS setup
// ===========================================================================
void CbmOnlineParWrite::AddSts()
{
  // Copied from macro/beamtime/mcbm2022/mcbm_reco.C

  auto* recoSts = new CbmTaskStsHitFinderParWrite{};
  // recoSts->SetMode(ECbmRecoMode::EventByEvent);

  // recoSts->SetTimeCutDigisAbs(20.0);     // cluster finder: time cut in ns
  // recoSts->SetTimeCutClustersAbs(20.0);  // hit finder: time cut in ns

  TString geoTag;
  fSetup->GetGeoTag(ECbmModuleId::kTof, geoTag);
  LOG(info) << "STS geo tag: " << geoTag;

  // Sensor params
  CbmStsParSensor sensor6cm(CbmStsSensorClass::kDssdStereo);
  sensor6cm.SetPar(0, 6.2092);  // Extension in x
  sensor6cm.SetPar(1, 6.2);     // Extension in y
  sensor6cm.SetPar(2, 0.03);    // Extension in z
  sensor6cm.SetPar(3, 5.9692);  // Active size in y
  sensor6cm.SetPar(4, 1024.);   // Number of strips front side
  sensor6cm.SetPar(5, 1024.);   // Number of strips back side
  sensor6cm.SetPar(6, 0.0058);  // Strip pitch front side
  sensor6cm.SetPar(7, 0.0058);  // Strip pitch back side
  sensor6cm.SetPar(8, 0.0);     // Stereo angle front side
  sensor6cm.SetPar(9, 7.5);     // Stereo angle back side

  CbmStsParSensor sensor12cm(sensor6cm);  // copy all parameters, change then only the y size
  sensor12cm.SetPar(1, 12.4);             // Extension in y
  sensor12cm.SetPar(3, 12.1692);          // Active size in y

  // --- Addresses for sensors
  // --- They are defined in each station as sensor 1, module 1, halfladderD (2), ladder 1
  //  Int_t GetAddress(UInt_t unit = 0, UInt_t ladder = 0, UInt_t halfladder = 0, UInt_t module = 0, UInt_t sensor = 0,
  //                   UInt_t side = 0, UInt_t version = kCurrentVersion);

  // --- Now we can define the sensor parameter set and tell recoSts to use it
  auto sensorParSet = new CbmStsParSetSensor("CbmStsParSetSensor", "STS sensor parameters"
                                                                   "mcbm2021");

  // TODO: is it possible to read these values from a parameter file?
  if (fConfig.setupType == Setup::mCBM2022) {

    Int_t stsAddress01 = CbmStsAddress::GetAddress(0, 0, 1, 0, 0, 0);  // U0 L0 M0  6 cm
    Int_t stsAddress02 = CbmStsAddress::GetAddress(0, 0, 1, 1, 0, 0);  // U0 L0 M1  6 cm
    Int_t stsAddress03 = CbmStsAddress::GetAddress(0, 1, 1, 0, 0, 0);  // U0 L1 M0  6 cm
    Int_t stsAddress04 = CbmStsAddress::GetAddress(0, 1, 1, 1, 0, 0);  // U0 L1 M1  6 cm
    Int_t stsAddress05 = CbmStsAddress::GetAddress(1, 0, 1, 0, 0, 0);  // U1 L0 M0  6 cm
    Int_t stsAddress06 = CbmStsAddress::GetAddress(1, 0, 1, 1, 0, 0);  // U1 L0 M1 12 cm
    Int_t stsAddress07 = CbmStsAddress::GetAddress(1, 1, 1, 0, 0, 0);  // U1 L1 M0  6 cm
    Int_t stsAddress08 = CbmStsAddress::GetAddress(1, 1, 1, 1, 0, 0);  // U1 L1 M1 12 cm
    Int_t stsAddress09 = CbmStsAddress::GetAddress(1, 2, 1, 0, 0, 0);  // U1 L2 M0  6 cm
    Int_t stsAddress10 = CbmStsAddress::GetAddress(1, 2, 1, 1, 0, 0);  // U1 L2 M1  6 cm
    Int_t stsAddress11 = CbmStsAddress::GetAddress(1, 2, 1, 2, 0, 0);  // U1 L2 M2  6 cm

    LOG(info) << "STS address01 " << CbmStsAddress::ToString(stsAddress01);
    LOG(info) << "STS address02 " << CbmStsAddress::ToString(stsAddress02);
    LOG(info) << "STS address03 " << CbmStsAddress::ToString(stsAddress03);
    LOG(info) << "STS address04 " << CbmStsAddress::ToString(stsAddress04);
    LOG(info) << "STS address05 " << CbmStsAddress::ToString(stsAddress05);
    LOG(info) << "STS address06 " << CbmStsAddress::ToString(stsAddress06);
    LOG(info) << "STS address07 " << CbmStsAddress::ToString(stsAddress07);
    LOG(info) << "STS address08 " << CbmStsAddress::ToString(stsAddress08);
    LOG(info) << "STS address09 " << CbmStsAddress::ToString(stsAddress09);
    LOG(info) << "STS address10 " << CbmStsAddress::ToString(stsAddress10);
    LOG(info) << "STS address11 " << CbmStsAddress::ToString(stsAddress11);

    sensorParSet->SetParSensor(stsAddress01, sensor6cm);
    sensorParSet->SetParSensor(stsAddress02, sensor6cm);
    sensorParSet->SetParSensor(stsAddress03, sensor6cm);
    sensorParSet->SetParSensor(stsAddress04, sensor6cm);
    sensorParSet->SetParSensor(stsAddress05, sensor6cm);
    sensorParSet->SetParSensor(stsAddress06, sensor12cm);
    sensorParSet->SetParSensor(stsAddress07, sensor6cm);
    sensorParSet->SetParSensor(stsAddress08, sensor12cm);
    sensorParSet->SetParSensor(stsAddress09, sensor6cm);
    sensorParSet->SetParSensor(stsAddress10, sensor6cm);
    sensorParSet->SetParSensor(stsAddress11, sensor6cm);
  }
  else if (fConfig.setupType == Setup::mCBM2024_03 || fConfig.setupType == Setup::mCBM2024_05
           || fConfig.setupType == Setup::mCBM2025_02) {
    uint32_t addr01 = 0x10008012;
    uint32_t addr02 = 0x10018012;
    uint32_t addr03 = 0x10008412;
    uint32_t addr04 = 0x10018412;
    uint32_t addr05 = 0x10008422;
    uint32_t addr06 = 0x10018422;
    uint32_t addr07 = 0x10008822;
    uint32_t addr08 = 0x10018822;
    uint32_t addr09 = 0x10028822;
    uint32_t addr10 = 0x10008022;
    uint32_t addr11 = 0x10018022;
    uint32_t addr00 = 0x10000002;  // New station 0 in mCBM2024

    LOG(info) << "STS address01 " << CbmStsAddress::ToString(addr01);
    LOG(info) << "STS address02 " << CbmStsAddress::ToString(addr02);
    LOG(info) << "STS address03 " << CbmStsAddress::ToString(addr03);
    LOG(info) << "STS address04 " << CbmStsAddress::ToString(addr04);
    LOG(info) << "STS address05 " << CbmStsAddress::ToString(addr05);
    LOG(info) << "STS address06 " << CbmStsAddress::ToString(addr06);
    LOG(info) << "STS address07 " << CbmStsAddress::ToString(addr07);
    LOG(info) << "STS address08 " << CbmStsAddress::ToString(addr08);
    LOG(info) << "STS address09 " << CbmStsAddress::ToString(addr09);
    LOG(info) << "STS address10 " << CbmStsAddress::ToString(addr10);
    LOG(info) << "STS address11 " << CbmStsAddress::ToString(addr11);
    LOG(info) << "STS address00 " << CbmStsAddress::ToString(addr00);

    sensorParSet->SetParSensor(addr01, sensor6cm);
    sensorParSet->SetParSensor(addr02, sensor6cm);
    sensorParSet->SetParSensor(addr03, sensor6cm);
    sensorParSet->SetParSensor(addr04, sensor6cm);
    sensorParSet->SetParSensor(addr05, sensor6cm);
    sensorParSet->SetParSensor(addr06, sensor12cm);
    sensorParSet->SetParSensor(addr07, sensor6cm);
    sensorParSet->SetParSensor(addr08, sensor6cm);
    sensorParSet->SetParSensor(addr09, sensor6cm);
    sensorParSet->SetParSensor(addr10, sensor6cm);
    sensorParSet->SetParSensor(addr11, sensor12cm);
    sensorParSet->SetParSensor(addr00, sensor6cm);
  }
  else {
    throw std::runtime_error("STS: Unknown setup type");
  }

  recoSts->UseSensorParSet(sensorParSet);

  // ASIC params: #ADC channels, dyn. range, threshold, time resol., dead time,
  // noise RMS, zero-threshold crossing rate
  auto parAsic = new CbmStsParAsic(128, 31, 75000., 3000., 5., 800., 1000., 3.9789e-3);

  // Module params: number of channels, number of channels per ASIC
  auto parMod = new CbmStsParModule(2048, 128);
  parMod->SetAllAsics(*parAsic);
  recoSts->UseModulePar(parMod);

  // Sensor conditions: full depletion voltage, bias voltage, temperature,
  // coupling capacitance, inter-strip capacitance
  auto sensorCond = new CbmStsParSensorCond(70., 140., 268., 17.5, 1.);
  recoSts->UseSensorCond(sensorCond);

  fRun->AddTask(recoSts);
}

void CbmOnlineParWrite::AddCa()
{

  auto* pSetupBuilder{cbm::kf::TrackingSetupBuilder::Instance()};
  pSetupBuilder->SetIgnoreHitPresence();

  // Tracking detector interfaces initialization
  fRun->AddTask(new CbmTrackingDetectorInterfaceInit{});

  // Tracking initialization class
  auto* pCa = new CbmL1();
  pCa->SetMcbmMode();
  pCa->SetInitMode(CbmL1::EInitMode::Param);
  pCa->SetParameterFilename(Form("./%s.ca.par", fGeoSetupTag.Data()));
  fRun->AddTask(pCa);
}


void CbmOnlineParWrite::Run(const Config& config)
{
  // Copied and adjusted from macro/beamtime/mcbm2022/trd_hitfinder_run.C

  static bool callOnce = true;
  if (!callOnce) {
    throw std::runtime_error("CbmOnlineParWrite::Run() can only be called once at the moment!");
  }
  callOnce = false;

  fConfig = config;

  // -----   Environment   --------------------------------------------------
  fSrcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------

  // -----   In- and output file names   ------------------------------------

  // --- Load the geometry setup ----
  TString geoSetupTag = "";
  try {
    uint64_t runId = -1;
    switch (fConfig.setupType) {
      case Setup::mCBM2022: runId = 2391; break;
      case Setup::mCBM2024_03: runId = 2724; break;
      case Setup::mCBM2024_05: runId = 2918; break;
      case Setup::mCBM2025_02: runId = 3453; break;
      default: throw std::runtime_error("Unknown setup type");
    }
    geoSetupTag = cbm::mcbm::GetSetupFromRunId(runId);
  }
  catch (const std::invalid_argument& e) {
    std::cout << "Error in mapping from runID to setup name: " << e.what() << std::endl;
    return;
  }

  LOG(info) << "Using geometry setup: " << geoSetupTag;
  fGeoSetupTag = geoSetupTag;

  TString geoFile = fSrcDir + "/macro/run/data/" + geoSetupTag + ".geo.root";
  fSetup          = CbmSetup::Instance();
  fSetup->LoadSetup(geoSetupTag);

  //-----  Load Parameters --------------------------------------------------
  fParList = new TList();

  // -----   FairRunAna   ---------------------------------------------------
  fRun = new FairRunAna();

  // Dummy source required for the FairRunAna as it will crash without a source
  fRun->SetSource(new CbmSourceDummy{});
  fRun->SetSink(new CbmSinkDummy{});


  // =========================================================================
  // ===                   Alignment Correction                            ===
  // =========================================================================
  if (fConfig.doAlignment) {

    TString alignmentMatrixFileName = fSrcDir + "/parameters/mcbm/AlignmentMatrices_" + geoSetupTag + ".root";
    if (alignmentMatrixFileName.Length() == 0) {
      throw std::runtime_error{"Alignment matrix file name is empty"};
    }

    LOG(info) << "Applying alignment for file '" << alignmentMatrixFileName << "'";

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
      throw std::runtime_error{"Could not open alignment matrix file: " + alignmentMatrixFileName};
    }

    if (matrices) {
      fRun->AddAlignmentMatrices(*matrices);
    }
    else {
      throw std::runtime_error{"Could not read alignment matrices from file: " + alignmentMatrixFileName};
    }
  }


  // -----   Add detectors   ------------------------------------------------
  AddDetectors();
  AddCa();

  // -----  Parameter database   --------------------------------------------
  FairRuntimeDb* rtdb        = fRun->GetRuntimeDb();
  FairParAsciiFileIo* parIo2 = new FairParAsciiFileIo();
  parIo2->setAutoWritable(false);
  parIo2->open(fParList, "in");
  rtdb->setSecondInput(parIo2);
  // ------------------------------------------------------------------------

  // -----   Run initialisation   -------------------------------------------
  fRun->SetGeomFile(geoFile);
  fRun->Init();

  // No need to run the event loop, parameters are written during the initialization


  // ----- Clean up ---------------------------------------------------------
  gGeoManager->GetListOfVolumes()->Delete();
  gGeoManager->GetListOfShapes()->Delete();
  delete gGeoManager;

  // Delete files created by FairRun that I don't know how disable otherwise
  gSystem->Exec("rm all_*.par");
}
