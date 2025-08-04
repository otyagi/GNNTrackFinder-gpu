/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#include "HitFactory2D.h"

#include "AlgoFairloggerCompat.h"

#include <numeric>

using std::endl;
using std::vector;

namespace cbm::algo::trd
{

  //_______________________________________________________________________________
  std::pair<double, double> HitFactory2D::CorrectPosition(double dx, double dy, const double xcorr,
                                                          const double padSizeX, const double padSizeY)
  {
    dx -= xcorr;
    RecenterXoffset(dx);
    dy = dx - dy;
    RecenterYoffset(dy);

    const double ycorr = GetYcorr(dy) / padSizeY;
    dy += ycorr;
    RecenterYoffset(dy);
    dx *= padSizeX;
    dy *= padSizeY;
    return std::make_pair(dx, dy);
  }


  //_______________________________________________________________________________
  std::pair<double, double> HitFactory2D::GetDxDy(const int n0)
  {
    double dx, dy;
    switch (n0) {
      case 1:
        if (IsMaxTilt()) {  // T
          dx = -0.5;
          dy = 0;
        }
        else {  // R
          dx = 0.5;
          dy = 0;
        }
        break;
      case 2:
        if (IsOpenLeft() && IsOpenRight()) {  // RT
          dx = viM == 1 ? 0. : -1;
          dy = -0.5;
        }
        else {  // TR
          dx = 0.;
          dy = 0.5;
        }
        break;
      case 3:
        if (IsMaxTilt() && !IsSymmHit()) {  // TRT asymm
          dx = viM == 1 ? 0. : -1;
          dy = GetYoffset();
        }
        else if (!IsMaxTilt() && IsSymmHit()) {  // TRT symm
          dx = 0.;
          dy = GetYoffset();
        }
        else if (IsMaxTilt() && IsSymmHit()) {  // RTR symm
          dx = GetXoffset();
          dy = 0.;
        }
        else if (!IsMaxTilt() && !IsSymmHit()) {  // RTR asymm
          dx = GetXoffset();
          dy = viM == 1 ? -0.5 : 0.5;
        }
        break;
      default:
        dx = GetXoffset();
        dy = GetYoffset();
        break;
    }
    RecenterXoffset(dx);
    return std::make_pair(dx, dy);
  }

  //_______________________________________________________________________________
  double HitFactory2D::GetXoffset(int n0) const
  {
    double dx(0.), R(0.);
    int n(n0 ? n0 : fSignal.size());
    for (int ir(0); ir < n; ir++) {
      if (fSignal[ir].xe > 0) continue;  // select rectangular coupling
      R += fSignal[ir].s;
      dx += fSignal[ir].s * fSignal[ir].x;
    }
    if (std::abs(R) > 0) return dx / R;
    //L_(debug) << "HitFactory2D::GetXoffset : Null total charge for hit size " << n;
    return 0.;
  }

  //_______________________________________________________________________________
  double HitFactory2D::GetYoffset(int n0) const
  {
    double dy(0.), T(0.);
    int n(n0 ? n0 : fSignal.size());
    for (int it(0); it < n; it++) {
      if (fSignal[it].xe > 0) {  // select tilted coupling
        T += fSignal[it].s;
        dy += fSignal[it].s * fSignal[it].x;
      }
    }
    if (std::abs(T) > 0) return dy / T;
    // L_(debug) << "HitFactory2D::GetYoffset : Null total charge for hit size " << n;
    //if (CWRITE(1))
    return 0.;
  }

  //_______________________________________________________________________________
  void HitFactory2D::RecenterXoffset(double& dx)
  {
    /** Shift graph representation to fit dx[pw] in [-0.5, 0.5]
   */

    if (dx >= -0.5 && dx < 0.5) return;
    int ishift = int(dx - 0.5) + (dx > 0.5 ? 1 : 0);
    if (vcM + ishift < 0)
      ishift = -vcM;
    else if (vcM + ishift >= nCols)
      ishift = nCols - vcM - 1;

    dx -= ishift;
    vcM += ishift;
    for (uint idx(0); idx < fSignal.size(); idx++)
      fSignal[idx].x -= ishift;
  }

