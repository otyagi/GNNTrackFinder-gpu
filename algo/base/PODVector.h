/* Copyright (C) 2022 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_POD_VECTOR_H
#define CBM_ALGO_BASE_POD_VECTOR_H

#include "util/PODAllocator.h"

#include <vector>

namespace cbm::algo
{
  /**
  * @brief PODVector is a std::vector that doesn't initialize its elements.
  */
  template<class T>
  using PODVector = std::vector<T, PODAllocator<T>>;

  template<typename T>
  std::vector<T> ToStdVector(const PODVector<T>& vec)
  {
    return std::vector<T>(vec.begin(), vec.end());
  }

  template<typename T>
  PODVector<T> ToPODVector(const std::vector<T>& vec)
  {
    return PODVector<T>(vec.begin(), vec.end());
  }

}  // namespace cbm::algo

#endif  // CBM_ALGO_BASE_POD_VECTOR_H
