/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaHit.cxx
/// \date   23.10.2023
/// \brief  A generic hit for the CA tracker (header) (implementation)
/// \author S.Zharko <s.zharko@gsi.de>

#include "CaHit.h"

#include <iomanip>
#include <sstream>

using cbm::algo::ca::Hit;

std::string Hit::ToString() const
{
  std::stringstream ss;
  ss << "Hit " << fId << " station " << fStation << " front key " << fFrontKey << " back key " << fBackKey << " X "
     << fX << " Y " << fY << " Z " << fZ << " T " << fT << " dX2 " << fDx2 << " dY2 " << fDy2 << " dXY " << fDxy
     << " dT2 " << fDt2 << " rangeX " << fRangeX << " rangeY " << fRangeY << " rangeT " << fRangeT;
  return ss.str();
}

std::string Hit::ToString(int verbose, bool bHeader) const
{
  if (verbose < 1) {
    return "";
  }
  using std::setw;
  constexpr int widthF = 12;
  constexpr int widthI = 4;
  std::stringstream msg;
  if (bHeader) {
    msg << setw(widthI) << "IDe" << ' ';
    msg << setw(widthI) << "st." << ' ';
    if (verbose > 1) {
      msg << setw(widthI) << "keyF" << ' ';
      msg << setw(widthI) << "keyB" << ' ';
    }
    msg << setw(widthF) << "x [cm]" << ' ';
    msg << setw(widthF) << "y [cm]" << ' ';
    msg << setw(widthF) << "z [cm]" << ' ';
    msg << setw(widthF) << "t [ns]" << ' ';
    if (verbose > 1) {
      msg << setw(widthF) << "dx2 [cm2]" << ' ';
      msg << setw(widthF) << "dy2 [cm2]" << ' ';
      msg << setw(widthF) << "dxy [cm2]" << ' ';
      msg << setw(widthF) << "dt2 [ns2]" << ' ';
      if (verbose > 2) {
        msg << setw(widthF) << "rangeX [cm]" << ' ';
        msg << setw(widthF) << "rangeY [cm]" << ' ';
        msg << setw(widthF) << "rangeT [ns]" << ' ';
      }
    }
  }
  else {
    msg << setw(widthI) << fId << ' ';
    msg << setw(widthI) << fStation << ' ';
    if (verbose > 1) {
      msg << setw(widthI) << fFrontKey << ' ';
      msg << setw(widthI) << fBackKey << ' ';
    }
    msg << setw(widthF) << fX << ' ';
    msg << setw(widthF) << fY << ' ';
    msg << setw(widthF) << fZ << ' ';
    msg << setw(widthF) << fT << ' ';
    if (verbose > 1) {
      msg << setw(widthF) << fDx2 << ' ';
      msg << setw(widthF) << fDy2 << ' ';
      msg << setw(widthF) << fDxy << ' ';
      msg << setw(widthF) << fDt2 << ' ';
      if (verbose > 2) {
        msg << setw(widthF) << fRangeX << ' ';
        msg << setw(widthF) << fRangeY << ' ';
        msg << setw(widthF) << fRangeT << ' ';
      }
    }
  }
  return msg.str();
}
