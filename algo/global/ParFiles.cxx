/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "ParFiles.h"

#include "Exceptions.h"

using namespace cbm::algo;

ParFiles::ParFiles(uint32_t runId)
{
  if (runId < 2724) {
    setup = Setup::mCBM2022;
  }
  else if (runId < 2918) {
    setup = Setup::mCBM2024_03;
  }
  else if (runId < 3400) {
    setup = Setup::mCBM2024_05;
  }
  else {
    setup = Setup::mCBM2025_02;
  }

  switch (setup) {

    case Setup::mCBM2022:
      bmon.readout   = "BmonReadout_mcbm2022.yaml";
      bmon.calibrate = "BmonCalibratePar_mcbm2022.yaml";
      bmon.hitfinder = "BmonHitfinderPar_mcbm2022.yaml";

      sts.readout   = "StsReadout_mcbm2022.yaml";
      sts.chanMask  = "StsChannelMaskSet_mcbm2022.yaml";
      sts.walkMap   = "StsWalkMap_mcbm2022.yaml";
      sts.hitfinder = "StsHitfinder_mcbm2022.yaml";

      tof.readout   = "TofReadout_mcbm2022.yaml";
      tof.calibrate = "TofCalibratePar_mcbm2022.yaml";
      tof.hitfinder = "TofHitfinderPar_mcbm2022.yaml";

      trd.readout     = "TrdReadoutSetup_mcbm2022.yaml";
      trd.readout2d   = "TrdReadout2DSetup_mcbm2022.yaml";
      trd.fee2d       = "Trd2dUnCalibFee.yaml";
      trd.hitfinder   = "TrdHitfinderPar_mcbm2022.yaml";
      trd.hitfinder2d = "TrdHitfinder2DPar_mcbm2022.yaml";

      ca.mainConfig = "TrackingChainConfig_mcbm2022.yaml";

      kfp.V0FinderConfig = "kfp_lambda_v22a.yaml";
      break;

    case Setup::mCBM2024_03:
      bmon.readout   = "BmonReadout_mcbm2024.yaml";
      bmon.calibrate = "BmonCalibratePar_mcbm2024.yaml";
      bmon.hitfinder = "BmonHitfinderPar_mcbm2024.yaml";

      sts.readout   = "StsReadout_mcbm2024.yaml";
      sts.chanMask  = "StsChannelMaskSet_mcbm2024.yaml";
      sts.walkMap   = "StsWalkMap_mcbm2024.yaml";
      sts.hitfinder = "StsHitfinder_mcbm2024.yaml";

      tof.readout   = "TofReadout_mcbm2024.yaml";
      tof.calibrate = "TofCalibratePar_mcbm2024.yaml";
      tof.hitfinder = "TofHitfinderPar_mcbm2024.yaml";

      trd.readout     = "TrdReadoutSetup_mcbm2024.yaml";
      trd.readout2d   = "TrdReadout2DSetup_mcbm2022.yaml";  // same mCBM2022 readout for TRD2D
      trd.fee2d       = "Trd2dUnCalibFee.yaml";             // dummy calibration
      trd.hitfinder   = "TrdHitfinderPar_mcbm2024.yaml";
      trd.hitfinder2d = "TrdHitfinder2DPar_mcbm2024.yaml";

      ca.mainConfig = "TrackingChainConfig_mcbm2024.yaml";

      kfp.V0FinderConfig = "kfp_lambda_v24a.yaml";
      break;

    case Setup::mCBM2024_05:
      bmon.readout   = "BmonReadout_mcbm2024.yaml";
      bmon.calibrate = "mcbm2024_05/BmonCalibratePar.yaml";
      bmon.hitfinder = "mcbm2024_05/BmonHitfinderPar.yaml";

      sts.readout   = "StsReadout_mcbm2024.yaml";
      sts.chanMask  = "StsChannelMaskSet_mcbm2024.yaml";
      sts.walkMap   = "mcbm2024_05/StsWalkMap.yaml";
      sts.hitfinder = "mcbm2024_05/StsHitfinder.yaml";

      tof.readout   = "mcbm2024_05/TofReadout.yaml";
      tof.calibrate = "mcbm2024_05/TofCalibratePar.yaml";
      tof.hitfinder = "mcbm2024_05/TofHitfinderPar.yaml";

      trd.readout     = "mcbm2024_05/TrdReadoutSetup.yaml";
      trd.readout2d   = "TrdReadout2DSetup_mcbm2022.yaml";  // same mCBM2022 readout for TRD2D
      trd.fee2d       = "Trd2dUnCalibFee.yaml";             // dummy calibration
      trd.hitfinder   = "mcbm2024_05/TrdHitfinderPar.yaml";
      trd.hitfinder2d = "mcbm2024_05/TrdHitfinder2DPar.yaml";

      ca.mainConfig = "mcbm2024_05/TrackingChainConfig.yaml";

      kfp.V0FinderConfig = "kfp_lambda_v24b.yaml";
      break;

    case Setup::mCBM2025_02:
      bmon.readout = "mcbm2025_02/BmonReadout_mcbm2025.yaml";
      bmon.calibrate = "mcbm2025_02/BmonCalibratePar.yaml";
      bmon.hitfinder = "mcbm2025_02/BmonHitfinderPar.yaml";

      sts.readout   = "mcbm2025_02/StsReadout_mcbm2025.yaml";
      sts.chanMask  = "mcbm2025_02/StsChannelMaskSet_mcbm2025.yaml";
      sts.walkMap   = "mcbm2025_02/StsWalkMap_mcbm2025.yaml";
      sts.hitfinder = "mcbm2025_02/StsHitfinder.yaml";

      tof.readout   = "mcbm2025_02/TofReadout.yaml";
      tof.calibrate = "mcbm2025_02/TofCalibratePar.yaml";
      tof.hitfinder = "mcbm2025_02/TofHitfinderPar.yaml";

      trd.readout     = "mcbm2025_02/TrdReadoutSetup.yaml";
      trd.readout2d   = "mcbm2025_02/Trd2dReadoutSetup.yaml";
      trd.fee2d       = "mcbm2025_02/Trd2dCalibFee.yaml";
      trd.hitfinder   = "mcbm2025_02/TrdHitfinderPar.yaml";
      trd.hitfinder2d = "mcbm2025_02/TrdHitfinder2DPar.yaml";

      ca.mainConfig = "mcbm2025_02/TrackingChainConfig.yaml";

      kfp.V0FinderConfig = "mcbm2025_02/kfp_lambda_v25a.yaml";
      break;

    default: throw FatalError("Unknown setup: {}", ToString(setup));
  }
}
