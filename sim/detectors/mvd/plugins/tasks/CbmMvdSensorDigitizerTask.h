/* Copyright (C) 2014-2017 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Michael Deveaux, Philipp Sitzmann [committer] */

// ------------------------------------------------------------------------
// -----                  CbmMvdSensorDigitizerTask header file       -----
// -----                   Created 02/02/12  by M. Deveaux            -----
// ------------------------------------------------------------------------

/**  CbmMvdSensorDigitizerTask.h
 **  @author M.Deveaux <M.Deveaux@gsi.de>
 **  Acknowlegments to: C.Dritsa
 **
 **
 **/

#ifndef CBMMVDSENSORDIGITIZERTASK_H
#define CBMMVDSENSORDIGITIZERTASK_H 1

#include "CbmMvdSensorTask.h"  // for CbmMvdSensorTask

#include <FairTask.h>  // for InitStatus

#include <Rtypes.h>      // for ClassDef
#include <RtypesCore.h>  // for Double_t, Int_t, Float_t, Bool_t, kTRUE
#include <TRandom3.h>    // for TRandom3
#include <TStopwatch.h>  // for TStopwatch
#include <TString.h>     // for TString

#include <map>      // for map
#include <utility>  // for pair
#include <vector>   // for vector

class CbmMvdPileupManager;
class CbmMvdPixelCharge;
class CbmMvdPoint;
class CbmMvdSensor;
class CbmMvdSensorDataSheet;
class TBuffer;
class TClass;
class TClonesArray;
class TH1F;
class TH2F;
class TMemberInspector;
class TObjArray;
class TObject;
class TRefArray;

class CbmMvdSensorDigitizerTask : public CbmMvdSensorTask {

 public:
  /** Default constructor **/
  CbmMvdSensorDigitizerTask();
  CbmMvdSensorDigitizerTask(Int_t iMode);

  /** Destructor **/
  virtual ~CbmMvdSensorDigitizerTask();

  /** Intialisation **/
  virtual void InitTask(CbmMvdSensor* mySensor);

  /** fill buffer **/
  void SetInputArray(TClonesArray* inputStream);
  void SetInput(TObject*);  //overwrite

  /** Execute **/
  void Exec();
  void ExecChain();

  TClonesArray* GetOutputArray() { return fOutputBuffer; };
  TClonesArray* GetMatchArray() { return fDigiMatch; };
  TClonesArray* GetWriteArray() { return fDigis; };

  InitStatus ReadSensorInformation();
  void ProduceIonisationPoints(CbmMvdPoint* point);
  void ProducePixelCharge(CbmMvdPoint* point);
  void ProduceNoise();
  Bool_t GetSignalAboveThreshold(CbmMvdPixelCharge* myPixel, Double_t readoutTime);
  Int_t GetPixelCharge(CbmMvdPixelCharge* myPixel, Double_t readoutTime);

  void ProduceDigis();
  void CleanPixelChargeList();
  // Bool_t  GetSignalAboveThreshold (CbmMvdPixelCharge* myPixel, Double_t readoutTime);
  //Int_t GetPixelCharge(CbmMvdPixelCharge* myPixel, Double_t readoutTime);
  Int_t CheckForHit(CbmMvdPixelCharge* pixel);


  void SetProduceNoise() { fproduceNoise = kTRUE; };

  /** Switch from time based mode to evnt based mode
      The difference is that in the event based mode the internal
      buffer is flushed after each input event
   **/
  void SetEventMode() { fEventMode = kTRUE; }

  /** Modifiers **/
  void SetSegmentLength(Double_t segmentLength) { fSegmentLength = segmentLength; }
  void SetDiffusionCoef(Double_t diffCoeff) { fDiffusionCoefficient = diffCoeff; }
  void SetElectronsPerKeV(Double_t electronsPerKeV) { fElectronsPerKeV = electronsPerKeV; }
  void SetWidthOfCluster(Double_t widthOfCluster) { fWidthOfCluster = widthOfCluster; }
  void SetCutOnDeltaRays(Double_t cutOnDeltaRays) { fCutOnDeltaRays = cutOnDeltaRays; }
  void SetChargeThreshold(Float_t chargeThreshold) { fChargeThreshold = chargeThreshold; }

  void GetEventInfo(Int_t& inputNr, Int_t& eventNr, Double_t& eventTime);


  //protected:
 public:
  // ----------   Protected data members  ------------------------------------

  // Information about event and sensor status
  Int_t fcurrentFrameNumber;
  Int_t fEventNr;
  Int_t fInputNr;
  Double_t fEventTime;

  // Information about sensor
  Double_t fEpiTh;
  Double_t fSegmentLength;

  Double_t fDiffusionCoefficient;
  Double_t fElectronsPerKeV;
  Double_t fWidthOfCluster;
  Double_t fPixelSizeX;
  Double_t fPixelSizeY;
  Double_t fCutOnDeltaRays;
  Float_t fChargeThreshold;
  Double_t fFanoSilicium;

