/* Copyright (C) 2023-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Dominik Smith [committer], Alex Bercuci */

#include "ReadoutConfig.h"

//#include "CbmTrdAddress.h"
#include "AlgoFairloggerCompat.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>

using std::pair;
using std::setw;

CBM_YAML_INSTANTIATE(cbm::algo::trd2d::ReadoutSetup);
CBM_YAML_INSTANTIATE(cbm::algo::trd2d::ReadoutCalib);

namespace cbm::algo::trd2d
{

  // ---  Constructor  ------------------------------------------------------------------
  ReadoutCalib::ReadoutCalib() {}

  // ------------------------------------------------------------------------------------


  // ---   Destructor   -----------------------------------------------------------------
  ReadoutCalib::~ReadoutCalib() {}
  // ------------------------------------------------------------------------------------

  ReadoutCalib::ChanDescriptor ReadoutCalib::GetChannelFeeCalib(uint16_t modId, uint16_t padId)
  {
    ChanDescriptor result;
    if (fCalibMap.find(modId) == fCalibMap.end()) return result;
    if (padId >= NFASPMOD * NFASPCH) return result;
    result = fCalibMap[modId][padId];
    return result;
  }


  // ---  Constructor  ------------------------------------------------------------------
  ReadoutSetup::ReadoutSetup() {}

  // ------------------------------------------------------------------------------------


  // ---   Destructor   -----------------------------------------------------------------
  ReadoutSetup::~ReadoutSetup() {}
  // ------------------------------------------------------------------------------------


