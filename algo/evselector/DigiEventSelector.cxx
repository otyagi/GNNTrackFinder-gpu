/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Shreya Roy [committer], Pierre-Alain Loizeau, Norbert Herrmann, Volker Friese, Dominik Smith */

#include "DigiEventSelector.h"

#include "AlgoFairloggerCompat.h"
#include "CbmStsDigi.h"

#include <gsl/span>
#include <iterator>
#include <unordered_map>
#include <unordered_set>


namespace cbm::algo::evbuild
{

  // -----   Test one digi event   --------------------------------------------
  bool DigiEventSelector::operator()(const DigiEvent& event) const
  {

    // --- Test number of digis per detector system
    for (auto& entry : fConfig.fMinNumDigis) {
      if (!(event.Size(entry.first) >= entry.second)) {
        return false;
        break;
      }
    }

    // --- Test number of layers with digis per detector system
    for (auto& entry : fConfig.fMinNumLayers) {
      if (entry.second == 0) continue;
      switch (entry.first) {
        case ECbmModuleId::kSts:
          if (!CheckStsStations(event.fSts, entry.second)) return false;
          break;
        case ECbmModuleId::kTof:
          if (!CheckTofLayers(event.fTof, entry.second)) return false;
          break;
        default:
          throw std::runtime_error("Number of layers for " + ::ToString(entry.first) + " is not implemented");
          break;
      }
    }

    // --- Test masked channels (if any)
    for (auto& entry : fConfig.fMaskedChannels) {
      size_t nDigisAccepted = 0;
      auto det              = entry.first;
      auto itMinNumDigis    = fConfig.fMinNumDigis.find(det);
      if (itMinNumDigis != fConfig.fMinNumDigis.end() && itMinNumDigis->second > 0) {
        switch (det) {
          case ECbmModuleId::kBmon:
            for (const auto& digi : event.fBmon) {
              if (entry.second.find(digi.GetAddress()) == entry.second.end()) {
                ++nDigisAccepted;
              }
            }
            break;
          default:
            // This was met in the first block of the function
            break;
        }
        if (nDigisAccepted < itMinNumDigis->second) {
          return false;
        }
      }
    }

    return true;
  }
  // --------------------------------------------------------------------------


  // -----   Check number of STS stations   -----------------------------------
  bool DigiEventSelector::CheckStsStations(gsl::span<const CbmStsDigi> digis, size_t minNum) const
  {

    // A station is considered activated if it has at least one activated module. A module is considered
    // activated if it has at least one digi on each of the sensor sides.
    const uint16_t chanPerSide = 1024;
    std::unordered_set<uint32_t> stations;      // active stations
    std::unordered_map<int32_t, bool> modules;  // active modules, value false means front side hit, true is back side

    for (auto& digi : digis) {
      const int32_t addr = digi.GetAddress();
      auto module        = modules.find(addr);
      if (module == modules.end())
        modules[addr] = digi.GetChannel() / chanPerSide;  // = 0,1 for sides
      else {
        if (digi.GetChannel() / chanPerSide != module->second) {  // other sensor side found, chance for cluster
          const uint32_t stationAddr = CbmStsAddress::GetElementId(addr, EStsElementLevel::kStsUnit);
          if (stations.count(stationAddr) == 0) {
            stations.insert(stationAddr);
            if (stations.size() == minNum) break;
          }
        }
      }
    }

    if (stations.size() < minNum)
      return false;
    else
      return true;
  }
  // TODO: (VF 14/07/23) I do not like this implementation very much. It is mCBM-specific in the sense
  // that what actually is checked are the units, not the stations. Only for mCBM geometries, stations
  // and units are identical. Moreover, I feel that the association of digis (addresses) to stations
  // should be implemented in the STS geometry handler, like for TOF in the method below.
  // And the number of channels per sensor side is hard-coded here. Not nice.
  // --------------------------------------------------------------------------


  // -----   Check number of TOF layers   -------------------------------------
  bool DigiEventSelector::CheckTofLayers(gsl::span<const CbmTofDigi> digis, size_t minNum) const
  {
    // A station is considered activated if it has at least one activated RPC. An RPC is considered
    // activated if it has at least one activated strip. A strip is considered activated
    // if it has digis on each side.
    std::unordered_set<int32_t> rpcs;          // active RPCs (address)
    std::unordered_set<int32_t> stations;      // active TOF stations
    std::unordered_map<int32_t, bool> strips;  // active strips
    for (auto& digi : digis) {
      const int32_t digiAddr  = digi.GetAddress();
      const int32_t stripAddr = CbmTofAddress::GetStripFullId(digiAddr);

      auto strip = strips.find(stripAddr);
      if (strip == strips.end())
        strips[stripAddr] = digi.GetSide();
      else {
        if (digi.GetSide() != strip->second) {  // Found other end => full strip, insert into counter set
          const int32_t rpcAddr = CbmTofAddress::GetRpcFullId(digiAddr);
          if (rpcs.count(rpcAddr) == 0) {                                     // new RPC activated
            const int32_t TofStationId = fpTrackingSetup->GetTrackingStation<fles::Subsystem::TOF>(digiAddr);
            if (TofStationId < 0) {
              continue;
            }  // unused tracking station (BMON)
            stations.insert(TofStationId);
            if (stations.size() == minNum) break;
          }
        }
      }
    }

    if (stations.size() < minNum)
      return false;
    else
      return true;
  }
  // --------------------------------------------------------------------------


  // -----   Info to string   -------------------------------------------------
  std::string DigiEventSelector::ToString() const
  {
    std::stringstream out;
    out << "--- Using DigiEventSelector with";
    out << (fConfig.IsEmpty() ? " no selection criteria" : " selection criteria: ");
    if (!fConfig.fMinNumDigis.empty()) {
      out << "\n   min. digis : ";
      for (const auto& entry : fConfig.fMinNumDigis)
        out << ::ToString(entry.first) << " " << entry.second << "  ";
    }
    if (!fConfig.fMinNumLayers.empty()) {
      out << "\n   min. layers: ";
      for (const auto& entry : fConfig.fMinNumLayers)
        out << ::ToString(entry.first) << " " << entry.second << "  ";
    }
    return out.str();
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::algo::evbuild
