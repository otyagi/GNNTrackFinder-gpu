/* Copyright (C) 2011-2021 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Michael Deveaux, Philipp Sitzmann [committer] */

// -------------------------------------------------------------------------
// -----                     CbmMvdSensor source file                  -----
// -----                  Created 31/01/11  by M. Deveaux              -----
// -------------------------------------------------------------------------

#include "CbmMvdSensor.h"

#include "CbmMvdSensorBuffer.h"
#include "CbmMvdSensorPlugin.h"
#include "CbmMvdSensorTask.h"

#include <FairEventHeader.h>
#include <FairMCEventHeader.h>
#include <FairPrimaryGenerator.h>
#include <FairRunAna.h>
#include <FairRunSim.h>
#include <Logger.h>

#include <TClonesArray.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoVolume.h>


/// includes from C
#include <ostream>  // for operator<<, stringstream, basic_ostream

// -----   Default constructor   -------------------------------------------
CbmMvdSensor::CbmMvdSensor()
  : TNamed()
  , fStationNr(0)
  , fSensorNr(0)
  , fVolumeId(-1)
  , fDetectorID(-1)
  , fDigiPlugin(-1)
  , fHitPlugin(-1)
  , fDigiLen(-1)
  , fClusterPlugin(-1)
  , fVolName("")
  , fNodeName("")
  , fcurrentEventTime(0.)
  , epsilon()
  , fShape(nullptr)
  , fMCMatrix(nullptr)
  , fRecoMatrix(nullptr)
  , fAlignmentCorr(nullptr)
  , fTempCoordinate()
  , fSensorPosition()
  , fSensorData(new CbmMvdSensorDataSheet())
  , fSensorMap()
  , fPluginArray(new TObjArray(1))
  , fSensorStartTime(0.)
  , initialized(kFALSE)
{
  LOG(warning) << "MVD-Sensor initialized without technical data. Assuming default sensor.";

  LOG(warning) << "MVD-Sensor initialized without geometry data. Must be added manually before using this class.";

  LOG(warning) << "MVD-Sensor initialized without valid start time. Must be added manually before using this class.";
}
// -------------------------------------------------------------------------


