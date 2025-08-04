/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMFORMATTYPES_H_
#define CBMFORMATTYPES_H_

#include <Hal/DataFormatManager.h>
#include <Hal/Std.h>

namespace HalCbm
{

  enum class EFormatType
  {
    kAnaTree,
    kHbt,
    kUnknown
  };
  EFormatType GetFormatType(Int_t task_id, Hal::EFormatDepth depht = Hal::EFormatDepth::kAll);

}  // namespace HalCbm

#endif /* CBMFORMATTYPES_H_ */
