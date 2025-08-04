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

using namespace std;

CBM_YAML_INSTANTIATE(cbm::algo::tof::ReadoutSetup);

namespace cbm::algo::tof
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

        const uint32_t asicId =
          pars.CheckInnerComp(equipment) ? ElinkIdxToGet4IdxInner(elink, pars) : ElinkIdxToGet4Idx(elink, pars);

        for (uint16_t channel = 0; channel < pars.nChannelsPerAsic; channel++) {
          const uint32_t febId     = (asicId / pars.nAsicsPerFeb);
          const uint32_t chanInFeb = (asicId % pars.nAsicsPerFeb) * pars.nChannelsPerAsic + channel;
          const uint32_t remappedChan =
            comp * numChanPerComp + febId * pars.NChansPerFeb() + Get4ChanToPadiChan(chanInFeb, pars);
          const uint32_t chanUId                 = fviRpcChUId[remappedChan];
          fReadoutMap[equipment][elink][channel] = chanUId;
        }  //# channel
      }    //# elink
    }      //# component
  }

  int32_t ReadoutConfig::ElinkIdxToGet4Idx(uint32_t elink, const ReadoutSetup& pars)
  {
    if (gdpbv100::kuChipIdMergedEpoch == elink) {
      return elink;
    }
    else if (elink < pars.NElinksPerComponent()) {
      return pars.elink2Asic[elink % pars.NElinksPerCrob()] + pars.NElinksPerCrob() * (elink / pars.NElinksPerCrob());
    }
    else {
      throw FatalError("tof::ReadoutConfig::ElinkIdxToGet4Idx => Index out of bound, {} vs {}, stop there!", elink,
                       static_cast<uint32_t>(pars.NElinksPerComponent()));
    }
  }

  int32_t ReadoutConfig::ElinkIdxToGet4IdxInner(uint32_t elink, const ReadoutSetup& pars)
  {
    if (0 == pars.elink2AsicInner.size()) {
      throw FatalError("tof::ReadoutConfig::ElinkIdxToGet4IdxInner => map is empty, stop there, check your Config!");
    }
    else if (gdpbv100::kuChipIdMergedEpoch == elink) {
      return elink;
    }
    else if (elink < pars.NElinksPerComponent()) {
      return pars.elink2AsicInner[elink % pars.NElinksPerCrob()]
             + pars.NElinksPerCrob() * (elink / pars.NElinksPerCrob());
    }
    else {
      throw FatalError("tof::ReadoutConfig::ElinkIdxToGet4IdxInner => Index out of bound, {} vs {}, stop there!", elink,
                       static_cast<uint32_t>(pars.NElinksPerComponent()));
    }
  }
  // -------------------------------------------------------------------------


  // -------------------------------------------------------------------------
  int32_t ReadoutConfig::Get4ChanToPadiChan(uint32_t channelInFee, const ReadoutSetup& pars)
  {
    if (channelInFee < pars.NChansPerFeb()) {
      return pars.asic2PadI[channelInFee];
    }
    else {
      throw FatalError("CbmMcbm2018TofPar::Get4ChanToPadiChan => Index out of bound, {} vs {}, returning crazy value!",
                       channelInFee, static_cast<uint32_t>(pars.NChansPerFeb()));
    }
  }
  // -------------------------------------------------------------------------

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMap(const ReadoutSetup& pars)
  {
    uint32_t uNrOfGet4     = pars.NComponents() * pars.nFebsPerComponent * pars.nAsicsPerFeb;
    uint32_t uNrOfChannels = uNrOfGet4 * pars.nChannelsPerAsic;
    fviRpcChUId.resize(uNrOfChannels);

    // D.Smith 03.06.2024: TO DO. Clarify
    uint32_t nbRobPerComp = 2;  // number of Gbtx per Gdpb (flim) for the final channel count check
    if (pars.elink2AsicInner.size() > 0) {
      nbRobPerComp = 1;  // Hack for 2024 TOF mapping
    }

    L_(debug) << "============================================================";
    L_(debug) << "================== TOF Mapping =============================";

    uint32_t uCh = 0;
    for (uint32_t uGbtx = 0; uGbtx < pars.NCrobs(); ++uGbtx) {
      uint32_t uCh0    = uCh;
      uint32_t uGdpb   = uCh0 / (pars.nFebsPerComponent * pars.nAsicsPerFeb * pars.nChannelsPerAsic);
      const auto& crob = pars.crobs.at(uGbtx);
      switch (crob.rpcType) {
        case 2:  // intended fall-through
        case 1:  // intended fall-through
        case 0: {
          // CBM modules
          BuildChannelsUidMapCbm(uCh, crob);
          break;
        }
        case 11: {
          // STAR eTOF  modules
          BuildChannelsUidMapStar(uCh, crob);
          break;
        }
        case 78: {
          // cern-20-gap + ceramic module
          BuildChannelsUidMapCern(uCh, crob);
        }
          [[fallthrough]];  // fall through is intended
        case 8:             // ceramics
        {
          BuildChannelsUidMapCera(uCh, crob);
          break;
        }
        case 4:  // intended fallthrough
          [[fallthrough]];
        case 7: [[fallthrough]];
        case 9:  // Star2 boxes
        {
          if ((pars.eqIds[uGdpb] & 0xF000) == 0xB000) {
            BuildChannelsUidMapStar2Inner(uCh, crob);
          }
          else {
            BuildChannelsUidMapStar2(uCh, crob);
          }
          break;
        }
        case 6:  // Buc box
        {
          if ((pars.eqIds[uGdpb] & 0xF000) == 0xB000) {
            BuildChannelsUidMapStar2Inner(uCh, crob);
          }
          else {
            BuildChannelsUidMapBuc(uCh, crob);
          }
          break;
        }
        case 66:  // Buc box 2024
        {
          BuildChannelsUidMapBuc(uCh, crob);
          break;
        }
        case 69: {
          /// 2022 case: 69 is followed by 4 and 9
          BuildChannelsUidMapBuc(uCh, crob);
          /// Map also 4 and 9 (equivalent to fallthrough to 4 then 9 but without changing past behaviors)
          uCh -= 80;  // PAL, 2022/03/17: ?!?
          BuildChannelsUidMapStar2(uCh, crob);
          uCh -= 80;  // PAL, 2022/03/17: ?!?
          break;
        }
        case -1: {
          L_(warning) << " Found unused GBTX link at uCh = " << uCh;
          uCh += 160;
          break;
        }
        default: {
          L_(error) << "Invalid Tof Type specifier for GBTx " << std::setw(2) << uGbtx << ": " << crob.rpcType;
        }
      }  // switch (crob.rpcType)

      if (uCh - uCh0 != pars.nFebsPerComponent * pars.nAsicsPerFeb * pars.nChannelsPerAsic / nbRobPerComp) {
        throw FatalError("Tof mapping error for Gbtx {},  diff = {}, type {}", uGbtx, uCh - uCh0, crob.rpcType);
      }

      L_(debug) << " Map for CROB " << uGbtx;
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


  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapCbm(uint32_t& uCh, const CROB& crob)
  {
    if (crob.rpcSide < 4) {  // mTof modules
      const int32_t RpcMap[5]    = {4, 2, 0, 3, 1};
      const int32_t RpcMapInv[5] = {0, 2, 4, 1, 3};
      for (int32_t iRpc = 0; iRpc < crob.nRPC; iRpc++) {
        int32_t iStrMax  = 32;
        uint32_t uChNext = 1;
        int32_t iRpcMap  = -1;
        if (crob.rpcSide < 2) {
          iRpcMap = RpcMap[iRpc];
        }
        else {
          iRpcMap = RpcMapInv[iRpc];
        }

        for (int32_t iStr = 0; iStr < iStrMax; iStr++) {
          int32_t iStrMap = iStr;

          if (crob.rpcSide % 2 == 0) {
            iStrMap = 31 - iStr;
          }
          if (crob.moduleId > -1)
            fviRpcChUId.at(uCh) =
              CbmTofAddress::GetUniqueAddress(crob.moduleId, iRpcMap, iStrMap, crob.rpcSide % 2, crob.rpcType);
          else
            fviRpcChUId.at(uCh) = 0;
          uCh += uChNext;
        }  // for (int32_t iStr = 0; iStr < iStrMax; iStr++)
      }    // for (int32_t iRpc = 0; iRpc < crob.nRPC; iRpc++)
    }      // if (crob.rpcSide < 2)
  }


  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapStar(uint32_t& uCh, const CROB& crob)
  {
    if (crob.rpcSide < 2) {
      // mTof modules
      L_(debug) << "Start eTOF module side " << crob.rpcSide << " at " << uCh;
      const int32_t RpcMap[3] = {0, 1, 2};
      for (int32_t iRpc = 0; iRpc < crob.nRPC; iRpc++) {
        int32_t iStrMax = 32;
        int32_t uChNext = 1;

        for (int32_t iStr = 0; iStr < iStrMax; iStr++) {
          int32_t iStrMap = iStr;
          int32_t iRpcMap = RpcMap[iRpc];

          if (crob.rpcSide == 0) iStrMap = 31 - iStr;
          if (crob.moduleId > -1)
            fviRpcChUId[uCh] =
              CbmTofAddress::GetUniqueAddress(crob.moduleId, iRpcMap, iStrMap, crob.rpcSide, crob.rpcType);
          else
            fviRpcChUId[uCh] = 0;
          uCh += uChNext;
        }
      }
    }
    uCh += 64;
  }

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapCern(uint32_t& uCh, const CROB&)
  {
    L_(debug) << " Map CERN 20 gap  at GBTX  -  uCh = " << uCh;
    // clang-format off
  const int32_t StrMap[32] = {0,  1,  2,  3,  4,  31, 5,  6,  7,  30, 8,
                            9,  10, 29, 11, 12, 13, 14, 28, 15, 16, 17,
                            18, 27, 26, 25, 24, 23, 22, 21, 20, 19};
    // clang-format on
    int32_t iModuleId   = 0;
    int32_t iModuleType = 7;
    int32_t iRpcMap     = 0;
    for (int32_t iFeet = 0; iFeet < 2; iFeet++) {
      for (int32_t iStr = 0; iStr < 32; iStr++) {
        int32_t iStrMap  = 31 - 12 - StrMap[iStr];
        int32_t iSideMap = iFeet;
        if (iStrMap < 20)
          fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(iModuleId, iRpcMap, iStrMap, iSideMap, iModuleType);
        else
          fviRpcChUId[uCh] = 0;
        uCh++;
      }
    }
    L_(debug) << " Map end CERN 20 gap  at GBTX  -  uCh = " << uCh;
  }

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapCera(uint32_t& uCh, const CROB&)
  {
    int32_t iModuleId   = 0;
    int32_t iModuleType = 8;
    for (int32_t iRpc = 0; iRpc < 8; iRpc++) {
      fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(iModuleId, 7 - iRpc, 0, 0, iModuleType);
      uCh++;
    }
    uCh += (24 + 4 * 32);
    L_(debug) << " Map end ceramics  box  at GBTX  -  uCh = " << uCh;
  }


  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapStar2(uint32_t& uCh, const CROB& crob)
  {
    const int32_t iRpc[5]  = {1, -1, 1, 0, 0};
    const int32_t iSide[5] = {1, -1, 0, 1, 0};
    for (int32_t iFeet = 0; iFeet < 5; iFeet++) {
      for (int32_t iStr = 0; iStr < 32; iStr++) {
        int32_t iStrMap  = iStr;
        int32_t iRpcMap  = iRpc[iFeet];
        int32_t iSideMap = iSide[iFeet];
        if (iSideMap == 0) iStrMap = 31 - iStr;
        switch (crob.rpcSide) {
          case 0:; break;
          case 1:;
            iRpcMap = 1 - iRpcMap;  // swap counters
            break;
          case 2:
            switch (iFeet) {
              case 1:
                iRpcMap  = iRpc[4];
                iSideMap = iSide[4];
                iStrMap  = 31 - iStrMap;
                break;
              case 4:
                iRpcMap  = iRpc[1];
                iSideMap = iSide[1];
                break;
              default:;
            }
            break;
          case 3:  // direct beam 20210524
            switch (iFeet) {
              case 0:
                iRpcMap  = 0;
                iSideMap = 0;
                iStrMap  = iStr;
                break;
              case 1:
                iRpcMap  = 0;
                iSideMap = 1;
                iStrMap  = 31 - iStr;
                break;
              default: iSideMap = -1;
            }
            break;
        }
        if (iSideMap > -1)
          fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(crob.moduleId, iRpcMap, iStrMap, iSideMap, crob.rpcType);
        else
          fviRpcChUId[uCh] = 0;
        uCh++;
      }
    }
  }

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapStar2Inner(uint32_t& uCh, const CROB& crob)
  {
    if (crob.rpcSide < 3) {
      int32_t NrFeet = 2;
      if (crob.rpcSide < 2) NrFeet = 1;
      int32_t iFeet = 0;
      for (; iFeet < NrFeet; iFeet++) {
        for (int32_t iStr = 0; iStr < 32; iStr++) {
          int32_t iStrMap  = iStr;
          int32_t iRpcMap  = crob.nRPC;
          int32_t iSideMap = crob.rpcSide;
          if (crob.rpcSide == 2) {
            if (iFeet == 0)
              iSideMap = 0;
            else
              iSideMap = 1;
          }
          if (crob.rpcType != 6)
            if (iSideMap == 0) iStrMap = 31 - iStr;

          if (iSideMap > -1)
            fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(crob.moduleId, iRpcMap, iStrMap, iSideMap, crob.rpcType);
          else
            fviRpcChUId[uCh] = 0;

          uCh++;
        }
      }
      while (iFeet < 2) {
        for (int32_t iStr = 0; iStr < 32; iStr++) {
          fviRpcChUId[uCh] = 0;
          uCh++;
        }
        iFeet++;
      }
    }
    else {
      if (crob.rpcSide == 3) {
        int iSideMap = -1;
        int iStrMap  = -1;
        int iRpcMap  = -1;
        for (int32_t iFeet = 0; iFeet < 5; iFeet++) {
          for (int32_t iStr = 0; iStr < 32; iStr++) {
            switch (iFeet) {
              case 0: iSideMap = -1; break;
              case 1:
                iRpcMap  = 0;
                iStrMap  = iStr;
                iSideMap = 0;
                break;
              case 2:
                iRpcMap  = 0;
                iStrMap  = iStr;
                iSideMap = 1;
                break;
              case 3:
                iRpcMap  = 1;
                iStrMap  = iStr;
                iSideMap = 0;
                break;
              case 4:
                iRpcMap  = 1;
                iStrMap  = iStr;
                iSideMap = 1;
                break;
            }
            if (iSideMap > -1)
              fviRpcChUId[uCh] =
                CbmTofAddress::GetUniqueAddress(crob.moduleId, iRpcMap, iStrMap, iSideMap, crob.rpcType);
            else
              fviRpcChUId[uCh] = 0;
            uCh++;
          }
        }
      }
      else if (crob.rpcSide == 4) {
        int iSideMap        = -1;
        int iStrMap         = -1;
        int iRpcMap         = -1;
        const int ConOff[8] = {0, 2, 4, 6, 7, 1, 3, 5};  //Get4 after Gbtx
        for (int32_t iFeet = 0; iFeet < 5; iFeet++) {
          for (int32_t iStr = 0; iStr < 32; iStr++) {
            switch (iFeet) {
              case 0: iSideMap = -1; break;
              case 1:
                iRpcMap  = 0;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 0;
                break;
              case 2:
                iRpcMap  = 0;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 1;
                break;
              case 3:
                iRpcMap  = 1;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 1;
                break;
              case 4:
                iRpcMap  = 1;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 0;
                break;
            }
            if (iSideMap > -1)
              fviRpcChUId[uCh] =
                CbmTofAddress::GetUniqueAddress(crob.moduleId, iRpcMap, iStrMap, iSideMap, crob.rpcType);
            else
              fviRpcChUId[uCh] = 0;
            uCh++;
          }
        }
      }
      else if (crob.rpcSide == 5) {
        int iSideMap        = -1;
        int iStrMap         = -1;
        int iRpcMap         = -1;
        const int ConOff[8] = {0, 2, 4, 6, 7, 1, 3, 5};  // Get4 after Gbtx
        for (int32_t iFeet = 0; iFeet < 5; iFeet++) {
          for (int32_t iStr = 0; iStr < 32; iStr++) {
            switch (iFeet) {
              case 0: iSideMap = -1; break;
              case 1:
                iRpcMap  = 0;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iStrMap  = 31 - iStrMap;
                iSideMap = 1;
                break;
              case 2:
                iRpcMap  = 0;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iStrMap  = 31 - iStrMap;
                iSideMap = 0;
                break;
              case 3:
                iRpcMap  = 1;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iStrMap  = 31 - iStrMap;
                iSideMap = 0;
                break;
              case 4:
                iRpcMap  = 1;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iStrMap  = 31 - iStrMap;
                iSideMap = 1;
                break;
            }
            if (iSideMap > -1)
              fviRpcChUId[uCh] =
                CbmTofAddress::GetUniqueAddress(crob.moduleId, iRpcMap, iStrMap, iSideMap, crob.rpcType);
            else
              fviRpcChUId[uCh] = 0;
            uCh++;
          }
        }
      }
    }
  }

  // -------------------------------------------------------------------------
  void ReadoutConfig::BuildChannelsUidMapBuc(uint32_t& uCh, const CROB& crob)
  {
    int32_t iModuleIdMap   = crob.moduleId;
    const int32_t iRpc[5]  = {0, -1, 0, 1, 1};
    const int32_t iSide[5] = {1, -1, 0, 1, 0};
    for (int32_t iFeet = 0; iFeet < 5; iFeet++) {
      for (int32_t iStr = 0; iStr < 32; iStr++) {
        int32_t iStrMap  = iStr;
        int32_t iRpcMap  = iRpc[iFeet];
        int32_t iSideMap = iSide[iFeet];
        switch (crob.rpcSide) {
          case 0:; break;
          case 1:  // HD cosmic 2019, Buc2018, v18n
            iStrMap = 31 - iStr;
            iRpcMap = 1 - iRpcMap;
            break;
          case 2:  // v18m_cosmicHD
            //   iStrMap=31-iStr;
            iSideMap = 1 - iSideMap;
            break;
          case 3: {
            const int ConOff[8] = {0, 2, 4, 6, 7, 1, 3, 5};  // Get4 after Gbtx
            switch (iFeet) {
              case 0: iSideMap = -1; break;
              case 1:
                iRpcMap  = 0;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 0;
                break;
              case 2:
                iRpcMap  = 0;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 1;
                break;
              case 3:
                iRpcMap  = 1;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 1;
                break;
              case 4:
                iRpcMap  = 1;
                iStrMap  = 3 - iStr % 4 + 4 * ConOff[iStr / 4];
                iSideMap = 0;
                break;
            }
          } break;
          case 4:  // HD cosmic 2019, Buc2018, v18o
            iRpcMap = 1 - iRpcMap;
            break;
          case 5:  // Buc2025, mCBM
            iRpcMap  = 1 - iRpcMap;
            iSideMap = 1 - iSideMap;
            break;
          case 55:  // HD cosmic 2020, Buc2018, v20a
            iStrMap = 31 - iStr;
            break;
          case 6:  //BUC special
          {
            switch (crob.moduleId) {
              case 0: iRpcMap = 0; break;
              case 1: iRpcMap = 1; break;
            }
            if (iFeet % 2 == 1)
              iModuleIdMap = 1;
            else
              iModuleIdMap = 0;

            switch (iFeet) {
              case 0:
              case 3: iSideMap = 0; break;
              case 1:
              case 2: iSideMap = 1; break;
            }
          } break;
          case 8: {
            // Special case for two channels in 2022
            // Fallthrough to 7 for all other channels
            if (2 == iFeet) {
              if (7 == iStr) {
                ///                                               SM Rpc St Si Type
                fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(0, 0, 0, 0, 8);
                uCh++;
                continue;
              }
              else if (23 == iStr) {
                ///                                               SM Rpc St Si Type
                fviRpcChUId[uCh] = CbmTofAddress::GetUniqueAddress(1, 0, 0, 0, 8);
                uCh++;
                continue;
              }
            }
          }
            [[fallthrough]];  // fall through is intended
          case 7: {
            // clang-format off
          const int32_t iChMap[160]={
          124, 125, 126, 127,  12,  13,  14,  15,   4,   5,   6,   7,  28,  29,  30,  31, 120, 121, 122, 123,   8,  9,   10,  11, 104, 105, 106, 107, 108, 109, 110, 111,
           36,  37,  38,  39,  52,  53,  54,  55,  60,  61,  62,  63, 128, 129, 130, 131,  40,  41,  42,  43, 148, 149, 150, 151,  56,  57,  58,  59, 132, 133, 134, 135,
          136, 137, 138, 139, 140, 141, 142, 143,  96,  97,  98,  99,  64,  65,  66,  67, 100, 101, 102, 103,  84,  85,  86,  87, 152, 153, 154, 155,  68,  69,  70,  71,
          156, 157, 158, 159, 144, 145, 146, 147,  44,  45,  46,  47,  76,  77,  78,  79,  48,  49,  50,  51,  20,  21,  22,  23,  32,  33,  34,  35, 116, 117, 118, 119,
           75,  74,  73,  72,  92,  93,  94,  95,  16,  17,  18,  19,  80,  81,  82,  83, 115, 114, 113, 112,  24,  25,  26,  27,  88,  89,  90,  91,   0,   1,   2,   3
          };
            // clang-format on
            int32_t iInd = iFeet * 32 + iStr;
            int32_t i    = 0;
            for (; i < 160; i++)
              if (iInd == iChMap[i]) break;
            iStrMap          = i % 32;
            int32_t iFeetInd = (i - iStrMap) / 32;
            switch (iFeetInd) {
              case 0:
                iRpcMap  = 0;
                iSideMap = 1;
                break;
              case 1:
                iRpcMap  = 1;
                iSideMap = 1;
                break;
              case 2:
                iRpcMap  = 1;
                iSideMap = 0;
                break;
              case 3:
                iRpcMap  = 0;
                iSideMap = 0;
                break;
              case 4: iSideMap = -1; break;
            }
            iModuleIdMap = crob.moduleId;
          } break;
          default:;
        }  // switch (crob.rpcSide)
        if (iSideMap > -1)
          fviRpcChUId[uCh] =
            CbmTofAddress::GetUniqueAddress(iModuleIdMap, iRpcMap, iStrMap, iSideMap, crob.rpcType % 10);
        else
          fviRpcChUId[uCh] = 0;

        uCh++;
      }  // for (int32_t iStr = 0; iStr < 32; iStr++)
    }    // for (int32_t iFeet = 0; iFeet < 5; iFeet++)
  }
  // -------------------------------------------------------------------------

}  // namespace cbm::algo::tof