// -----   Standard constructor   ------------------------------------------
CbmMvdSensor::CbmMvdSensor(const char* name, CbmMvdSensorDataSheet* dataSheet, TString volName, TString nodeName,
                           Int_t stationNr, Int_t volumeId, Double_t sensorStartTime)
  : TNamed(name, "")
  , fStationNr()
  , fSensorNr(stationNr)
  , fVolumeId(volumeId)
  , fDetectorID(DetectorId(stationNr))
  , fDigiPlugin(-1)
  , fHitPlugin(-1)
  , fDigiLen(-1)
  , fClusterPlugin(-1)
  , fVolName(volName)
  , fNodeName(nodeName)
  , fcurrentEventTime(0.)
  , epsilon()
  , fShape(nullptr)
  , fMCMatrix(nullptr)
  , fRecoMatrix(nullptr)
  , fAlignmentCorr(nullptr)
  , fTempCoordinate()
  , fSensorPosition()
  , fSensorData(dataSheet)
  , fSensorMap()
  , fPluginArray(new TObjArray(1))
  , fSensorStartTime(sensorStartTime)
  , initialized(kFALSE)
{
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmMvdSensor::~CbmMvdSensor() { delete fSensorData; }
// -------Setters -----------------------------------------------------

void CbmMvdSensor::SetAlignment(TGeoHMatrix* alignmentMatrix)
{

  if (fAlignmentCorr) { delete fAlignmentCorr; };
  fAlignmentCorr = (TGeoHMatrix*) alignmentMatrix->Clone(fNodeName + "_AlignmentData");
}


// -------Initialization tools  -----------------------------------------------------
void CbmMvdSensor::ReadSensorGeometry(TString nodeName)
{

  LOG(debug1) << "-I- nodeName is " << nodeName;

  if (fMCMatrix) { delete fMCMatrix; };  //delete local copy of the position information

  TGeoVolume* volume;
  gGeoManager->cd(nodeName);
  volume = gGeoManager->GetCurrentVolume();
  LOG(debug2) << "At volume: " << volume->GetName();
  fShape    = (TGeoBBox*) volume->GetShape();
  fMCMatrix = (TGeoHMatrix*) (gGeoManager->GetCurrentMatrix())->Clone(nodeName + "_MC_Matrix");
  fMCMatrix->SetName(nodeName + "_MC_Matrix");

  Double_t first[3], last[3];
  PixelToLocal(0, 0, first);
  PixelToLocal(fSensorData->GetNPixelsX(), fSensorData->GetNPixelsY(), last);
  LOG(debug2) << "pixel 0,0 at: " << first[0] << ", " << first[1] << " Local";
  LOG(debug2) << "pixel " << fSensorData->GetNPixelsX() << " " << fSensorData->GetNPixelsY() << " at: " << last[0]
              << ", " << last[1] << " Local";

  Double_t* tempCoordinate;

  if (!fRecoMatrix) {
    Double_t pre[3], past[3], global[3];
    Double_t local[3] = {0, 0, 0};
    //The initial guess on the reconstructed position is that the MC-position is correct
    //Short cut to misalignment, add a small error
    fRecoMatrix = (TGeoHMatrix*) fMCMatrix->Clone(nodeName + "_Reco_Matrix");
    PixelToTop(0, 0, pre);
    tempCoordinate = fRecoMatrix->GetTranslation();
    for (Int_t i = 0; i < 3; i++) {
      tempCoordinate[i] = tempCoordinate[i] + epsilon[i];
    }
    fRecoMatrix->SetTranslation(tempCoordinate);
    PixelToTop(0, 0, past);
    LOG(debug2) << "shifted pixel 0,0 to: " << past[0] << ", " << past[1] << " at z = " << past[2];
    LocalToTop(local, global);
    LOG(debug2) << "shifted local center to: " << global[0] << ", " << global[1] << " at z = " << global[2];
  }
  else {
    tempCoordinate = fRecoMatrix->GetTranslation();
  }
  if (!fAlignmentCorr) {
    //If no knowledge on the reco matrix is available there is plausibly no correction data.
    fAlignmentCorr = new TGeoHMatrix(nodeName + "_AlignmentData");
  }

  for (Int_t i = 0; i < 3; i++) {
    fSensorPosition[i] = tempCoordinate[i];
  }
}

//-------------------------------------------------------------------

//-------------------------------------------------------------------
void CbmMvdSensor::Init()
{

  ReadSensorGeometry(fNodeName);

  Int_t nPlugin = fPluginArray->GetEntriesFast();


  TClonesArray* dataArray;

  CbmMvdSensorTask* gentask;
  CbmMvdSensorBuffer* genBuffer;

  CbmMvdSensorPlugin *pluginFirst, *pluginNext;

  for (Int_t i = 0; i < nPlugin; i++) {
    pluginFirst = (CbmMvdSensorPlugin*) fPluginArray->At(i);
    if (pluginFirst->GetPluginType() == buffer) {
      genBuffer = (CbmMvdSensorBuffer*) fPluginArray->At(i);
      genBuffer->InitBuffer(this);
    }
    if (pluginFirst->GetPluginType() == task) {
      gentask = (CbmMvdSensorTask*) fPluginArray->At(i);
      gentask->InitTask(this);
    }
  }
  //Link data chain
  if (nPlugin > 1) {

    for (Int_t i = 0; i < nPlugin - 1; i++) {

      pluginFirst = (CbmMvdSensorPlugin*) fPluginArray->At(i);
      pluginNext  = (CbmMvdSensorPlugin*) fPluginArray->At(i + 1);

      //Rules: The output array of the previous plugin is input of the next
      dataArray = pluginFirst->GetOutputArray();
      pluginNext->SetInputArray(dataArray);

      //The plugin knows its precessor and sucessor to communicate
      pluginFirst->SetNextPlugin(pluginNext);
      pluginNext->SetPreviousPlugin(pluginFirst);
    }
  }


  if (nPlugin == 0) {
    LOG(debug) << "No Plugins on this Sensor ";
    pluginFirst = nullptr;
  }
  initialized = kTRUE;
}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
void CbmMvdSensor::ShowDebugHistos()
{
  //if(fDetectorID == 1537)
  {
    CbmMvdSensorPlugin* pluginLast;
    pluginLast = (CbmMvdSensorPlugin*) fPluginArray->At(fPluginArray->GetEntriesFast() - 1);
    LOG(info) << "Set debug mode on Plugin " << fPluginArray->GetEntriesFast() - 1 << " at sensor " << GetName();
    pluginLast->ShowDebugHistos();
  }
}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------

void CbmMvdSensor::SendInputToPlugin(Int_t nPlugin, TObject* input)
{
  CbmMvdSensorPlugin* digitask;
  digitask = (CbmMvdSensorPlugin*) fPluginArray->At(nPlugin);
  digitask->SetInput(input);
}

// -------------------------------------------------------------------------

void CbmMvdSensor::ExecChain()
{

  FairRunSim* run = FairRunSim::Instance();
  FairRunAna* ana = FairRunAna::Instance();
  if (run) {
    FairEventHeader* event = run->GetEventHeader();
    fcurrentEventTime      = event->GetEventTime();
  }
  if (ana) {
    FairEventHeader* event = ana->GetEventHeader();
    fcurrentEventTime      = event->GetEventTime();
  }


  CbmMvdSensorPlugin* plugin;
  Int_t nPlugin = fPluginArray->GetEntriesFast();

  plugin = (CbmMvdSensorPlugin*) fPluginArray->At(0);

  plugin->ExecChain();

  if (plugin->PluginReady()) {

    for (Int_t i = 1; i < nPlugin; i++) {

      plugin = (CbmMvdSensorPlugin*) fPluginArray->At(i);
      plugin->ExecChain();
    }
  }
}
// -------------------------------------------------------------------------

void CbmMvdSensor::Exec(UInt_t nPlugin)
{
  UInt_t nPluginMax = fPluginArray->GetEntriesFast();
  if (nPlugin > nPluginMax) { Fatal(GetName(), " Error - Called non-existing plugin"); }
  CbmMvdSensorPlugin* plugin = (CbmMvdSensorPlugin*) fPluginArray->At(nPlugin);
  plugin->Exec();
}
// -------------------------------------------------------------------------
void CbmMvdSensor::ExecTo(UInt_t nPlugin)
{
  FairRunSim* run           = FairRunSim::Instance();
  FairPrimaryGenerator* gen = run->GetPrimaryGenerator();
  FairMCEventHeader* event  = gen->GetEvent();

  fcurrentEventTime = event->GetT();

  CbmMvdSensorPlugin* plugin;
  UInt_t maxPlugin = fPluginArray->GetEntriesFast();

  plugin = (CbmMvdSensorPlugin*) fPluginArray->At(0);
  if (nPlugin < maxPlugin) {
    plugin->ExecChain();

    if (plugin->PluginReady()) {

      for (UInt_t i = 1; i <= nPlugin; i++) {
        plugin = (CbmMvdSensorPlugin*) fPluginArray->At(i);
        plugin->ExecChain();
      }
    }
  }
  else {
    LOG(info) << "nPlugin to large";
  }
}


// -------------------------------------------------------------------------
void CbmMvdSensor::ExecFrom(UInt_t nPlugin)
{
  FairRunSim* run           = FairRunSim::Instance();
  FairPrimaryGenerator* gen = run->GetPrimaryGenerator();
  FairMCEventHeader* event  = gen->GetEvent();

  fcurrentEventTime = event->GetT();

  CbmMvdSensorPlugin* plugin;
  UInt_t maxPlugin = fPluginArray->GetEntriesFast();

  plugin = (CbmMvdSensorPlugin*) fPluginArray->At(nPlugin);
  if (nPlugin <= maxPlugin) {
    plugin->ExecChain();
    //LOG(info) << "is plugin ready? "<< plugin->PluginReady() << " on sensor "<< this->GetName();
    if (plugin->PluginReady()) {
      //LOG(info) << "exec chain on sensor "<< this->GetName();
      for (UInt_t i = nPlugin + 1; i < maxPlugin; i++) {
        //LOG(info) << "exec plugin " << i << " on sensor "<< this->GetName();
        plugin = (CbmMvdSensorPlugin*) fPluginArray->At(i);
        plugin->ExecChain();
      }
    }
  }
  else {
    LOG(info) << "nPlugin to large";
  }
}

// -------------------------------------------------------------------------

TClonesArray* CbmMvdSensor::GetOutputArray(Int_t nPlugin) const
{

  CbmMvdSensorPlugin* plugin = (CbmMvdSensorPlugin*) fPluginArray->At(nPlugin);
  return plugin->GetOutputArray();
}


// -------------------------------------------------------------------------
TClonesArray* CbmMvdSensor::GetMatchArray(Int_t nPlugin) const
{

  CbmMvdSensorPlugin* plugin = (CbmMvdSensorPlugin*) fPluginArray->At(nPlugin);
  return plugin->GetMatchArray();
}
// -------------------------------------------------------------------------
Int_t CbmMvdSensor::GetOutputArrayLen(Int_t nPlugin) const
{
  TClonesArray* tempArray = GetOutputArray(nPlugin);  // make sure that the arrays are filled
  if (tempArray) { return tempArray->GetEntriesFast() - 1; }
  else {
    LOG(fatal) << "undefined plugin called";
    return -1;
  }
}
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
TClonesArray* CbmMvdSensor::GetOutputBuffer() const
{


  CbmMvdSensorPlugin* plugin = (CbmMvdSensorPlugin*) fPluginArray->At(fPluginArray->GetLast());
  LOG(warning) << "CbmMvdSensor::GetOutputBuffer: Warning - Use of method depreciated.";
  return (plugin->GetOutputArray());
}
// -------------------------------------------------------------------------

TH1* CbmMvdSensor::GetHistogram(UInt_t nPlugin, UInt_t nHisto)
{
  if (!fPluginArray) return 0;
  if (nPlugin >= (UInt_t) fPluginArray->GetEntriesFast()) return 0;
  CbmMvdSensorPlugin* myPlugin = (CbmMvdSensorPlugin*) fPluginArray->At(nPlugin);
  return myPlugin->GetHistogram(nHisto);
}

UInt_t CbmMvdSensor::GetNumberOfHistograms(UInt_t nPlugin)
{
  if (!fPluginArray) return -1;
  if (nPlugin >= (UInt_t) fPluginArray->GetEntriesFast()) return -1;
  return ((CbmMvdSensorPlugin*) fPluginArray->At(nPlugin))->GetMaxHistoNumber();
}


// -----  Coordinate Transformations --------------------------------

void CbmMvdSensor::LocalToTop(Double_t* local, Double_t* lab) { fMCMatrix->LocalToMaster(local, lab); };
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMvdSensor::TopToLocal(Double_t* lab, Double_t* local)
{
  fMCMatrix->MasterToLocal(lab, local);
  //LOG(info) << "local 0 nach TopToLocal " << local[0];
  //LOG(info) << "local 1 nach TopToLocal " << local[1];
};
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMvdSensor::LocalToPixel(Double_t* local, Int_t& pixelNumberX, Int_t& pixelNumberY)
{

  //Compute position of the frame relativ to the border of the matrix
  //which contains the pixel (0/0)
  Double_t x = local[0] + (fSensorData->GetPixelSignX() * GetDX());
  //LOG(info) << "From " << local[0] << " to Double_t x " << x;
  Double_t y = local[1] + (fSensorData->GetPixelSignY() * GetDY());
  //LOG(info) << "From " << local[1] << " to Double_t y " << y;
  //Compute the number of the pixel hit.
  //Note: substract 0.5 to move from border to center of pixel
  pixelNumberX = Int_t(x / fSensorData->GetPixelPitchX() - 0.5);
  //if (pixelNumberX < 0) LOG(info)<< "pixelNumberX = " << pixelNumberX << " on Sensor " << this->GetName();
  pixelNumberY = Int_t(y / fSensorData->GetPixelPitchY() - 0.5);
  //if (pixelNumberY < 0) LOG(info) << "pixelNumberY = " << pixelNumberY << " on Sensor " << this->GetName();
};
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMvdSensor::PixelToLocal(Int_t pixelNumberX, Int_t pixelNumberY, Double_t* local)
{

  //Compute distance from border of the pixel matrix
  //Ignore the direction of pixel numbering so far
  //By definiton (x,y) is in the center of the pixel

  Double_t x = (pixelNumberX + 0.5) * fSensorData->GetPixelPitchX();
  Double_t y = (pixelNumberY + 0.5) * fSensorData->GetPixelPitchY();

  //Perform coordinate transformation from border of matrix to center of volume
  local[0] = x - fSensorData->GetPixelSignX() * GetDX();
  local[1] = y - fSensorData->GetPixelSignY() * GetDY();

  local[2] = 0;  //per definition always at the sensor surface;
};
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
void CbmMvdSensor::PixelToTop(Int_t pixelNumberX, Int_t pixelNumberY, Double_t* lab)
{
  PixelToLocal(pixelNumberX, pixelNumberY, fTempCoordinate);
  LocalToTop(fTempCoordinate, lab);
};
// -------------------------------------------------------------------------


void CbmMvdSensor::TopToPixel(Double_t* lab, Int_t& pixelNumberX, Int_t& pixelNumberY)
{
  TopToLocal(lab, fTempCoordinate);
  LocalToPixel(fTempCoordinate, pixelNumberX, pixelNumberY);
};
// -------------------------------------------------------------------------


// -------------------------------------------------------------------------
Int_t CbmMvdSensor::GetFrameNumber(Double_t absoluteTime, Int_t pixelNumberY) const
{


  Double_t timeSinceStart = absoluteTime - fSensorStartTime;

  Double_t timeInUnitsOfFrames = timeSinceStart / fSensorData->GetIntegrationTime();

  // Get time substracting the full frames (140.3 -> 0.3)
  Double_t timeSinceStartOfTheFrame = timeInUnitsOfFrames - (Int_t) timeInUnitsOfFrames;

  Int_t rowUnderReadout = Int_t(fSensorData->GetNPixelsY() * timeSinceStartOfTheFrame);


  return (Int_t)(timeInUnitsOfFrames + (pixelNumberY < rowUnderReadout));
  //
  /* This statement is equivalent to:
  if (pixelNumberY<rowUnderReadout) {return timeInUnitsOfFrames;}
  else				    {return timeInUnitsOfFrames+1;}
  */
}
// -------------------------------------------------------------------------

Double_t CbmMvdSensor::ComputeIndecatedAnalogTime(Double_t hitMCTime, Float_t diodeCharge)
{

  Double_t delay  = fSensorData->ComputeHitDelay(diodeCharge);
  Double_t jitter = fSensorData->ComputeHitJitter(diodeCharge);

  return hitMCTime + delay + jitter;
}

Double_t CbmMvdSensor::ComputeEndOfBusyTime(Double_t hitMCTime, Float_t diodeCharge, Int_t pixelNumberY)
{

  Double_t endOfAnalogChannelDeadTime = hitMCTime + fSensorData->ComputeHitDeadTime(diodeCharge);
  Int_t frameNumberAtEndOfDeadTime    = GetFrameNumber(endOfAnalogChannelDeadTime, pixelNumberY);
  Double_t endOfDeadTime              = GetFrameEndTime(frameNumberAtEndOfDeadTime);
  /* Anticipated condition for end of deadtime:
   * Analog electronics must be below threshold AND HEREAFTER a new frame must have started.
   * Neglected: Tail of the falling edge of the analog pulse is artificially cut.
   */
  return endOfDeadTime;
}

// -------------------------------------------------------------------------
Double_t CbmMvdSensor::GetFrameStartTime(Int_t frameNumber)
{

  return fSensorStartTime + frameNumber * fSensorData->GetIntegrationTime();
}

// -------------------------------------------------------------------------
Double_t CbmMvdSensor::GetReadoutTime(Double_t absoluteTime) const
{


  Double_t timeSinceStart = absoluteTime - fSensorStartTime;

  Double_t timeInUnitsOfFrames = timeSinceStart / fSensorData->GetIntegrationTime();

  Int_t nextFrame = (Int_t)(timeInUnitsOfFrames + 1);

  Double_t roTime = nextFrame * fSensorData->GetIntegrationTime();

  return roTime;
}
// -------------------------------------------------------------------------


void CbmMvdSensor::Print(Option_t* /*opt*/) const { LOG(info) << ToString(); }

// -----   Public method Print   -------------------------------------------
std::string CbmMvdSensor::ToString() const
{
  std::stringstream ss;
  ss << " --- " << GetName() << ", sensor name" << fVolName << std::endl;
  ss << " MC - ID: " << fVolumeId << std::endl;
  ss << "---------------------------------------------------------" << std::endl;
  ss << " Position information: " << std::endl;
  fRecoMatrix->Print();
  ss << " --------------------------------------------------------" << std::endl;
  ss << " Technical information: " << std::endl;
  ss << fSensorData->ToString();
  ss << " , MC Id " << fVolumeId << std::endl;
  ss << "---------------------------------------------------------" << std::endl;
  return ss.str();
}
// -------------------------------------------------------------------------

//-----------------------------------------------------------------------
void CbmMvdSensor::Finish()
{
  CbmMvdSensorPlugin* plugin;
  Int_t nPlugin = fPluginArray->GetEntriesFast();
  for (Int_t i = 0; i < nPlugin; i++) {
    plugin = (CbmMvdSensorPlugin*) fPluginArray->At(i);
    plugin->Finish();
  }
}
// -------------------------------------------------------------------------


ClassImp(CbmMvdSensor)
