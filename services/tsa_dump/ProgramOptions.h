/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#pragma once

#include "MicrosliceDescriptor.hpp"

#include <cstdint>  // For SIZE_MAX
#include <string>

struct ProgramOptions {
  ProgramOptions(int argc, char** argv);

  std::string sFullFilename;
  uint64_t uNbTimeslices = 0;
  // FIXME: define "reserved/undefined" system ID somewhere (best in flesnet microslice descriptor header)
  uint16_t selSysId  = 0x00;
  size_t nbMsPerComp = SIZE_MAX;

  void ConvertSysId(const std::string& option);
};
