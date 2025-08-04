/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], P.-A. Loizeau */
#include "StorableRecoResults.h"

using namespace cbm::algo;

size_t StorableRecoResults::SizeBytes() const
{
  size_t size = 0;
  size += fBmonDigis.size() * sizeof(CbmBmonDigi);
  size += fStsDigis.size() * sizeof(CbmStsDigi);
  size += fMuchDigis.size() * sizeof(CbmMuchDigi);
  size += fTrd2dDigis.size() * sizeof(CbmTrdDigi);
  size += fTrdDigis.size() * sizeof(CbmTrdDigi);
  size += fTofDigis.size() * sizeof(CbmTofDigi);
  size += fRichDigis.size() * sizeof(CbmRichDigi);

  for (const auto& ev : fDigiEvents) {
    size += ev.fData.SizeBytes();
  }

  size += fStsClusters.SizeBytes();
  size += fStsHits.SizeBytes();
  size += fTofHits.SizeBytes();
  size += fTrdHits.SizeBytes();

  size += fTracks.size() * sizeof(ca::Track);

  // Exclude TrackHitIndexContainers for now to avoid looping over all tracks
  // Better way to do this: Just query from boost the size of the written archive.
  // Requires changes in flesnet to the archive classes for this

  return size;
}
