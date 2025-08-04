/* Copyright (C) 2019 Frankfurt Institute for Advanced Studies, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andreas Redelbach [committer] */

// ----------------------------------------------------------------------------
// -----               CbmMvdDigiToHit header file                    -----
// -----                   Created by P.Sitzmann 03.12.2014               -----
// ----------------------------------------------------------------------------


#ifndef CBMMVDDIGITOHIT_H
#define CBMMVDDIGITOHIT_H 1


#include "FairTask.h"
#include "TStopwatch.h"

#include <string>

class CbmMvdDetector;
class TClonesArray;
class TString;


class CbmMvdDigiToHit : public FairTask {

 public:
  /** Default constructor **/
  CbmMvdDigiToHit();


  /** Standard constructor 
  *@param name  Task name
  *@param mode  0 = MAPS, 1 = Ideal
  **/
  CbmMvdDigiToHit(const char* name, Int_t mode = 0, Int_t iVerbose = 1);


  /** Destructor **/
  ~CbmMvdDigiToHit();

  void Exec(Option_t* opt);

  void ShowDebugHistos() { fShowDebugHistos = kTRUE; }

 private:
  /** Hit producer mode (0 = MAPS, 1 = Ideal) **/
  Int_t fMode;
  Bool_t fShowDebugHistos;
  CbmMvdDetector* fDetector;

  TClonesArray* fInputDigis;
  TClonesArray* fHit;

  UInt_t fHitPluginNr;

  TString fBranchName;  // Name of input branch (MvdDigi)


  TStopwatch fTimer;  ///< ROOT timer


  // -----   Private methods   ---------------------------------------------
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
  CbmMvdDigiToHit(const CbmMvdDigiToHit&);
  CbmMvdDigiToHit operator=(const CbmMvdDigiToHit&);

  ClassDef(CbmMvdDigiToHit, 1);
};


#endif
