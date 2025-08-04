/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmMcbmUtils.h"

#include "TString.h"

#include <stdexcept>

namespace cbm
{
  namespace mcbm
  {
    std::string GetSetupFromRunId(uint64_t ulRunId)
    {
      /// General remark: only runs known to exist on disk/tape are mapped so "holes" are not an oversight
      /// => if necessary exception throwing can also be added for them but anyway analysis will crash (no raw data)

      /// 2021 "CRI" runs: 1575 - 1588 = 15/07/2021
      std::string sSetupName = "mcbm_beam_2021_07_surveyed";
      if (ulRunId < 1575) {
        /// Only runs at earliest in the 2021 benchmark beamtime and with all 6 systems are supported by this function
        /// From mCBM redmine wiki page: "run 1575: 1st run with 6 subsystems"
        throw(std::invalid_argument("RunId smaller than the earliest run mapped (1575 in 2021 campaign)"));
      }
      else if (1575 <= ulRunId && ulRunId <= 1588) {
        /// 2021 "CRI" runs: 1575 - 1588 = 15/07/2021
        /// => Nothing to do, this is the default name
      }
      /// Setup changed multiple times between the 2022 carbon and uranium runs
      else if (2060 <= ulRunId && ulRunId <= 2065) {
        /// Carbon runs: 2060 - 2065 = 10/03/2022
        sSetupName = "mcbm_beam_2022_03_09_carbon";
      }
      else if (2150 <= ulRunId && ulRunId <= 2160) {
        /// Iron runs: 2150 - 2160 = 24-25/03/2022
        sSetupName = "mcbm_beam_2022_03_22_iron";
      }
      else if (2176 <= ulRunId && ulRunId <= 2310) {
        /// Uranium runs: 2176 - 2310 = 30/03/2022 - 01/04/2022
        sSetupName = "mcbm_beam_2022_03_28_uranium";
      }
      else if (2350 <= ulRunId && ulRunId <= 2397) {
        /// Nickel runs: 2350 - 2397 = 23/05/2022 - 25/05/2022 (Lambda Benchmark but mTOF troubles)
        sSetupName = "mcbm_beam_2022_05_23_nickel";
      }
      else if (2454 <= ulRunId && ulRunId <= 2497) {
        /// Lambda Benchmark Gold runs: 2454 - 2497 = 16/06/2022 - 18/06/2022
        sSetupName = "mcbm_beam_2022_06_16_gold";
      }
      else if (2498 <= ulRunId && ulRunId <= 2610) {
        /// High Rate Gold runs with GEMs in Acceptance: 2498 - 2610 = 18/06/2022 - 20/06/2022
        sSetupName = "mcbm_beam_2022_06_18_gold";
      }
      else if (2724 <= ulRunId && ulRunId <= 2917) {
        /// 2024/03 Gold runs
        sSetupName = "mcbm_beam_2024_03_22_gold";
      }
      else if (2918 <= ulRunId && ulRunId <= 3399) {
        /// Dummy needed to run the unpack macro until we have a setup ready
        sSetupName = "mcbm_beam_2024_05_08_nickel";
      }
      else if (3400 <= ulRunId) {
        /// Dummy needed to run the unpack macro until we have a setup ready
        sSetupName = "mcbm_beam_2025_02_14_silver";
      }
      else {
        /// Missing runs, exception there to force implementation and support from users side
        throw(std::invalid_argument(Form("RunId %d is not mapped! Please complete the map!", ulRunId)));
      }

      return sSetupName;
    }
  }  // namespace mcbm
}  // namespace cbm
