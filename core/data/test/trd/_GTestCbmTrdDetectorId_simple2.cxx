/* Copyright (C) 2012 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "CbmDetectorList.h"
#include "CbmTrdDetectorId.h"

#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"

class CbmTrdDetectorIdTest : public ::testing::Test {
protected:
  CbmTrdDetectorId fTrdId;
  int32_t system;
  int32_t station;
  int32_t layer;
  int32_t moduleType;
  int32_t moduleNr;
  int32_t sector;
  int32_t detInfo_array[6];
  int32_t retVal;

  virtual void SetUp() {}

  virtual void TearDown() {}

  void FillDetArray(int32_t sys, int32_t stat, int32_t lay, int32_t type, int32_t copy, int32_t sect)
  {
    detInfo_array[0] = sys;
    detInfo_array[1] = stat;
    detInfo_array[2] = lay;
    detInfo_array[3] = type;
    detInfo_array[4] = copy;
    detInfo_array[5] = sect;
  }
};

TEST_F(CbmTrdDetectorIdTest, CheckDefaultSettings)
{
  FillDetArray(0, 0, 0, 0, 0, 0);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(0, retVal);

  FillDetArray(kTRD, 0, 0, 0, 0, 0);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(5, retVal);

  FillDetArray(0, 1, 0, 0, 0, 0);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(32, retVal);

  FillDetArray(0, 0, 1, 0, 0, 0);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(512, retVal);

  FillDetArray(0, 0, 0, 1, 0, 0);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(4096, retVal);

  FillDetArray(0, 0, 0, 0, 1, 0);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(131072, retVal);

  FillDetArray(0, 0, 0, 0, 0, 1);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(33554432, retVal);

  FillDetArray(kTRD, 1, 1, 1, 1, 1);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(33690149, retVal);

  FillDetArray(kTRD, 3, 3, 2, 34, 3);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(105129573, retVal);

  FillDetArray(0, 0, 0, 8, 0, 0);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(32768, retVal);

  FillDetArray(kTRD, 2, 3, 5, 17, 2);
  retVal = fTrdId.SetDetectorInfo(detInfo_array);
  EXPECT_EQ(69359173, retVal);
}
