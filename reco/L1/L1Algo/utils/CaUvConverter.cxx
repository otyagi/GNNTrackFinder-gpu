/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

#include "CaUvConverter.h"

#include <Logger.h>

#include <cmath>
#include <iostream>

using namespace cbm::ca;

//----------------------------------------------------------------------------------------------------------------------
//
void CaUvConverter::SetFromUV(double phiU, double phiV)
{
  fcosU = cos(phiU);
  fsinU = sin(phiU);
  fcosV = cos(phiV);
  fsinV = sin(phiV);

  double det = fcosU * fsinV - fsinU * fcosV;
  if (fabs(det) < 1.e-2) {
    LOG(error) << "CaUvConverter: U & V coordinates are too close: " << phiU << " " << phiV;
    phiV  = phiU + 10. / 180. * M_PI;
    fcosV = cos(phiV);
    fsinV = sin(phiV);
    det   = fcosU * fsinV - fsinU * fcosV;
  }
  fcosX = fsinV / det;
  fsinX = -fsinU / det;
  fcosY = -fcosV / det;
  fsinY = fcosU / det;
}

//----------------------------------------------------------------------------------------------------------------------
//
void CaUvConverter::SetFromXYCovMatrix(double phiU, double dx2, double dxy, double dy2)
{
  // take U coordinate from fPhiU
  double cosU = cos(phiU);
  double sinU = sin(phiU);
  double du2  = cosU * cosU * dx2 + 2 * sinU * cosU * dxy + sinU * sinU * dy2;

  // take V coordinate from the hit covariance matrix, making V uncorrelated with U

  // rotate X,Y coordinates on angle fPhiU (X,Y) -> (U,W)

  //double du2 = cosU * cosU * dx2 + 2. * sinU * cosU * dxy + sinU * sinU * dy2;
  double duw = sinU * cosU * (dy2 - dx2) + (cosU * cosU - sinU * sinU) * dxy;
  //double dw2 = cosU * cosU * dy2 - 2. * sinU * cosU * dxy + sinU * sinU * dx2;

  // find an angle in rotaded coordinates, that has 0 covariance with U
  double phiV = phiU + atan2(du2, -duw);

  SetFromUV(phiU, phiV);

  // check that u/v covariance is 0.
  auto [tmp1, duv, dv2] = ConvertCovMatrixXYtoUV(dx2, dxy, dy2);

  if (fabs(duv) > 1.e-8) {
    LOG(error) << "can not define V coordinate from XY covariance matrix";
  }

  //LOG(info) << " u " << phiU / M_PI * 180. << "v " << phiV / M_PI * 180. << " du " << sqrt(du2) << " dv " << sqrt(dv2)
  //        << " duv " << duv;
}
