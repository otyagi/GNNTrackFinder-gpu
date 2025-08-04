/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include "Definitions.h"

#include <string>

struct ProgramOptions {
  ProgramOptions(int argc, char** argv);

  cbm::algo::Setup setup;
  bool skipAlignment;
  std::string outputDir;
};
