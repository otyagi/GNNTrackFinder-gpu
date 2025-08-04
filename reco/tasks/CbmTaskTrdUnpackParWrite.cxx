/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "CbmTaskTrdUnpackParWrite.h"

#include "CbmTrdParFasp.h"
#include "CbmTrdParModAsic.h"
#include "CbmTrdParModDigi.h"
#include "CbmTrdParSetAsic.h"
#include "CbmTrdParSetDigi.h"
#include "CbmTrdParSpadic.h"
#include "trd/ReadoutConfig.h"
#include "trd2d/ReadoutConfig.h"
#include "yaml/Yaml.h"

#include <FairParamList.h>
#include <fairlogger/Logger.h>

#include <fstream>

using namespace cbm::algo;

InitStatus CbmTaskTrdUnpackParWrite::Init()
{

  assert(fPars.digi != nullptr);
  auto& digiparset = *fPars.digi;

  assert(fPars.asic != nullptr);
  auto& asicparset = *fPars.asic;


  // TRD2D ===================================================================
  {
    // Output object
    cbm::algo::trd2d::ReadoutSetup trd2dSetup;

    // Map (moduleId) -> (array of crobId)
    std::map<uint32_t, std::vector<uint16_t>> crobMap;
    // Map (equipId, asicId, chanId) -> (pad address, mask flag, daq offset [FASP clk])
    std::map<size_t, std::map<size_t, std::map<size_t, std::tuple<int32_t, bool, int16_t, uint16_t>>>> channelMap;

    // Loop through a list of module IDs from the .digi file (can in principle contradict crob_map).
    for (auto entry : digiparset.GetModuleMap()) {

      const auto moduleId = entry.first;

      // Get ASIC parameters for this module
      CbmTrdParModAsic* setDet = static_cast<CbmTrdParModAsic*>(asicparset.GetModulePar(moduleId));
      if (!setDet) continue;
      if (setDet->GetAsicType() != CbmTrdDigi::eCbmTrdAsicType::kFASP) continue;
      auto digipar = entry.second;

      const int* crobs = setDet->GetCrobAddresses();
      for (int icrob(0); icrob < NCROBMOD; icrob++) {
        crobMap[moduleId].emplace_back(crobs[icrob] & 0xffff);
        // check if there is an extra fiber defined on this ROB (version 2025 -)
        uint16_t eq_id = (crobs[icrob] >> 16) & 0xffff;
        if (eq_id) crobMap[moduleId].emplace_back(eq_id);
      }
      // Loop through ASICs for this module
      std::vector<int32_t> addresses;
      setDet->GetAsicAddresses(&addresses);
      for (auto add : addresses) {
        //Get local IDs for this component / equipment.
        const int32_t fasp_in_mod = add - 1000 * moduleId;
        const int32_t fasp_in_eq  = fasp_in_mod % (NFASPCROB);
        const int32_t crob_in_mod = fasp_in_mod / (NFASPCROB);
        const uint16_t eq_id      = crobMap[moduleId][crob_in_mod];

        // ASIC parameter set
        CbmTrdParFasp* fasppar = (CbmTrdParFasp*) setDet->GetAsicPar(add);

        // Loop through channels for this ASIC and fill map
        for (int chan = 0; chan < fasppar->GetNchannels(); chan++) {
          const CbmTrdParFaspChannel* faspch = fasppar->GetChannel(chan);
          const int32_t pad                  = fasppar->GetPadAddress(chan) * (faspch->HasPairingR() ? 1 : -1);
          const bool isMasked                = faspch->IsMasked();
          uint8_t daq_offset                 = 0;
          uint8_t thres                      = 0;
          if (((CbmTrdParModDigi*) digipar)->GetPadRow(pad) % 2 == 0) daq_offset = 3;
          channelMap[eq_id][fasp_in_eq][chan] = std::make_tuple(pad, isMasked, daq_offset, thres);
        }
      }
    }

    trd2dSetup.InitComponentMap(crobMap);
    trd2dSetup.InitChannelMap(channelMap);

    // Apply system time offset
    // See <source_dir>/macro/run/run_unpack_tsa.C
    if (fPars.setup == Setup::mCBM2022 || fPars.setup == Setup::mCBM2024_03 || fPars.setup == Setup::mCBM2024_05) {
      // Ni 2022, Au 2022, Ni 2024
      trd2dSetup.SetSystemTimeOffset(-510);
    }

    std::ofstream("Trd2dReadoutSetup.yaml") << yaml::Dump{}(trd2dSetup);
  }

  // TRD1D ===================================================================
  {
    // Output object
    cbm::algo::trd::ReadoutConfig trdConfig;

    FairParamList parlist;
    asicparset.putParams(&parlist);

    std::vector<int> moduleIds(asicparset.GetNrOfModules());
    parlist.fill("ModuleId", moduleIds.data(), moduleIds.size());

    std::map<size_t, std::map<size_t, std::map<size_t, size_t>>> addressMap;  //[criId][crobId][elinkId] -> asicAddress
    std::map<size_t, std::map<size_t, std::map<size_t, std::map<size_t, size_t>>>>
      channelMap;  //[criId][crobId][elinkId][chanId] -> chanAddress

    for (auto module : moduleIds) {
      CbmTrdParModAsic* moduleSet = (CbmTrdParModAsic*) asicparset.GetModulePar(module);

      // Skip entries for "Fasp" modules in .asic.par file
      if (moduleSet->GetAsicType() != CbmTrdDigi::eCbmTrdAsicType::kSPADIC) continue;

      std::vector<int> asicAddresses;
      moduleSet->GetAsicAddresses(&asicAddresses);

      for (auto address : asicAddresses) {
        CbmTrdParSpadic* asicPar = (CbmTrdParSpadic*) moduleSet->GetAsicPar(address);
        const uint16_t criId     = asicPar->GetCriId();
        const uint8_t crobId     = asicPar->GetCrobId();
        const uint8_t elinkId    = asicPar->GetElinkId(0);
        if (elinkId >= 98) {
          continue;
        }  // Don't add not connected asics to the map
        addressMap[criId][crobId][elinkId]     = address;
        addressMap[criId][crobId][elinkId + 1] = address;

        const uint8_t numChans = 16;
        for (uint8_t chan = 0; chan < numChans; chan++) {
          auto asicChannelId                       = (elinkId % 2) == 0 ? chan : chan + numChans;
          auto chanAddr                            = asicPar->GetChannelAddresses().at(asicChannelId);
          channelMap[criId][crobId][elinkId][chan] = chanAddr;
        }
        for (uint8_t chan = 0; chan < numChans; chan++) {
          auto asicChannelId                           = (elinkId + 1 % 2) == 0 ? chan : chan + numChans;
          auto chanAddr                                = asicPar->GetChannelAddresses().at(asicChannelId);
          channelMap[criId][crobId][elinkId + 1][chan] = chanAddr;
        }
        LOG(debug) << "componentID " << asicPar->GetComponentId() << " "
                   << "address " << address << " key " << criId << " " << unsigned(crobId) << " " << unsigned(elinkId);
      }
    }
    trdConfig.Init(addressMap, channelMap);

    // Apply system time offset
    // See <source_dir>/macro/run/run_unpack_tsa.C
    if (fPars.setup == Setup::mCBM2022 || fPars.setup == Setup::mCBM2024_03 || fPars.setup == Setup::mCBM2024_05) {
      // Ni 2022, Au 2022, Ni 2024
      trdConfig.SetSystemTimeOffset(1300);
    }

    // Apply time offset per elink
    // See: https://git.cbm.gsi.de/computing/cbmroot/-/merge_requests/1751
    // and <source_dir>/macro/run/run_unpack_tsa.C
    if (fPars.setup == Setup::mCBM2024_05) {
      for (int elinkId = 0; elinkId < 36; ++elinkId) {
        trdConfig.SetElinkTimeOffset(20736, elinkId, -36);
        trdConfig.SetElinkTimeOffset(20737, elinkId, -37);
        trdConfig.SetElinkTimeOffset(20738, elinkId, -65);
        trdConfig.SetElinkTimeOffset(20739, elinkId, -52);
        trdConfig.SetElinkTimeOffset(20740, elinkId, -50);
        trdConfig.SetElinkTimeOffset(20741, elinkId, -49);

        //trdConfig.SetElinkTimeOffset(20992, elinkId, 0); //no correlation
        trdConfig.SetElinkTimeOffset(20993, elinkId, -50);
        trdConfig.SetElinkTimeOffset(20994, elinkId, -57);
        trdConfig.SetElinkTimeOffset(20995, elinkId, -52);
        trdConfig.SetElinkTimeOffset(20996, elinkId, -52);
        trdConfig.SetElinkTimeOffset(20997, elinkId, -53);
      }
    }

    std::ofstream("TrdReadoutSetup.yaml") << yaml::Dump{}(trdConfig);
  }

  return kSUCCESS;
}
