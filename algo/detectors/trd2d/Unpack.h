/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#pragma once

#include "CommonUnpacker.h"
#include "ReadoutConfig.h"
#include "UnpackMS.h"

namespace cbm::algo::trd2d
{

  namespace detail
  {
    using UnpackBase = CommonUnpacker<CbmTrdDigi, UnpackMonitorData, UnpackAuxData>;
  }

  class Unpack : public detail::UnpackBase {

   public:
    struct Config {
      ReadoutSetup roSetup;
      ReadoutCalib roCalib;
    };
    using Result_t = detail::UnpackBase::Result_t;

    Unpack(const Config& config);
    //Unpack(const ReadoutConfig& readout);

    Result_t operator()(const fles::Timeslice&) const;

    const Config& Readout() const { return fConfig; }

   private:
    Config fConfig;
  };

}  // namespace cbm::algo::trd2d
