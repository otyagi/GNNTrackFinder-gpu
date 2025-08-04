/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATRECOEVENTINTERFACE_H_
#define NICACBMATRECOEVENTINTERFACE_H_

#include "CbmAnaTreeContainer.h"
#include "HalCbmDetectorID.h"
#include "HalCbmTrackInterface.h"

#include <Rtypes.h>
#include <RtypesCore.h>
#include <TVector3.h>

#include <Hal/ExpEventInterface.h>
#include <Hal/TrackClones.h>

namespace Hal
{
  class EventInterface;
}

class HalCbmEvent;
class CbmVertex;
class HalCbmFullEvent;
class HalCbmMCEventInterface;
class HalCbmEventInterface : public Hal::EventInterface {
  friend class HalCbmEvent;
  friend class HalCbmFullEvent;

 protected:
  HalCbm::DataFormat fFormatType                = {HalCbm::DataFormat::kUnknown};
  CbmAnaTreeRecoSourceContainer* fDataContainer = {nullptr};
  TClonesArray* fGlobalTracks                   = {nullptr};
  TClonesArray* fStsTracks                      = {nullptr};
  TClonesArray* fTrdTracks                      = {nullptr};
  TClonesArray* fTofHits                        = {nullptr};
  TClonesArray* fRichRings                      = {nullptr};
  TClonesArray* fMuchTracks                     = {nullptr};
  CbmVertex* fCbmVertex                         = {nullptr};
  void UpdateDst(HalCbmMCEventInterface* ie);
  void UpdateAnaTree(HalCbmMCEventInterface* ie);
  virtual void ConnectToTreeInternal(eMode mode);
  void Register(Bool_t write);

 public:
  HalCbmEventInterface();
  HalCbm::DataFormat GetFormatType() const { return fFormatType; };
  virtual Int_t GetTotalTrackNo() const;
  virtual Hal::TrackInterface* GetTrackInterface() const { return new HalCbmTrackInterface(); }
  CbmAnaTreeRecoSourceContainer* GetContainer() const { return fDataContainer; }
  virtual TObject* GetRawTrackPointer(Int_t index) const;
  /** GETTERS AND SETTERS **/
  virtual void SetRunInfoId(Int_t /*i*/){};
  virtual Int_t GetRunInfoId() const { return 0; };
  virtual TLorentzVector GetVertex() const;
  virtual ~HalCbmEventInterface();
  ClassDef(HalCbmEventInterface, 1)
};

#endif /* NICACBMATRECOEVENTINTERFACE_H_ */
