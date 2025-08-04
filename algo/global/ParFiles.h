/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

/**
 * @file ParFiles.h
 * @brief This file contains the definition of the ParFiles class.
 */

#include "Definitions.h"
#include "compat/Filesystem.h"

namespace cbm::algo
{

  /**
   * @class ParFiles
   * @brief Class to hold the paths to the parameter files for the different detectors.
  **/
  struct ParFiles {

    ParFiles(uint32_t runId);

    Setup setup;

    struct {
      fs::path readout;
      fs::path calibrate;
      fs::path hitfinder;
    } bmon;

    struct {
      fs::path readout;
      fs::path chanMask;
      fs::path walkMap;
      fs::path hitfinder;
    } sts;

    struct {
      fs::path readout;
      fs::path calibrate;
      fs::path hitfinder;
    } tof;

    struct {
      fs::path readout;
      fs::path readout2d;
      fs::path fee2d;
      fs::path hitfinder;
      fs::path hitfinder2d;
    } trd;

    struct {
      fs::path mainConfig;
    } ca;

    struct {
      fs::path V0FinderConfig;
    } kfp;
  };

}  // namespace cbm::algo
