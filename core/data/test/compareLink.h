/* Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#ifndef COMPARELINK_H
#define COMPARELINK_H 1

#include "CbmLink.h"

#include "gtest/gtest.h"

void compareLinkDataMembers(CbmLink& test, int32_t file, int32_t entry, int32_t index, double weight)
{
  int32_t intRetVal {-111};
  float floatRetVal {-111};

  intRetVal = test.GetFile();
  EXPECT_EQ(file, intRetVal);

  intRetVal = test.GetEntry();
  EXPECT_EQ(entry, intRetVal);

  intRetVal = test.GetIndex();
  EXPECT_EQ(index, intRetVal);

  floatRetVal = test.GetWeight();
  EXPECT_FLOAT_EQ(weight, floatRetVal);
}

#endif  //COMPARELINK_H
