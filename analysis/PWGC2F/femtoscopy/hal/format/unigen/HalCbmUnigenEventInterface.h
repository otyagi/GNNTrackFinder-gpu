/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICAUNIGENEVENTINTERFACE_H_
#define NICAUNIGENEVENTINTERFACE_H_

#include "HalCbmUnigenEvent.h"

#include <TObject.h>

#include <Hal/EventInterfaceAdvanced.h>

/**
 * event interface for UnigenData
 */


namespace Hal
{
  class TrackInterface;
}

class HalCbmUnigenEventInterface : public Hal::EventInterfaceAdvanced {
  friend class HalCbmUnigenEvent;
  UEvent* fEvent;

 protected:
  virtual void ConnectToTreeInternal(EventInterface::eMode mode);
#ifdef UNIGEN_OLD
  void CopyUnigen(UEvent* from, UEvent* to);
#endif
 public:
  HalCbmUnigenEventInterface();
  virtual Int_t GetTotalTrackNo() const { return fEvent->GetNpa(); }
  virtual void Boost(Double_t vx, Double_t vy, Double_t vz);
  virtual void Register(Bool_t write);
  virtual void Compress(Int_t* map, Int_t map_size);
  virtual void CopyData(Hal::EventInterface* s);
  virtual void CopyAndCompress(Hal::EventInterface* s, Int_t* map, Int_t map_size);
  virtual void FillTrackInterface(Hal::TrackInterface* track, Int_t index);
  TObject* GetRawEventPointer() const { return fEvent; };
  virtual TObject* GetRawTrackPointer(Int_t index) const { return fEvent->GetParticle(index); };
  virtual Hal::TrackInterface* GetTrackInterface() const;
  virtual void Clear(Option_t* opt = "") { fEvent->Clear(opt); };
  /** GETTER SETTER SECTION*/
  virtual void SetB(Double_t b) { fEvent->SetB(b); };
  virtual void SetVertex(Double_t /*x*/, Double_t /*y*/, Double_t /*z*/){};
  virtual void SetPhi(Double_t phi, Double_t /*phi_error*/) { fEvent->SetPhi(phi); };
  virtual Double_t GetB() const { return fEvent->GetB(); };
  virtual Double_t GetPhi() const { return fEvent->GetPhi(); };
  virtual ~HalCbmUnigenEventInterface();
  ClassDef(HalCbmUnigenEventInterface, 1)
};

#endif /* NICAUNIGENEVENTINTERFACE_H_ */
