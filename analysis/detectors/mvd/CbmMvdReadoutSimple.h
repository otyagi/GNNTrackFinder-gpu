/* Copyright (C) 2016-2017 Institut furr Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer] */

// -------------------------------------------------------------------------
// -----              CbmMvdReadoutSimple  header file                            -----
// -----              Created 17/10/16  by P. Sitzmann                 -----
// -------------------------------------------------------------------------


/**  CbmMvdReadoutSimple.h
 *@author P.Sitzmann <p.sitzmann@gsi.de>
 *
 *  Simple Readout simulations for the mvd
 *
 **/

#ifndef CBMMVDREADOUTSIMPLE_H
#define CBMMVDREADOUTSIMPLE_H 1

#include <FairTask.h>  // for FairTask, InitStatus

#include <Rtypes.h>      // for ClassDef
#include <RtypesCore.h>  // for Int_t, Bool_t, Option_t, kTRUE

class TBuffer;
class TClass;
class TClonesArray;
class TFile;
class TH1F;
class TH1I;
class TH2F;
class TH2I;
class TMemberInspector;

class CbmMvdReadoutSimple : public FairTask {
public:
  CbmMvdReadoutSimple();
  CbmMvdReadoutSimple(const char* name, Int_t iVerbose = 0);
  CbmMvdReadoutSimple(const CbmMvdReadoutSimple&) = delete;
  CbmMvdReadoutSimple& operator=(const CbmMvdReadoutSimple&) = delete;

  ~CbmMvdReadoutSimple();

  InitStatus Init();

  void Exec(Option_t* opt);

  void ShowHistograms() { fshow = kTRUE; };

  void SetHistogramFile(TFile* file) { foutFile = file; };

  void Finish();

private:
  TFile* foutFile;

  Bool_t fshow;
  TH2F* fMvdMCBank[63];
  TH2F* fMvdMCHitsStations[4];
  TH1F* fWordsPerRegion;
  TH2F* fWordsPerRegion2;
  TH1F* fWordsPerWorstRegion;
  TH1F* fWordsPerSuperRegion;
  TH1F* fWorstSuperPerEvent;
  TH2I* fMvdBankDist;
  TH2F* fMvdMCWorst;
  TH2F* fMvdMCWorstDelta;
  TH1I* fMvdDataLoadPerSensor;
  TH1I* fMvdDataLoadHotSensor;
  TH1F* fMvdDataPerRegion[64];
  TH1F* fMvdDataPerSuperRegion[16];

  TClonesArray* fMcPoints;
  TClonesArray* fListMCTracks;

  Int_t fEventNumber;

  void DrawHistograms();
  void WriteHistograms();
  void SetupHistograms();

  ClassDef(CbmMvdReadoutSimple, 1);
};
#endif
