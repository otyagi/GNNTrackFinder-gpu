/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "ReadoutConfig.h"

//#include "CbmTrdAddress.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>

using std::pair;
using std::setw;

CBM_YAML_INSTANTIATE(cbm::algo::trd::ReadoutConfig);

namespace cbm::algo::trd
{

  // ---  Constructor  ------------------------------------------------------------------
  ReadoutConfig::ReadoutConfig() {}

  // ------------------------------------------------------------------------------------


  // ---   Destructor   -----------------------------------------------------------------
  ReadoutConfig::~ReadoutConfig() {}
  // ------------------------------------------------------------------------------------


  // ---   Equipment IDs   --------------------------------------------------------------
  std::vector<uint16_t> ReadoutConfig::GetEquipmentIds()
  {
    std::vector<uint16_t> result;
    for (auto& entry : fReadoutMap)
      result.push_back(entry.first);
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---   Number of Crobs for a component / equipment   -------------------------------
  size_t ReadoutConfig::GetNumCrobs(uint16_t equipmentId)
  {
    size_t result = 0;
    auto it       = fReadoutMap.find(equipmentId);
    if (it != fReadoutMap.end()) result = fReadoutMap[equipmentId].size();
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---   Number of Elinks for a component / equipment, crob pair  ---------------------
  size_t ReadoutConfig::GetNumElinks(uint16_t equipmentId, uint16_t crobId)
  {
    size_t result = 0;
    if (crobId < GetNumCrobs(equipmentId)) result = fReadoutMap[equipmentId][crobId].size();
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---  Initialise the mapping structure   --------------------------------------------
  void ReadoutConfig::Init(const std::map<size_t, std::map<size_t, std::map<size_t, size_t>>>& addressMap,
                           std::map<size_t, std::map<size_t, std::map<size_t, std::map<size_t, size_t>>>>& channelMap)
  {
    // Constructing the map (equipmentId, crobId, eLink) -> (ASIC address)
    for (auto compMap : addressMap) {
      uint16_t equipmentId = compMap.first;
      uint16_t numCrobs    = compMap.second.size();
      fReadoutMap[equipmentId].resize(numCrobs);

      for (auto crobMap : compMap.second) {
        uint16_t crobId    = crobMap.first;
        uint16_t numElinks = crobMap.second.size();
        fReadoutMap[equipmentId][crobId].resize(numElinks);

        for (auto elinkMap : crobMap.second) {
          uint16_t elinkId                          = elinkMap.first;
          uint16_t address                          = elinkMap.second;
          fReadoutMap[equipmentId][crobId][elinkId] = address;
        }
      }
    }

    // Constructing the map (equipmentId, crobId, eLink, chan) -> (chan address)
    for (auto compMap : channelMap) {
      uint16_t equipmentId = compMap.first;
      uint16_t numCrobs    = compMap.second.size();
      fChannelMap[equipmentId].resize(numCrobs);

      for (auto crobMap : compMap.second) {
        uint16_t crobId    = crobMap.first;
        uint16_t numElinks = crobMap.second.size();
        fChannelMap[equipmentId][crobId].resize(numElinks);

        for (auto elinkMap : crobMap.second) {
          uint16_t elinkId  = elinkMap.first;
          uint16_t numChans = elinkMap.second.size();
          fChannelMap[equipmentId][crobId][elinkId].resize(numChans);

          for (auto chanMap : elinkMap.second) {
            uint16_t chanId                                   = chanMap.first;
            uint32_t address                                  = chanMap.second;
            fChannelMap[equipmentId][crobId][elinkId][chanId] = address;
          }
        }
      }
    }
  }
  // ------------------------------------------------------------------------------------


  // ---  Mapping (equimentId, crobId, elink) -> (ASIC address, channel addresses)  -----
  std::pair<int32_t, std::vector<uint32_t>> ReadoutConfig::Map(uint16_t equipmentId, uint16_t crobId, uint16_t elinkId)
  {
    std::pair<int32_t, std::vector<uint32_t>> result;
    result.first = -1;
    auto it      = fChannelMap.find(equipmentId);
    if (it != fChannelMap.end()) {
      if (crobId < fChannelMap[equipmentId].size()) {
        if (elinkId < fChannelMap[equipmentId][crobId].size()) {
          result.first  = fReadoutMap[equipmentId][crobId][elinkId];
          result.second = fChannelMap[equipmentId][crobId][elinkId];
        }
      }
    }
    return result;
  }
  // ------------------------------------------------------------------------------------

  void ReadoutConfig::SetElinkTimeOffset(uint32_t criid, uint8_t elinkid, int32_t offsetNs)
  {
    // create vector for criid if not yet existing
    if (fElinkTimeOffsetMap.find(criid) == fElinkTimeOffsetMap.end()) {
      //vector has fixed size of 256 (max elinkid) and is initialized to 0
      fElinkTimeOffsetMap.insert(std::make_pair(criid, std::vector<int32_t>(256, 0)));
    }

    fElinkTimeOffsetMap[criid][elinkid] = offsetNs;
  }

  int32_t ReadoutConfig::GetElinkTimeOffset(uint32_t criid, uint8_t elinkid)
  {
    auto it = fElinkTimeOffsetMap.find(criid);
    if (it == fElinkTimeOffsetMap.end()) {
      return 0;
    }

    const auto& elinks = it->second;
    return elinks.at(elinkid);
  }


  // -----   Print readout map   ------------------------------------------------
  std::string ReadoutConfig::PrintReadoutMap()
  {
    std::stringstream ss;
    for (auto compMap : fReadoutMap) {

      uint16_t equipmentId = compMap.first;
      uint16_t numCrobs    = compMap.second.size();
      ss << "\n Equipment " << equipmentId << " nCrobs " << numCrobs;
      for (size_t crobId = 0; crobId < numCrobs; crobId++) {

        uint16_t numElinks = compMap.second.at(crobId).size();
        ss << "\n Equipment " << equipmentId << " CrobId " << crobId << " nElinks " << numElinks;
        for (size_t elinkId = 0; elinkId < numElinks; elinkId++) {

          uint16_t address = compMap.second.at(crobId).at(elinkId);
          ss << "\n Equipment " << equipmentId << " CrobId " << crobId << " elinkID " << elinkId << " address "
             << address;

          //Now output channel addresses
          ss << "\n Channels ";
          auto vec = fChannelMap[equipmentId][crobId][elinkId];
          std::copy(vec.begin(), vec.end() - 1, std::ostream_iterator<uint32_t>(ss, " "));
          ss << vec.back();
        }
      }
    }
    ss << "\n";
    return ss.str();
  }
  // ----------------------------------------------------------------------------


}  // namespace cbm::algo::trd
