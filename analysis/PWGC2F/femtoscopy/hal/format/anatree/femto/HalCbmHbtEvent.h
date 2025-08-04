/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMHBTEVENT_H_
#define CBMHBTEVENT_H_

#include <Hal/Event.h>
#include <Hal/ExpEvent.h>
/**
 * class for HBT analysis, note: CAN BE USED ONLY AS BUFFERED EVENT!
 */

namespace Hal
{
  class EventInterface;
}

class HalCbmHbtEvent : public Hal::ExpEvent {
 protected:
 public:
  HalCbmHbtEvent();
  virtual Hal::EventInterface* CreateInterface() const;
  virtual Bool_t IsCompatible(const Hal::Event* non_buffered) const;
  virtual Hal::Event* GetNewEvent() const { return new HalCbmHbtEvent(); };
  virtual ~HalCbmHbtEvent();
  ClassDef(HalCbmHbtEvent, 1)
};
#endif /* CBMHBTEVENT_H_ */
