/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */

#pragma once

#include "CommonUnpacker.h"
#include "ReadoutConfig.h"
#include "UnpackMS.h"

namespace cbm::algo::rich
{

  namespace detail
  {
    using UnpackBase = CommonUnpacker<CbmRichDigi, UnpackMonitorData, UnpackAuxData>;
  }

  class Unpack : public detail::UnpackBase {

   public:
    using Result_t = detail::UnpackBase::Result_t;

    Unpack(const ReadoutConfig& readout);

    Result_t operator()(const fles::Timeslice&) const;

   private:
    ReadoutConfig fReadout;
  };

}  // namespace cbm::algo::rich
