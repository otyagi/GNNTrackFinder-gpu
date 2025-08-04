/* Copyright (C) 2013-2020 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Andrey Lebedev [committer] */

/** @file CbmStsAddress.cxx
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 17.05.2013
 **/

#include "CbmStsAddress.h"

#include "CbmDefs.h"  // for kSts

#include <cassert>  // for assert
#include <sstream>  // for operator<<, basic_ostream, stringstream

#include "AlgoFairloggerCompat.h"  // for Logger, LOG

// -----   Construct address from element Ids   ------------------------------
int32_t CbmStsAddress::GetAddress(uint32_t unit, uint32_t ladder, uint32_t halfladder, uint32_t module, uint32_t sensor,
                                  uint32_t side, uint32_t version)
{
  using namespace Detail;

  assert(version <= kCurrentVersion);

  // Catch overrun of allowed ranges
  uint32_t maxUnit = (1 << kBits[version][kStsUnit]) - 1;
  if (unit > maxUnit) {
    LOG(error) << "Unit Id " << unit << " exceeds maximum " << maxUnit;
    return 0;
  }
  uint32_t maxLadder = (1 << kBits[version][kStsLadder]) - 1;
  if (ladder > maxLadder) {
    LOG(error) << "Ladder Id " << ladder << " exceeds maximum " << maxLadder;
    return 0;
  }
  uint32_t maxHalfLadder = (1 << kBits[version][kStsHalfLadder]) - 1;
  if (halfladder > maxHalfLadder) {
    LOG(error) << "HalfLadder Id " << halfladder << " exceeds maximum " << maxHalfLadder;
    return 0;
  }
  uint32_t maxModule = (1 << kBits[version][kStsModule]) - 1;
  if (module > maxModule) {
    LOG(error) << "Module Id " << module << " exceeds maximum " << maxModule;
    return 0;
  }
  uint32_t maxSensor = (1 << kBits[version][kStsSensor]) - 1;
  if (sensor > maxSensor) {
    LOG(error) << "Sensor Id " << sensor << " exceeds maximum " << maxSensor;
    return 0;
  }
  uint32_t maxSide = (1 << kBits[version][kStsSide]) - 1;
  if (side > maxSide) {
    LOG(error) << "Side Id " << side << " exceeds maximum " << maxSide;
    return 0;
  }

  return ToIntegralType(ECbmModuleId::kSts) << kShift[version][kStsSystem] | unit << kShift[version][kStsUnit]
         | ladder << kShift[version][kStsLadder] | halfladder << kShift[version][kStsHalfLadder]
         | module << kShift[version][kStsModule] | sensor << kShift[version][kStsSensor]
         | side << kShift[version][kStsSide] | version << kVersionShift;
}
// ---------------------------------------------------------------------------


// -----   Construct address from array of element Ids   ----------------------
int32_t CbmStsAddress::GetAddress(uint32_t* elementId, uint32_t version)
{
  using namespace Detail;

  assert(version <= kCurrentVersion);

  int32_t address = ToIntegralType(ECbmModuleId::kSts) << kShift[version][kStsSystem];
  for (int32_t level = 1; level < kStsNofLevels; level++) {
    uint32_t maxId = (1 << kBits[version][level]) - 1;
    if (elementId[level] > maxId) {
      LOG(error) << "Id " << elementId[level] << " for STS level " << level << " exceeds maximum " << maxId;
      return 0;
    }
    address = address | (elementId[level] << kShift[version][level]);
  }
  address = address | (version << kVersionShift);

  return address;
}
// ---------------------------------------------------------------------------


// -----   Construct address from address of descendant element   ------------
int32_t CbmStsAddress::GetMotherAddress(int32_t address, int32_t level)
{
  using namespace Detail;
  assert(level >= kStsSystem && level < kStsNofLevels);
  if (level == kStsNofLevels - 1) return address;
  uint32_t version  = GetVersion(address);
  int32_t motherAdd = (address & ((1 << kShift[version][level + 1]) - 1));
  motherAdd       = motherAdd | (version << kVersionShift);
  return motherAdd;
}
// ---------------------------------------------------------------------------


// -----   Get the index of an element   -------------------------------------
uint32_t CbmStsAddress::GetElementId(int32_t address, int32_t level)
{
  using namespace Detail;
  assert(level >= kStsSystem && level < kStsNofLevels);
  uint32_t version = GetVersion(address);
  return (address & (kMask[version][level] << kShift[version][level])) >> kShift[version][level];
}
// ---------------------------------------------------------------------------


// -----   Get System ID   ---------------------------------------------------
ECbmModuleId CbmStsAddress::GetSystemId(int32_t address)
{
  return static_cast<ECbmModuleId>(GetElementId(address, kStsSystem));
  //  return GetElementId(address, kStsSystem);
}
// ---------------------------------------------------------------------------


// -----   Get the version number from the address   -------------------------
uint32_t CbmStsAddress::GetVersion(int32_t address)
{
  return uint32_t((address & (kVersionMask << kVersionShift)) >> kVersionShift);
}
// ---------------------------------------------------------------------------


// -----  Construct address by changing the index of an element   ------------
int32_t CbmStsAddress::SetElementId(int32_t address, int32_t level, uint32_t newId)
{
  using namespace Detail;
  assert(level >= kStsSystem && level < kStsNofLevels);
  uint32_t version = GetVersion(address);
  uint32_t maxId   = (1 << kBits[version][level]) - 1;
  if (newId > maxId) {
    LOG(fatal) << "Id " << newId << " for STS level " << level << " exceeds maximum " << maxId;
    return 0;
  }
  return (address & (~(kMask[version][level] << kShift[version][level]))) | (newId << kShift[version][level]);
}
// -------------------------------------------------------------------------

// -----   String output   -------------------------------------------------
std::string CbmStsAddress::ToString(int32_t address)
{
  std::stringstream ss;

  ss << "StsAddress: address " << address << " (version " << GetVersion(address) << ")"
     << ": system " << GetElementId(address, kStsSystem) << ", unit " << GetElementId(address, kStsUnit) << ", ladder "
     << GetElementId(address, kStsLadder) << ", half-ladder " << GetElementId(address, kStsHalfLadder) << ", module "
     << GetElementId(address, kStsModule) << ", sensor " << GetElementId(address, kStsSensor) << ", side "
     << GetElementId(address, kStsSide);
  return ss.str();
}
// -------------------------------------------------------------------------
