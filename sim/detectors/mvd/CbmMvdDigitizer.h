/* Copyright (C) 2014-2020 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer], Florian Uhlig */

// ----------------------------------------------------------------------------
// -----                    CbmMvdDigitizer header file                   -----
// -----                   Created by C. Dritsa (2009)                    -----
// -----                   Maintained by M.Deveaux (m.deveaux(att)gsi.de) -----
// -----                   Update by P.Sitzmann (p.sitzmann(att)gsi.de    -----
// ----------------------------------------------------------------------------

#ifndef CBMMVDDIGITIZER_H
#define CBMMVDDIGITIZER_H 1

#include "CbmDefs.h"      // for ECbmModuleId
#include "CbmDigitize.h"  // for CbmDigitize
#include "CbmMvdDigi.h"   // for CbmMvdDigi

#include <FairTask.h>  // for InitStatus

#include <Rtypes.h>      // for ClassDef
#include <RtypesCore.h>  // for Int_t, Float_t, Bool_t, kTRUE, Double_t
#include <TStopwatch.h>  // for TStopwatch
#include <TString.h>     // for TString

#include <string>   // for string
#include <utility>  // for pair
#include <vector>   // for vector

class CbmMatch;
class CbmMvdDetector;
class CbmMvdPileupManager;
class TBuffer;
class TClass;
class TClonesArray;
class TMemberInspector;

class CbmMvdDigitizer : public CbmDigitize<CbmMvdDigi> {

 public:
  /** Default constructor **/
  CbmMvdDigitizer();


  /** Standard constructor 
  *@param name  Task name
  *@param mode  0 = MAPS, 1 = Ideal
  **/
  CbmMvdDigitizer(const char* name, Int_t mode = 0, Int_t iVerbose = 1);


  /** Destructor **/
  ~CbmMvdDigitizer();

  void Exec(Option_t* opt);

  /** @brief Detector system ID
   ** @return kMvd
   **/
  ECbmModuleId GetSystemId() const { return ECbmModuleId::kMvd; }

  /** @brief Detector specific latency
   ** @return latency
   **/
  Double_t GetLatency() const { return fLatency; }

  void SetLatency(Float_t latency) { fLatency = latency; }

  void SetPileUp(Int_t pileUp) { fNPileup = pileUp; }
  void SetDeltaEvents(Int_t deltaEvents) { fNDeltaElect = deltaEvents; }
  void SetBgFileName(TString fileName) { fBgFileName = fileName; }
  void SetDeltaName(TString fileName) { fDeltaFileName = fileName; }
  void SetBgBufferSize(Int_t nBuffer) { fBgBufferSize = nBuffer; }
  void SetDeltaBufferSize(Int_t nBuffer) { fDeltaBufferSize = nBuffer; }
  void SetMisalignment(Float_t misalignment[3])
  {
    for (Int_t i = 0; i < 3; i++)
      epsilon[i] = misalignment[i];
  }  // set the misalignment in cm

  void BuildEvent();
  void ShowDebugHistograms()
  {
    fShowDebugHistos = kTRUE;
  }                           // Requests Task to create the histograms. Call before calling Init()
  void DisplayDebugHistos();  // Function to display histograms at Finish, must call ShowDebugHistograms before.
  void
  SafeDebugHistosToFile(TString histoFile);  // Function safes histograms to file, must call ShowDebugHistograms before.
  void CollectHistograms();  // Collects histograms from the sensors and safes them in TObjArray* fHistoArray.
                             // Histos are NOT copied, copy manually if wanted.
  TObjArray* GetHistograms()
  {
    return fHistoArray;
  };  // Access to Histos. They are NOT copied, clone manually if wanted.

  void SetProduceNoise() { fNoiseSensors = kTRUE; };
  Int_t DetectPlugin(Int_t pluginID);


  /** @brief Clear data arrays **/
  virtual void ResetArrays();


  /** Intialisation **/
  virtual InitStatus Init();


  /** Reinitialisation **/
  virtual InitStatus ReInit();


  /** Virtual method Finish **/
  virtual void Finish();


  /** Register the output arrays to the IOManager **/
  void Register();

  void GetMvdGeometry();


  /** Clear the hit arrays **/
  void Reset();


  /** Print digitisation parameters **/
  void PrintParameters() const;
  std::string ParametersToString() const;

 private:
  /** Hit producer mode (0 = MAPS, 1 = Ideal) **/
  Int_t fMode;

  Bool_t fShowDebugHistos;
  Bool_t fNoiseSensors;

  CbmMvdDetector* fDetector;

  TClonesArray* fInputPoints;
  TClonesArray* fMcPileUp;

  TClonesArray* fTmpMatch;  //! Temporary TClonesArray to absorb from MvdDetector
  TClonesArray* fTmpDigi;   //! Temporary TClonesArray to absorb from MvdDetector

  TObjArray* fHistoArray;

  std::vector<CbmMvdDigi*> fDigiVect;  //! Temporary storage for CbmDaq
  std::vector<CbmMatch*> fMatchVect;   //! Temporary storage for CbmDaq

  std::pair<Float_t, Int_t> fPerformanceDigi;

  Int_t fDigiPluginNr;

  Double_t fFakeRate;  // Fake hit rate
  Int_t fNPileup;      // Number of pile-up background events
  Int_t fNDeltaElect;  // Number of delta electron events
  Int_t fDeltaBufferSize;
  Int_t fBgBufferSize;
  Float_t epsilon[3];

  TString fInputBranchName;  // Name of input branch (MvdPoint)
  TString fBgFileName;       // Name of background (pileup) file
  TString fDeltaFileName;    // Name of the file containing delta electrons
  TString fHistoFileName;


  TStopwatch fTimer;  ///< ROOT timer

  Float_t fLatency{150000.};  // maximum time of digi disordering

  /** Pileup manager **/
  CbmMvdPileupManager* fPileupManager;
  CbmMvdPileupManager* fDeltaManager;


 private:
  CbmMvdDigitizer(const CbmMvdDigitizer&);
  CbmMvdDigitizer operator=(const CbmMvdDigitizer&);

  ClassDef(CbmMvdDigitizer, 1);
};


#endif
