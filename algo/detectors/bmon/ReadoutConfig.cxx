/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "ReadoutConfig.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTofAddress.h"
#include "Exceptions.h"
#include "gDpbMessv100.h"

#include <bitset>
#include <iomanip>

#include <fmt/format.h>

using namespace std;

namespace cbm::algo::bmon
{
  // ---  Constructor  ------------------------------------------------------------------
  ReadoutConfig::ReadoutConfig(const ReadoutSetup& pars) { Init(pars); }
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

  // ---   Number of elinks for a component / equipment   -------------------------------
  size_t ReadoutConfig::GetNumElinks(uint16_t equipmentId)
  {
    size_t result = 0;
    auto it       = fReadoutMap.find(equipmentId);
    if (it != fReadoutMap.end()) result = fReadoutMap[equipmentId].size();
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---  Mapping (equimentId, elink) -> address[channel]  ------------------------------
  std::vector<uint32_t> ReadoutConfig::Map(uint16_t equipmentId, uint16_t elinkId)
  {
    std::vector<uint32_t> result;
    auto equipIter = fReadoutMap.find(equipmentId);
    if (equipIter != fReadoutMap.end()) {
      if (elinkId < equipIter->second.size()) {
        result = equipIter->second.at(elinkId);
      }
    }
    return result;
  }
  // ------------------------------------------------------------------------------------


  // ---  Mapping (equimentId, elink) -> time offset  -------------------------------------
  int32_t ReadoutConfig::GetElinkTimeOffset(uint16_t equipmentId, uint16_t elinkId)
  {
    int32_t result = 0;
    auto equipIter = fTimeOffsetMap.find(equipmentId);
    if (equipIter != fTimeOffsetMap.end()) {
      if (elinkId < equipIter->second.size()) {
        result = equipIter->second.at(elinkId);
      }
    }
    return result;
  }
  // ------------------------------------------------------------------------------------


  void ReadoutConfig::Init(const ReadoutSetup& pars)
  {
    fTimeOffset = pars.timeOffset;

    // Constructing the map (equipmentId, eLink, channel) -> (TOF address)
    const uint32_t numChanPerComp = pars.NChansPerComponent();

    // Constructs the fviRpcChUId array
    BuildChannelsUidMap(pars);

    for (uint16_t comp = 0; comp < pars.NComponents(); comp++) {

      uint16_t equipment = pars.eqIds.at(comp);
      fReadoutMap[equipment].resize(pars.NElinksPerComponent());
      fTimeOffsetMap[equipment].resize(pars.NElinksPerComponent());

      for (uint16_t elink = 0; elink < pars.NElinksPerComponent(); elink++) {
        fReadoutMap[equipment][elink].resize(pars.nChannelsPerAsic);

        const uint32_t crob              = elink / pars.NElinksPerCrob() + comp * pars.NCrobsPerComponent();
        fTimeOffsetMap[equipment][elink] = pars.crobs[crob].timeOffset;

        for (uint16_t channel = 0; channel < pars.nChannelsPerAsic; channel++) {

          const uint32_t chanInComp = elink * pars.nChannelsPerAsic + channel;
          uint32_t chanInSys        = comp * numChanPerComp + chanInComp;

          {  // hack? perhaps can be removed
            const int numFullFlims = 8;
            if (comp > numFullFlims) {
              chanInSys -= (comp - numFullFlims) * numChanPerComp / 2;
            }
          }

          const uint32_t chanUId                 = fviRpcChUId[chanInSys];
          fReadoutMap[equipment][elink][channel] = chanUId;
        }  //# channel
      }    //# elink
    }      //# component
  }

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMap(const ReadoutSetup& pars)
  {
    const uint32_t numAsics = pars.NComponents() * pars.nFebsPerComponent * pars.nAsicsPerFeb;
    const uint32_t numChan  = numAsics * pars.nChannelsPerAsic;
    fviRpcChUId.resize(numChan);

    L_(debug) << "============================================================";
    L_(debug) << "================== BMON Mapping ============================";

    uint32_t uCh = 0;
    for (uint32_t uGbtx = 0; uGbtx < pars.NCrobs(); ++uGbtx) {
      const uint32_t uCh0 = uCh;
      const auto& crob    = pars.crobs.at(uGbtx);
      switch (crob.rpcType) {
        case 5: {
          /// Special Treatment for the Bmon diamond
          BuildChannelsUidMapBmon(uCh, uGbtx, pars);
          break;
        }
        case 99: {
          /// Special Treatment for the 2022 Bmon diamond, keep past behavior for older data!
          BuildChannelsUidMapBmon_2022(uCh, uGbtx, pars);
          break;
        }
        case -1: {
          L_(warning) << " Found unused GBTX link at uCh = " << uCh;
          uCh += 160;
          break;
        }
        default: {
          L_(error) << "Invalid Bmon Type specifier for GBTx " << std::setw(2) << uGbtx << ": " << crob.rpcType;
        }
      }  // switch (crob.rpcType)
      if ((int32_t)(uCh - uCh0) != pars.nFebsPerComponent * pars.nAsicsPerFeb * pars.nChannelsPerAsic / 2) {
        throw FatalError("Bmon mapping error for Gbtx {},  diff = {}, expected = {}", uGbtx, uCh - uCh0,
                         pars.nFebsPerComponent * pars.nAsicsPerFeb * pars.nChannelsPerAsic / 2);
      }

      L_(info) << " Map for CROB " << uGbtx;
      std::vector<int32_t> vAddrBunch(8, -1);
      for (uint32_t uChPrint = uCh0; uChPrint < uCh; ++uChPrint) {
        vAddrBunch[uChPrint % 8] = fviRpcChUId[uChPrint];
        if (7 == uChPrint % 8) {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << " 0x" << std::setw(8)
                    << vAddrBunch[1] << " 0x" << std::setw(8) << vAddrBunch[2] << " 0x" << std::setw(8) << vAddrBunch[3]
                    << " 0x" << std::setw(8) << vAddrBunch[4] << " 0x" << std::setw(8) << vAddrBunch[5] << " 0x"
                    << std::setw(8) << vAddrBunch[6] << " 0x" << std::setw(8) << vAddrBunch[7] << std::dec;
        }
      }
      switch (uCh % 8) {
        case 0: {
          break;
        }
        case 1: {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << std::dec;
          break;
        }
        case 2: {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << " 0x" << std::setw(8)
                    << vAddrBunch[1] << std::dec;
          break;
        }
        case 3: {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << " 0x" << std::setw(8)
                    << vAddrBunch[1] << " 0x" << std::setw(8) << vAddrBunch[2] << std::dec;
          break;
        }
        case 4: {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << " 0x" << std::setw(8)
                    << vAddrBunch[1] << " 0x" << std::setw(8) << vAddrBunch[2] << " 0x" << std::setw(8) << vAddrBunch[3]
                    << std::dec;
          break;
        }
        case 5: {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << " 0x" << std::setw(8)
                    << vAddrBunch[1] << " 0x" << std::setw(8) << vAddrBunch[2] << " 0x" << std::setw(8) << vAddrBunch[3]
                    << " 0x" << std::setw(8) << vAddrBunch[4] << std::dec;
          break;
        }
        case 6: {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << " 0x" << std::setw(8)
                    << vAddrBunch[1] << " 0x" << std::setw(8) << vAddrBunch[2] << " 0x" << std::setw(8) << vAddrBunch[3]
                    << " 0x" << std::setw(8) << vAddrBunch[4] << " 0x" << std::setw(8) << vAddrBunch[5] << std::dec;
          break;
        }
        case 7: {
          L_(debug) << std::hex << std::setfill('0') << "0x" << std::setw(8) << vAddrBunch[0] << " 0x" << std::setw(8)
                    << vAddrBunch[1] << " 0x" << std::setw(8) << vAddrBunch[2] << " 0x" << std::setw(8) << vAddrBunch[3]
                    << " 0x" << std::setw(8) << vAddrBunch[4] << " 0x" << std::setw(8) << vAddrBunch[5] << " 0x"
                    << std::setw(8) << vAddrBunch[6] << std::dec;
          break;
        }
      }
    }  // for (UInt_t uGbtx = 0; uGbtx < numCrob; ++uGbtx)

    L_(debug) << "============================================================";
  }

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapBmon(uint32_t& uCh, uint32_t uGbtx, const ReadoutSetup& pars)
  {
    const auto& crob = pars.crobs.at(uGbtx);
    L_(debug) << " Map diamond " << crob.moduleId << " at GBTX " << uGbtx << " -  uCh = " << uCh;
    for (uint32_t uGet4 = 0; uGet4 < pars.NElinksPerCrob(); ++uGet4) {
      for (uint32_t uGet4Ch = 0; uGet4Ch < pars.nChannelsPerAsic; ++uGet4Ch) {
        /// Mapping for the 2022 beamtime
        if (uGet4 < 32 && 0 == uGet4Ch && -1 < crob.moduleId) {
          uint32_t uChannelBmon = uGet4 + 32 * crob.rpcSide;
          uChannelBmon /= 8;
          fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(crob.moduleId, 0, uChannelBmon, 0, crob.rpcType);
          L_(debug) << "  Bmon channel: " << uChannelBmon << " from GBTx " << uGbtx << " , indx " << uCh
                    << " address 0x" << std::hex << std::setfill('0') << std::setw(8) << fviRpcChUId[uCh] << std::dec;
        }  // Valid Bmon channel
        else {
          fviRpcChUId[uCh] = 0;
        }  // Invalid Bmon channel
        uCh++;
      }
    }
  }

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapBmon_2022(uint32_t& uCh, uint32_t uGbtx, const ReadoutSetup& pars)
  {
    const auto& crob = pars.crobs.at(uGbtx);
    L_(debug) << " Map 2022 diamond " << crob.moduleId << " at GBTX " << uGbtx << " -  uCh = " << uCh;
    for (uint32_t uGet4 = 0; uGet4 < pars.NElinksPerCrob(); ++uGet4) {
      for (uint32_t uGet4Ch = 0; uGet4Ch < pars.nChannelsPerAsic; ++uGet4Ch) {
        /// Mapping for the 2022 beamtime
        if (-1 < crob.moduleId && uGet4 < 32 && 0 == uGet4 % 4 && 0 == uGet4Ch) {
          /// 1 channel per physical GET4, 2 links per physical GET4, 4 physical GET4s per GBTx, 1 GBTx per comp.
          /// 16 channels for one side, 16 for the other
          uint32_t uChannelBmon = (uGet4 / 8 + 4 * (uGbtx / 2)) % 16;
          /// Type hard-coded to allow different parameter values to separate 2022 Bmon and pre-2022 Bmon
          fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(crob.moduleId, 0, uChannelBmon, crob.rpcSide, 5);
          L_(debug) << "  Bmon channel: " << uChannelBmon << " side " << crob.rpcSide << " from GBTx " << uGbtx
                    << " , indx " << uCh << " address 0x" << std::hex << std::setfill('0') << std::setw(8)
                    << fviRpcChUId[uCh] << std::dec;
        }  // Valid Bmon channel
        else {
          fviRpcChUId[uCh] = 0;
        }  // Invalid Bmon channel
        uCh++;
      }
    }
  }
}  // namespace cbm::algo::bmon