  //_______________________________________________________________________________
  void HitFactory2D::RecenterYoffset(double& dy)
  {
    /** Shift graph representation to fit dy[ph] in [-0.5, 0.5]
   */

    if (dy >= -0.5 && dy < 0.5) return;
    int ishift = int(dy - 0.5) + (dy > 0.5 ? 1 : 0);
    dy -= ishift;
  }

  //_______________________________________________________________________________
  int HitFactory2D::GetHitClass() const
  {
    /** Incapsulate hit classification criteria based on signal topology
 * [0] : center hit type
 * [1]  : side hit type
 */

    int n0(fSignal.size() - 2);
    if ((n0 == 5 && ((IsMaxTilt() && IsSymmHit()) || (!IsMaxTilt() && !IsSymmHit()))) ||  // TRTRT symm/asymm
        n0 == 4 || (n0 == 3 && ((IsMaxTilt() && IsSymmHit()) || (!IsMaxTilt() && !IsSymmHit()))))
      return 1;  // RTR symm/asymm
    else if (n0 > 5 && HasOvf())
      return 2;
    return 0;
  }

  //_______________________________________________________________________________
  int HitFactory2D::GetHitRcClass(int a0) const
  {
    int a0m      = std::abs(a0);
    uint8_t xmap = vyM & 0xff;
    if (a0m == 2 && IsBiasXleft() && IsBiasXright() && !IsBiasXmid())
      return 0;
    else if (a0m == 3 && ((IsBiasXleft() && IsBiasXright()) || xmap == 116 || xmap == 149 || xmap == 208))
      return 1;
    else if (!IsBiasXleft()
             && (a0m == 2
                 || (a0m == 3 && ((!IsBiasXright() && IsBiasXmid()) || xmap == 209 || xmap == 212 || xmap == 145))))
      return 2;
    else if (!IsBiasXright()
             && (a0m == 2 || (a0m == 3 && ((!IsBiasXleft() && IsBiasXmid()) || xmap == 112 || xmap == 117))))
      return 3;
    else
      return -1;
  }

  //_______________________________________________________________________________
  double HitFactory2D::GetXcorr(double dxIn, int typ, int cls) const
  {
    /** Give the linear interpolation of SYS correction for current position offset "dx" based
 * on LUT calculated wrt MC EbyE data. The position offset is expresed in [pw] units
 * while the output is in [cm]
 */

    if (typ < 0 || typ > 2) {
      //L_(error)<< GetName() << "::GetXcorr : type in-param "<<typ<<" out of range.";
      return 0;
    }
    double dx = std::abs(dxIn);
    int ii    = std::max(0, Nint(dx / fgCorrXdx) - 1), i0;  //  i1;

    if (ii < 0 || ii > NBINSCORRX) {
      // L_(debug) << GetName() << "::GetXcorr : LUT idx " << ii << " out of range for dx=" << dxIn;
      return 0;
    }
    if (dx < fgCorrXdx * ii) {
      i0 = std::max(0, ii - 1);
      /*i1=ii;*/
    }
    else {
      i0 = ii;
      /*i1=TMath::Min(NBINSCORRX-1,ii+1);*/
    }

    float* xval = &fgCorrXval[typ][i0];
    if (cls == 1)
      xval = &fgCorrRcXval[typ][i0];
    else if (cls == 2)
      xval = &fgCorrRcXbiasXval[typ][i0];
    double DDx = (xval[1] - xval[0]), a = DDx / fgCorrXdx, b = xval[0] - DDx * (i0 + 0.5);
    return (dxIn > 0 ? 1 : -1) * b + a * dxIn;
  }

