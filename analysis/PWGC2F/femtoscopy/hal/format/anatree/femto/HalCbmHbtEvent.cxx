/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmHbtEvent.h"

#include "HalCbmEvent.h"
#include "HalCbmHbtTrack.h"

HalCbmHbtEvent::HalCbmHbtEvent() : Hal::ExpEvent("HalCbmHbtTrack") {}

Bool_t HalCbmHbtEvent::IsCompatible(const Hal::Event* non_buffered) const
{
  if (non_buffered->InheritsFrom("HalCbm::HalCbmEvent")) return kTRUE;
  return kFALSE;
}

HalCbmHbtEvent::~HalCbmHbtEvent() {}

Hal::EventInterface* HalCbmHbtEvent::CreateInterface() const { return nullptr; }
