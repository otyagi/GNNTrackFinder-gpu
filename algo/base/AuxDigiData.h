/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   AuxDigiData.h
/// \date   10.06.2024
/// \brief  Collection of auxiliary data from unpackers (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CommonUnpacker.h"
#include "bmon/UnpackMS.h"
#include "much/UnpackMS.h"
#include "rich/UnpackMS.h"
#include "sts/UnpackMS.h"
#include "tof/UnpackMS.h"
#include "trd/UnpackMS.h"
#include "trd2d/UnpackMS.h"

#include <vector>

namespace cbm::algo
{
  /// \struct AuxDigiData
  /// \brief  Collection of auxiliary digi objects from different module unpackers
  struct AuxDigiData {
    UnpackAux<bmon::UnpackAuxData> fBmon;
    UnpackAux<much::UnpackAuxData> fMuch;
    UnpackAux<rich::UnpackAuxData> fRich;
    UnpackAux<sts::UnpackAuxData> fSts;
    UnpackAux<tof::UnpackAuxData> fTof;
    UnpackAux<trd::UnpackAuxData> fTrd;
    UnpackAux<trd2d::UnpackAuxData> fTrd2d;
  };
}  // namespace cbm::algo
