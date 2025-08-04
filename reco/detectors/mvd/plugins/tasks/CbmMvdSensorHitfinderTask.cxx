/* Copyright (C) 2013-2020 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer], Samir Amar-Youcef, Florian Uhlig */

// ---------------------------------------------------------------------------------------------
// -----                    CbmMvdSensorHitfinderTask source file                            -----
// -----                      Created 11/09/13  by P.Sitzmann                              -----
// -----      				 						   -----
// ---------------------------------------------------------------------------------------------
#include "CbmMvdSensorHitfinderTask.h"

#include "CbmMvdCluster.h"  // for CbmMvdCluster
#include "CbmMvdDigi.h"     // for CbmMvdDigi
#include "CbmMvdHit.h"      // for CbmMvdHit
#include "CbmMvdSensor.h"   // for CbmMvdSensor

#include <TClonesArray.h>  // for TClonesArray
#include <TGeoMatrix.h>    // for TGeoHMatrix
#include <TMath.h>         // for Power
#include <TMathBase.h>     // for Abs
#include <TObjArray.h>     // for TObjArray
#include <TRandom.h>       // for TRandom
#include <TRandom3.h>      // for gRandom
#include <TVector3.h>      // for TVector3

#include <iomanip>   // for setprecision, setw
#include <iostream>  // for operator<<, basic_ostream, char_traits, endl
#include <map>       // for map, __map_iterator, operator!=, operator==
#include <vector>    // for vector

using std::endl;
using std::fixed;
using std::ios_base;
using std::left;
using std::map;
using std::pair;
using std::right;
using std::setprecision;
using std::setw;
using std::vector;


// -----   Default constructor   ------------------------------------------
CbmMvdSensorHitfinderTask::CbmMvdSensorHitfinderTask()
  : CbmMvdSensorTask()
  , fAdcDynamic(200)
  , fAdcOffset(0)
  , fAdcBits(1)
  , fAdcSteps(-1)
  , fAdcStepSize(-1.)
  , fClusters(new TClonesArray("CbmMvdCluster", 10000))
  , fPixelChargeHistos(nullptr)
  , fTotalChargeInNpixelsArray(nullptr)
  , fResolutionHistoX(nullptr)
  , fResolutionHistoY(nullptr)
  , fResolutionHistoCleanX(nullptr)
  , fResolutionHistoCleanY(nullptr)
  , fResolutionHistoMergedX(nullptr)
  , fResolutionHistoMergedY(nullptr)
  , fBadHitHisto(nullptr)
  , fGausArray(nullptr)
  , fGausArrayIt(-1)
  , fGausArrayLimit(5000)
  , fDigiMap()
  , fDigiMapIt()
  , h(nullptr)
  , h3(nullptr)
  , h1(nullptr)
  , h2(nullptr)
  , Qseed(nullptr)
  , fFullClusterHisto(nullptr)
  , c1(nullptr)
  , fNEvent(0)
  , fMode(0)
  , fCounter(0)
  , fSigmaNoise(15.)
  , fSeedThreshold(1.)
  , fNeighThreshold(1.)
  , fShowDebugHistos(kFALSE)
  , fUseMCInfo(kFALSE)
  , inputSet(kFALSE)
  , fLayerRadius(0.)
  , fLayerRadiusInner(0.)
  , fLayerPosZ(0.)
  , fHitPosX(0.)
  , fHitPosY(0.)
  , fHitPosZ(0.)
  , fHitPosErrX(0.0005)
  , fHitPosErrY(0.0005)
  , fHitPosErrZ(0.0)
  , fBranchName("MvdHit")
  , fDigisInCluster(0)
  , fAddNoise(kFALSE)
{
  fPluginIDNumber = 300;
}
// -------------------------------------------------------------------------


