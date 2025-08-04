/* Copyright (C) 2021-2023 Facility for Anti-Proton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "EventBuilder.h"
#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"

#include <yaml-cpp/yaml.h>

using namespace cbm::algo;

TEST(_GTestEventBuilder, CheckEventBuilderAlgorithmSimple)
{
  SCOPED_TRACE("CheckEventBuilderAlgorithSimple");

  //Initialize event builder
  YAML::Node configNode;
  configNode[ToString(ECbmModuleId::kMuch)] = std::pair<double, double>{-45., 45.};
  configNode[ToString(ECbmModuleId::kSts)]  = std::pair<double, double>{-45., 45.};
  configNode[ToString(ECbmModuleId::kTof)]  = std::pair<double, double>{-45., 45.};
  configNode[ToString(ECbmModuleId::kTrd)]  = std::pair<double, double>{-45., 45.};
  configNode[ToString(ECbmModuleId::kRich)] = std::pair<double, double>{-45., 45.};
  configNode[ToString(ECbmModuleId::kPsd)]  = std::pair<double, double>{-45., 45.};
  configNode[ToString(ECbmModuleId::kFsd)]  = std::pair<double, double>{-45., 45.};
  configNode[ToString(ECbmModuleId::kBmon)] = std::pair<double, double>{-45., 45.};
  cbm::algo::evbuild::EventBuilderConfig config(configNode);
  cbm::algo::evbuild::EventBuilder evbuild(config);

  DigiData tsIn;
  const uint nInput         = 1000;
  const double inputSpacing = 10.0;

  //Produce digis with some arbitrary but valid addresses
  for (uint i = 0; i < nInput; i++) {
    tsIn.fMuch.push_back(CbmMuchDigi(1111, 1, i * inputSpacing));
    tsIn.fSts.push_back(CbmStsDigi(268502050, 1, i * inputSpacing, 1.0));
    tsIn.fTof.push_back(CbmTofDigi(1111, i * inputSpacing, 1.0));
    tsIn.fTrd.push_back(CbmTrdDigi(475, 37, 150, i * inputSpacing, CbmTrdDigi::eTriggerType::kBeginTriggerTypes, 0));
    tsIn.fRich.push_back(CbmRichDigi(1111, i * inputSpacing, 1.0));
    tsIn.fPsd.push_back(CbmPsdDigi(1111, i * inputSpacing, 1.0));
    tsIn.fFsd.push_back(CbmFsdDigi(1111, i * inputSpacing, 1.0));
    tsIn.fBmon.push_back(CbmBmonDigi(1111, i * inputSpacing, 1.0));
  }

  std::vector<double> triggerIn;
  const uint nTrigger         = 99;
  const double triggerSpacing = 100.0;

  for (uint i = 1; i <= nTrigger; i++) {
    triggerIn.push_back(i * triggerSpacing);
  }

  cbm::algo::evbuild::EventBuilder::resultType result  = evbuild(tsIn, triggerIn, std::nullopt);
  std::vector<DigiEvent>& eventsOut                    = result.first;
  cbm::algo::evbuild::EventBuilderMonitorData& monitor = result.second;

  EXPECT_EQ(eventsOut.size(), nTrigger);

  for (uint i = 0; i < eventsOut.size(); i++) {
    EXPECT_EQ(eventsOut[i].fMuch.size(), 9);
    EXPECT_EQ(eventsOut[i].fSts.size(), 9);
    EXPECT_EQ(eventsOut[i].fTof.size(), 9);
    EXPECT_EQ(eventsOut[i].fTrd.size(), 9);
    EXPECT_EQ(eventsOut[i].fRich.size(), 9);
    EXPECT_EQ(eventsOut[i].fPsd.size(), 9);
    EXPECT_EQ(eventsOut[i].fFsd.size(), 9);
    EXPECT_EQ(eventsOut[i].fBmon.size(), 9);
    EXPECT_EQ(eventsOut[i].fTime, triggerIn[i]);
  }

  EXPECT_EQ(monitor.much.nDigisInEvents, 9 * nTrigger);
  EXPECT_EQ(monitor.sts.nDigisInEvents, 9 * nTrigger);
  EXPECT_EQ(monitor.tof.nDigisInEvents, 9 * nTrigger);
  EXPECT_EQ(monitor.trd.nDigisInEvents, 9 * nTrigger);
  EXPECT_EQ(monitor.rich.nDigisInEvents, 9 * nTrigger);
  EXPECT_EQ(monitor.psd.nDigisInEvents, 9 * nTrigger);
  EXPECT_EQ(monitor.fsd.nDigisInEvents, 9 * nTrigger);
  EXPECT_EQ(monitor.bmon.nDigisInEvents, 9 * nTrigger);

  EXPECT_EQ(monitor.much.nDigis, nInput);
  EXPECT_EQ(monitor.sts.nDigis, nInput);
  EXPECT_EQ(monitor.tof.nDigis, nInput);
  EXPECT_EQ(monitor.trd.nDigis, nInput);
  EXPECT_EQ(monitor.rich.nDigis, nInput);
  EXPECT_EQ(monitor.psd.nDigis, nInput);
  EXPECT_EQ(monitor.fsd.nDigis, nInput);
  EXPECT_EQ(monitor.bmon.nDigis, nInput);
}
