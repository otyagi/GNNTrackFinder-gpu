/* Copyright (C) 2019 Frankfurt Institute for Advanced Studies, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors:  Andreas Redelbach [committer] */

// ----------------------------------------------------------------------------
// -----               CbmMvdDigiToHitTB header file                    -----
// -----                   Created by P.Sitzmann 03.12.2014               -----
// ----------------------------------------------------------------------------


#ifndef CBMMVDDIGITOHITTB_H
#define CBMMVDDIGITOHITTB_H 1


#include "FairTask.h"
#include "TStopwatch.h"

#include <string>

class CbmMvdDetector;
class TClonesArray;
class TString;


class CbmMvdDigiToHitTB : public FairTask {

 public:
  /** Default constructor **/
  CbmMvdDigiToHitTB();


  /** Standard constructor 
  *@param name  Task name
  *@param mode  0 = MAPS, 1 = Ideal
  **/
  CbmMvdDigiToHitTB(const char* name, Int_t mode = 0, Int_t iVerbose = 1);


  /** Destructor **/
  ~CbmMvdDigiToHitTB();

  void Exec(Option_t* opt);

  void ShowDebugHistos() { fShowDebugHistos = kTRUE; }

 private:
  /** Hit producer mode (0 = MAPS, 1 = Ideal) **/
  Int_t fMode;
  Bool_t fShowDebugHistos;
  CbmMvdDetector* fDetector;

  TClonesArray* fEvents;
  TClonesArray* fInputDigis;
  TClonesArray* fEventDigis;
  TClonesArray* fCluster;


  UInt_t fClusterPluginNr;

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
  CbmMvdDigiToHitTB(const CbmMvdDigiToHitTB&);
  CbmMvdDigiToHitTB operator=(const CbmMvdDigiToHitTB&);

  ClassDef(CbmMvdDigiToHitTB, 1);
};


#endif
