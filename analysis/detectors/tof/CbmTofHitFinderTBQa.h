/* Copyright (C) 2017 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Timur Ablyazimov, Pierre-Alain Loizeau [committer] */

#ifndef CBMTOFHITFINDERTBQA_H
#define CBMTOFHITFINDERTBQA_H

#include "CbmMCDataArray.h"
#include "CbmMCEventList.h"
#include "CbmTimeSlice.h"

#include "FairTask.h"

#include "TClonesArray.h"

class CbmTofHitFinderTBQa : public FairTask {
public:
  CbmTofHitFinderTBQa();
  CbmTofHitFinderTBQa(const CbmTofHitFinderTBQa&) = delete;
  CbmTofHitFinderTBQa& operator=(const CbmTofHitFinderTBQa&) = delete;

  InitStatus Init();
  void Exec(Option_t* option);
  void Finish();
  void SetIsEvByEv(bool v) { isEvByEv = v; }

private:
  bool isEvByEv;
  TClonesArray* fTofHits;
  TClonesArray* fTofDigiMatchs;
  TClonesArray* fTofDigis;
  TClonesArray* fTofDigiPointMatchs;
  CbmMCDataArray* fTofMCPoints;
  CbmMCDataArray* fMCTracks;
  CbmTimeSlice* fTimeSlice;
  CbmMCEventList* fEventList;

  ClassDef(CbmTofHitFinderTBQa, 1)
};

#endif /* CBMTOFHITFINDERTBQA_H */
