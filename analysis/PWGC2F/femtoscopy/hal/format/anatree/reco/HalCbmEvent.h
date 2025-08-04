/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMDSTEVENT_H_
#define NICACBMDSTEVENT_H_

#include "HalCbmTrack.h"

#include <Rtypes.h>
#include <RtypesCore.h>
#include <TString.h>

#include <Hal/ExpEvent.h>
namespace Hal
{
  class Event;
  class EventInterface;
}  // namespace Hal
class HalCbmEventInterface;
class HalCbmEvent : public Hal::ExpEvent {
 protected:
  HalCbmEvent(TString classname);
  void UpdateDST(HalCbmEventInterface* ei);
  void UpdateAnaTree(HalCbmEventInterface* ei);

 public:
  HalCbmEvent();
  HalCbmEvent(const HalCbmEvent& other);
  virtual void Update(Hal::EventInterface* interface);
  virtual Bool_t ExistInTree() const;
  virtual Hal::EventInterface* CreateInterface() const;
  virtual Hal::Track* GetNewTrack() const { return new HalCbmTrack(); };
  virtual Hal::Event* GetNewEvent() const { return new HalCbmEvent(); };
  virtual TString GetFormatName() const { return "HalCbmEvent"; };
  virtual ~HalCbmEvent();
  ClassDef(HalCbmEvent, 1)
};
#endif /* NICACBMDSTEVENT_H_ */
