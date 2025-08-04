/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaHitRecord.h
/// \brief  An implementation for cbm::ca::HitRecord structure
/// \since  17.09.2023
/// \author S.Zharko <s.zharko@gsi.de>

#include "CaToolsHitRecord.h"

#include "Logger.h"

#include <cmath>
#include <sstream>

using cbm::ca::tools::HitRecord;

// ---------------------------------------------------------------------------------------------------------------------
//
bool HitRecord::Accept() const
{
  if constexpr (kVerboseAccept) {
    std::stringstream msg;
    if (std::isnan(fX) || std::isinf(fX)) {
      msg << ", x = " << fX;
    }
    if (std::isnan(fY) || std::isinf(fY)) {
      msg << ", y = " << fY;
    }
    if (std::isnan(fZ) || std::isinf(fZ)) {
      msg << ", z = " << fZ;
    }
    if (std::isnan(fT) || std::isinf(fT)) {
      msg << ", t = " << fT;
    }
    if (std::isnan(fDx2) || std::isinf(fDx2)) {
      msg << ", dx2 = " << fDx2;
    }
    if (std::isnan(fDy2) || std::isinf(fDy2)) {
      msg << ", dy2 = " << fDy2;
    }
    if (std::isnan(fDxy) || std::isinf(fDxy)) {
      msg << ", dxy = " << fDxy;
    }
    if (std::isnan(fDt2) || std::isinf(fDt2)) {
      msg << ", dt2 = " << fDt2;
    }
    const auto& sMsg = msg.str();
    if (sMsg.size()) {
      LOG(warn) << "HitRecord: Discarding hit " << fExtId << ": det = " << fDet
                << ", addr = " << static_cast<int32_t>(fDataStream) << ", " << sMsg;
      return false;
    }
  }
  else {
    bool res = true;
    res      = res && !(std::isnan(fX) || std::isinf(fX));
    res      = res && !(std::isnan(fY) || std::isinf(fY));
    res      = res && !(std::isnan(fZ) || std::isinf(fZ));
    res      = res && !(std::isnan(fT) || std::isinf(fT));
    res      = res && !(std::isnan(fDx2) || std::isinf(fDx2));
    res      = res && !(std::isnan(fDy2) || std::isinf(fDy2));
    res      = res && !(std::isnan(fDt2) || std::isinf(fDt2));
    res      = res && !(std::isnan(fDxy) || std::isinf(fDxy));
    return res;
  }
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string HitRecord::ToString() const
{
  std::stringstream msg;
  msg << "HitRecord: det = " << fDet << ", addr = " << static_cast<int32_t>(fDataStream) << ", extId = " << fExtId
      << ", staId = " << fStaId << ", pointId = " << fPointId << ", stripF = " << fStripF << ", stripB = " << fStripB
      << ", x = " << fX << ", y = " << fY << ", z = " << fZ << ", t = " << fT << ", dx2 = " << fDx2
      << ", dy2 = " << fDy2 << ", dt2 = " << fDt2 << ", dxy = " << fDxy << ", rangeX = " << fRangeX
      << ", rangeY = " << fRangeY << ", rangeT = " << fRangeT;
  return msg.str();
}
