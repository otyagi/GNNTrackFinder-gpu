/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICACBMATEVENT_H_
#define NICACBMATEVENT_H_
#include <Hal/ComplexEvent.h>

/**
 * class for analyses that base on anatree events
 */
namespace Hal
{
  class Event;
  class EventInterface;
  class ComplexEventInterface;
}  // namespace Hal


class HalCbmFullEvent : public Hal::ComplexEvent {


 protected:
  HalCbmFullEvent(Hal::Event* re, Hal::Event* im);
  void UpdateAnalysisTree(Hal::ComplexEventInterface* interface);
  void UpdateDst(Hal::ComplexEventInterface* interface);

 public:
  HalCbmFullEvent();
  virtual void Update(Hal::EventInterface* interface);
  virtual TString GetFormatName() const { return "HalCbmFullEvent"; };
  virtual Hal::Event* GetNewEvent() const;
  virtual ~HalCbmFullEvent();
  ClassDef(HalCbmFullEvent, 1)
};
#endif /* NICACBMATEVENT_H_ */
