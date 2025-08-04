/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef UNIGENTRACKINTERFACE_H_
#define UNIGENTRACKINTERFACE_H_

#include "UParticle.h"

#include <Hal/McTrackInterface.h>
/**
 * interface to UParticle
 */

class HalCbmUnigenTrackInterface : public Hal::McTrackInterface {
  TObject* fRawObject;

 public:
  HalCbmUnigenTrackInterface();
  void SetID(Int_t id) { ((UParticle*) fRawObject)->SetUniqueID(id); };
  void SetPxPyPzE(Double_t px, Double_t py, Double_t pz, Double_t e)
  {
    ((UParticle*) fRawObject)->SetMomentum(px, py, pz, e);
  };
  void SetStatus(Int_t stat) { ((UParticle*) fRawObject)->SetStatus(stat); };
  void SetPrimary(Int_t /*prim*/){};
  Double_t GetPx() const { return ((UParticle*) fRawObject)->Px(); };
  Double_t GetPy() const { return ((UParticle*) fRawObject)->Py(); };
  Double_t GetPz() const { return ((UParticle*) fRawObject)->Pz(); };
  Double_t GetE() const { return ((UParticle*) fRawObject)->E(); };
  Int_t GetMotherIndex() const { return ((UParticle*) fRawObject)->GetMate(); };
  Int_t GetStatus() const { return ((UParticle*) fRawObject)->GetStatus(); };
  Bool_t IsPrimary() const { return kTRUE; };
  virtual ~HalCbmUnigenTrackInterface();
  ClassDef(HalCbmUnigenTrackInterface, 1)
};

#endif /* ROOT_DATAFORMAT_FORMATS_NICAUNIGENTRACKINTERFACE_H_ */
