/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATMCEVENTINTERFACE_H_
#define NICACBMATMCEVENTINTERFACE_H_

#include "CbmAnaTreeContainer.h"
#include "HalCbmDetectorID.h"
#include "HalCbmMCTrackInterface.h"

#include <TVector3.h>

#include <Hal/EventInterface.h>

class TClonesArray;
class FairMCEventHeader;
class CbmAnaTreeMcSourceContainer;
class HalCbmFullEvent;

class HalCbmMCEventInterface : public Hal::EventInterface {
  friend class HalCbmMCEvent;
  friend class HalCbmFullEvent;

 protected:
  HalCbm::DataFormat fFormatType              = {HalCbm::DataFormat::kUnknown};
  CbmAnaTreeMcSourceContainer* fDataContainer = {nullptr};
  FairMCEventHeader* fEventHeader             = {nullptr};
  TClonesArray* fCbmMCtracks                  = {nullptr};
  TClonesArray* fStsMatches                   = {nullptr};
  TClonesArray* fTofMatches                   = {nullptr};
  TClonesArray* fTrdMatches                   = {nullptr};
  TClonesArray* fRichMatches                  = {nullptr};
  TClonesArray* fMuchMatches                  = {nullptr};
  virtual void ConnectToTreeInternal(eMode mode);

  void Register(Bool_t write);

 public:
  HalCbmMCEventInterface();
  virtual void FillTrackInterface(Hal::TrackInterface* track, Int_t index);
  virtual Int_t GetTotalTrackNo() const;
  virtual TObject* GetRawEventPointer() const { return fDataContainer; };
  virtual Hal::TrackInterface* GetTrackInterface() const { return new HalCbmMCTrackInterface(); }
  virtual TObject* GetRawTrackPointer(Int_t index) const;
  /** GETTERS AND SETTERS **/
  virtual void SetRunInfoId(Int_t /*i*/){};
  virtual Int_t GetRunInfoId() const { return 0; };
  virtual void SetMagneticField(TVector3 /*mag*/) const {};
  virtual TVector3 GetMagneticField() const { return TVector3(0, 0, 0); };
  virtual TLorentzVector GetVertexError() const;
  virtual TLorentzVector GetVertex() const;
  virtual ~HalCbmMCEventInterface();
  ClassDef(HalCbmMCEventInterface, 1)
};
#endif /* NICACBMATMCEVENTINTERFACE_H_ */
