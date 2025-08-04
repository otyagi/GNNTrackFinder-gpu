/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Andrey Lebedev, Lukas Chlad [committer] */

/** @file CbmFsdAddress.cxx
 ** @author Lukas Chlad <l.chlad@gsi.de>
 ** @date 16.06.2023
 **
 ** Inspired by CbmStsAddress and only accomodate physical properties of expected FSD detector
 **
 **/

#include "CbmFsdAddress.h"

#include "CbmDefs.h"  // for kFsd

#include <cassert>  // for assert
#include <sstream>  // for operator<<, basic_ostream, stringstream

#include "AlgoFairloggerCompat.h"  // for Logger, LOG

// -----   Construct address from element Ids   ------------------------------
int32_t CbmFsdAddress::GetAddress(uint32_t Unit, uint32_t Module, uint32_t PhotoDet, uint32_t Version)
{
  using namespace Detail;

  assert(Version <= kCurrentVersion);

  // Catch overrun of allowed ranges
  if (Unit > static_cast<uint32_t>(kMask[Version][static_cast<int32_t>(CbmFsdAddress::Level::Unit)])) {
    LOG(error) << "Unit Id " << Unit << " exceeds maximum "
               << kMask[Version][static_cast<int32_t>(CbmFsdAddress::Level::Unit)];
    return 0;
  }
  if (Module > static_cast<uint32_t>(kMask[Version][static_cast<int32_t>(CbmFsdAddress::Level::Module)])) {
    LOG(error) << "Module Id " << Module << " exceeds maximum "
               << kMask[Version][static_cast<int32_t>(CbmFsdAddress::Level::Module)];
    return 0;
  }
  if (PhotoDet > static_cast<uint32_t>(kMask[Version][static_cast<int32_t>(CbmFsdAddress::Level::PhotoDet)])) {
    LOG(error) << "PhotoDetector Id " << PhotoDet << " exceeds maximum "
               << kMask[Version][static_cast<int32_t>(CbmFsdAddress::Level::PhotoDet)];
    return 0;
  }

  return ToIntegralType(ECbmModuleId::kFsd) << kShift[Version][static_cast<uint32_t>(CbmFsdAddress::Level::System)]
         | Unit << kShift[Version][static_cast<int32_t>(CbmFsdAddress::Level::Unit)]
         | Module << kShift[Version][static_cast<int32_t>(CbmFsdAddress::Level::Module)]
         | PhotoDet << kShift[Version][static_cast<int32_t>(CbmFsdAddress::Level::PhotoDet)] | Version << kVersionShift;
}
// ---------------------------------------------------------------------------


// -----   Construct address from address of descendant element   ------------
int32_t CbmFsdAddress::GetMotherAddress(int32_t address, int32_t level)
{
  using namespace Detail;
  if (!(level >= static_cast<int32_t>(CbmFsdAddress::Level::System)
        && level < static_cast<int32_t>(CbmFsdAddress::Level::NumLevels))) {
    throw std::out_of_range(std::string("CbmFsdAddress: Illegal element level ") + std::to_string(level));
    return 0;
  }
  if (level == static_cast<uint32_t>(CbmFsdAddress::Level::NumLevels) - 1) return address;
  uint32_t version  = GetVersion(address);
  int32_t motherAdd = (address & ((1 << kShift[version][level + 1]) - 1));
  motherAdd         = motherAdd | (version << kVersionShift);
  return motherAdd;
}
// ---------------------------------------------------------------------------


// -----   Get the index of an element   -------------------------------------
uint32_t CbmFsdAddress::GetElementId(int32_t address, int32_t level)
{
  using namespace Detail;
  if (!(level >= static_cast<int32_t>(CbmFsdAddress::Level::System)
        && level < static_cast<int32_t>(CbmFsdAddress::Level::NumLevels))) {
    throw std::out_of_range(std::string("CbmFsdAddress: Illegal element level ") + std::to_string(level));
    return 0;
  }
  uint32_t version = GetVersion(address);
  return (address & (kMask[version][level] << kShift[version][level])) >> kShift[version][level];
}
// ---------------------------------------------------------------------------


// -----   Get System ID   ---------------------------------------------------
uint32_t CbmFsdAddress::GetSystemId(int32_t address)
{
  return GetElementId(address, static_cast<uint32_t>(CbmFsdAddress::Level::System));
}
// ---------------------------------------------------------------------------


// -----   Get the version number from the address   -------------------------
uint32_t CbmFsdAddress::GetVersion(int32_t address)
{
  return uint32_t((address & (kVersionMask << kVersionShift)) >> kVersionShift);
}
// ---------------------------------------------------------------------------


// -----  Construct address by changing the index of an element   ------------
int32_t CbmFsdAddress::SetElementId(int32_t address, int32_t level, uint32_t newId)
{
  using namespace Detail;
  if (!(level >= static_cast<int32_t>(CbmFsdAddress::Level::System)
        && level < static_cast<int32_t>(CbmFsdAddress::Level::NumLevels))) {
    throw std::out_of_range(std::string("CbmFsdAddress: Illegal element level ") + std::to_string(level));
    return 0;
  }
  uint32_t version = GetVersion(address);
  uint32_t maxId   = (1 << kBits[version][level]) - 1;
  if (newId > maxId) {
    LOG(fatal) << "Id " << newId << " for FSD level " << level << " exceeds maximum " << maxId;
    return 0;
  }
  return (address & (~(kMask[version][level] << kShift[version][level]))) | (newId << kShift[version][level]);
}
// -------------------------------------------------------------------------

// -----   String output   -------------------------------------------------
std::string CbmFsdAddress::ToString(int32_t address)
{
  std::stringstream ss;

  ss << "FsdAddress: address " << address << " (version " << GetVersion(address) << ")"
     << ": system " << GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::System)) << ", unit "
     << GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::Unit)) << ", module "
     << GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::Module)) << ", photo detector "
     << GetElementId(address, static_cast<int32_t>(CbmFsdAddress::Level::PhotoDet));
  return ss.str();
}
// -------------------------------------------------------------------------
