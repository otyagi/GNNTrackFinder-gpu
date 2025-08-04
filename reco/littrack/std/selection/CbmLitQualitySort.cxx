/* Copyright (C) 2011-2013 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer] */

#include "selection/CbmLitQualitySort.h"

#include "data/CbmLitTrack.h"

#include <algorithm>

CbmLitQualitySort::CbmLitQualitySort() {}

CbmLitQualitySort::~CbmLitQualitySort() {}

//LitStatus CbmLitQualitySort::DoSort(
//   TrackPtrIterator itBegin,
//   TrackPtrIterator itEnd)
//{
//   if (itBegin == itEnd) { return kLITSUCCESS; }
//
//   SortNofHits(itBegin, itEnd);
//   //SortLastPlaneId(itBegin, itEnd);
//
//   return kLITSUCCESS;
//}
//
//LitStatus CbmLitQualitySort::DoSort(
//   TrackPtrVector& tracks)
//{
//   return DoSort(tracks.begin(), tracks.end());
//}

LitStatus CbmLitQualitySort::DoSortNofHits(TrackPtrIterator itBegin, TrackPtrIterator itEnd)
{
  std::sort(itBegin, itEnd, [](const CbmLitTrack* track1, const CbmLitTrack* track2) {
    return track1->GetNofHits() > track2->GetNofHits();
  });

  Int_t maxNofHits = (*itBegin)->GetNofHits();
  Int_t minNofHits = (*(itEnd - 1))->GetNofHits();

  for (Int_t iNofHits = minNofHits; iNofHits <= maxNofHits; iNofHits++) {
    CbmLitTrack value;
    value.SetNofHits(iNofHits);

    std::pair<TrackPtrIterator, TrackPtrIterator> bounds;
    bounds = std::equal_range(itBegin, itEnd, &value, [](const CbmLitTrack* track1, const CbmLitTrack* track2) {
      return track1->GetNofHits() > track2->GetNofHits();
    });

    if (bounds.first == bounds.second) {
      continue;
    }

    std::sort(bounds.first, bounds.second, [](const CbmLitTrack* track1, const CbmLitTrack* track2) {
      return ((track1->GetChi2() / track1->GetNDF()) < (track2->GetChi2() / track2->GetNDF()));
    });
  }
  return kLITSUCCESS;
}

LitStatus CbmLitQualitySort::DoSortLastStation(TrackPtrIterator itBegin, TrackPtrIterator itEnd)
{
  std::sort(itBegin, itEnd, [](const CbmLitTrack* track1, const CbmLitTrack* track2) {
    return track1->GetLastStationId() > track2->GetLastStationId();
  });

  Int_t maxPlaneId = (*itBegin)->GetLastStationId();
  Int_t minPlaneId = (*(itEnd - 1))->GetLastStationId();

  for (Int_t iPlaneId = minPlaneId; iPlaneId <= maxPlaneId; iPlaneId++) {
    CbmLitTrack value;
    value.SetLastStationId(iPlaneId);

    std::pair<TrackPtrIterator, TrackPtrIterator> bounds;
    bounds = std::equal_range(itBegin, itEnd, &value, [](const CbmLitTrack* track1, const CbmLitTrack* track2) {
      return track1->GetLastStationId() > track2->GetLastStationId();
    });

    if (bounds.first == bounds.second) {
      continue;
    }

    std::sort(bounds.first, bounds.second, [](const CbmLitTrack* track1, const CbmLitTrack* track2) {
      return ((track1->GetChi2() / track1->GetNDF()) < (track2->GetChi2() / track2->GetNDF()));
    });
  }
  return kLITSUCCESS;
}

LitStatus CbmLitQualitySort::DoSortChiSqOverNDF(TrackPtrIterator itBegin, TrackPtrIterator itEnd)
{
  std::sort(itBegin, itEnd, [](const CbmLitTrack* track1, const CbmLitTrack* track2) {
    return ((track1->GetChi2() / track1->GetNDF()) < (track2->GetChi2() / track2->GetNDF()));
  });
  return kLITSUCCESS;
}
