/* Copyright (C) 2010-2014 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer]*/

#pragma once  // include this header only once per compilation unit

#include "KfSimd.h"
#include "KfUtils.h"

namespace cbm::algo::ca
{
  // Backward compatebility to ca::fvec etc.
  using fvec  = kf::fvec;
  using fscal = kf::fscal;
  using fmask = kf::fmask;

  // Utils namespace
  namespace kfutils = cbm::algo::kf::utils;
  namespace kfdefs  = cbm::algo::kf::defs;
}  // namespace cbm::algo::ca
