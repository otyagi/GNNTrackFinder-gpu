/* Copyright (C) 2008-2013 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

/**
 * \file CbmLitTrackSelectionMuch.h
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2008
 * \brief Track selection for MUCH.
 */

#ifndef CBMLITTRACKSELECTIONMUCH_H_
#define CBMLITTRACKSELECTIONMUCH_H_

#include "base/CbmLitPtrTypes.h"
#include "interface/CbmLitTrackSelection.h"

class CbmLitTrackSelectionMuch : public CbmLitTrackSelection {
 public:
  /**
    * \brief Constructor.
    */
  CbmLitTrackSelectionMuch();

  /**
    * \brief Destructor.
    */
  virtual ~CbmLitTrackSelectionMuch();

  /**
    * \brief Inherited from CbmLitTrackSelection.
    */
  virtual LitStatus DoSelect(TrackPtrIterator itBegin, TrackPtrIterator itEnd);

  /**
    * \brief Inherited from CbmLitTrackSelection.
    */
  virtual LitStatus DoSelect(TrackPtrVector& tracks);

  /* Setters */
  void SetNofSharedHits(Int_t nofHits) { fNofSharedHits = nofHits; }

 private:
  TrackSelectionPtr fSharedHitsSelection;  // Shared hits track selection tool.
  Int_t fNofSharedHits;                    // Maximum number of shared hits.
};

#endif /*CBMLITTRACKSELECTIONMUCH_H_*/
