/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaHitQaData.cxx
/// @date   01.09.2023
/// @brief  A helper class to store hit and MC-point parameter and calculate related quantities (implementation)
/// @author S.Zharko <s.zharko@gsi.de>

#include "CbmCaHitQaData.h"

using cbm::ca::HitQaData;

// ---------------------------------------------------------------------------------------------------------------------
//
void HitQaData::Reset() { this->operator=(HitQaData()); }
