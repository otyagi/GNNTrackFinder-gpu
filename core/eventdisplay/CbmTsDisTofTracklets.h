/* Copyright (C) 2023 PI-UHd, Heidelberg
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann [committer], Pierre-Alain Loizeau */

/** CbmTsDisTofTracklets
 *  @author N. Herrmann
 *  @since 05.11.23
 *  Task to display TOF Tracklets
 *  Timeslice compatible version of CbmEvDisTracks
 **
 **/

#define TOFDisplay 1  // =1 means active, other: without Label and not relying on TEvePointSet

#ifndef CBMTSDISTRACKS_H
#define CBMTSDISTRACKS_H

#include <CbmTimesliceManager.h>  // IWYU pragma: keep needed by cling

#include <FairTask.h>  // for FairTask, InitStatus

#include <Rtypes.h>               // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>           // for Bool_t, Int_t, Double_t, kFALSE, kTRUE, Opti...
#include <TEveTrackPropagator.h>  // IWYU pragma: keep needed by cling
#include <TString.h>              // for TString

class TClonesArray;
class TEveElementList;
class TEveTrackList;
class TObjArray;

class CbmTsDisTofTracklets : public FairTask {

 public:
  /** Default constructor **/
  CbmTsDisTofTracklets();


  /** Standard constructor
    *@param name        Name of task
    *@param iVerbose    Verbosity level
    **/
  CbmTsDisTofTracklets(const char* name, Int_t iVerbose = 1, Bool_t renderP = kFALSE, Bool_t renderT = kTRUE);

  /** Destructor **/
  virtual ~CbmTsDisTofTracklets();

  inline static CbmTsDisTofTracklets* Instance() { return fInstance; }

  /** Set verbosity level. For this task and all of the subtasks. **/
  void SetVerbose(Int_t iVerbose) { fVerbose = iVerbose; }
  void SetRenderP(Bool_t render) { fRenderP = render; }
  void SetRenderT(Bool_t render) { fRenderT = render; }
  /** Executed task **/
  virtual void Exec(Option_t* option);
  virtual InitStatus Init();
  virtual void SetParContainers();

  /** Action after each event**/
  virtual void Finish();
  void GotoEvent(uint32_t uEventIdx);
  void Reset();
  TEveTrackList* GetTrGroup(Int_t ihmul, Int_t iOpt);
#if TOFDisplay == 1  //List for TEvePointSets
  TEveElementList* GetPSGroup(Int_t ihuml, Int_t iOpt);
#endif

 protected:
  TClonesArray* fCbmEvents               = nullptr;  //!
  TClonesArray* fTrackList               = nullptr;  //!
  TEveTrackPropagator* fTrPr             = nullptr;  //!
  CbmTimesliceManager* fTimesliceManager = nullptr;  //!
  TObjArray* fEveTrList                  = nullptr;  //!
  TString fEvent                         = "";       //!
  TEveTrackList* fTrList                 = nullptr;  //!
  TObjArray* fEvePSList                  = nullptr;  //!
  TEveElementList* fPSList               = nullptr;  //!
  //TEveElementList *fTrackCont;

  Bool_t fRenderP         = kFALSE;
  Bool_t fRenderT         = kFALSE;
  Double_t MinEnergyLimit = -1.;
  Double_t MaxEnergyLimit = -1.;
  Double_t PEnergy        = -1.;
  uint32_t fEventIdx      = 0;

 private:
  static CbmTsDisTofTracklets* fInstance;
  CbmTsDisTofTracklets(const CbmTsDisTofTracklets&);
  CbmTsDisTofTracklets& operator=(const CbmTsDisTofTracklets&);

  ClassDef(CbmTsDisTofTracklets, 1);
};


#endif