// -----   Standard constructor   ------------------------------------------
CbmMvdSensorHitfinderTask::CbmMvdSensorHitfinderTask(Int_t iMode)
  : CbmMvdSensorTask()
  , fAdcDynamic(200)
  , fAdcOffset(0)
  , fAdcBits(1)
  , fAdcSteps(-1)
  , fAdcStepSize(-1.)
  , fClusters(new TClonesArray("CbmMvdCluster", 10000))
  , fPixelChargeHistos(nullptr)
  , fTotalChargeInNpixelsArray(nullptr)
  , fResolutionHistoX(nullptr)
  , fResolutionHistoY(nullptr)
  , fResolutionHistoCleanX(nullptr)
  , fResolutionHistoCleanY(nullptr)
  , fResolutionHistoMergedX(nullptr)
  , fResolutionHistoMergedY(nullptr)
  , fBadHitHisto(nullptr)
  , fGausArray(nullptr)
  , fGausArrayIt(-1)
  , fGausArrayLimit(5000)
  , fDigiMap()
  , fDigiMapIt()
  , h(nullptr)
  , h3(nullptr)
  , h1(nullptr)
  , h2(nullptr)
  , Qseed(nullptr)
  , fFullClusterHisto(nullptr)
  , c1(nullptr)
  , fNEvent(0)
  , fMode(iMode)
  , fCounter(0)
  , fSigmaNoise(15.)
  , fSeedThreshold(1.)
  , fNeighThreshold(1.)
  , fShowDebugHistos(kFALSE)
  , fUseMCInfo(kFALSE)
  , inputSet(kFALSE)
  , fLayerRadius(0.)
  , fLayerRadiusInner(0.)
  , fLayerPosZ(0.)
  , fHitPosX(0.)
  , fHitPosY(0.)
  , fHitPosZ(0.)
  , fHitPosErrX(0.0005)
  , fHitPosErrY(0.0005)
  , fHitPosErrZ(0.0)
  , fBranchName("MvdHit")
  , fDigisInCluster(0)
  , fAddNoise(kFALSE)
{
  fPluginIDNumber = 300;
}
// -------------------------------------------------------------------------

// -----   Destructor   ----------------------------------------------------
CbmMvdSensorHitfinderTask::~CbmMvdSensorHitfinderTask()
{

  if (fOutputBuffer) {
    fOutputBuffer->Delete();
    delete fOutputBuffer;
  }

  if (fClusters) {
    fClusters->Delete();
    delete fClusters;
  }

  if (fInputBuffer) {
    fInputBuffer->Delete();
    delete fInputBuffer;
  }
}
// -------------------------------------------------------------------------

// -----    Virtual private method Init   ----------------------------------
void CbmMvdSensorHitfinderTask::InitTask(CbmMvdSensor* mysensor)
{


  fSensor = mysensor;
  LOG(debug) << "CbmMvdSensorHitfinderTask: Initialisation of sensor " << fSensor->GetName();

  fInputBuffer  = new TClonesArray("CbmMvdCluster", 100);
  fOutputBuffer = new TClonesArray("CbmMvdHit", 100);


  //Add charge collection histograms
  fPixelChargeHistos = new TObjArray();

  fTotalChargeInNpixelsArray = new TObjArray();

  fAdcSteps    = (Int_t) TMath::Power(2, fAdcBits);
  fAdcStepSize = fAdcDynamic / fAdcSteps;

  fGausArray = new Float_t[fGausArrayLimit];
  for (Int_t i = 0; i < fGausArrayLimit; i++) {
    fGausArray[i] = gRandom->Gaus(0, fSigmaNoise);
  };
  fGausArrayIt = 0;


  initialized = kTRUE;

  //LOG(debug) << "-Finished- " << GetName() << ": Initialisation of sensor " << fSensor->GetName();
}
// -------------------------------------------------------------------------

// -----   Virtual public method Reinit   ----------------------------------
InitStatus CbmMvdSensorHitfinderTask::ReInit()
{
  LOG(info) << "CbmMvdSensorHitfinderTask::ReInt---------------";
  return kSUCCESS;
}
// -------------------------------------------------------------------------

// -----   Virtual public method ExecChain   --------------
void CbmMvdSensorHitfinderTask::ExecChain() { Exec(); }
// -------------------------------------------------------------------------

