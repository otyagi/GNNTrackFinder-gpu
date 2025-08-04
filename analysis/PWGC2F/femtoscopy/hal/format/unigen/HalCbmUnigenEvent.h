/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef NICAUNIGENEVENT_H_
#define NICAUNIGENEVENT_H_

#include "UEvent.h"
#include "UParticle.h"

#include <Hal/McEvent.h>
#include <HalCbmUnigenTrack.h>
/**
 * class used for representation of unigen event in "fake" format
 */
namespace Hal
{
  class EventInterface;
}
class HalCbmUnigenEvent : public Hal::McEvent {

 public:
  /**
	 * default constructor
	 */
  HalCbmUnigenEvent();
  HalCbmUnigenEvent(const HalCbmUnigenEvent& other);
  virtual Hal::EventInterface* CreateInterface() const;
  virtual void Update(Hal::EventInterface* interface);
  virtual void Clear(Option_t* opt = " ");
  virtual Bool_t ExistInTree() const;
  virtual TString GetFormatName() const;
  virtual ~HalCbmUnigenEvent();
  ClassDef(HalCbmUnigenEvent, 1)
};

#endif /* NICAUNIGENEVENT_H_ */
