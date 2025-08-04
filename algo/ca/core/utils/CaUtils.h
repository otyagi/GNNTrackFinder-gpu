/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaUtils.h
/// \brief  Compile-time constants definition for the CA tracking algorithm
/// \since  02.06.2022
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaHit.h"
#include "KfDefs.h"
#include "KfMeasurementTime.h"
#include "KfMeasurementXy.h"
#include "KfSimd.h"
#include "KfTrackKalmanFilter.h"


namespace cbm::algo::ca::utils
{

  template<typename T>
  inline void FilterHit(kf::TrackKalmanFilter<T>& fit, const ca::Hit& hit, const kf::utils::masktype<T>& timeInfo)
  {
    kf::MeasurementXy<T> m;
    m.SetDx2(hit.dX2());
    m.SetDy2(hit.dY2());
    m.SetDxy(hit.dXY());
    m.SetX(hit.X());
    m.SetY(hit.Y());
    m.SetNdfX(T(1.));
    m.SetNdfY(T(1.));
    fit.FilterXY(m);
    fit.FilterTime(hit.T(), hit.dT2(), timeInfo);
  }

}  // namespace cbm::algo::ca::utils
