/* Copyright (C) 2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmErrorMessage.h"

#include "compareErrorMessage.h"
#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"

TEST(_GTestCbmErrorMessage, CheckDefaultConstructor)
{
  CbmErrorMessage test;
  {
    SCOPED_TRACE("CheckDefaultConstructor");
    compareErrorMessageDataMembers(test, ECbmModuleId::kLastModule, -1.0, 0, 0, 0);
  }
}

TEST(_GTestCbmErrorMessage, CheckStandardConstructor)
{
  CbmErrorMessage test {ECbmModuleId::kLastModule, -1.0, 0, 0};
  {
    SCOPED_TRACE("CheckStandardConstructor");
    compareErrorMessageDataMembers(test, ECbmModuleId::kLastModule, -1.0, 0, 0, 0);
  }
}

TEST(_GTestCbmErrorMessage, CheckStandardConstructorWithPayload)
{
  CbmErrorMessage test {ECbmModuleId::kLastModule, -1.0, 0, 0, 0};
  {
    SCOPED_TRACE("CheckStandardConstructorWithPayload");
    compareErrorMessageDataMembers(test, ECbmModuleId::kLastModule, -1.0, 0, 0, 0);
  }
}

TEST(_GTestCbmErrorMessage, CheckToString)
{
  CbmErrorMessage test {ECbmModuleId::kRef, -1.1, 0, 0, 0};
  EXPECT_STREQ("Error message: System 0 | time -1.1 | address 0 | flags "
               "0x00000000 | fulPayload 0x00000000",
               test.ToString().c_str());
}
