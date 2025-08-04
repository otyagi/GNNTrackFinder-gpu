/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Felix Weiglhofer */
#include "ReadoutConfig.h"

#include "AlgoFairloggerCompat.h"
#include "CbmStsAddress.h"
#include "ChannelMaskSet.h"
#include "Exceptions.h"

#include <cassert>
#include <iomanip>

using std::pair;
using std::setw;

using namespace cbm::algo;

CBM_YAML_INSTANTIATE(sts::ReadoutSetup);

sts::FEBType sts::ReadoutSetup::Component::GetFEBType() const
{
  size_t febsPerCrob = FEBsPerCrob();
  switch (febsPerCrob) {
    case 5: return FEBType::FEB8_1;
    case 1: return FEBType::FEB8_5;
  }
  throw FatalError("Invalid number of FEBs per CROB: {}", febsPerCrob);
}

const std::vector<sts::ReadoutSetup::Elink>& sts::ReadoutSetup::GetElinks(FEBType type) const
{
  switch (type) {
    case FEBType::FEB8_1: return elinksFeb8_1;
    case FEBType::FEB8_5: return elinksFeb8_5;
  }
  throw FatalError("Unknown FEB type: {}", static_cast<int>(type));
}

sts::ReadoutConfig::ReadoutConfig(const ReadoutSetup& config, const ChannelMaskSet& chanMaskSet)
{
  Init(config, chanMaskSet);
}

std::vector<u16> sts::ReadoutConfig::GetEquipmentIds()
{
  std::vector<uint16_t> result;
  for (auto& entry : fReadoutConfig)
    result.push_back(entry.first);
  return result;
}

size_t sts::ReadoutConfig::GetNumElinks(u16 equipmentId)
{
  size_t result = 0;
  auto it       = fReadoutConfig.find(equipmentId);
  if (it != fReadoutConfig.end()) result = fReadoutConfig[equipmentId].size();
  return result;
}

size_t sts::ReadoutConfig::GetNumElinks()
{
  size_t result = 0;
  for (auto& entry : fReadoutConfig) {
    result += entry.second.size();
  }
  return result;
}

void sts::ReadoutConfig::Init(const ReadoutSetup& config, const ChannelMaskSet& chanMaskSet)
{
  // The readout hierarchy is: component - CROB - FEB - ASIC (elink). Each elink
  // connects one ASIC. One FEB comprises 8 ASICs and reads out one side of a module (sensor).
  // In this setup, there is only one CROB per component. The code below is formulate such
  // as to support also multiple CROBs per component. In that case, the elinks are numbered
  // consecutively within one component.

  // Constants
  const uint16_t numModules     = config.modules.size();                      // Number of modules in the setup
  const uint16_t numComp        = config.components.size();                   // Number of components
  const uint16_t numCrobPerComp = config.components.at(0).feb2module.size();  // Number of CROBs per component
  // const uint16_t numFebsPerCrob   = config.components.at(0).feb2module.at(0).size();  // Number of FEBs per CROB
  const uint16_t numAsicsPerFeb   = config.numAsicsPerFeb;  // Number of ASICs per FEB
  const uint16_t numAsicsPerMod   = 2 * numAsicsPerFeb;     // Number of ASICs per module
  const uint16_t numElinksPerCrob = 42;                     // Number of elinks per CROB
  const uint16_t numChanPerAsic   = 128;                    ///< Number of channels per ASIC
  const uint16_t numElinksPerComp = numCrobPerComp * numElinksPerCrob;

  // Constructing the map (equipmentId, eLink) -> (module, ASIC within module)
  int32_t febBaseIdx = 0;
  for (uint16_t compIdx = 0; compIdx < numComp; compIdx++) {
    const auto& component = config.components.at(compIdx);
    uint16_t equipment    = component.equipmentId;
    fReadoutConfig[equipment].resize(numElinksPerComp);
    const int32_t numFebsPerCrob = component.FEBsPerCrob();
    for (uint16_t crobIdx = 0; crobIdx < numCrobPerComp; crobIdx++) {
      for (uint16_t elinkIdx = 0; elinkIdx < numElinksPerCrob; elinkIdx++) {

        int32_t moduleAddress = -1;
        uint16_t asicInModule = 0;
        bool isPulser         = false;

        uint16_t elinkId   = numElinksPerCrob * crobIdx + elinkIdx;  // elink within component
        const auto& elinks = config.GetElinks(component.GetFEBType());
        const auto& elink  = elinks.at(elinkId);

        int16_t feb = elink.toFeb;  // FEB within CROB

        if (feb == -1) {
          continue;
        }

        int16_t moduleIdx = component.feb2module[crobIdx][feb];  // Module index

        if (moduleIdx == -1) {
          continue;
        }

        isPulser = component.febIsPulser.at(crobIdx).at(feb);  // Pulser flag

        assert(moduleIdx < numModules);
        const auto& module = config.modules.at(moduleIdx);
        moduleAddress      = module.address;                                // Module address
        bool moduleType    = module.type;                                   // 0 or 1
        int16_t moduleSide = component.feb2moduleSide[crobIdx][feb];        // 0 or 1, -1 is inactive
        int16_t febType    = (moduleType == 0 ? moduleSide : !moduleSide);  // 0 = FEB A, 1 = FEB B
        uint32_t asicIndex = (febType == 0 ? elink.toAsicFebA : elink.toAsicFebB);
        uint32_t asicInFeb = asicIndex % numAsicsPerFeb;  // ASIC number within FEB
        // Asic number is counted downward from numAsicsPerMod - 1 for p side
        asicInModule = (moduleSide == 1 ? asicInFeb : numAsicsPerMod - 1 - asicInFeb);

        // Init channel mask
        const int32_t febId = feb + febBaseIdx;
        auto mapIt          = chanMaskSet.values.find(febId);
        if (mapIt != chanMaskSet.values.end()) {
          const auto& mask = mapIt->second;

          for (uint32_t chan = 0; chan < numChanPerAsic; chan++) {
            const uint32_t chanInFeb = chan + numChanPerAsic * asicInFeb;
            if (mask.Contains(chanInFeb)) {
              std::vector<bool>& chanMask = fMaskMap[equipment][elinkIdx];
              if (chanMask.empty()) {
                chanMask.resize(numChanPerAsic, false);
              }
              chanMask[chan] = true;
            }
          }
        }

        fReadoutConfig[equipment][elinkIdx] = {moduleAddress, asicInModule, isPulser};

      }                                             //# elink
    }                                               //# CROB
    febBaseIdx += numCrobPerComp * numFebsPerCrob;  // Add the proper offset for the current CROB
  }                                                 //# component
}

