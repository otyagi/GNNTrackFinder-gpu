/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfFramework.cxx
/// @brief  The Kalman-filter framework main class (header)
/// @since  28.03.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#include "KfFramework.h"

#include "AlgoFairloggerCompat.h"

using cbm::algo::kf::Framework;


namespace cbm::algo::kf
{
  template class Framework<float>;
  template class Framework<double>;
  template class Framework<fvec>;
}  // namespace cbm::algo::kf
