/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include <Timeslice.hpp>

namespace cbm::algo::ts_utils
{

  inline size_t SizeBytes(const fles::Timeslice& ts)
  {
    size_t size = 0;
    for (size_t i = 0; i < ts.num_components(); i++) {
      size += ts.size_component(i);
    }
    return size;
  }

}  // namespace cbm::algo::ts_utils
