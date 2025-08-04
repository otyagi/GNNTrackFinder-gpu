/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICAUNIGENSOURCE_H_
#define NICAUNIGENSOURCE_H_
/**
 *class  only for reading pure unigen files
 */


#include "UEvent.h"

#include <FairFileSource.h>

#include <TChain.h>

class CbmUnigenSource : public FairSource {
  TChain* fUnigenChain;
  std::vector<TString> fFileName;
  UEvent* fEvent;
  Bool_t fPrintTreeInfo;

 public:
  /**
	 * defaut constructor should not be used
	 */
  CbmUnigenSource();
  /**
	 * main constructor
	 * @param inFile unigen file
	 */
  CbmUnigenSource(TString inFile);
  /**
	 * copy constructor
	 * @param source
	 */
  CbmUnigenSource(const CbmUnigenSource& source) = delete;
  void PrintTreeInfo() { fPrintTreeInfo = kTRUE; };
  void AddFile(TString name) { fFileName.push_back(name); };
  CbmUnigenSource& operator=(const CbmUnigenSource&) = delete;
  virtual void Boost(Double_t vx, Double_t vy, Double_t vz);
  virtual ~CbmUnigenSource();
  virtual Bool_t Init();
  virtual Int_t ReadEvent(UInt_t = 0);
  virtual void Close();
  virtual void Reset(){};
  virtual Bool_t ActivateObject(TObject**, const char*) { return kFALSE; }
  virtual Source_Type GetSourceType() { return kFILE; };
  virtual void SetParUnpackers(){};
  virtual Bool_t InitUnpackers() { return kTRUE; };
  virtual Bool_t ReInitUnpackers() { return kTRUE; };
  virtual Int_t CheckMaxEventNo(Int_t = 0);
  virtual void ReadBranchEvent(const char* /*BrName*/){};
  virtual void ReadBranchEvent(const char* /*BrName*/, Int_t /*Event*/){};
  virtual void FillEventHeader(FairEventHeader* /*feh*/){};
  virtual Bool_t SpecifyRunId();
  void SetRunId(Int_t runId) { fRunId = runId; }
  Int_t GetRunId() const { return fRunId; }
  ClassDef(CbmUnigenSource, 1)
};

#endif /* NICAUNIGENSOURCE_H_ */
