/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov[committer] */

/// \file   CaToolsField.h
/// \brief  Tracking Field class (header)
/// \since  17.10.2023
/// \author S.Gorbunov

#pragma once  // include this header only once per compilation unit

#include "FairField.h"
#include "FairRunAna.h"
#include "KfFieldRegion.h"

#include <mutex>

namespace cbm::ca::tools
{
  /// pass the original magnetic field to L1Algo
  inline void SetOriginalCbmField()
  {
    static auto fld = [&](double x, double y, double z) {
      assert(FairRunAna::Instance());
      if (FairRunAna::Instance()->GetField()) {
        double pos[3] = {x, y, z};
        double B[3]   = {0., 0., 0.};
        // protect the field access
        // TODO: make CbmField thread-safe
        static std::mutex mymutex;
        mymutex.lock();
        FairRunAna::Instance()->GetField()->GetFieldValue(pos, B);
        mymutex.unlock();
        return std::tuple(B[0], B[1], B[2]);
      }
      else {
        return std::tuple(0., 0., 0.);
      }
    };

    cbm::algo::kf::GlobalField::SetFieldFunction(                                                                  //
      (FairRunAna::Instance()->GetField() ? cbm::algo::kf::EFieldType::Normal : cbm::algo::kf::EFieldType::Null),  //
      fld);
  }

}  // namespace cbm::ca::tools