  // ---   Equipment IDs   --------------------------------------------------------------
  std::vector<uint16_t> ReadoutSetup::GetEquipmentIds()
  {
    std::vector<uint16_t> result;
    for (auto& entry : fReadoutMap)
      result.push_back(entry.first);
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---   Number of Asics for a component / equipment   -------------------------------
  size_t ReadoutSetup::GetNumAsics(uint16_t equipmentId)
  {
    size_t result = 0;
    auto it       = fChannelMap.find(equipmentId);
    if (it != fChannelMap.end()) result = fChannelMap[equipmentId].size();
    return result;
  }
  // ------------------------------------------------------------------------------------

  // ---   List of ASICs registered to the ROB  ---------------------
  std::vector<uint8_t> ReadoutSetup::GetAsicList(uint16_t equipmentId)
  {
    std::vector<uint8_t> result;
    for (auto& entry : fChannelMap[equipmentId])
      result.push_back(entry.first);
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---   Number of Channels for a component / equipment, asic pair  ---------------------
  size_t ReadoutSetup::GetNumChans(uint16_t equipmentId, uint16_t asicId)
  {
    size_t result = 0;
    auto it       = fChannelMap.find(equipmentId);
    if (it != fChannelMap.end()) {
      auto fiberMap = fChannelMap[equipmentId];
      auto jt       = fiberMap.find(asicId);
      if (jt != fiberMap.end()) {
        result = fiberMap[asicId].size();
      }
    }
    return result;
  }
  // ------------------------------------------------------------------------------------

  // ---  Initialise the component mapping structure   ----------------------------------
  void ReadoutSetup::InitComponentMap(const std::map<uint32_t, std::vector<uint16_t>>& map)
  {
    // Receive map (moduleId, crobId) -> (equipId)
    // Invert to obtain component map (equipId) -> (module iq, crob id)
    for (auto& entry : map) {
      uint16_t mod_id = entry.first;
      //for (uint8_t eq_id = 0; eq_id < NROBMOD * NELINKROB; eq_id++) {
      uint8_t eq_id = 0;
      for (const auto& eq_add : entry.second) {
        //uint16_t eq_id = entry.second[elink_id];
        fReadoutMap[eq_add] = {mod_id, eq_id++};
      }
    }
  }
  // ------------------------------------------------------------------------------------

  // ---  Initialise the mapping structure   --------------------------------------------
  void ReadoutSetup::InitChannelMap(
    const std::map<size_t, std::map<size_t, std::map<size_t, std::tuple<int32_t, bool, int16_t, uint16_t>>>>&
      channelMap)
  {
    // Constructing the map (equipId, asicId, chanId) -> (pad address, mask flag, daq offset)
    for (auto compMap : channelMap) {
      uint16_t equipmentId = compMap.first;
      // uint16_t numAsics    = compMap.second.size();
      for (auto asicMap : compMap.second) {
        uint16_t asicId   = asicMap.first;
        // uint16_t numChans = asicMap.second.size();
        fChannelMap[equipmentId][asicId].resize(16);
        for (auto chanMap : asicMap.second) {
          uint16_t chanId                              = chanMap.first;
          std::tuple<int32_t, bool, int8_t, uint16_t> chanPars = chanMap.second;
          const ChanMapping entry = {std::get<0>(chanPars), std::get<1>(chanPars), std::get<2>(chanPars),
                                     std::get<3>(chanPars)};
          fChannelMap[equipmentId][asicId][chanId] = entry;
        }
      }
    }
  }
  // ------------------------------------------------------------------------------------


  // ---  Mapping (equimentId, asicId, channel) -> (pad address, mask flag, daq offset)  -----
  ReadoutSetup::ChanMapping ReadoutSetup::ChanMap(uint16_t equipId, uint16_t asicId, uint16_t chanId)
  {
    ChanMapping result = {-1, false, 0, 0};
    auto it            = fChannelMap.find(equipId);
    if (it != fChannelMap.end()) {
      auto fiberMap = fChannelMap[equipId];
      auto jt       = fiberMap.find(asicId);
      if (jt != fiberMap.end()) {
        auto asic = fiberMap[asicId];
        if (chanId < asic.size()) {
          result = asic[chanId];
        }
      }
    }
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---  Mapping (equimentId) -> (module id, crob id)  ---------------------------------
  ReadoutSetup::CompMapping ReadoutSetup::CompMap(uint16_t equipId)
  {
    CompMapping result = {};
    auto equipIter     = fReadoutMap.find(equipId);
    if (equipIter != fReadoutMap.end()) {
      result = equipIter->second;
    }
    return result;
  }
  // ------------------------------------------------------------------------------------


  // -----   Print readout map   ------------------------------------------------
  std::string ReadoutSetup::PrintReadoutMap()
  {
    std::stringstream ss;
    for (auto comp : fReadoutMap) {
      uint16_t equipmentId = comp.first;
      auto value           = comp.second;
      uint16_t moduleId    = value.moduleId;
      uint16_t fiberId     = value.fiberId;
      ss << "Equipment 0x" << std::hex << (int) equipmentId << " Module " << moduleId << " fiberId " << fiberId << "\n";
    }
    ss << "\n";

    for (auto asicMap : fChannelMap) {
      uint16_t equipmentId = asicMap.first;
      uint16_t numAsics    = asicMap.second.size();
      ss << "\n Equipment 0x" << std::hex << (int) equipmentId << " nAsics " << numAsics;

      int asicCnt(0);
      auto asics = asicMap.second;
      for (auto asic : asics) {
        int asicId        = asic.first;
        auto asicChs      = asic.second;
        uint16_t numChans = asicChs.size();
        ss << "\n    " << asicCnt << " AsicId " << asicId << " nChans " << numChans;
        for (size_t chanId = 0; chanId < numChans; chanId++) {
          auto entry      = asicChs.at(chanId);
          int32_t address = entry.padAddress;
          bool isMasked   = entry.maskFlag;
          uint8_t tOffset = entry.tOffset;
          uint16_t thres  = entry.lThreshold;
          ss << "\n      chanID " << chanId << " {pad " << address << " mask " << isMasked << " time offset[clk] "
             << (int) tOffset << " threshold[" << (thres > 0 ? "on" : "off") << "]}";
        }
        asicCnt++;
      }
    }
    ss << "\n";
    return ss.str();
  }
  // ----------------------------------------------------------------------------


  // -----   Print readout map   ------------------------------------------------
  std::string ReadoutCalib::PrintCalibMap()
  {
    std::stringstream ss;
    ss << "fCalibMap.size=" << fCalibMap.size() << "\n";
    for (const auto& [mod, pars] : fCalibMap) {
      ss << "Mod 0x" << mod << "\n";
      int ipar(0);
      for (auto par : pars) {
        if (ipar % NFASPCH == 0) ss << "\t";
        ss << " " << std::setw(4) << par.gainfee;
        if (ipar % NFASPCH == NFASPCH - 1) ss << "\n";
        ipar++;
      }
    }
    return ss.str();
  }
  // ----------------------------------------------------------------------------


}  // namespace cbm::algo::trd2d
