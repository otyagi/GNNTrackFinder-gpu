/* Copyright (C) 2015-2020 Justus-Liebig-Universitaet Giessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Julian Book [committer], Etienne Bechtel */

#ifndef PAIRANALYSISCUTQA_H
#define PAIRANALYSISCUTQA_H
//#################################################################
//#                                                               #
//#             Class PairAnalysisCutQa                             #
//#              PairAnalysis Group of cuts                         #
//#                                                               #
//#  Authors:                                                     #
//#   Julian    Book,     Uni Ffm / Julian.Book@cern.ch           #
//#                                                               #
//#################################################################

#include <TH1I.h>
#include <TH2I.h>
#include <THashList.h>
#include <TList.h>
#include <TNamed.h>
#include <TProfile2D.h>

#include "AnalysisFilter.h"

class TCollection;

class PairAnalysisCutQa : public TNamed {

public:
  enum class ETypes
  {
    kEvent = 0,
    kTrack,
    kTrack2,
    kTrackMC,
    kPair,
    kPrePair,
    kNtypes
  };

  PairAnalysisCutQa();
  PairAnalysisCutQa(const char* name, const char* title);

  virtual ~PairAnalysisCutQa();

  void Init();
  void AddTrackFilterMC(AnalysisFilter* trkFilterMC);
  void AddTrackFilter(AnalysisFilter* trkFilter);
  void AddPrePairFilter(AnalysisFilter* pairFilter);
  void AddTrackFilter2(AnalysisFilter* trkFilter2);
  void AddPairFilter(AnalysisFilter* pairFilter);
  void AddEventFilter(AnalysisFilter* eventFilter);

  void Fill(UInt_t mask, TObject* obj, UInt_t addIdx = 0);
  void FillAll(TObject* obj, UInt_t addIdx = 0);  // { fCutQA->Fill(0); }

  const THashList* GetQAHistList() const { return &fQAHistList; }


private:
  THashList fQAHistList;  //-> list of QA histograms
  static constexpr Int_t fNtypes = static_cast<Int_t>(ETypes::kNtypes);
  Int_t fNCuts[fNtypes];               // number of cuts
  const char* fCutNames[20][fNtypes];  // cut names
  const char* fTypeKeys[fNtypes];      // type names


  UInt_t GetObjIndex(TObject* obj);  // return object index

  PairAnalysisCutQa(const PairAnalysisCutQa&);
  PairAnalysisCutQa& operator=(const PairAnalysisCutQa&);

  ClassDef(PairAnalysisCutQa, 3)  // Simple automatic cut QA
};

#endif
