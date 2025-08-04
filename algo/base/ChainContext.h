/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_CHAINCONTEXT_H
#define CBM_ALGO_BASE_CHAINCONTEXT_H

#include "Options.h"
#include "RecoParams.h"

#include <memory>
#include <optional>

namespace cbm
{
  // cbm::Monitor must be forward declared. This prevents an issue in older ROOT versions,
  // where cling would crash upon parsing the header file (in some stl header)
  class Monitor;
}  // namespace cbm

namespace cbm::algo
{
  struct ChainContext {
    // default constructor / destructor
    // But have to be defined in the .cxx file, because of forward declaration of cbm::Monitor
    ChainContext();
    ~ChainContext();

    Options opts;
    RecoParams recoParams;
    std::unique_ptr<cbm::Monitor> monitor;  //! Monitor
  };
}  // namespace cbm::algo

#endif