// -----   Virtual public method Exec   --------------
void CbmMvdSensorHitfinderTask::Exec()
{

  // if(!inputSet)
  //  {
  //  fInputBuffer->Clear();
  //  fInputBuffer->AbsorbObjects(fPreviousPlugin->GetOutputArray());
  //  LOG(debug) << "absorbt object from previous plugin at " << fSensor->GetName() << " got " << fInputBuffer->GetEntriesFast() << " entrys";
  //  }
  if (fInputBuffer->GetEntriesFast() > 0) {

    fOutputBuffer->Clear();
    inputSet = kFALSE;
    for (Int_t i = 0; i < fInputBuffer->GetEntriesFast(); i++) {
      CbmMvdCluster* cluster = (CbmMvdCluster*) fInputBuffer->At(i);
      TVector3 pos(0, 0, 0);
      TVector3 dpos(0, 0, 0);
      CreateHit(cluster, pos, dpos);
    }
  }
  fInputBuffer->Delete();
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
void CbmMvdSensorHitfinderTask::CreateHit(CbmMvdCluster* cluster, TVector3& pos, TVector3& dpos)
{
  // Calculate the center of gravity of the charge of a cluster
  double startTime     = 0;
  double endTime       = 0;
  double indicatedTime = 0;

  ComputeCenterOfGravity(cluster, pos, dpos, indicatedTime, startTime, endTime);


  Int_t indexX, indexY;
  Double_t local[3] = {pos.X(), pos.Y(), pos.Z()};

  //LOG(debug)<< "found center of gravity at: " << local[0] << " , " << local[1];

  fSensor->TopToPixel(local, indexX, indexY);

  //LOG(debug) << "Center is on pixel: " << indexX << " , " << indexY;


  // Save hit into array

  //LOG(debug) << "adding new hit to fHits at X: " << pos.X() << " , Y: "<< pos.Y() << " , Z: " << pos.Z() << " , from cluster nr " << cluster->GetRefId();
  Int_t nHits = fOutputBuffer->GetEntriesFast();
  new ((*fOutputBuffer)[nHits]) CbmMvdHit(fSensor->GetStationNr(), pos, dpos, indexX, indexY, cluster->GetRefId(),
                                          0);  // Bug - Should be the sensor ID
  CbmMvdHit* currentHit = (CbmMvdHit*) fOutputBuffer->At(nHits);

  currentHit->SetTime(indicatedTime);
  currentHit->SetTimeError((endTime - startTime) / 3.64);  //Assume Sqrt(12) - Error
  currentHit->SetValidityStartTime(startTime);
  currentHit->SetValidityEndTime(endTime);
  currentHit->SetRefId(cluster->GetRefId());
}

//--------------------------------------------------------------------------

/*void CbmMvdSensorHitfinderTask::UpdateDebugHistos(vector<Int_t>* clusterArray, Int_t seedIndexX, Int_t seedIndexY)
{ 
;  
} */


//--------------------------------------------------------------------------

void CbmMvdSensorHitfinderTask::ComputeCenterOfGravity(CbmMvdCluster* cluster, TVector3& pos, TVector3& dpos,
                                                       double& indicatedTime, double& startTime, double& endTime)
{

  Double_t numeratorX  = 0;
  Double_t numeratorY  = 0;
  Double_t denominator = 0;
  Int_t charge;
  Int_t xIndex;
  Int_t yIndex;
  Double_t x, y;
  Double_t layerPosZ                           = fSensor->GetZ();
  Double_t lab[3]                              = {0, 0, 0};
  std::map<pair<Int_t, Int_t>, Int_t> PixelMap = cluster->GetPixelMap();
  Int_t clusterSize                            = cluster->GetNofDigis();

  // UInt_t shape  = 0;
  // Int_t xIndex0 = 0;
  // Int_t yIndex0 = 0;

  Double_t sigmaIn[3] = {0., 0., 0.}, sigmaOut[3] = {0., 0., 0.}, shiftIn[3] = {0., 0., 0.}, shiftOut[3] = {0., 0., 0.};

  //Double_t GetFrameStartTime(Int_t frameNumber);
  //Double_t GetFrameEndTime(Int_t frameNumber) {return GetFrameStartTime (frameNumber+1);}


  //Scan digis for the digi with minimal frame number.
  //For the time being, it is anticipated that this digi provides the best time estimate as it has lowest signal delay

  //   CbmMvdDigi* myDigi=(CbmMvdDigi*)cluster->GetDigi(0);
  //   int32_t minFrameNumber=myDigi->GetFrameNumber();
  //
  //   for(Int_t i=0;i<clusterSize; i++){
  //     myDigi=(CbmMvdDigi*)cluster->GetDigi(i);
  //     int32_t frame=myDigi->GetFrameNumber();
  //     if(frame<minFrameNumber){minFrameNumber=frame;}
  //   }

  //Estimate the highest possible delay and jitter of the analog amplifier chain from the sensor data sheet.

  CbmMvdSensorDataSheet* sensorDataSheet = fSensor->GetDataSheet();
  int32_t minFrameNumber                 = cluster->GetEarliestFrameNumber();


  //Compute time walk and jitter assuming that the maximum possible signal is reasonably well represented with 10k signal electrons
  Double_t timeWalk =
    sensorDataSheet->GetDelay(sensorDataSheet->GetAnalogThreshold()) - sensorDataSheet->GetDelay(10000);
  timeWalk =
    timeWalk
    + 2 * sensorDataSheet->GetDelaySigma(sensorDataSheet->GetAnalogThreshold());        //include pixel-to-pixel spread.
  Double_t jitter = sensorDataSheet->GetJitter(sensorDataSheet->GetAnalogThreshold());  //include jitter.

  // fixme: pixel-to-pixel spread not accounted for

  startTime = fSensor->GetFrameStartTime(minFrameNumber) - timeWalk - 2 * jitter;
  endTime   = fSensor->GetFrameEndTime(minFrameNumber) + 2 * jitter;

  indicatedTime = startTime + (endTime - startTime) / 2;


  for (map<pair<Int_t, Int_t>, Int_t>::iterator it = PixelMap.begin(); it != PixelMap.end(); ++it) {
    pair<Int_t, Int_t> pixel = it->first;

    charge = GetAdcCharge(it->second);
    xIndex = pixel.first;
    yIndex = pixel.second;

    // Determine Cluster Shape
    //if (PixelMap.size() <= 4) {
    //  if (it == PixelMap.begin()) {
    //    xIndex0 = xIndex;
    //    yIndex0 = yIndex;
    //  }
    //  shape += TMath::Power(2, (4 * (yIndex - yIndex0 + 3)) + (xIndex - xIndex0));
    //}

    LOG(debug) << "CbmMvdSensorHitfinderTask:: iCluster= " << cluster->GetRefId() << " , clusterSize= " << clusterSize;
    LOG(debug) << "CbmMvdSensorHitfinderTask::xIndex " << xIndex << " , yIndex " << yIndex << " , charge = " << charge;

    fSensor->PixelToTop(xIndex, yIndex, lab);

    x = lab[0];
    y = lab[1];

    Double_t xc = x * charge;
    Double_t yc = y * charge;

    numeratorX += xc;
    numeratorY += yc;
    denominator += charge;
  }

  LOG(debug) << "CbmMvdSensorHitfinderTask::=========================";
  LOG(debug) << "CbmMvdSensorHitfinderTask::numeratorX: " << numeratorX << " , numeratorY: " << numeratorY
             << ", denominator: " << denominator << ", indicated time: " << indicatedTime;

  //Calculate x,y coordinates of the pixel in the laboratory ref frame
  if (denominator != 0) {
    fHitPosX = (numeratorX / denominator);
    fHitPosY = (numeratorY / denominator);
    fHitPosZ = layerPosZ;
  }
  else {
    fHitPosX = 0;
    fHitPosY = 0;
    fHitPosZ = 0;
  }

  LOG(debug) << "CbmMvdSensorHitfinderTask::-----------------------------------";
  LOG(debug) << "CbmMvdSensorHitfinderTask::X hit= " << fHitPosX << " Y hit= " << fHitPosY << " Z hit= " << fHitPosZ;
  LOG(debug) << "CbmMvdSensorHitfinderTask::-----------------------------------";

  // Quick fix to get some errors for the MvdHits.
  // Proposed value 8mum
  // See Redmine issue 3066
  // See plots with residuals from GitLab MR 1504
  sigmaIn[0] = 0.0008;
  sigmaIn[1] = 0.0008;
  sigmaIn[2] = 0.;

  //   // Treat Sigma/Shift of the Cluster according to the Shape
  //   // This goes back to some results of a specific MIMOSA-26 beam time. Switch off for the time being.
  //
  //   if (shape == 12288) {
  //     sigmaIn[0] = 0.00053;
  //     sigmaIn[1] = 0.00063;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = -0.00000;
  //     shiftIn[1] = -0.00001;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 208896) {
  //     sigmaIn[0] = 0.00035;
  //     sigmaIn[1] = 0.00036;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = -0.00000;
  //     shiftIn[1] = -0.00002;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 69632) {
  //     sigmaIn[0] = 0.00028;
  //     sigmaIn[1] = 0.00028;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = -0.00000;
  //     shiftIn[1] = -0.00002;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 28672) {
  //     sigmaIn[0] = 0.00028;
  //     sigmaIn[1] = 0.00039;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = -0.00000;
  //     shiftIn[1] = -0.00001;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 143360) {
  //     sigmaIn[0] = 0.00024;
  //     sigmaIn[1] = 0.00022;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = +0.00020;
  //     shiftIn[1] = +0.00008;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 200704) {
  //     sigmaIn[0] = 0.00024;
  //     sigmaIn[1] = 0.00022;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = -0.00020;
  //     shiftIn[1] = -0.00011;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 77824) {
  //     sigmaIn[0] = 0.00024;
  //     sigmaIn[1] = 0.00022;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = -0.00020;
  //     shiftIn[1] = +0.00008;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 12800) {
  //     sigmaIn[0] = 0.00024;
  //     sigmaIn[1] = 0.00022;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = +0.00020;
  //     shiftIn[1] = -0.00011;
  //     shiftIn[2] = 0.;
  //   }
  //   else if (shape == 4096) {
  //     sigmaIn[0] = 0.00027;
  //     sigmaIn[1] = 0.00092;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = +0.00002;
  //     shiftIn[1] = +0.00004;
  //     shiftIn[2] = 0.;
  //   }
  //   else {
  //     sigmaIn[0] = 0.00036;
  //     sigmaIn[1] = 0.00044;
  //     sigmaIn[2] = 0.;
  //     shiftIn[0] = -0.00000;
  //     shiftIn[1] = -0.00002;
  //     shiftIn[2] = 0.;
  //   }
  // Consider Sensor Orientation
  TGeoHMatrix* RecoMatrix = fSensor->GetRecoMatrix();
  TGeoHMatrix RotMatrix;
  RotMatrix.SetRotation(RecoMatrix->GetRotationMatrix());

  RotMatrix.LocalToMaster(sigmaIn, sigmaOut);
  RotMatrix.LocalToMaster(shiftIn, shiftOut);

  fHitPosErrX = TMath::Abs(sigmaOut[0]);
  fHitPosErrY = TMath::Abs(sigmaOut[1]);
  fHitPosErrZ = TMath::Abs(sigmaOut[2]);

  fHitPosX += shiftOut[0];
  fHitPosY += shiftOut[1];
  fHitPosZ += shiftOut[2];

  // pos = center of gravity (labframe), dpos uncertainty LOG(info)<<setw(10)<<setprecision(2)<< VolumeShape->GetDX();
  pos.SetXYZ(fHitPosX, fHitPosY, fHitPosZ);
  dpos.SetXYZ(fHitPosErrX, fHitPosErrY, fHitPosErrZ);
}

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
void CbmMvdSensorHitfinderTask::Finish() {}
//--------------------------------------------------------------------------

Int_t CbmMvdSensorHitfinderTask::GetAdcCharge(Float_t charge)
{

  Int_t adcCharge;

  if (charge < fAdcOffset) {
    return 0;
  };

  adcCharge = int((charge - fAdcOffset) / fAdcStepSize);
  if (adcCharge > fAdcSteps - 1) {
    adcCharge = fAdcSteps - 1;
  }

  return adcCharge;
}


// -----   Private method Reset   ------------------------------------------
void CbmMvdSensorHitfinderTask::Reset() { fClusters->Clear("C"); }

// -------------------------------------------------------------------------


ClassImp(CbmMvdSensorHitfinderTask);
