/* Copyright (C) 2016-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: David Gutierrez [committer] */

#include "CbmStsParAsic.h"
#include "gtest/gtest.h"

#include <gtest.h>


static const CbmStsParAsic par_asic(128, 31, 75000, 3000, 5, 800, 0, 0);

TEST(_GTestCbmStsParAsic, ExaustiveChargeConvertion)
{
  for (uint16_t adc = 0; adc < par_asic.GetNofAdc(); adc++) {
    const auto q    = par_asic.AdcToCharge(adc);
    const auto adc_ = par_asic.ChargeToAdc(q);
    ASSERT_EQ(adc_, adc);
  }
}

TEST(_GTestCbmStsParAsic, AdcBelowLimit)
{
  const double adc = 0;
  ASSERT_LT(par_asic.AdcToCharge(adc), par_asic.GetThreshold());
}

TEST(_GTestCbmStsParAsic, AdcAboveLimit)
{
  const double adc = 32;
  ASSERT_GT(adc, par_asic.GetNofAdc());
  ASSERT_GT(par_asic.AdcToCharge(adc), par_asic.GetThreshold() + par_asic.GetDynRange());
}

TEST(_GTestCbmStsParAsic, ChargeBelowLimit)
{
  const double q = 2000;
  ASSERT_LT(q, par_asic.GetThreshold());
  ASSERT_EQ(par_asic.ChargeToAdc(q), 0);
}

TEST(_GTestCbmStsParAsic, ChargeAboveLimit)
{
  const double q = 80000;
  ASSERT_GT(q, par_asic.GetThreshold() + par_asic.GetDynRange());
  ASSERT_EQ(par_asic.ChargeToAdc(q), 31);
}
