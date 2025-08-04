/* Copyright (C) 2023 Facility for AntiProton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

#ifndef BMON_H
#define BMON_H

#include "FairDetector.h"
#include "FairModule.h"
#include "FairRootManager.h"

#include "TClonesArray.h"

#include <map>
#include <string>

class FairVolume;
class TGeoNode;

class TGeoCombiTrans;


//class CbmBmon : public FairDetector {

class CbmBmon : public FairModule {
public:
  CbmBmon();
  CbmBmon(const char* name = "BMON", const char* Title = "CBM BMON");
  CbmBmon(const CbmBmon&) = delete;
  CbmBmon& operator=(const CbmBmon&) = delete;
  virtual ~CbmBmon();

  virtual void ConstructGeometry();

  virtual void ConstructRootGeometry(TGeoMatrix* shift = nullptr);
  //virtual void ConstructGeometry(TGeoMatrix* shift = NULL);

private:
  TGeoCombiTrans* fCombiTrans;  // Transformation matrix for geometry positioning
  std::string fVolumeName;

  ClassDef(CbmBmon, 1)  //CbmBmon
};

#endif  //BMON_H
