/* Copyright (C) 2018-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci*/

#include "CbmTrdParMod.h"

#include <Logger.h>  // for LOG

//___________________________________________________________________
CbmTrdParMod::CbmTrdParMod(const char* name, const char* title) : TNamed(name, title) {}

//___________________________________________________________________
CbmTrdParMod::~CbmTrdParMod() { LOG(debug) << GetName() << "::delete[" << GetTitle() << "]"; }

ClassImp(CbmTrdParMod)
