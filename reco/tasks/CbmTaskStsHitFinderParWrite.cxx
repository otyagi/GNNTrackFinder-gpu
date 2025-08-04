/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "CbmTaskStsHitFinderParWrite.h"

#include "CbmAddress.h"
#include "CbmStsModule.h"
#include "CbmStsParSetModule.h"
#include "CbmStsParSetSensor.h"
#include "CbmStsParSetSensorCond.h"
#include "CbmStsParSim.h"
#include "CbmStsPhysics.h"
#include "CbmStsRecoModule.h"
#include "CbmStsSetup.h"
#include "sts/HitfinderPars.h"
#include "yaml/Yaml.h"

#include <FairField.h>
#include <FairRootManager.h>
#include <FairRun.h>
#include <FairRuntimeDb.h>

#include <TGeoBBox.h>
#include <TGeoPhysicalNode.h>

#include <iomanip>

// -----   Constructor   ---------------------------------------------------
CbmTaskStsHitFinderParWrite::CbmTaskStsHitFinderParWrite() : FairTask("CbmTaskStsHitFinderParWrite", 1) {}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmTaskStsHitFinderParWrite::~CbmTaskStsHitFinderParWrite() = default;
// -------------------------------------------------------------------------


// -----   Initialise the cluster finding modules   ------------------------
UInt_t CbmTaskStsHitFinderParWrite::CreateModules()
{
  assert(fSetup);

  std::vector<cbm::algo::sts::HitfinderPars::Module> gpuModules;  // for gpu reco

  LOG(info) << GetName() << ": Creating modules";
  LOG(info) << fParSetSensor->ToString();

  for (Int_t iModule = 0; iModule < fSetup->GetNofModules(); iModule++) {

    // --- Setup module and sensor
    CbmStsModule* setupModule = fSetup->GetModule(iModule);
    assert(setupModule);
    Int_t moduleAddress = Int_t(setupModule->GetAddress());
    assert(setupModule->GetNofDaughters() == 1);
    CbmStsElement* setupSensor = setupModule->GetDaughter(0);
    assert(setupSensor);
    Int_t sensorAddress = Int_t(setupSensor->GetAddress());

    // --- Module parameters
    const CbmStsParModule& modPar       = fParSetModule->GetParModule(moduleAddress);
    const CbmStsParSensor& sensPar      = fParSetSensor->GetParSensor(sensorAddress);
    const CbmStsParSensorCond& sensCond = fParSetCond->GetParSensor(sensorAddress);

    // --- Calculate and set average Lorentz shift
    // --- This will be used in hit finding for correcting the position.
    Double_t lorentzF = 0.;
    Double_t lorentzB = 0.;
    if (fParSim->LorentzShift()) {

      TGeoBBox* shape = dynamic_cast<TGeoBBox*>(setupSensor->GetPnode()->GetShape());
      assert(shape);
      Double_t dZ = 2. * shape->GetDZ();  // Sensor thickness

      // Get the magnetic field in the sensor centre
      Double_t by = 0.;
      if (FairRun::Instance()->GetField()) {
        Double_t local[3] = {0., 0., 0.};  // sensor centre in local C.S.
        Double_t global[3];                // sensor centre in global C.S.
        setupSensor->GetPnode()->GetMatrix()->LocalToMaster(local, global);
        Double_t field[3] = {0., 0., 0.};  // magnetic field components
        FairRun::Instance()->GetField()->Field(global, field);
        by = field[1] / 10.;  // kG->T
      }                       //? field present

      // Calculate average Lorentz shift on sensor sides.
      // This is needed in hit finding for correcting the cluster position.
      auto lorentzShift = LorentzShift(sensCond, dZ, by);
      lorentzF          = lorentzShift.first;
      lorentzB          = lorentzShift.second;
    }  //? Lorentz-shift correction

    // --- Create reco module
    CbmStsRecoModule recoModule{setupModule, modPar, sensPar, lorentzF, lorentzB};

    // Get Transformation Matrix
    cbm::algo::sts::HitfinderPars::ModuleTransform localToGlobal;
    TGeoHMatrix* matrix = recoModule.getMatrix();
    std::copy_n(matrix->GetRotationMatrix(), 9, localToGlobal.rotation.begin());
    std::copy_n(matrix->GetTranslation(), 3, localToGlobal.translation.begin());

    // Collect GPU parameters
    cbm::algo::sts::HitfinderPars::Module gpuModulePars{
      .address       = moduleAddress,
      .dY            = sensPar.GetPar(3),
      .pitch         = sensPar.GetPar(6),
      .stereoF       = sensPar.GetPar(8),
      .stereoB       = sensPar.GetPar(9),
      .lorentzF      = float(lorentzF),
      .lorentzB      = float(lorentzB),
      .localToGlobal = localToGlobal,
    };
    gpuModules.emplace_back(gpuModulePars);
  }

  const CbmStsParModule& firstModulePars = fParSetModule->GetParModule(gpuModules[0].address);

  CbmStsParAsic asic = firstModulePars.GetParAsic(0);
  cbm::algo::sts::HitfinderPars::Asic algoAsic{
    .nAdc           = asic.GetNofAdc(),
    .dynamicRange   = float(asic.GetDynRange()),
    .threshold      = float(asic.GetThreshold()),
    .timeResolution = float(asic.GetTimeResol()),
    .deadTime       = float(asic.GetDeadTime()),
    .noise          = float(asic.GetNoise()),
    .zeroNoiseRate  = float(asic.GetZeroNoiseRate()),
  };

  int nChannels = firstModulePars.GetNofChannels();

  auto [landauValues, landauStepSize] = CbmStsPhysics::Instance()->GetLandauWidthTable();
  std::vector<float> landauValuesF;
  std::copy(landauValues.begin(), landauValues.end(), std::back_inserter(landauValuesF));
  cbm::algo::sts::HitfinderPars pars{
    .asic      = algoAsic,
    .nChannels = nChannels,
    .modules   = gpuModules,
    .landauTable =
      {
        .values   = landauValuesF,
        .stepSize = float(landauStepSize),
      },
  };

  // Write to file
  std::string filename = "StsHitfinder.yaml";
  std::ofstream{filename} << cbm::algo::yaml::Dump{}(pars, 4);

  return pars.modules.size();
}
// -------------------------------------------------------------------------