  //_______________________________________________________________________________
  double HitFactory2D::GetYcorr(double dy, int /* cls*/) const
  {
    /** Process y offset. Apply systematic correction for y (MC derived).
 * The position offset is expresed in [pw] units while the output is in [cm]
 */
    float fdy(1.), yoff(0.);
    int n0(fSignal.size() - 2);
    switch (n0) {
      case 3:
        fdy  = fgCorrYval[0][0];
        yoff = fgCorrYval[0][1];
        if (IsMaxTilt() && IsSymmHit()) {
          fdy  = 0.;
          yoff = (dy > 0 ? -1 : 1) * 1.56;
        }
        else if (!IsMaxTilt() && !IsSymmHit()) {
          fdy  = 0.;
          yoff = (dy > 0 ? -1 : 1) * 1.06;
        }
        else if (!IsMaxTilt() && IsSymmHit()) {
          fdy  = 2.114532;
          yoff = -0.263;
        }
        else /*if(IsMaxTilt()&&!IsSymmHit())*/ {
          fdy  = 2.8016010;
          yoff = -1.38391;
        }
        break;
      case 4:
        fdy  = fgCorrYval[1][0];
        yoff = fgCorrYval[1][1];
        if ((!IsMaxTilt() && IsLeftHit()) || (IsMaxTilt() && !IsLeftHit())) yoff *= -1;
        break;
      case 5:
      case 7:
      case 9:
      case 11:
        fdy  = fgCorrYval[2][0];
        yoff = fgCorrYval[2][1];
        break;
      case 6:
      case 8:
      case 10:
        fdy  = fgCorrYval[3][0];
        yoff = fgCorrYval[3][1];
        if ((!IsMaxTilt() && IsLeftHit()) || (IsMaxTilt() && !IsLeftHit())) yoff *= -1;
        break;
    }
    return dy * fdy + yoff;
  }

  //_______________________________________________________________________________
  bool HitFactory2D::IsOpenRight() const
  {
    int nR = fSignal.size() - 1 - viM;
    return (nR % 2 && IsMaxTilt()) || (!(nR % 2) && !IsMaxTilt());
  }