sts::ReadoutConfig::Entry sts::ReadoutConfig::Map(uint16_t equipmentId, uint16_t elinkId)
{
  Entry result{-1, 0, false};
  auto equipIter = fReadoutConfig.find(equipmentId);
  if (equipIter != fReadoutConfig.end()) {
    if (elinkId < equipIter->second.size()) {
      result = equipIter->second.at(elinkId);
    }
  }
  return result;
}

std::vector<bool> sts::ReadoutConfig::MaskMap(uint16_t equipmentId, uint16_t elinkId)
{
  std::vector<bool> result;
  auto equipIter = fMaskMap.find(equipmentId);
  if (equipIter != fMaskMap.end()) {
    auto elinkMap  = equipIter->second;
    auto elinkIter = elinkMap.find(elinkId);
    if (elinkIter != elinkMap.end()) {
      result = elinkIter->second;
    }
  }
  return result;
}

uint32_t sts::ReadoutConfig::AdcCutMap(uint16_t equipmentId, uint16_t elinkId)
{
  uint32_t result = 0;
  auto equipIter  = fAdcCutMap.find(equipmentId);
  if (equipIter != fAdcCutMap.end()) {
    auto elinkMap  = equipIter->second;
    auto elinkIter = elinkMap.find(elinkId);
    if (elinkIter != elinkMap.end()) {
      result = elinkIter->second;
    }
  }
  return result;
}

std::string sts::ReadoutConfig::PrintReadoutMap()
{

  std::stringstream ss;
  for (auto& equipment : fReadoutConfig) {
    auto eqId = equipment.first;
    for (size_t elink = 0; elink < equipment.second.size(); elink++) {
      auto address = equipment.second.at(elink).moduleAddress;
      auto asicNr  = equipment.second.at(elink).asicNumber;
      ss << "\n Equipment " << eqId << "  elink " << setw(2) << elink;
      ss << "  ASIC " << setw(2) << asicNr << "  module " << address;
      if (address != -1) {
        ss << "  Unit " << setw(2) << CbmStsAddress::GetElementId(address, kStsUnit);
        ss << "  Ladd " << setw(2) << CbmStsAddress::GetElementId(address, kStsLadder);
        ss << "  Hlad " << setw(2) << CbmStsAddress::GetElementId(address, kStsHalfLadder);
        ss << "  Modu " << setw(2) << CbmStsAddress::GetElementId(address, kStsModule);
      }
      else
        ss << "  Inactive";
    }  //# elink
  }    //# component
  return ss.str();
}
