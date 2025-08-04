/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmHbtFullEvent.h"

#include "HalCbmHbtEvent.h"
#include "HalCbmMCEvent.h"


HalCbmHbtFullEvent::HalCbmHbtFullEvent() : Hal::ComplexEvent(new HalCbmHbtEvent(), new HalCbmMCEvent()) {}

HalCbmHbtFullEvent::~HalCbmHbtFullEvent() {}
/*
Bool_t CbmHbtFullEvent::IsCompatible(const NicaEvent *non_buffered) const {
    if(non_buffered->InheritsFrom("NicaCbmATFullEvent")) return kTRUE;
    return kFALSE;
}
*/
