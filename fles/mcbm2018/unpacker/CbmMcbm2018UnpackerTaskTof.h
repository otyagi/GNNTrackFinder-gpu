/* Copyright (C) 2019-2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

// -----------------------------------------------------------------------------
// -----                                                                   -----
// -----                     CbmMcbm2018UnpackerTaskTof                    -----
// -----               Created 10.02.2019 by P.-A. Loizeau                 -----
// -----                                                                   -----
// -----------------------------------------------------------------------------

#ifndef CbmMcbm2018UnpackerTaskTof_H
#define CbmMcbm2018UnpackerTaskTof_H

#include "CbmMcbmUnpack.h"
#include "CbmTofDigi.h"

#include "Timeslice.hpp"

class CbmMcbm2018UnpackerAlgoTof;
//class TClonesArray;
class TH1;

class CbmMcbm2018UnpackerTaskTof : public CbmMcbmUnpack {
public:
  CbmMcbm2018UnpackerTaskTof(UInt_t uNbSdpb = 1);

  CbmMcbm2018UnpackerTaskTof(const CbmMcbm2018UnpackerTaskTof&) = delete;
  CbmMcbm2018UnpackerTaskTof operator=(const CbmMcbm2018UnpackerTaskTof&) = delete;

  virtual ~CbmMcbm2018UnpackerTaskTof();

  virtual Bool_t Init();
  virtual Bool_t DoUnpack(const fles::Timeslice& ts, size_t component);
  virtual void Reset();

  virtual void Finish();

  void SetParContainers();

  Bool_t InitContainers();

  Bool_t ReInitContainers();

  /// Temp until we change from CbmMcbmUnpack to something else
  void AddMsComponentToList(size_t component, UShort_t usDetectorId);
  void SetNbMsInTs(size_t /*uCoreMsNb*/, size_t /*uOverlapMsNb*/) {};

  /// Algo settings setters
  inline void SetMonitorMode(Bool_t bFlagIn = kTRUE) { fbMonitorMode = bFlagIn; }
  void SetIgnoreOverlapMs(Bool_t bFlagIn = kTRUE);
  void SetTimeOffsetNs(Double_t dOffsetIn = 0.0);
  void SetDiamondDpbIdx(UInt_t uIdx = 2);

  /// Task settings
  void SetSeparateArrayBmon(Bool_t bFlagIn = kTRUE) { fbSeparateArrayBmon = bFlagIn; }
  void SetDigiMaskAndIdBmon(UInt_t uMask, UInt_t uMaskedIdBmon)
  {
    fuDigiMaskId     = uMask;
    fuDigiMaskedIdBmon = uMaskedIdBmon;
  }
  void SetWriteOutputFlag(Bool_t bFlagIn) { fbWriteOutput = bFlagIn; }

private:
  /// Control flags
  Bool_t fbMonitorMode;       //! Switch ON the filling of a minimal set of histograms
  Bool_t fbDebugMonitorMode;  //! Switch ON the filling of a additional set of histograms
  Bool_t fbSeparateArrayBmon;  //! If ON, Bmon digis are saved in separate TClonesArray
  Bool_t fbWriteOutput;       //! If ON the output TClonesArray of digi is written to disk

  /// Parameters
  UInt_t fuDigiMaskedIdBmon;
  UInt_t fuDigiMaskId;

  /// Statistics & first TS rejection
  uint64_t fulTsCounter;

  /// Output vectors
  std::vector<CbmTofDigi>* fpvDigiTof = nullptr;
  std::vector<CbmTofDigi>* fpvDigiBmon = nullptr;

  /// Histograms
  TH1* fhArraySize     = nullptr;
  TH1* fhArrayCapacity = nullptr;

  /// Processing algo
  CbmMcbm2018UnpackerAlgoTof* fUnpackerAlgo;

  ClassDef(CbmMcbm2018UnpackerTaskTof, 2)
};

#endif
