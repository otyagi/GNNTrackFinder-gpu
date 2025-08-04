/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include <optional>

#include "compat/Filesystem.h"

namespace cbm::explore
{
  class Options {
  public:
    Options(int argc, char** argv);

    int Port() const { return fPort; }

    algo::fs::path Input() const { return fInput; }
    algo::fs::path Input2() const { return fInput2; }

    std::optional<uint32_t> Sensor() const { return fFilterSensor ? std::make_optional(fSensor) : std::nullopt; }

  private:
    int fPort;
    uint32_t fSensor;
    bool fFilterSensor = false;
    std::string fInput;
    std::string fInput2;
  };
}  // namespace cbm::explore
