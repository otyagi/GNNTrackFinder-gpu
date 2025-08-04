/* Copyright (C) 2011-2017 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

/**
 * \file CbmLitFindMvdTracks.h
 * \brief MVD tracking based on littrack package.
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2011
 *
 */
#ifndef CBMLITFINDMVDTRACKS_H_
#define CBMLITFINDMVDTRACKS_H_

#include "FairTask.h"
#include "base/CbmLitPtrTypes.h"
#include "data/CbmLitHit.h"

class CbmEvent;
class TClonesArray;

/**
 * \class CbmLitFindMvdTracks.h
 * \brief MVD tracking based on littrack package.
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2011
 *
 * This MVD tracking uses STS tracks as seeds,
 * propagates tracks through MVD and attaches
 * nearest MVD hits.
 *
 */
class CbmLitFindMvdTracks : public FairTask {
 public:
  /**
    * \brief Constructor.
    */
  CbmLitFindMvdTracks();

  /**
    * \brief Destructor.
    */
  virtual ~CbmLitFindMvdTracks();

  /**
    * \brief Inherited from FairTask.
    */
  virtual InitStatus Init();

  /**
    * \brief Inherited from FairTask.
    */
  virtual void Exec(Option_t* opt);

  /**
    * \brief Inherited from FairTask.
    */
  virtual void SetParContainers();

 private:
  /**
    * \brief Inherited from FairTask.
    */
  virtual void Finish();

  /**
    * \brief Read necessary data branches from the input data files.
    */
  void ReadAndCreateDataBranches();

  /**
    * \brief Convert input data from CBMROOT data classes to littrack data classes.
    */
  void ConvertInputData(CbmEvent* event);

  /**
    * \brief Convert output littrack data classes to CBMROOT data classes.
    */
  void ConvertOutputData();

  /**
    * \brief Run track reconstruction.
    */
  void RunTrackReconstruction();

  /**
    * \brief Clear arrays and free memory.
    */
  void ClearArrays();

  // Pointers to data arrays
  TClonesArray* fStsTracks;  // CbmStsTrack array
  TClonesArray* fMvdHits;    // CbmMvdHits
  TClonesArray* fEvents;

  // littrack data arrays
  TrackPtrVector fLitStsTracks;     // STS tracks
  HitPtrVector fLitMvdHits;         // MVD
  TrackPtrVector fLitOutputTracks;  // output Lit tracks

  // Tools
  TrackFinderPtr fFinder;  // track finder

  Int_t fEventNo;  // event counter

  CbmLitFindMvdTracks(const CbmLitFindMvdTracks&);
  CbmLitFindMvdTracks& operator=(const CbmLitFindMvdTracks&);

  ClassDef(CbmLitFindMvdTracks, 1);
};

#endif /* CBMLITFINDMVDTRACKS_H_ */