  float HitFactory2D::fgCorrXdx                 = 0.01;
  float HitFactory2D::fgCorrXval[3][NBINSCORRX] = {
    {-0.001, -0.001, -0.002, -0.002, -0.003, -0.003, -0.003, -0.004, -0.004, -0.006, -0.006, -0.006, -0.007,
     -0.007, -0.008, -0.008, -0.008, -0.009, -0.009, -0.011, -0.011, -0.011, -0.012, -0.012, -0.012, -0.012,
     -0.013, -0.013, -0.013, -0.013, -0.014, -0.014, -0.014, -0.014, -0.014, -0.016, -0.016, -0.016, -0.016,
     -0.017, -0.017, -0.017, -0.018, -0.018, -0.018, -0.018, -0.018, 0.000,  0.000,  0.000},
    {0.467, 0.430, 0.396, 0.364, 0.335, 0.312, 0.291, 0.256, 0.234, 0.219, 0.207, 0.191, 0.172,
     0.154, 0.147, 0.134, 0.123, 0.119, 0.109, 0.122, 0.113, 0.104, 0.093, 0.087, 0.079, 0.073,
     0.067, 0.063, 0.058, 0.053, 0.049, 0.046, 0.042, 0.038, 0.036, 0.032, 0.029, 0.027, 0.024,
     0.022, 0.019, 0.017, 0.014, 0.013, 0.011, 0.009, 0.007, 0.004, 0.003, 0.001},
    {0.001,  0.001,  0.001,  0.001,  0.002,  0.002,  0.001,  0.002,  0.004,  0.003,  0.002,  0.002,  0.002,
     0.002,  0.002,  0.002,  0.003,  0.004,  0.003,  0.004,  0.004,  0.007,  0.003,  0.004,  0.002,  0.002,
     -0.011, -0.011, -0.012, -0.012, -0.012, -0.013, -0.013, -0.013, -0.014, -0.014, -0.014, -0.016, -0.016,
     -0.016, -0.017, -0.017, -0.017, -0.018, -0.018, -0.018, -0.019, 0.029,  0.018,  0.001}};
  float HitFactory2D::fgCorrYval[NBINSCORRY][2]   = {{2.421729, 0.},
                                                   {0.629389, -0.215285},
                                                   {0.23958, 0.},
                                                   {0.151913, 0.054404}};
  float HitFactory2D::fgCorrRcXval[2][NBINSCORRX] = {
    {-0.00050, -0.00050, -0.00150, -0.00250, -0.00250, -0.00350, -0.00450, -0.00450, -0.00550, -0.00650,
     -0.00650, -0.00750, -0.00850, -0.00850, -0.00850, -0.00950, -0.00950, -0.00950, -0.01050, -0.01150,
     -0.01150, -0.01150, -0.01250, -0.01250, -0.01250, -0.01250, -0.01350, -0.01350, -0.01350, -0.01350,
     -0.01450, -0.01450, -0.01450, -0.01550, -0.01550, -0.01550, -0.01550, -0.01650, -0.01650, -0.01550,
     -0.01650, -0.01614, -0.01620, -0.01624, -0.01626, -0.01627, -0.01626, -0.01624, -0.01620, -0.01615},
    {0.36412, 0.34567, 0.32815, 0.31152, 0.29574, 0.28075, 0.26652, 0.25302, 0.24020, 0.22803,
     0.21647, 0.21400, 0.19400, 0.18520, 0.17582, 0.16600, 0.14600, 0.13800, 0.14280, 0.14200,
     0.13400, 0.12600, 0.12200, 0.11000, 0.10200, 0.09400, 0.09000, 0.08600, 0.08200, 0.07400,
     0.07000, 0.06600, 0.06600, 0.06200, 0.05800, 0.05400, 0.05400, 0.05000, 0.04600, 0.04600,
     0.04200, 0.03800, 0.03800, 0.03400, 0.03400, 0.03000, 0.03000, 0.02600, 0.02200, 0.02200}};
  float HitFactory2D::fgCorrRcXbiasXval[3][NBINSCORRX] = {
    {0.00100, 0.00260, 0.00540, 0.00740, 0.00900, 0.01060, 0.01300, 0.01460, 0.01660, 0.01900,
     0.02060, 0.02260, 0.02420, 0.02700, 0.02860, 0.02980, 0.03220, 0.03340, 0.03540, 0.03620,
     0.03820, 0.04020, 0.04180, 0.04340, 0.04460, 0.04620, 0.04740, 0.04941, 0.05088, 0.05233,
     0.05375, 0.05515, 0.05653, 0.05788, 0.05921, 0.06052, 0.06180, 0.06306, 0.06430, 0.06551,
     0.06670, 0.06786, 0.06901, 0.07012, 0.07122, 0.07229, 0.07334, 0.07436, 0.07536, 0.07634},
    {0.00100, 0.00380, 0.00780, 0.00900, 0.01220, 0.01460, 0.01860, 0.01940, 0.02260, 0.02540,
     0.02820, 0.03060, 0.03220, 0.03660, 0.03980, 0.04094, 0.04420, 0.04620, 0.04824, 0.04980,
     0.05298, 0.05532, 0.05740, 0.05991, 0.06217, 0.06500, 0.06540, 0.06900, 0.07096, 0.07310,
     0.07380, 0.07729, 0.07935, 0.08139, 0.08340, 0.08538, 0.08734, 0.08928, 0.08900, 0.09307,
     0.09493, 0.09340, 0.09858, 0.09620, 0.09740, 0.10386, 0.09980, 0.10726, 0.10892, 0.11056},
    {0.00011, 0.00140, 0.00340, 0.00420, 0.00500, 0.00620, 0.00820, 0.00860, 0.01060, 0.01100,
     0.01220, 0.01340, 0.01500, 0.01540, 0.01700, 0.01820, 0.01900, 0.02060, 0.02180, 0.02260,
     0.02340, 0.02420, 0.02500, 0.02500, 0.02660, 0.02740, 0.02820, 0.02900, 0.03020, 0.03180,
     0.03300, 0.03260, 0.03380, 0.03460, 0.03500, 0.03580, 0.03780, 0.03820, 0.03860, 0.03900,
     0.04100, 0.04180, 0.04060, 0.04300, 0.04340, 0.04340, 0.04380, 0.04460, 0.04580, 0.04540}};

}  // namespace cbm::algo::trd
