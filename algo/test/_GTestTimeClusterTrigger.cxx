/* Copyright (C) 2016-2017 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "TimeClusterTrigger.h"
#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"

using namespace cbm::algo::evbuild;

TEST(_GTestTimeClusterTrigger, CheckTriggerAlgorithmSimple)
{
  SCOPED_TRACE("CheckTriggerAlgorithSimple");

  const uint nInput         = 1000.;
  const double inputSpacing = 10.0;
  const double deadTime     = 5.;
  const double windowSize   = 1000.;
  const uint nMinNumber     = 100;

  TimeClusterTrigger trigger(windowSize, nMinNumber, deadTime);

  std::vector<double> dataIn;
  for (uint i = 0; i < nInput; i++) {
    dataIn.push_back(i * inputSpacing);
  }

  TimeClusterTrigger::resultType result  = trigger(dataIn);
  std::vector<double>& dataOut           = result.first;
  TimeClusterTriggerMonitorData& monitor = result.second;

  EXPECT_EQ(dataOut.size(), 10);

  for (uint i = 0; i < dataOut.size(); i++) {
    EXPECT_EQ(dataOut[i], 495. + i * 1000.);
  }

  EXPECT_EQ(dataIn.size(), monitor.num);
  EXPECT_EQ(nMinNumber * dataOut.size(), monitor.numInTrigger);
}
