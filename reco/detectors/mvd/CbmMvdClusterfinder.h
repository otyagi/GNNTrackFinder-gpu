/* Copyright (C) 2014-2020 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer] */

// ----------------------------------------------------------------------------
// -----               CbmMvdClusterfinder header file                    -----
// -----                   Created by P.Sitzmann 03.12.2014               -----
// ----------------------------------------------------------------------------


#ifndef CBMMVDCLUSTERFINDER_H
#define CBMMVDCLUSTERFINDER_H 1

#include "CbmDefs.h"
#include "FairTask.h"  // for InitStatus, FairTask

#include <Rtypes.h>      // for ClassDef
#include <RtypesCore.h>  // for Int_t, Bool_t, Option_t, UInt_t, kTRUE
#include <TStopwatch.h>  // for TStopwatch
#include <TString.h>     // for TString

class CbmDigiManager;
class CbmEvent;
class CbmMvdDetector;
class TBuffer;
class TClass;
class TClonesArray;
class TMemberInspector;


class CbmMvdClusterfinder : public FairTask {

 public:
  /** Default constructor **/
  CbmMvdClusterfinder();


  /** Standard constructor 
  *@param name  Task name
  *@param mode  0 = MAPS, 1 = Ideal
  **/
  CbmMvdClusterfinder(const char* name, Int_t mode = 0, Int_t iVerbose = 1);


  /** @brief Set execution mode
     ** @param mode  Time-slice or event
     **
     ** In the time-slice mode, the entire time-slice (input arrays)
     ** will be processed. In the event mode, events read from the event
     ** branch are processed one after the other.
     **/
  void SetMode(ECbmRecoMode mode) { fEventMode = mode; }

  /** Destructor **/
  ~CbmMvdClusterfinder();

  void Exec(Option_t* opt);

  void ShowDebugHistos() { fShowDebugHistos = kTRUE; }

 private:
  /** Hit producer mode (0 = MAPS, 1 = Ideal) **/
  Int_t fMode;
  Bool_t fShowDebugHistos;
  CbmMvdDetector* fDetector;
  CbmDigiManager* fDigiMan;

  TClonesArray* fCluster;
  TClonesArray* fEvents = nullptr;  //! Input array of events

  UInt_t fClusterPluginNr;

  TString fBranchName;  // Name of input branch (MvdDigi)


  TStopwatch fTimer;  ///< ROOT timer

  ECbmRecoMode fEventMode = ECbmRecoMode::Timeslice;  ///< Time-slice or event-by-event

  Int_t fNofTs = 0;  ///< Number of time slices processed

  // -----   Private methods   ---------------------------------------------
  /** Intialisation **/
  virtual InitStatus Init();


  /** Reinitialisation **/
  virtual InitStatus ReInit();


  /** Called once when finishing a run **/
  virtual void Finish();

  /** Register the output arrays to the IOManager **/
  void Register();

  void GetMvdGeometry();


  /** Clear the hit arrays **/
  void Reset();


  /** Print digitisation parameters **/
  void PrintParameters() const;
  std::string ParametersToString() const;

  void ProcessData(CbmEvent* event);

 private:
  CbmMvdClusterfinder(const CbmMvdClusterfinder&);
  CbmMvdClusterfinder operator=(const CbmMvdClusterfinder&);

  ClassDef(CbmMvdClusterfinder, 1);
};


#endif