// -----   Initialisation   ------------------------------------------------
InitStatus CbmTaskStsHitFinderParWrite::Init()
{

  // --- Something for the screen
  LOG(info) << "==========================================================";
  LOG(info) << GetName() << ": Initialising ";

  // --- Check IO-Manager
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  // --- Simulation settings
  assert(fParSim);  // Set from SetParContainers()
  LOG(info) << GetName() << ": Sim settings " << fParSim->ToString();

  // --- Parameters
  InitParams();

  // --- Initialise STS setup
  fSetup = CbmStsSetup::Instance();
  fSetup->Init(nullptr);
  //fSetup->SetSensorParameters(fParSetSensor);

  // --- Create reconstruction modules
  UInt_t nModules = CreateModules();
  LOG(info) << GetName() << ": Created " << nModules << " modules";

  LOG(info) << GetName() << ": Initialisation successful.";
  LOG(info) << "==========================================================";

  return kSUCCESS;
}
// -------------------------------------------------------------------------


// -----   Parameter initialisation   --------------------------------------
void CbmTaskStsHitFinderParWrite::InitParams()
{

  // --- Module parameters
  TString sourceModu = "database";
  assert(fParSetModule);
  if (fUserParSetModule) {
    fParSetModule->clear();
    *fParSetModule = *fUserParSetModule;
    fParSetModule->setChanged();
    fParSetModule->setInputVersion(-2, 1);
    sourceModu = "user-defined";
  }
  if (fUserParModule) {  // global settings override
    fParSetModule->clear();
    fParSetModule->SetGlobalPar(*fUserParModule);
    fParSetModule->setChanged();
    fParSetModule->setInputVersion(-2, 1);
    sourceModu = "user-defined, global";
  }
  LOG(info) << GetName() << ": Module parameters (" << sourceModu << ") " << fParSetModule->ToString();

  // --- Sensor parameters
  TString sourceSens = "database";
  assert(fParSetSensor);
  if (fUserParSetSensor) {
    fParSetSensor->clear();
    *fParSetSensor = *fUserParSetSensor;
    fParSetSensor->setChanged();
    fParSetSensor->setInputVersion(-2, 1);
    sourceSens = "user-defined";
  }
  if (fUserParSensor) {  // global settings override
    fParSetSensor->clear();
    fParSetSensor->SetGlobalPar(*fUserParSensor);
    fParSetSensor->setChanged();
    fParSetSensor->setInputVersion(-2, 1);
    sourceSens = "user-defined, global";
  }
  LOG(info) << GetName() << ": Sensor parameters (" << sourceSens << ")" << fParSetSensor->ToString();

  // --- Sensor conditions
  TString sourceCond = "database";
  assert(fParSetCond);
  if (fUserParSetCond) {
    fParSetCond->clear();
    *fParSetCond = *fUserParSetCond;
    fParSetCond->setChanged();
    fParSetCond->setInputVersion(-2, 1);
    sourceSens = "user-defined";
  }
  if (fUserParCond) {  // global settings override
    fParSetCond->clear();
    fParSetCond->SetGlobalPar(*fUserParCond);
    fParSetCond->setChanged();
    fParSetCond->setInputVersion(-2, 1);
    sourceCond = "user-defined, global";
  }
  LOG(info) << GetName() << ": Sensor conditions (" << sourceCond << ")" << fParSetCond->ToString();
}
// -------------------------------------------------------------------------

