/* Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "CbmDeviceStsLocalReco.h"

#include <iostream>

#include "runFairMQDevice.h"

namespace bpo = boost::program_options;

void addCustomOptions(bpo::options_description& options)
{
  options.add_options()("max-timeslices", bpo::value<uint64_t>()->default_value(0),
                        "Maximum number of timeslices to process for "
                        "Run/ConditionalRun/OnData (0 - infinite)")(
    "vmcworkdir", bpo::value<std::string>()->default_value("."), "Directory where to find needed input data")(
    "run-id", bpo::value<std::string>()->default_value("0"),
    "Rund ID which is needed to retrieve the proper parameters from Parameter "
    "Manager");
}

FairMQDevicePtr getDevice(const FairMQProgOptions& /*config*/) { return new CbmDeviceStsLocalReco(); }
