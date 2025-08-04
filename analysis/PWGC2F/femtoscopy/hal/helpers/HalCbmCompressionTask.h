/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMNICACOMPRESSION_H_
#define CBMNICACOMPRESSION_H_

#include "FairTask.h"

#include <TClonesArray.h>

#include <Hal/Array.h>
#include <Hal/TrackClones.h>


class HalCbmCompressionTask : public FairTask {
  Hal::TrackClones* fStsMatches;
  Hal::TrackClones* fTofMatches;
  Hal::TrackClones* fMCTracks;
  TClonesArray* fStsLinks;
  TClonesArray* fTofLinks;
  Hal::Array_1<Int_t> fMapUse;
  Hal::Array_1<Int_t> fMapIndex;
  Bool_t fAllDep;
  void NoDep();
  void WithDep();

 public:
  HalCbmCompressionTask();
  InitStatus Init();
  void AllDependencies() { fAllDep = kTRUE; };
  void Exec(Option_t* opt);
  virtual ~HalCbmCompressionTask();
  ClassDef(HalCbmCompressionTask, 1)
};

#endif /* CBMNICACOMPRESSION_H_ */
