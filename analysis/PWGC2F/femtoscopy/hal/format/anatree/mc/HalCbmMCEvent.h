/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATMCEVENT_H_
#define NICACBMATMCEVENT_H_

#include <Hal/McEvent.h>
namespace Hal
{
  class EventInterface;
}
class HalCbmMCEvent : public Hal::McEvent {

 protected:
  virtual void ShallowCopyTracks(Hal::Event* event);
  void UpdateAnalysisTree(Hal::EventInterface* interface);
  void UpdateDst(Hal::EventInterface* interface);

 public:
  HalCbmMCEvent();
  HalCbmMCEvent(const HalCbmMCEvent& other);
  virtual void Update(Hal::EventInterface* interface);
  virtual Hal::EventInterface* CreateInterface() const;
  virtual Hal::Event* GetNewEvent() const { return new HalCbmMCEvent(); };
  virtual Bool_t ExistInTree() const;
  virtual TString GetFormatName() const { return "CbmMCEvent"; };
  virtual ~HalCbmMCEvent();
  ClassDef(HalCbmMCEvent, 1)
};

#endif /* NICACBMATMCEVENT_H_ */
