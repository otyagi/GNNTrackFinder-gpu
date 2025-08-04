/* Copyright (C) 2024 Hulubei National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer] */

#include "CbmTrdDefs.h"

using namespace cbm::trd;

bool cbm::trd::HasFaspFEE(uint16_t config) { return TESTBIT(config, eModuleConfig::kFEEtyp); }
bool cbm::trd::HasSpadicFEE(uint16_t config) { return !HasFaspFEE(config); }
bool cbm::trd::HasPadPlane2D(uint16_t config) { return TESTBIT(config, eModuleConfig::kPPtyp); }
bool cbm::trd::HasPadPlane1D(uint16_t config) { return !HasPadPlane2D(config); }
void cbm::trd::SetFEE(uint16_t config, bool fasp)
{
  fasp ? SETBIT(config, eModuleConfig::kFEEtyp) : CLRBIT(config, eModuleConfig::kFEEtyp);
}
void cbm::trd::SetPP(uint16_t config, bool twod)
{
  twod ? SETBIT(config, eModuleConfig::kPPtyp) : CLRBIT(config, eModuleConfig::kPPtyp);
}
