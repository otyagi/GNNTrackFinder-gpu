/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer], Norbert Herrmann */


#ifndef CbmTimesliceRecoTracks_H
#define CbmTimesliceRecoTracks_H

#include <FairTask.h>  // for FairTask, InitStatus

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Double_t, Option_t
#include <TEveVector.h>  // for TEveVector
#include <TObjArray.h>   // for TObjArray, needed for FairRoot > v18.8.0
#include <TString.h>     // for TString

class CbmPixelHit;
class CbmStsTrack;
class CbmTrack;

class TClonesArray;
class TEveTrack;
class TEveTrackList;
class TEveTrackPropagator;
class TObjArray;
class TParticle;

#ifndef CbmTimesliceManager_H
class CbmTimesliceManager;
#endif  // CbmTimesliceManager_H

/** @class CbmTimesliceRecoTracks
 ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
 ** @brief Interface class to add Cbm Reco tracks drawing to CbmTimesliceManager. Cannot be used alone!
 **/
class CbmTimesliceRecoTracks : public FairTask {
public:
  /** Default constructor **/
  CbmTimesliceRecoTracks() : CbmTimesliceRecoTracks("CbmTimesliceRecoTracks", 0) {}

  /** Standard constructor
    * @param name        Name of task
    * @param iVerbose    Verbosity level
    **/
  CbmTimesliceRecoTracks(const char* name, Int_t iVerbose = 1) : FairTask(name, iVerbose) {}

  /** Destructor **/
  virtual ~CbmTimesliceRecoTracks() = default;

  CbmTimesliceRecoTracks(const CbmTimesliceRecoTracks&) = delete;
  CbmTimesliceRecoTracks& operator=(const CbmTimesliceRecoTracks&) = delete;

  /** Set verbosity level. For this task and all of the subtasks. **/
  void SetVerbose(Int_t iVerbose) { fVerbose = iVerbose; }

  /**
   ** switch track color: to be called by GUI element
   ** @param PDG color if true, red if false (see TimesliceRecoTracks)
   **/
  void SwitchPdgColorTrack(bool pdg_color) { fbPdgColorTrack = pdg_color; }

  virtual InitStatus Init();
  virtual void Exec(Option_t* option);
  virtual void SetParContainers() { ; }
  virtual void Finish() { ; }

  /**
   ** @brief Load tracks from selected event in timeslice. RESERVED FOR GUI CALLS!
   **/
  void GotoEvent(uint32_t uEventIdx);
  void Reset();
  TEveTrackList* GetTrGroup(TParticle* P);

protected:
  void HandlePixelHit(TEveTrack* eveTrack, Int_t& n, const CbmPixelHit* hit, TEveVector* pMom);
  void HandleTrack(TEveTrack* eveTrack, Int_t& n, const CbmTrack* recoTrack);
  void HandleStsTrack(TEveTrack* eveTrack, Int_t& n, const CbmStsTrack* stsTrack);

  TClonesArray* fCbmEvents           = nullptr;            //!
  TClonesArray* fGlobalTracks        = nullptr;            //!
  TClonesArray* fMvdHits             = nullptr;            //!
  TClonesArray* fStsHits             = nullptr;            //!
  TClonesArray* fStsTracks           = nullptr;            //!
  TClonesArray* fRichRings           = nullptr;            //!
  TClonesArray* fRichHits            = nullptr;            //!
  TClonesArray* fMuchPixelHits       = nullptr;            //!
  TClonesArray* fMuchTracks          = nullptr;            //!
  TClonesArray* fTrdHits             = nullptr;            //!
  TClonesArray* fTrdTracks           = nullptr;            //!
  TClonesArray* fTofHits             = nullptr;            //!
  TClonesArray* fTofTracks           = nullptr;            //!
  TEveTrackPropagator* fTrPr         = nullptr;            //!
  CbmTimesliceManager* fEventManager = nullptr;            //!
  TObjArray* fEveTrList              = new TObjArray(16);  //!
  uint32_t fEventIdx                 = 0;                  //!
  TEveTrackList* fTrList             = nullptr;            //!
  bool fbPdgColorTrack               = false;              //!

 private:
  ClassDef(CbmTimesliceRecoTracks, 1);
};


#endif  // CbmTimesliceRecoTracks_H
