/* Copyright (C) 2010-2016 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

/**
 * \file LitTrackSelection.h
 *
 * \brief Implementation of the track selection algorithms.
 *
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2010
 *
 */
#ifndef LITTRACKSELECTION_H_
#define LITTRACKSELECTION_H_

#include "LitScalTrack.h"

#include <algorithm>
#include <functional>
#include <set>
#include <utility>
#include <vector>

using std::equal_range;
using std::pair;
using std::set;
using std::sort;

namespace lit
{
  namespace parallel
  {


    void DoSortNofHits(vector<LitScalTrack*>& tracks)
    {
      sort(tracks.begin(), tracks.end(), [](const LitScalTrack* track1, const LitScalTrack* track2) {
        return track1->GetNofHits() > track2->GetNofHits();
      });

      unsigned int maxNofHits = tracks.front()->GetNofHits();
      unsigned int minNofHits = tracks.back()->GetNofHits();

      for (unsigned int iNofHits = minNofHits; iNofHits <= maxNofHits; iNofHits++) {
        LitScalTrack value;
        value.SetNofHits(iNofHits);

        pair<vector<LitScalTrack*>::iterator, vector<LitScalTrack*>::iterator> bounds;
        bounds =
          equal_range(tracks.begin(), tracks.end(), &value, [](const LitScalTrack* track1, const LitScalTrack* track2) {
            return track1->GetNofHits() > track2->GetNofHits();
          });

        if (bounds.first == bounds.second) {
          continue;
        }

        sort(bounds.first, bounds.second, [](const LitScalTrack* track1, const LitScalTrack* track2) {
          return ((track1->GetChiSq() / track1->GetNDF()) < (track2->GetChiSq() / track2->GetNDF()));
        });
      }
    }

    void DoSortLastStation(vector<LitScalTrack*>& tracks)
    {
      sort(tracks.begin(), tracks.end(), [](const LitScalTrack* track1, const LitScalTrack* track2) {
        return track1->GetLastStationId() > track2->GetLastStationId();
      });

      unsigned int maxPlaneId = tracks.front()->GetLastStationId();
      unsigned int minPlaneId = tracks.back()->GetLastStationId();

      for (unsigned int iPlaneId = minPlaneId; iPlaneId <= maxPlaneId; iPlaneId++) {
        LitScalTrack value;
        value.SetLastStationId(iPlaneId);

        pair<vector<LitScalTrack*>::iterator, vector<LitScalTrack*>::iterator> bounds;
        bounds =
          equal_range(tracks.begin(), tracks.end(), &value, [](const LitScalTrack* track1, const LitScalTrack* track2) {
            return track1->GetLastStationId() > track2->GetLastStationId();
          });

        if (bounds.first == bounds.second) {
          continue;
        }

        sort(bounds.first, bounds.second, [](const LitScalTrack* track1, const LitScalTrack* track2) {
          return ((track1->GetChiSq() / track1->GetNDF()) < (track2->GetChiSq() / track2->GetNDF()));
        });
      }
    }

    void DoSortChiSqOverNDF(vector<LitScalTrack*>& tracks)
    {
      sort(tracks.begin(), tracks.end(), [](const LitScalTrack* track1, const LitScalTrack* track2) {
        return ((track1->GetChiSq() / track1->GetNDF()) < (track2->GetChiSq() / track2->GetNDF()));
      });
    }


    void DoSelectSharedHits(vector<LitScalTrack*>& tracks)
    {
      static const int NOF_SHARED_HITS = 3;
      if (tracks.empty()) return;

      DoSortNofHits(tracks);

      set<int> hitsId;
      //   const int STRIPSTART = 100000;
      //   const int TRDSTART = 1000000;

      unsigned int nofTracks = tracks.size();
      for (unsigned int iTrack = 0; iTrack < nofTracks; iTrack++) {
        LitScalTrack* track = tracks[iTrack];

        if (!track->IsGood()) {
          continue;
        }

        int nofSharedHits = 0;
        int nofHits       = track->GetNofHits();
        for (int iHit = 0; iHit < nofHits; iHit++) {
          int hitId = track->GetHit(iHit)->refId;
          // LitHitType type = track->GetHit(iHit)->GetType();
          // LitSystemId sysId = track->GetHit(iHit)->GetSystem();
          // if (type == kLITSTRIPHIT) { hitId += STRIPSTART; }
          // if (sysId == kLITTRD) { hitId += TRDSTART; }
          if (hitsId.find(hitId) != hitsId.end()) {
            nofSharedHits++;
            if (nofSharedHits > NOF_SHARED_HITS) {
              track->IsGood(false);
              break;
            }
          }
        }

        if (!track->IsGood()) {
          continue;
        }

        for (int iHit = 0; iHit < nofHits; iHit++) {
          int hitId = track->GetHit(iHit)->refId;
          //LitHitType type = track->GetHit(iHit)->GetType();
          //LitSystemId detId = track->GetHit(iHit)->GetSystem();
          // if (type == kLITSTRIPHIT) { hitId += STRIPSTART; }
          // if (detId == kLITTRD) { hitId += TRDSTART; }
          hitsId.insert(hitId);
        }
      }
      hitsId.clear();
    }

  }  // namespace parallel
}  // namespace lit
#endif /* LITTRACKSELECTION_H_ */
