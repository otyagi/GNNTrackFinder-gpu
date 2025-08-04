/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "DigiEventSelector.h"
#include "DigiEventSelectorConfig.h"
#include "MicrosliceDescriptor.hpp"
#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"

#include <unordered_set>

#include <yaml-cpp/yaml.h>

using namespace cbm::algo;

TEST(_GTestDigiEventSelector, CheckDigiEventSelectorAlgorithmSimple)
{
  SCOPED_TRACE("CheckDigiEventSelectorAlgorithSimple");


  // --- Test number of digis per system
  size_t nBmon  = 1;
  size_t nSts   = 2;
  size_t nMuch  = 3;
  size_t nRich  = 4;
  size_t nTrd   = 5;
  size_t nTrd2d = 6;
  size_t nTof   = 7;
  size_t nPsd   = 8;
  size_t nFsd   = 9;

  DigiEvent event;
  for (size_t i = 0; i < nBmon; i++)
    event.fBmon.push_back(CbmBmonDigi());
  for (size_t i = 0; i < nSts; i++)
    event.fSts.push_back(CbmStsDigi());
  for (size_t i = 0; i < nMuch; i++)
    event.fMuch.push_back(CbmMuchDigi());
  for (size_t i = 0; i < nRich; i++)
    event.fRich.push_back(CbmRichDigi());
  for (size_t i = 0; i < nTrd; i++)
    event.fTrd.push_back(CbmTrdDigi());
  for (size_t i = 0; i < nTrd2d; i++)
    event.fTrd2d.push_back(CbmTrdDigi());
  for (size_t i = 0; i < nTof; i++)
    event.fTof.push_back(CbmTofDigi());
  for (size_t i = 0; i < nPsd; i++)
    event.fPsd.push_back(CbmPsdDigi());
  for (size_t i = 0; i < nFsd; i++)
    event.fFsd.push_back(CbmFsdDigi());

  // Tracking setup
  auto trackingSetup = std::make_shared<cbm::algo::TrackingSetup>();
  trackingSetup->Use(fles::Subsystem::TOF);  // Init TOF withot a context
  trackingSetup->Init();


  YAML::Node node;
  node["minDigis"][ToString(ECbmModuleId::kBmon)]  = nBmon;
  node["minDigis"][ToString(ECbmModuleId::kSts)]   = nSts;
  node["minDigis"][ToString(ECbmModuleId::kMuch)]  = nMuch;
  node["minDigis"][ToString(ECbmModuleId::kRich)]  = nRich;
  node["minDigis"][ToString(ECbmModuleId::kTrd)]   = nTrd;
  node["minDigis"][ToString(ECbmModuleId::kTrd2d)] = nTrd2d;
  node["minDigis"][ToString(ECbmModuleId::kTof)]   = nTof;
  node["minDigis"][ToString(ECbmModuleId::kPsd)]   = nPsd;
  node["minDigis"][ToString(ECbmModuleId::kFsd)]   = nFsd;

  {
    // --- Check with created numbers of digis - should pass
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), true);
  }

  {
    // --- Increment Bmon - should fail
    node["minDigis"][ToString(ECbmModuleId::kBmon)] = nBmon + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment STS - should fail
    node["minDigis"][ToString(ECbmModuleId::kSts)] = nSts + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment MUCH - should fail
    node["minDigis"][ToString(ECbmModuleId::kMuch)] = nMuch + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment RICH - should fail
    node["minDigis"][ToString(ECbmModuleId::kRich)] = nRich + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment TRD - should fail
    node["minDigis"][ToString(ECbmModuleId::kTrd)] = nTrd + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment TRD2D - should fail
    node["minDigis"][ToString(ECbmModuleId::kTrd2d)] = nTrd2d + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment TOF - should fail
    node["minDigis"][ToString(ECbmModuleId::kTof)] = nTof + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment PSD - should fail
    node["minDigis"][ToString(ECbmModuleId::kPsd)] = nPsd + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Increment FSD - should fail
    node["minDigis"][ToString(ECbmModuleId::kFsd)] = nFsd + 1;
    cbm::algo::evbuild::DigiEventSelectorConfig config(node);
    cbm::algo::evbuild::DigiEventSelector select(config);
    select.RegisterTrackingSetup(trackingSetup);
    EXPECT_EQ(select(event), false);
  }

  {
    // --- Test number of STS stations
    {
      const uint maxStsStations = 8;
      const uint maxStsModules  = 12;
      const uint maxStsLadders  = 4;

      for (uint numStations = 1; numStations < maxStsStations; numStations++) {
        //Prepare input
        DigiEvent eventIn;
        //Produce digi pairs with valid addresses
        for (uint station = 0; station < numStations; station++) {
          for (uint module = 0; module < maxStsModules; module++) {
            for (uint ladder = 0; ladder < maxStsLadders; ladder++) {
              for (uint halfladder = 0; halfladder <= 1; halfladder++) {
                //add digis pairs
                int32_t address = CbmStsAddress::GetAddress(station, ladder, halfladder, module);
                eventIn.fSts.push_back(CbmStsDigi(address, 0, 0.0, 0.0));
                eventIn.fSts.push_back(CbmStsDigi(address, 1024, 0.0, 0.0));  //other side channel

                //add digis from next station without partner for intentionally failed test
                int32_t nextAddress = CbmStsAddress::GetAddress(numStations, ladder, 0, module);
                eventIn.fSts.push_back(CbmStsDigi(nextAddress, 1024, 0.0, 0.0));
              }
            }
          }
        }

        YAML::Node node2;
        //L_(info) << "tof min layers: " << numStations;
        {  // Test correct number of stations - should pass
          node2["minLayers"][ToString(ECbmModuleId::kSts)] = numStations;
          cbm::algo::evbuild::DigiEventSelectorConfig config(node2);
          cbm::algo::evbuild::DigiEventSelector select(config);
          select.RegisterTrackingSetup(trackingSetup);
          EXPECT_EQ(select(eventIn), true);
        }

        {
          //  Test if digi without partner is properly disregarded
          node2["minLayers"][ToString(ECbmModuleId::kSts)] = numStations + 1;
          cbm::algo::evbuild::DigiEventSelectorConfig config(node2);
          cbm::algo::evbuild::DigiEventSelector select(config);
          select.RegisterTrackingSetup(trackingSetup);
          EXPECT_EQ(select(eventIn), false);
        }
      }
    }


    // --- Test number of TOF layers
    {
      //Prepare input
      DigiEvent eventIn;

      const uint8_t numSmTypes         = 10;
      const uint8_t numSm[numSmTypes]            = {5, 0, 1, 0, 0, 1, 1, 1, 0, 1};
      const uint8_t numRpc[numSmTypes] = {5, 3, 5, 1, 1, 1, 2, 2, 1, 2};
      std::vector<std::vector<int>> trkStationId = {
        {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3},
        {},
        {2, 2, 2, 2, 2},
        {},
        {},
        {0},
        {1, 1},
        {1, 1},
        {},
        {2, 2}};
      std::unordered_set<int32_t> setTofStation;

      for (uint smType = 0; smType < numSmTypes; smType++) {
        for (uint sm = 0; sm < numSm[smType]; sm++) {
          for (uint rpc = 0; rpc < numRpc[smType]; rpc++) {

            uint32_t addrFront = CbmTofAddress::GetUniqueAddress(sm, rpc, 0, 0, smType, 0);
            uint32_t addrBack  = CbmTofAddress::GetUniqueAddress(sm, rpc, 0, 1, smType, 0);
            eventIn.fTof.push_back(CbmTofDigi(addrFront, 0.0, 0.0));
            eventIn.fTof.push_back(CbmTofDigi(addrBack, 0.0, 0.0));

            int32_t TofStationId = -1;
            if (smType < numSmTypes) {
              if (sm < numSm[smType] && rpc < numRpc[smType]) {
                TofStationId = trkStationId[smType][sm * numRpc[smType] + rpc];
              }
            }
            setTofStation.insert(TofStationId);

            YAML::Node node2;

            {  // Test actual number of layers -- should pass
              node2["minLayers"][ToString(ECbmModuleId::kTof)] = setTofStation.size();
              cbm::algo::evbuild::DigiEventSelectorConfig config(node2);
              cbm::algo::evbuild::DigiEventSelector select(config);
              select.RegisterTrackingSetup(trackingSetup);
              EXPECT_EQ(select(eventIn), true);
            }

            {  // Test with one more station - should fail
              node2["minLayers"][ToString(ECbmModuleId::kTof)] = setTofStation.size() + 1;
              cbm::algo::evbuild::DigiEventSelectorConfig config(node2);
              cbm::algo::evbuild::DigiEventSelector select(config);
              select.RegisterTrackingSetup(trackingSetup);
              EXPECT_EQ(select(eventIn), false);
            }
          }
        }
      }
    }
  }
}
