/* Copyright (C) 2017 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer] */

// -------------------------------------------------------------------------
// -----                      CbmMvdMimosis header file                -----
// -----                                                               -----
// -------------------------------------------------------------------------


/** CbmMvdMimosis.h
 **
 ** Data base for the Properties of MIMOSIS
 **   
 **/


#ifndef CBMMVDMIMOSIS_H
#define CBMMVDMIMOSIS_H 1

#include "CbmMvdSensorDataSheet.h"  // for CbmMvdSensorDataSheet

#include <Rtypes.h>  // for ClassDef
#include <TRandom.h>

class TBuffer;
class TClass;
class TMemberInspector;

class CbmMvdMimosis : public CbmMvdSensorDataSheet {

public:
  /** Default constructor **/
  CbmMvdMimosis();

  Double_t ComputeExpireTime(Double_t hitMCTime, Double_t deadTime, Double_t endOfFrameTime);

  Double_t ComputeHitDeadTime(Float_t charge);
  Double_t ComputeHitDelay(Float_t charge);
  Double_t ComputeHitJitter(Float_t charge);
  Double_t GetJitter(Float_t charge);
  Double_t GetDelay(Float_t charge);
  Double_t GetDelaySigma(Float_t charge);

  TRandom* fRandom = gRandom;

  /** Destructor **/
  ~CbmMvdMimosis();


  ClassDef(CbmMvdMimosis, 1);
};


#endif
