/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Matus Kalisky, Florian Uhlig, Andrey Lebedev */

/**
 * \file Hit.cxx
 * \author Dominik Smith <d.smith@gsi.de>
 * \date 2024
 **/
#include "Hit.h"

namespace cbm::algo::trd
{

  Hit::Hit() : Hit(-1, 0., 0., 0., 0., 0., 0., 0., -1, -1., -1.) {}

  Hit::Hit(int32_t address, const ROOT::Math::XYZVector& pos, const ROOT::Math::XYZVector& dpos, double dxy,
           int32_t refId, double eLoss, double time, double timeError)
    : Hit(address, pos.X(), pos.Y(), pos.Z(), dpos.X(), dpos.Y(), dpos.Z(), dxy, refId, time, timeError)
  {
    fELoss = eLoss;
  }

  Hit::Hit(int32_t address, double x, double y, double z, double dx, double dy, double dz, double dxy, int32_t refId,
           double time, double timeError)
    : fX(x)
    , fY(y)
    , fZ(z)
    , fDx(dx)
    , fDy(dy)
    , fDz(dz)
    , fDxy(dxy)
    , fRefId(refId)
    , fAddress(address)
    , fTime(time)
    , fTimeError(timeError)
    , fDefine(0)
    , fNeighborId(-1)
    , fELoss(-1.)
  {
  }

  void Hit::Position(ROOT::Math::XYZVector& pos) const { pos.SetXYZ(X(), Y(), Z()); }

  void Hit::PositionError(ROOT::Math::XYZVector& dpos) const { dpos.SetXYZ(Dx(), Dy(), Dz()); }

  void Hit::SetPosition(const ROOT::Math::XYZVector& pos)
  {
    SetX(pos.X());
    SetY(pos.Y());
    SetZ(pos.Z());
  }

  void Hit::SetPositionError(const ROOT::Math::XYZVector& dpos)
  {
    SetDx(dpos.X());
    SetDy(dpos.Y());
    SetDz(dpos.Z());
  }


}  // namespace cbm::algo::trd
