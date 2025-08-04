/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmFormatTypes.h"

#include <Hal/DataFormatManager.h>
#include <Hal/Event.h>
#include <Hal/Std.h>


namespace HalCbm
{

  EFormatType GetFormatType(Int_t task_id, Hal::EFormatDepth depth)
  {
    const Hal::Event* ev = Hal::DataFormatManager::Instance()->GetFormat(task_id, depth);
    if (ev->InheritsFrom("HalCbmEvent")) {
      return EFormatType::kAnaTree;
    }
    else if (ev->InheritsFrom("HalCbmHbtEvent")) {
      return EFormatType::kHbt;
    }
    return EFormatType::kUnknown;
  }
}  // namespace HalCbm