  Double_t fEsum;
  Double_t fSegmentDepth;
  Double_t fCurrentTotalCharge;
  Double_t fCurrentParticleMass;
  Double_t fCurrentParticleMomentum;
  Int_t fCurrentParticlePdg;

  TH1F* fRandomGeneratorTestHisto;
  TH2F* fPosXY;
  TH1F* fpZ;
  TH1F* fPosXinIOut;
  TH1F* fAngle;
  TH1F* fSegResolutionHistoX;
  TH1F* fSegResolutionHistoY;
  TH1F* fSegResolutionHistoZ;
  TH1F* fTotalChargeHisto;
  TH1F* fTotalSegmentChargeHisto;


  Double_t fLorentzY0;
  Double_t fLorentzXc;
  Double_t fLorentzW;
  Double_t fLorentzA;
  Double_t fLorentzNorm;

  Double_t fLandauMPV;
  Double_t fLandauSigma;
  Double_t fLandauGain;
  TRandom3* fLandauRandom;

  Double_t fPixelSize;
  Double_t fPar0;
  Double_t fPar1;
  Double_t fPar2;

  Double_t fCompression;

  TH1F* fResolutionHistoX;
  TH1F* fResolutionHistoY;

  Int_t fNumberOfSegments;
  Int_t fCurrentLayer;
  Int_t fEvent;
  Int_t fVolumeId;
  Int_t fNPixelsX;
  Int_t fNPixelsY;

  TClonesArray* fPixelCharge;

  TClonesArray* fDigis;

  TClonesArray* fDigiMatch;

  Bool_t fproduceNoise;

  Bool_t fEventMode{kFALSE};

  std::vector<CbmMvdPixelCharge*> fPixelChargeShort;

  TObjArray* fPixelScanAccelerator;
  std::map<std::pair<Int_t, Int_t>, CbmMvdPixelCharge*> fChargeMap;
  std::map<std::pair<Int_t, Int_t>, CbmMvdPixelCharge*>::iterator fChargeMapIt;


 private:
  CbmMvdSensorDataSheet* fSensorDataSheet;

  /** Hit producer mode (0 = MAPS, 1 = Ideal) **/
  Int_t fMode;


  /** MAPS properties **/
  Double_t fSigmaX, fSigmaY;  // MAPS resolution in [cm]
  Double_t fReadoutTime;      // MAPS readout time in [s]
  Double_t fEfficiency;       // MAPS detection efficiency
  Double_t fMergeDist;        // Merging distance
  Double_t fFakeRate;         // Fake hit rate
  Int_t fNPileup;             // Number of pile-up background events
  Int_t fNDeltaElect;         // Number of delta electron events
  Int_t fDeltaBufferSize;
  Int_t fBgBufferSize;


  /** IO arrays **/
  TString fBranchName;         // Name of input branch (STSPoint)
  TString fBgFileName;         // Name of background (pileup) file
  TString fDeltaFileName;      // Name of the file containing delta electrons
  TClonesArray* fInputPoints;  // Array of MCPoints (input)

  TRefArray* fPoints;  // Array of all MCPoints (including background files)


  /** Random generator and Stopwatch **/
  TRandom3 fRandGen;
  TStopwatch fTimer;


  /** Pileup manager **/
  CbmMvdPileupManager* fPileupManager;
  CbmMvdPileupManager* fDeltaManager;


  /** Counters **/
  Int_t fNEvents;
  Double_t fNPoints;
  Double_t fNReal;
  Double_t fNBg;
  Double_t fNFake;
  Double_t fNLost;
  Double_t fNMerged;
  Double_t fTime;

  // -----   Private methods   ---------------------------------------------

  struct SignalPoint {
    double x;
    double y;
    double z;
    double sigmaX;
    double sigmaY;
    double charge;
    double eloss;
  };


  typedef std::vector<SignalPoint> SignalPointVec;  //!

  SignalPointVec fSignalPoints;  //!


  /** Set parameter containers **/
  virtual void SetParContainers();


  /** Reinitialisation **/
  virtual void ReInit(CbmMvdSensor* mySensor);


  /** Virtual method Finish **/
  virtual void Finish();


  /** Register the output arrays to the IOManager **/
  void Register();


  /** Clear the hit arrays **/
  void Reset();


  /** Print digitisation parameters **/
  void PrintParameters() const;
  std::string ToString() const;

  /** Get MVD geometry parameters from database 
   **@value Number of MVD stations
   **/
  Int_t GetMvdGeometry();

  /** Create MvdDigi and MvdDigiMatch object and store
   ** them in the output buffers
   **/
  void FlushBuffer(CbmMvdPixelCharge* pixel, Int_t i);

  TH1F* h_trackLength;
  TH1F* h_numSegments;
  TH2F* h_LengthVsAngle;
  TH2F* h_LengthVsEloss;
  TH2F* h_ElossVsMomIn;


 private:
  CbmMvdSensorDigitizerTask(const CbmMvdSensorDigitizerTask&);
  CbmMvdSensorDigitizerTask operator=(const CbmMvdSensorDigitizerTask&);

  ClassDef(CbmMvdSensorDigitizerTask, 1);
};


#endif
