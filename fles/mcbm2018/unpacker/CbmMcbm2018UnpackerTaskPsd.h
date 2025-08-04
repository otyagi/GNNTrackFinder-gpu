/* Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Nikolay Karpushkin, David Emschermann [committer] */

// -----------------------------------------------------------------------------
// -----                                                                   -----
// -----                     CbmMcbm2018UnpackerTaskPsd                    -----
// -----                 Created 09.10.2019 by N.Karpushkin                -----
// -----        based on CbmMcbm2018UnpackerTaskTof by P.-A. Loizeau       -----
// -----                                                                   -----
// -----------------------------------------------------------------------------

#ifndef CbmMcbm2018UnpackerTaskPsd_H
#define CbmMcbm2018UnpackerTaskPsd_H

#include "CbmMcbmUnpack.h"
#include "CbmPsdDigi.h"
#include "CbmPsdDsp.h"

#include "Timeslice.hpp"

class CbmMcbm2018UnpackerAlgoPsd;
//class TClonesArray;
class TH1;

class CbmMcbm2018UnpackerTaskPsd : public CbmMcbmUnpack {
public:
  CbmMcbm2018UnpackerTaskPsd(UInt_t uNbSdpb = 1);

  /** Copy Constructor */
  CbmMcbm2018UnpackerTaskPsd(const CbmMcbm2018UnpackerTaskPsd&);
  /** Assignment Operator */
  CbmMcbm2018UnpackerTaskPsd operator=(const CbmMcbm2018UnpackerTaskPsd&);

  virtual ~CbmMcbm2018UnpackerTaskPsd();

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
  void SetDspWriteMode(Bool_t bFlagIn = kTRUE);
  void SetIgnoreOverlapMs(Bool_t bFlagIn = kTRUE);
  void SetTimeOffsetNs(Double_t dOffsetIn = 0.0);

  /// Task settings
  void SetWriteOutputFlag(Bool_t bFlagIn) { fbWriteOutput = bFlagIn; }

private:
  /// Control flags
  Bool_t fbMonitorMode;       //! Switch ON the filling of a minimal set of histograms
  Bool_t fbDebugMonitorMode;  //! Switch ON the filling of a additional set of histograms
  Bool_t fbWriteOutput;       //! If ON the output vector of digi is written to disk
  Bool_t fbDebugWriteOutput;  //! If ON the output vector of dsp debug information is written to disk

  /// Parameters


  /// Statistics & first TS rejection
  uint64_t fulTsCounter;

  /// Output Digi vector
  std::vector<CbmPsdDigi>* fPsdDigiVector;

  /// Output Dsp vector -- preprocessed info for debugging
  std::vector<CbmPsdDsp>* fPsdDspVector;

  /// Processing algo
  CbmMcbm2018UnpackerAlgoPsd* fUnpackerAlgo;

  ClassDef(CbmMcbm2018UnpackerTaskPsd, 3)
};

#endif