// -----   Calculate the mean Lorentz shift in a sensor   ------------------
std::pair<Double_t, Double_t> CbmTaskStsHitFinderParWrite::LorentzShift(const CbmStsParSensorCond& conditions,
                                                                        Double_t dZ, Double_t bY)
{

  Double_t vBias  = conditions.GetVbias();  // Bias voltage
  Double_t vFd    = conditions.GetVfd();    // Full-depletion voltage
  Double_t eField = (vBias + vFd) / dZ;     // Electric field

  // --- Integrate in 1000 steps over the sensor thickness
  Int_t nSteps     = 1000;
  Double_t deltaZ  = dZ / nSteps;
  Double_t dxMeanE = 0.;
  Double_t dxMeanH = 0.;
  for (Int_t j = 0; j <= nSteps; j++) {
    eField -= 2 * vFd / dZ * deltaZ / dZ;  // Electric field [V/cm]
    Double_t muHallE = conditions.GetHallMobility(eField, 0);
    Double_t muHallH = conditions.GetHallMobility(eField, 1);
    dxMeanE += muHallE * (dZ - Double_t(j) * deltaZ);
    dxMeanH += muHallH * Double_t(j) * deltaZ;
  }
  dxMeanE /= Double_t(nSteps);
  dxMeanH /= Double_t(nSteps);
  Double_t shiftF = dxMeanE * bY * 1.e-4;
  Double_t shiftB = dxMeanH * bY * 1.e-4;
  // The factor 1.e-4 is because bZ is in T = Vs/m**2, but muHall is in
  // cm**2/(Vs) and z in cm.

  return {shiftF, shiftB};
}
// -------------------------------------------------------------------------

// -----   Connect parameter container   -----------------------------------
void CbmTaskStsHitFinderParWrite::SetParContainers()
{
  FairRuntimeDb* db = FairRun::Instance()->GetRuntimeDb();
  fParSim           = dynamic_cast<CbmStsParSim*>(db->getContainer("CbmStsParSim"));
  fParSetModule     = dynamic_cast<CbmStsParSetModule*>(db->getContainer("CbmStsParSetModule"));
  fParSetSensor     = dynamic_cast<CbmStsParSetSensor*>(db->getContainer("CbmStsParSetSensor"));
  fParSetCond       = dynamic_cast<CbmStsParSetSensorCond*>(db->getContainer("CbmStsParSetSensorCond"));
}
// -------------------------------------------------------------------------
