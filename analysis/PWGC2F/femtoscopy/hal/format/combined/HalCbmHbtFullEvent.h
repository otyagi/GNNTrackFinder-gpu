/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMHBTFULLEVENT_H_
#define CBMHBTFULLEVENT_H_

#include <Hal/ComplexEvent.h>

namespace Hal
{
  class Event;
  class EventInterface;
}  // namespace Hal
/**
 * class for ana tree data and HBT analysis
 */
class HalCbmHbtFullEvent : public Hal::ComplexEvent {
 public:
  HalCbmHbtFullEvent();
  virtual TString GetFormatName() const { return "CbmHbtFullEvent"; };
  virtual Hal::Event* GetNewEvent() const { return new HalCbmHbtFullEvent(); };
  //   virtual Bool_t IsCompatible(const NicaEvent *non_buffered) const;
  virtual ~HalCbmHbtFullEvent();
  ClassDef(HalCbmHbtFullEvent, 1)
};
#endif /* CBMHBTFULLEVENT_H_ */
