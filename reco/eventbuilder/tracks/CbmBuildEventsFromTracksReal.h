/* Copyright (C) 2017 IKF-UFra, GSI
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Valentina Akishina , Maksym Zyzak, Valentina Akishina [committer] */

/** @file CbmBuildEventsFromTracksReal.h
 ** @author Valentina Akishina <v.akishina@gsi.de>, Maksym Zyzak <m.zyzak@gsi.de>
 ** @date 14.03.2017
 **/
#ifndef CBMBUILDEVENTSFROMTRACKSREAL_H_
#define CBMBUILDEVENTSFROMTRACKSREAL_H 1

#include "CbmStsTrack.h"

#include <FairTask.h>

class TClonesArray;
class CbmMCEventList;

class CbmBuildEventsFromTracksReal : public FairTask {
 public:
  /** Constructor **/
  CbmBuildEventsFromTracksReal();

  CbmBuildEventsFromTracksReal(const CbmBuildEventsFromTracksReal&) = delete;
  CbmBuildEventsFromTracksReal& operator=(const CbmBuildEventsFromTracksReal&) = delete;
  /** Destructor **/
  virtual ~CbmBuildEventsFromTracksReal();

  /** Task execution **/
  virtual void Exec(Option_t* opt);

 private:
  struct SortTracks {
    CbmStsTrack Track;
    int index;
    bool used;

    SortTracks() : Track(), index(-1), used(false) {}
  };

  static bool CompareTrackTime(const SortTracks& a, const SortTracks& b)
  {
    return (a.Track.GetStartTime() < b.Track.GetStartTime());
  }

  TClonesArray* fStsTracks;  ///< Input array (class CbmStsDigi)
  TClonesArray* fEvents;     ///< Output array (class CbmEvent)

  /** Task initialisation **/
  virtual InitStatus Init();


  ClassDef(CbmBuildEventsFromTracksReal, 1);
};

#endif /* CBMBUILDEVENTSFROMTRACKS_H */
