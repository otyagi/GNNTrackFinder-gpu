/* Copyright (C) 2006-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel, Sergey Gorbunov, Denis Bertini [committer], Volker Friese */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction 
 *  
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de 
 *
 *====================================================================
 *
 *  CbmL1StsTrackFinder header
 *
 *====================================================================
 */

#ifndef CBML1STSTRACKFINDER
#define CBML1STSTRACKFINDER 1


#include "CbmL1.h"
#include "CbmStsTrackFinder.h"

class TClonesArray;
class CbmEvent;


class CbmL1StsTrackFinder : public CbmStsTrackFinder {

 public:
  /** Default constructor **/
  CbmL1StsTrackFinder();


  /** Destructor **/
  ~CbmL1StsTrackFinder();


  /** Initialisation **/
  void Init();


  /** Track finding algorithm
   **/
  Int_t DoFind();

  /** Execute track finding on one event
   ** @param event  Pointer to event object
   ** @value Number of created tracks
   **/
  Int_t FindTracks(CbmEvent* event);


  /// set a default particle mass for the track fit
  /// it is used during reconstruction
  /// for the multiple scattering and energy loss estimation
  void SetDefaultParticlePDG(int pdg = 13);  // muon

 private:
  /** Copy the tracks from the L1-internal format and array
   ** to the output TClonesArray.
   ** @value  Number of created tracks
   **/
  Int_t CopyL1Tracks(CbmEvent* event = nullptr);


  ClassDef(CbmL1StsTrackFinder, 1);
};


#endif
