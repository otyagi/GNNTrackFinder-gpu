/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Florian Uhlig, Lukas Chlad [committer] */

/** @file CbmFsdDigi.cxx
 ** @author Lukas Chlad <l.chlad@gsi.de>
 ** @date 15.06.2023
 **
 ** Code for Data class for FSD digital information
 **/

#include "CbmFsdDigi.h"

#include <iomanip>  // for hex, setw, setfill, fixed, setprecission
#include <sstream>  // for operator<<, basic_ostream, char_trait
#include <string>   // for basic_string

// --- Set address from module and section number
// --- version in CbmFsdAddress is taken the latest
void CbmFsdDigi::SetAddress(uint32_t unitId, uint32_t moduleId, uint32_t photodetId)
{
  fAddress = CbmFsdAddress::GetAddress(unitId, moduleId, photodetId);
}


// --- Info to string
std::string CbmFsdDigi::ToString() const
{
  // Example output
  // CbmFsdDigi: address = 0x00001018 Charge = 0.011590 Time = 1006.438294

  std::stringstream ss;
  ss << "CbmFsdDigi: address = 0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << fAddress
     << " Charge = " << std::fixed << std::setprecision(6) << fEdep << " Time = " << fTime;
  return ss.str();
}

#ifndef NO_ROOT
ClassImp(CbmFsdDigi)
#endif
