/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#pragma once

#include "CommonUnpacker.h"
#include "ReadoutConfig.h"
#include "UnpackMS.h"
#include "WalkMap.h"

namespace cbm::algo::sts
{

  namespace detail
  {
    using UnpackBase = CommonUnpacker<CbmStsDigi, UnpackMonitorData, UnpackAuxData>;
  }

  class Unpack : public detail::UnpackBase {

   public:
    struct Config {
      ReadoutConfig readout;
      WalkMap walkMap;
      bool bCollectAuxData = false;
    };

    using Result_t = detail::UnpackBase::Result_t;

    Unpack(const Config& config);

    Result_t operator()(const fles::Timeslice&) const;

   private:
    Config fConfig;

    void PrintDigisPerModule(const PODVector<CbmStsDigi>& digis) const;
  };

}  // namespace cbm::algo::sts
