/* Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak [committer], Valentina Akishina */

#include "KfTrackKalmanFilter.h"

#include "AlgoFairloggerCompat.h"
#include "KfMeasurementU.h"
#include "KfMeasurementXy.h"

namespace cbm::algo::kf
{

  template<typename DataT>
  void TrackKalmanFilter<DataT>::Filter1d(const kf::MeasurementU<DataT>& m)
  {
    DataT zeta, HCH;
    DataT F0, F1, F2, F3, F4, F5, F6;
    DataT K1, K2, K3, K4, K5, K6;

    zeta = m.CosPhi() * fTr.X() + m.SinPhi() * fTr.Y() - m.U();

    // F = CH'
    F0 = m.CosPhi() * fTr.C00() + m.SinPhi() * fTr.C10();
    F1 = m.CosPhi() * fTr.C10() + m.SinPhi() * fTr.C11();

    HCH = (F0 * m.CosPhi() + F1 * m.SinPhi());

    F2 = m.CosPhi() * fTr.C20() + m.SinPhi() * fTr.C21();
    F3 = m.CosPhi() * fTr.C30() + m.SinPhi() * fTr.C31();
    F4 = m.CosPhi() * fTr.C40() + m.SinPhi() * fTr.C41();
    F5 = m.CosPhi() * fTr.C50() + m.SinPhi() * fTr.C51();
    F6 = m.CosPhi() * fTr.C60() + m.SinPhi() * fTr.C61();

    constexpr bool doProtect = !std::is_same<DataTscal, double>::value;

    const DataTmask maskDoFilter = doProtect ? (HCH < m.Du2() * 16.f) : DataTmask(true);

    // correction to HCH is needed for the case when sigma2 is so small
    // with respect to HCH that it disappears due to the roundoff error
    //
    DataT w  = m.Du2() + (doProtect ? (DataT(1.0000001) * HCH) : HCH);
    DataT wi = kf::utils::iif(fMask, DataT(1.) / w, DataT(0.));

    DataT zetawi = zeta / (kf::utils::iif(maskDoFilter, m.Du2(), DataT(0.)) + HCH);
    zetawi       = kf::utils::iif(fMask, zetawi, DataT(0.));

    wi = kf::utils::iif(m.Du2() > DataT(0.), wi, DataT(0.));

    fTr.ChiSq() += zeta * zeta * wi;
    fTr.Ndf() += kf::utils::iif(fMask, m.Ndf(), DataT(0.));

    K1 = F1 * wi;
    K2 = F2 * wi;
    K3 = F3 * wi;
    K4 = F4 * wi;
    K5 = F5 * wi;
    K6 = F6 * wi;

    fTr.X() -= F0 * zetawi;
    fTr.Y() -= F1 * zetawi;
    fTr.Tx() -= F2 * zetawi;
    fTr.Ty() -= F3 * zetawi;
    fTr.Qp() -= F4 * zetawi;
    fTr.Time() -= F5 * zetawi;
    fTr.Vi() -= F6 * zetawi;

    fTr.C00() -= F0 * F0 * wi;

    fTr.C10() -= K1 * F0;
    fTr.C11() -= K1 * F1;

    fTr.C20() -= K2 * F0;
    fTr.C21() -= K2 * F1;
    fTr.C22() -= K2 * F2;

    fTr.C30() -= K3 * F0;
    fTr.C31() -= K3 * F1;
    fTr.C32() -= K3 * F2;
    fTr.C33() -= K3 * F3;

    fTr.C40() -= K4 * F0;
    fTr.C41() -= K4 * F1;
    fTr.C42() -= K4 * F2;
    fTr.C43() -= K4 * F3;
    fTr.C44() -= K4 * F4;

    fTr.C50() -= K5 * F0;
    fTr.C51() -= K5 * F1;
    fTr.C52() -= K5 * F2;
    fTr.C53() -= K5 * F3;
    fTr.C54() -= K5 * F4;
    fTr.C55() -= K5 * F5;

    fTr.C60() -= K6 * F0;
    fTr.C61() -= K6 * F1;
    fTr.C62() -= K6 * F2;
    fTr.C63() -= K6 * F3;
    fTr.C64() -= K6 * F4;
    fTr.C65() -= K6 * F5;
    fTr.C66() -= K6 * F6;
  }

  template<typename DataT>
  void TrackKalmanFilter<DataT>::FilterTime(DataT t, DataT dt2, const DataTmask& timeInfo)
  {
    // filter track with a time measurement

    // F = CH'
    DataT F0 = fTr.C50();
    DataT F1 = fTr.C51();
    DataT F2 = fTr.C52();
    DataT F3 = fTr.C53();
    DataT F4 = fTr.C54();
    DataT F5 = fTr.C55();
    DataT F6 = fTr.C65();

    DataT HCH = fTr.C55();

    DataTmask mask = fMask && timeInfo;

    // when dt0 is much smaller than current time error,
    // set track time exactly to the measurement value without filtering
    // it helps to keep the initial time errors reasonably small
    // the calculations in the covariance matrix are not affected

    const DataTmask maskDoFilter = mask && (HCH < dt2 * 16.f);

    DataT wi     = kf::utils::iif(mask, DataT(1.) / (dt2 + DataT(1.0000001) * HCH), DataT(0.));
    DataT zeta   = kf::utils::iif(mask, fTr.Time() - t, DataT(0.));
    DataT zetawi = kf::utils::iif(mask, zeta / (kf::utils::iif(maskDoFilter, dt2, DataT(0.)) + HCH), DataT(0.));

    fTr.ChiSqTime() += kf::utils::iif(maskDoFilter, zeta * zeta * wi, DataT(0.));
    fTr.NdfTime() += kf::utils::iif(mask, DataT(1.), DataT(0.));

    DataT K1 = F1 * wi;
    DataT K2 = F2 * wi;
    DataT K3 = F3 * wi;
    DataT K4 = F4 * wi;
    DataT K5 = F5 * wi;
    DataT K6 = F6 * wi;

    fTr.X() -= F0 * zetawi;
    fTr.Y() -= F1 * zetawi;
    fTr.Tx() -= F2 * zetawi;
    fTr.Ty() -= F3 * zetawi;
    fTr.Qp() -= F4 * zetawi;
    fTr.Time() -= F5 * zetawi;
    fTr.Vi() -= F6 * zetawi;

    fTr.C00() -= F0 * F0 * wi;

    fTr.C10() -= K1 * F0;
    fTr.C11() -= K1 * F1;

    fTr.C20() -= K2 * F0;
    fTr.C21() -= K2 * F1;
    fTr.C22() -= K2 * F2;

    fTr.C30() -= K3 * F0;
    fTr.C31() -= K3 * F1;
    fTr.C32() -= K3 * F2;
    fTr.C33() -= K3 * F3;

    fTr.C40() -= K4 * F0;
    fTr.C41() -= K4 * F1;
    fTr.C42() -= K4 * F2;
    fTr.C43() -= K4 * F3;
    fTr.C44() -= K4 * F4;

    fTr.C50() -= K5 * F0;
    fTr.C51() -= K5 * F1;
    fTr.C52() -= K5 * F2;
    fTr.C53() -= K5 * F3;
    fTr.C54() -= K5 * F4;
    fTr.C55() -= K5 * F5;

    fTr.C60() -= K6 * F0;
    fTr.C61() -= K6 * F1;
    fTr.C62() -= K6 * F2;
    fTr.C63() -= K6 * F3;
    fTr.C64() -= K6 * F4;
    fTr.C65() -= K6 * F5;
    fTr.C66() -= K6 * F6;
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::FilterXY(const kf::MeasurementXy<DataT>& mxy, bool skipUnmeasuredCoordinates)
  {
    {
      kf::MeasurementU<DataT> mx;
      mx.SetCosPhi(DataT(1.));
      mx.SetSinPhi(DataT(0.));
      mx.SetU(mxy.X());
      mx.SetDu2(mxy.Dx2());
      mx.SetNdf(mxy.NdfX());

      kf::MeasurementU<DataT> mu;
      mu.SetCosPhi(-mxy.Dxy() / mxy.Dx2());
      mu.SetSinPhi(DataT(1.));
      mu.SetU(mu.CosPhi() * mxy.X() + mxy.Y());
      mu.SetDu2(mxy.Dy2() - mxy.Dxy() * mxy.Dxy() / mxy.Dx2());
      mu.SetNdf(mxy.NdfY());

      auto maskOld = fMask;
      if (skipUnmeasuredCoordinates) {
        fMask = maskOld & (mxy.NdfX() > DataT(0.));
      }
      Filter1d(mx);
      if (skipUnmeasuredCoordinates) {
        fMask = maskOld & (mxy.NdfY() > DataT(0.));
      }
      Filter1d(mu);
      fMask = maskOld;

      return;
    }

    //----------------------------------------------------------------------------------------------
    // the other way: filter 2 dimensions at once

    const DataT TWO(2.);

    DataT zeta0, zeta1, S00, S10, S11, si;
    DataT F00, F10, F20, F30, F40, F50, F60;
    DataT F01, F11, F21, F31, F41, F51, F61;
    DataT K00, K10, K20, K30, K40, K50, K60;
    DataT K01, K11, K21, K31, K41, K51, K61;

    zeta0 = fTr.X() - mxy.X();
    zeta1 = fTr.Y() - mxy.Y();

    // F = CH'
    F00 = fTr.C00();
    F10 = fTr.C10();
    F20 = fTr.C20();
    F30 = fTr.C30();
    F40 = fTr.C40();
    F50 = fTr.C50();
    F60 = fTr.C60();

    F01 = fTr.C10();
    F11 = fTr.C11();
    F21 = fTr.C21();
    F31 = fTr.C31();
    F41 = fTr.C41();
    F51 = fTr.C51();
    F61 = fTr.C61();

    S00 = F00 + mxy.Dx2();
    S10 = F10 + mxy.Dxy();
    S11 = F11 + mxy.Dy2();

    si           = 1.f / (S00 * S11 - S10 * S10);
    DataT S00tmp = S00;
    S00          = si * S11;
    S10          = -si * S10;
    S11          = si * S00tmp;

    fTr.ChiSq() += zeta0 * zeta0 * S00 + 2.f * zeta0 * zeta1 * S10 + zeta1 * zeta1 * S11;
    fTr.Ndf() += TWO;

    K00 = F00 * S00 + F01 * S10;
    K01 = F00 * S10 + F01 * S11;

    K10 = F10 * S00 + F11 * S10;
    K11 = F10 * S10 + F11 * S11;

    K20 = F20 * S00 + F21 * S10;
    K21 = F20 * S10 + F21 * S11;

    K30 = F30 * S00 + F31 * S10;
    K31 = F30 * S10 + F31 * S11;

    K40 = F40 * S00 + F41 * S10;
    K41 = F40 * S10 + F41 * S11;

    K50 = F50 * S00 + F51 * S10;
    K51 = F50 * S10 + F51 * S11;

    K60 = F60 * S00 + F61 * S10;
    K61 = F60 * S10 + F61 * S11;

    fTr.X() -= K00 * zeta0 + K01 * zeta1;
    fTr.Y() -= K10 * zeta0 + K11 * zeta1;
    fTr.Tx() -= K20 * zeta0 + K21 * zeta1;
    fTr.Ty() -= K30 * zeta0 + K31 * zeta1;
    fTr.Qp() -= K40 * zeta0 + K41 * zeta1;
    fTr.Time() -= K50 * zeta0 + K51 * zeta1;
    fTr.Vi() -= K60 * zeta0 + K61 * zeta1;

    fTr.C00() -= K00 * F00 + K01 * F01;

    fTr.C10() -= K10 * F00 + K11 * F01;
    fTr.C11() -= K10 * F10 + K11 * F11;

    fTr.C20() -= K20 * F00 + K21 * F01;
    fTr.C21() -= K20 * F10 + K21 * F11;
    fTr.C22() -= K20 * F20 + K21 * F21;

    fTr.C30() -= K30 * F00 + K31 * F01;
    fTr.C31() -= K30 * F10 + K31 * F11;
    fTr.C32() -= K30 * F20 + K31 * F21;
    fTr.C33() -= K30 * F30 + K31 * F31;

    fTr.C40() -= K40 * F00 + K41 * F01;
    fTr.C41() -= K40 * F10 + K41 * F11;
    fTr.C42() -= K40 * F20 + K41 * F21;
    fTr.C43() -= K40 * F30 + K41 * F31;
    fTr.C44() -= K40 * F40 + K41 * F41;

    fTr.C50() -= K50 * F00 + K51 * F01;
    fTr.C51() -= K50 * F10 + K51 * F11;
    fTr.C52() -= K50 * F20 + K51 * F21;
    fTr.C53() -= K50 * F30 + K51 * F31;
    fTr.C54() -= K50 * F40 + K51 * F41;
    fTr.C55() -= K50 * F50 + K51 * F51;

    fTr.C60() -= K60 * F00 + K61 * F01;
    fTr.C61() -= K60 * F10 + K61 * F11;
    fTr.C62() -= K60 * F20 + K61 * F21;
    fTr.C63() -= K60 * F30 + K61 * F31;
    fTr.C64() -= K60 * F40 + K61 * F41;
    fTr.C65() -= K60 * F50 + K61 * F51;
    fTr.C66() -= K60 * F60 + K61 * F61;
  }

  template<typename DataT>
  void TrackKalmanFilter<DataT>::FilterExtrapolatedXY(const kf::MeasurementXy<DataT>& m, DataT extrX, DataT extrY,
                                                      const std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jx,
                                                      const std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jy)
  {
    // add a 2-D measurenent (x,y) at some z, that differs from fTr.GetZ()
    // extrX, extrY are extrapolated track parameters at z, Jx, Jy are derivatives of the extrapolation

    // ! it is assumed that in the track covariance matrix all non-diagonal covariances are 0
    // ! except of C10

    auto& T = fTr;

    //zeta0 = T.x + Jx[2]*T.Tx() + Jx[3]*T.Ty() + Jx[4]*T.qp - x;
    //zeta1 = T.y + Jy[2]*T.Tx() + Jy[3]*T.Ty() + Jy[4]*T.qp - y;

    DataT zeta0 = extrX - m.X();
    DataT zeta1 = extrY - m.Y();

    // H = 1 0 Jx[2] Jx[3] Jx[4] 0
    //     0 1 Jy[2] Jy[3] Jy[4] 0

    // F = CH'
    DataT F00 = T.C00();
    DataT F01 = T.C10();
    DataT F10 = T.C10();
    DataT F11 = T.C11();
    DataT F20 = Jx[2] * T.C22();
    DataT F21 = Jy[2] * T.C22();
    DataT F30 = Jx[3] * T.C33();
    DataT F31 = Jy[3] * T.C33();
    DataT F40 = Jx[4] * T.C44();
    DataT F41 = Jy[4] * T.C44();

    // Jx[5,6] and Jy[5,6] are 0.

    DataT S00 = m.Dx2() + F00 + Jx[2] * F20 + Jx[3] * F30 + Jx[4] * F40;
    DataT S10 = m.Dxy() + F10 + Jy[2] * F20 + Jy[3] * F30 + Jy[4] * F40;
    DataT S11 = m.Dy2() + F11 + Jy[2] * F21 + Jy[3] * F31 + Jy[4] * F41;

    DataT si = DataT(1.) / (S00 * S11 - S10 * S10);

    DataT S00tmp = S00;
    S00          = si * S11;
    S10          = -si * S10;
    S11          = si * S00tmp;

    T.ChiSq() += zeta0 * zeta0 * S00 + DataT(2.) * zeta0 * zeta1 * S10 + zeta1 * zeta1 * S11;
    T.Ndf() += m.NdfX() + m.NdfY();

    DataT K00 = F00 * S00 + F01 * S10;
    DataT K01 = F00 * S10 + F01 * S11;
    DataT K10 = F10 * S00 + F11 * S10;
    DataT K11 = F10 * S10 + F11 * S11;
    DataT K20 = F20 * S00 + F21 * S10;
    DataT K21 = F20 * S10 + F21 * S11;
    DataT K30 = F30 * S00 + F31 * S10;
    DataT K31 = F30 * S10 + F31 * S11;
    DataT K40 = F40 * S00 + F41 * S10;
    DataT K41 = F40 * S10 + F41 * S11;

    T.X() -= K00 * zeta0 + K01 * zeta1;
    T.Y() -= K10 * zeta0 + K11 * zeta1;
    T.Tx() -= K20 * zeta0 + K21 * zeta1;
    T.Ty() -= K30 * zeta0 + K31 * zeta1;
    T.Qp() -= K40 * zeta0 + K41 * zeta1;

    T.C00() -= (K00 * F00 + K01 * F01);
    T.C10() -= (K10 * F00 + K11 * F01);
    T.C11() -= (K10 * F10 + K11 * F11);
    T.C20() = -(K20 * F00 + K21 * F01);
    T.C21() = -(K20 * F10 + K21 * F11);
    T.C22() -= (K20 * F20 + K21 * F21);
    T.C30() = -(K30 * F00 + K31 * F01);
    T.C31() = -(K30 * F10 + K31 * F11);
    T.C32() = -(K30 * F20 + K31 * F21);
    T.C33() -= (K30 * F30 + K31 * F31);
    T.C40() = -(K40 * F00 + K41 * F01);
    T.C41() = -(K40 * F10 + K41 * F11);
    T.C42() = -(K40 * F20 + K41 * F21);
    T.C43() = -(K40 * F30 + K41 * F31);
    T.C44() -= (K40 * F40 + K41 * F41);
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::MeasureVelocityWithQp()
  {
    // measure velocity using measured qp
    // assuming particle mass == fMass;

    const DataT kClightNsInv = kf::defs::SpeedOfLightInv<DataT>;

    DataT zeta, HCH;
    DataT F0, F1, F2, F3, F4, F5, F6;
    DataT K1, K2, K3, K4, K5, K6;

    //FilterVi(sqrt(DataT(1.) + fMass2 * fQp0 * fQp0) * kClightNsInv);
    //return;

    DataT vi0 = sqrt(DataT(1.) + fMass2 * fQp0 * fQp0) * kClightNsInv;

    DataT h = fMass2 * fQp0 / sqrt(DataT(1.) + fMass2 * fQp0 * fQp0) * kClightNsInv;

    zeta = vi0 + h * (fTr.Qp() - fQp0) - fTr.Vi();

    fTr.Vi() = vi0;

    // H = (0,0,0,0, h,0, -1)

    // F = CH'

    F0 = h * fTr.C40() - fTr.C60();
    F1 = h * fTr.C41() - fTr.C61();
    F2 = h * fTr.C42() - fTr.C62();
    F3 = h * fTr.C43() - fTr.C63();
    F4 = h * fTr.C44() - fTr.C64();
    F5 = h * fTr.C54() - fTr.C65();
    F6 = h * fTr.C64() - fTr.C66();

    HCH = F4 * h - F6;

    DataT wi     = kf::utils::iif(fMask, DataT(1.) / HCH, DataT(0.));
    DataT zetawi = kf::utils::iif(fMask, zeta / HCH, DataT(0.));
    fTr.ChiSqTime() += kf::utils::iif(fMask, zeta * zeta * wi, DataT(0.));
    fTr.NdfTime() += kf::utils::iif(fMask, DataT(1.), DataT(0.));

    K1 = F1 * wi;
    K2 = F2 * wi;
    K3 = F3 * wi;
    K4 = F4 * wi;
    K5 = F5 * wi;
    K6 = F6 * wi;

    fTr.X() -= F0 * zetawi;
    fTr.Y() -= F1 * zetawi;
    fTr.Tx() -= F2 * zetawi;
    fTr.Ty() -= F3 * zetawi;
    fTr.Qp() -= F4 * zetawi;
    fTr.Time() -= F5 * zetawi;
    fTr.Vi() -= F6 * zetawi;

    fTr.C00() -= F0 * F0 * wi;

    fTr.C10() -= K1 * F0;
    fTr.C11() -= K1 * F1;

    fTr.C20() -= K2 * F0;
    fTr.C21() -= K2 * F1;
    fTr.C22() -= K2 * F2;

    fTr.C30() -= K3 * F0;
    fTr.C31() -= K3 * F1;
    fTr.C32() -= K3 * F2;
    fTr.C33() -= K3 * F3;

    fTr.C40() -= K4 * F0;
    fTr.C41() -= K4 * F1;
    fTr.C42() -= K4 * F2;
    fTr.C43() -= K4 * F3;
    fTr.C44() -= K4 * F4;

    fTr.C50() -= K5 * F0;
    fTr.C51() -= K5 * F1;
    fTr.C52() -= K5 * F2;
    fTr.C53() -= K5 * F3;
    fTr.C54() -= K5 * F4;
    fTr.C55() -= K5 * F5;

    fTr.C60() -= K6 * F0;
    fTr.C61() -= K6 * F1;
    fTr.C62() -= K6 * F2;
    fTr.C63() -= K6 * F3;
    fTr.C64() -= K6 * F4;
    fTr.C65() -= K6 * F5;
    fTr.C66() -= K6 * F6;

    //  fTr.Vi()( fTr.Vi() < DataT(TrackParamV::kClightNsInv) ) = DataT(TrackParamV::kClightNsInv);
  }

  template<typename DataT>
  void TrackKalmanFilter<DataT>::FilterVi(DataT vi)
  {
    // set inverse velocity to vi

    DataT zeta, HCH;
    DataT F0, F1, F2, F3, F4, F5, F6;
    DataT K1, K2, K3, K4, K5;  //, K6;

    zeta = fTr.Vi() - vi;

    // H = (0,0,0,0, 0, 0, 1)

    // F = CH'

    F0 = fTr.C60();
    F1 = fTr.C61();
    F2 = fTr.C62();
    F3 = fTr.C63();
    F4 = fTr.C64();
    F5 = fTr.C65();
    F6 = fTr.C66();

    HCH = F6;

    DataT wi     = kf::utils::iif(fMask, DataT(1.) / HCH, DataT(0.));
    DataT zetawi = kf::utils::iif(fMask, zeta / HCH, DataT(0.));
    fTr.ChiSqTime() += kf::utils::iif(fMask, zeta * zeta * wi, DataT(0.));
    fTr.NdfTime() += kf::utils::iif(fMask, DataT(1.), DataT(0.));

    K1 = F1 * wi;
    K2 = F2 * wi;
    K3 = F3 * wi;
    K4 = F4 * wi;
    K5 = F5 * wi;
    // K6 = F6 * wi;

    fTr.X() -= F0 * zetawi;
    fTr.Y() -= F1 * zetawi;
    fTr.Tx() -= F2 * zetawi;
    fTr.Ty() -= F3 * zetawi;
    fTr.Qp() -= F4 * zetawi;
    fTr.Time() -= F5 * zetawi;
    // fTr.Vi() -= F6 * zetawi;
    fTr.Vi() = vi;

    fTr.C00() -= F0 * F0 * wi;

    fTr.C10() -= K1 * F0;
    fTr.C11() -= K1 * F1;

    fTr.C20() -= K2 * F0;
    fTr.C21() -= K2 * F1;
    fTr.C22() -= K2 * F2;

    fTr.C30() -= K3 * F0;
    fTr.C31() -= K3 * F1;
    fTr.C32() -= K3 * F2;
    fTr.C33() -= K3 * F3;

    fTr.C40() -= K4 * F0;
    fTr.C41() -= K4 * F1;
    fTr.C42() -= K4 * F2;
    fTr.C43() -= K4 * F3;
    fTr.C44() -= K4 * F4;

    fTr.C50() -= K5 * F0;
    fTr.C51() -= K5 * F1;
    fTr.C52() -= K5 * F2;
    fTr.C53() -= K5 * F3;
    fTr.C54() -= K5 * F4;
    fTr.C55() -= K5 * F5;

    //fTr.C60() -= K6 * F0;
    //fTr.C61() -= K6 * F1;
    //fTr.C62() -= K6 * F2;
    //fTr.C63() -= K6 * F3;
    //fTr.C64() -= K6 * F4;
    //fTr.C65() -= K6 * F5;
    //fTr.C66() -= K6 * F6;
    fTr.C60() = DataT(0.);
    fTr.C61() = DataT(0.);
    fTr.C62() = DataT(0.);
    fTr.C63() = DataT(0.);
    fTr.C64() = DataT(0.);
    fTr.C65() = DataT(0.);
    fTr.C66() = DataT(1.e-8);  // just for a case..
  }

  template<typename DataT>
  void TrackKalmanFilter<
    DataT>::Extrapolate  // extrapolates track parameters and returns jacobian for extrapolation of CovMatrix
    (DataT z_out,        // extrapolate to this z position
     const kf::FieldRegion<DataT>& F)
  {
    // use Q/p linearisation at fQp0

    if (F.GetFieldType() == kf::EFieldType::Null) {
      ExtrapolateLineNoField(z_out);
    }
    else {
      DataT sgn = kf::utils::iif(fTr.GetZ() < z_out, DataT(1.), DataT(-1.));
      while (
        !kf::utils::isFull(kf::utils::iif(fMask, kf::utils::fabs(z_out - fTr.GetZ()), DataT(0.)) <= DataT(1.e-6))) {
        DataT zNew = fTr.GetZ() + sgn * fMaxExtraplationStep;  // max. 50 cm step
        zNew       = kf::utils::iif(sgn * (z_out - zNew) <= DataT(0.), z_out, zNew);
        ExtrapolateStep(zNew, F);
      }
    }
  }

  template<typename DataT>
  void TrackKalmanFilter<
    DataT>::ExtrapolateStep  // extrapolates track parameters and returns jacobian for extrapolation of CovMatrix
    (DataT zOut,             // extrapolate to this z position
     const kf::FieldRegion<DataT>& Field)
  {
    // use Q/p linearisation at fQp0
    // implementation of the Runge-Kutta method without optimization
    //

    //
    // Forth-order Runge-Kutta method for solution of the equation
    // of motion of a particle with parameter qp = Q /P
    //              in the magnetic field B()
    //
    //   ( x )            tx
    //   ( y )            ty
    //   ( tx)        c_light * qp * L * (     tx*ty * Bx - (1+tx*tx) * By + ty * Bz  )
    // d ( ty) / dz = c_light * qp * L * ( (1+ty*ty) * Bx     - tx*ty * By - tx * Bz  )  ,
    //   ( qp)             0.
    //   ( t )         L * vi
    //   ( vi)             0.
    //
    //   where  L = sqrt ( 1 + tx*tx + ty*ty ) .
    //   c_light = 0.000299792458 [(GeV/c)/kG/cm]
    //   c_light_ns =  29.9792458 [cm/ns]
    //
    //  In the following for RK step :
    //   r[7] = {x, y, tx, ty, qp, t, vi}
    //   dr(z)/dz = f(z,r)
    //
    //
    //========================================================================
    //
    //  NIM A395 (1997) 169-184; NIM A426 (1999) 268-282.
    //
    //  the routine is based on LHC(b) utility code
    //
    //========================================================================

    // TODO: check the units, where does 1.e-5 factor come?
    // c_light = 0.000299792458; //  [(GeV/c)/kG/cm]
    // c_light_ns =  29.9792458 [cm/ns]

    cnst c_light = DataT(1.e-5 * kf::defs::SpeedOfLight<double>);

    //----------------------------------------------------------------

    cnst zMasked = kf::utils::iif(fMask, zOut, fTr.GetZ());

    cnst h = (zMasked - fTr.GetZ());

    cnst stepDz[5] = {0., 0., h * DataT(0.5), h * DataT(0.5), h};

    DataT f[5][7]    = {{DataT(0.)}};    // ( d*/dz  ) [step]
    DataT F[5][7][7] = {{{DataT(0.)}}};  // ( d *new [step] / d *old  )

    //   Runge-Kutta steps
    //

    DataT r0[7]    = {fTr.X(), fTr.Y(), fTr.Tx(), fTr.Ty(), fQp0, fTr.Time(), fTr.Vi()};
    DataT R0[7][7] = {{DataT(0.)}};
    for (int i = 0; i < 7; ++i) {
      R0[i][i] = 1.;
    }

    for (int step = 1; step <= 4; ++step) {

      DataT rstep[7] = {DataT(0.)};
      for (int i = 0; i < 7; ++i) {
        rstep[i] = r0[i] + stepDz[step] * f[step - 1][i];
      }
      DataT z = fTr.GetZ() + stepDz[step];

      kf::FieldValue B = Field.Get(rstep[0], rstep[1], z);
      DataT Bx         = B.GetBx();
      DataT By         = B.GetBy();
      DataT Bz         = B.GetBz();
      // NOTE: SZh 13.08.2024: The rstep[0] and rstep[1] make no effect on the B value, if Field is an approximation

      DataT tx    = rstep[2];
      DataT ty    = rstep[3];
      DataT tx2   = tx * tx;
      DataT ty2   = ty * ty;
      DataT txty  = tx * ty;
      DataT L2    = DataT(1.) + tx2 + ty2;
      DataT L2i   = DataT(1.) / L2;
      DataT L     = sqrt(L2);
      DataT cL    = c_light * L;
      DataT cLqp0 = cL * fQp0;

      f[step][0]    = tx;
      F[step][0][2] = 1.;

      f[step][1]    = ty;
      F[step][1][3] = 1.;

      DataT f2tmp = txty * Bx - (DataT(1.) + tx2) * By + ty * Bz;
      f[step][2]  = cLqp0 * f2tmp;

      F[step][2][2] = cLqp0 * (tx * f2tmp * L2i + ty * Bx - DataT(2.) * tx * By);
      F[step][2][3] = cLqp0 * (ty * f2tmp * L2i + tx * Bx + Bz);
      F[step][2][4] = cL * f2tmp;

      DataT f3tmp   = -txty * By - tx * Bz + (DataT(1.) + ty2) * Bx;
      f[step][3]    = cLqp0 * f3tmp;
      F[step][3][2] = cLqp0 * (tx * f3tmp * L2i - ty * By - Bz);
      F[step][3][3] = cLqp0 * (ty * f3tmp * L2i + DataT(2.) * ty * Bx - tx * By);
      F[step][3][4] = cL * f3tmp;

      f[step][4] = 0.;

      if (fDoFitVelocity) {
        DataT vi      = rstep[6];
        f[step][5]    = vi * L;
        F[step][5][2] = vi * tx / L;
        F[step][5][3] = vi * ty / L;
        F[step][5][4] = 0.;
        F[step][5][5] = 0.;
        F[step][5][6] = L;
      }
      else {
        DataT vi      = sqrt(DataT(1.) + fMass2 * fQp0 * fQp0) * kf::defs::SpeedOfLightInv<DataT>;
        f[step][5]    = vi * L;
        F[step][5][2] = vi * tx / L;
        F[step][5][3] = vi * ty / L;
        F[step][5][4] = fMass2 * fQp0 * L / sqrt(DataT(1.) + fMass2 * fQp0 * fQp0) * kf::defs::SpeedOfLightInv<DataT>;
        F[step][5][5] = 0.;
        F[step][5][6] = 0.;
      }

      f[step][6] = 0.;

    }  // end of Runge-Kutta step

    DataT r[7]    = {DataT(0.)};    // extrapolated parameters
    DataT R[7][7] = {{DataT(0.)}};  // Jacobian of the extrapolation

    cnst stepW[5] = {0., h / DataT(6.), h / DataT(3.), h / DataT(3.), h / DataT(6.)};

    DataT k[5][7][7] = {{{DataT(0.)}}};
    for (int step = 1; step <= 4; ++step) {
      for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
          k[step][i][j] = F[step][i][j];
          for (int m = 0; m < 7; m++) {
            k[step][i][j] += stepDz[step] * F[step][i][m] * k[step - 1][m][j];
          }
        }
      }
    }

    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 7; j++) {
        R[i][j] = R0[i][j];
        for (int step = 1; step <= 4; step++) {
          R[i][j] += stepW[step] * k[step][i][j];
        }
      }
    }

    DataT dqp = fTr.Qp() - fQp0;

    for (int i = 0; i < 7; i++) {
      r[i] = r0[i];
      for (int step = 1; step <= 4; step++) {
        r[i] += stepW[step] * f[step][i];
      }
      // take into account linearisation at fQp0
      r[i] += R[i][4] * dqp;
    }

    // update parameters

    fTr.X()    = r[0];
    fTr.Y()    = r[1];
    fTr.Tx()   = r[2];
    fTr.Ty()   = r[3];
    fTr.Qp()   = r[4];
    fTr.Time() = r[5];
    fTr.Vi()   = r[6];

    //fTr.Vi()( fTr.Vi() < DataT(TrackParamV::kClightNsInv) ) = DataT(TrackParamV::kClightNsInv);
    fTr.Z() = zMasked;

    //          covariance matrix transport

    DataT C[7][7];
    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 7; j++) {
        C[i][j] = fTr.C(i, j);
      }
    }

    DataT RC[7][7];
    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 7; j++) {
        RC[i][j] = 0.;
        for (int m = 0; m < 7; m++) {
          RC[i][j] += R[i][m] * C[m][j];
        }
      }
    }
    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 7; j++) {
        DataT Cij = 0.;
        for (int m = 0; m < 7; m++) {
          Cij += RC[i][m] * R[j][m];
        }
        fTr.C(i, j) = Cij;
      }
    }
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::ExtrapolateLine(DataT z_out, const kf::FieldRegion<DataT>& F)
  {
    // extrapolate the track assuming fQp0 == 0
    // TODO: write special simplified procedure
    //
    auto qp0 = fQp0;
    fQp0     = DataT(0.);
    Extrapolate(z_out, F);
    fQp0 = qp0;
  }

  template<typename DataT>
  void TrackKalmanFilter<DataT>::ExtrapolateLineNoField(DataT zOut)
  {
    // extrapolate the track assuming no field

    //   x += dz * tx
    //   y += dz * ty
    //   t += dz * sqrt ( 1 + tx*tx + ty*ty )  * vi

    if (0) {  // debug: full Runge-Kutta extrapolation with zero field values
      kf::FieldRegion<DataT> F;
      ExtrapolateStep(zOut, F);
      return;
    }

    auto& t = fTr;  // use reference to shorten the text

    cnst zMasked = kf::utils::iif(fMask, zOut, t.GetZ());

    cnst dz = (zMasked - t.GetZ());

    DataT tx = t.GetTx();
    DataT ty = t.GetTy();
    DataT vi = t.GetVi();

    DataT L = sqrt(DataT(1.) + tx * tx + ty * ty);

    DataT j52 = dz * tx * vi / L;
    DataT j53 = dz * ty * vi / L;
    DataT j56 = dz * L;

    // transport parameters

    t.X() += tx * dz;
    t.Y() += ty * dz;
    t.Time() += L * vi * dz;
    t.Z() = zMasked;

    // transport covariance matrix

    //     ( 1   0  dz   0   0   0   0 )
    //     ( 0   1   0  dz   0   0   0 )
    //     ( 0   0   1   0   0   0   0 )
    // J = ( 0   0   0   1   0   0   0 )
    //     ( 0   0   0   0   1   0   0 )
    //     ( 0   0 j52 j53   0   1 j56 )
    //     ( 0   0   0   0   0   0   1 )


    // JC = J * C

    DataT jc00 = t.C00() + dz * t.C20();
    //DataT jc01 = t.C01() + dz * t.C21();
    DataT jc02 = t.C02() + dz * t.C22();
    //DataT jc03 = t.C03() + dz * t.C23();
    //DataT jc04 = t.C04() + dz * t.C24();
    //DataT jc05 = t.C05() + dz * t.C25();
    //DataT jc06 = t.C06() + dz * t.C26();


    DataT jc10 = t.C10() + dz * t.C30();
    DataT jc11 = t.C11() + dz * t.C31();
    DataT jc12 = t.C12() + dz * t.C32();
    DataT jc13 = t.C13() + dz * t.C33();
    //DataT jc14 = t.C14() + dz * t.C34();
    //DataT jc15 = t.C15() + dz * t.C35();
    //DataT jc16 = t.C16() + dz * t.C36();

    // jc2? = t.C2?
    // jc3? = t.C3?
    // jc4? = t.C4?

    DataT jc50 = t.C50() + j52 * t.C20() + j53 * t.C30() + j56 * t.C60();
    DataT jc51 = t.C51() + j52 * t.C21() + j53 * t.C31() + j56 * t.C61();
    DataT jc52 = t.C52() + j52 * t.C22() + j53 * t.C32() + j56 * t.C62();
    DataT jc53 = t.C53() + j52 * t.C23() + j53 * t.C33() + j56 * t.C63();
    DataT jc54 = t.C54() + j52 * t.C24() + j53 * t.C34() + j56 * t.C64();
    DataT jc55 = t.C55() + j52 * t.C25() + j53 * t.C35() + j56 * t.C65();
    DataT jc56 = t.C56() + j52 * t.C26() + j53 * t.C36() + j56 * t.C66();

    // jc6? = t.C6?

    // transpose J
    //
    //      (  1   0   0   0   0   0   0 )
    //      (  0   1   0   0   0   0   0 )
    //      ( dz   0   1   0   0 j52   0 )
    // J' = (  0  dz   0   1   0 j53   0 )
    //      (  0   0   0   0   1   0   0 )
    //      (  0   0   0   0   0   1   0 )
    //      (  0   0   0   0   0 j56   1 )


    // C = JC * J'

    t.C00() = jc00 + jc02 * dz;
    t.C10() = jc10 + jc12 * dz;
    t.C20() = t.C20() + t.C22() * dz;
    t.C30() = t.C30() + t.C32() * dz;
    t.C40() = t.C40() + t.C42() * dz;
    t.C50() = jc50 + jc52 * dz;
    t.C60() = t.C60() + t.C62() * dz;

    t.C11() = jc11 + jc13 * dz;
    t.C21() = t.C21() + t.C23() * dz;
    t.C31() = t.C31() + t.C33() * dz;
    t.C41() = t.C41() + t.C43() * dz;
    t.C51() = jc51 + jc53 * dz;
    t.C61() = t.C61() + t.C63() * dz;

    // t.C22 = jc22 == t.C22 -> unchanged
    // t.C32 = jc32 == t.C32 -> unchanged
    // t.C42 = jc42 == t.C42 -> unchanged
    t.C52() = jc52;
    // t.C62 = jc62 == t.C62 -> unchanged

    // t.C33 = jc33 == t.C33 -> unchanged
    // t.C43 = jc43 == t.C43 -> unchanged
    t.C53() = jc53;
    // t.C63 = jc63 == t.C63 -> unchanged

    // t.C44 = jc44 == t.C44 -> unchanged
    t.C54() = jc54;
    // t.C64 = jc64 == t.C64 -> unchanged

    t.C55() = jc55 + jc52 * j52 + jc53 * j53 + jc56 * j56;
    t.C65() = t.C65() + t.C62() * j52 + t.C63() * j53 + t.C66() * j56;

    // t.C66 = jc66 = t.C66 -> unchanged
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::MultipleScattering(DataT radThick, DataT tx, DataT ty, DataT qp)
  {
    // 1974 - Highland (PDG) correction for multiple scattering
    // In the formula there is a replacement:
    // log(X/X0) == log( RadThick * sqrt(1+h) ) -> log(RadThick) + 0.5 * log(1+h);
    // then we use an approximation:
    // log(1+h) -> h - h^2/2 + h^3/3 - h^4/4

    DataT txtx  = tx * tx;
    DataT tyty  = ty * ty;
    DataT txtx1 = DataT(1.) + txtx;
    DataT tyty1 = DataT(1.) + tyty;
    DataT t     = sqrt(txtx1 + tyty);
    // DataT qpt   = qp * t;

    DataT lg = DataT(.0136) * (DataT(1.) + DataT(0.038) * log(radThick * t));
    lg       = kf::utils::iif(lg > DataT(0.), lg, DataT(0.));

    DataT s0 = lg * qp * t;
    DataT a  = (DataT(1.) + fMass2 * qp * qp) * s0 * s0 * t * radThick;

    // Approximate formula

    // DataT h     = txtx + tyty;
    // DataT h2    = h * h;
    // cnst c1 = 0.0136f, c2 = c1 * 0.038f, c3 = c2 * 0.5f, c4 = -c3 / 2.0f, c5 = c3 / 3.0f, c6 = -c3 / 4.0f;
    // DataT s0 = (c1 + c2 * log(radThick) + c3 * h + h2 * (c4 + c5 * h + c6 * h2)) * qp0t;
    // DataT a = ( (kONE+mass2*qp0*qp0t)*radThick*s0*s0 );
    // DataT a = ((t + fMass2 * qp0 * qp0t) * radThick * s0 * s0);

    fTr.C22() += kf::utils::iif(fMask, txtx1 * a, DataT(0.));
    fTr.C32() += kf::utils::iif(fMask, tx * ty * a, DataT(0.));
    fTr.C33() += kf::utils::iif(fMask, tyty1 * a, DataT(0.));
  }

  template<typename DataT>
  void TrackKalmanFilter<DataT>::MultipleScatteringInThickMaterial(DataT radThick, DataT thickness, bool fDownstream)
  {
    cnst kONE = 1.;

    DataT tx    = fTr.Tx();
    DataT ty    = fTr.Ty();
    DataT txtx  = tx * tx;
    DataT tyty  = ty * ty;
    DataT txtx1 = txtx + kONE;
    DataT h     = txtx + tyty;
    DataT t     = sqrt(txtx1 + tyty);
    DataT h2    = h * h;
    DataT qp0t  = fQp0 * t;

    cnst c1(0.0136), c2 = c1 * DataT(0.038), c3 = c2 * DataT(0.5), c4 = -c3 / DataT(2.0), c5 = c3 / DataT(3.0),
                     c6 = -c3 / DataT(4.0);

    DataT s0 = (c1 + c2 * log(radThick) + c3 * h + h2 * (c4 + c5 * h + c6 * h2)) * qp0t;
    //DataT a = ( (ONE+mass2*qp0*qp0t)*radThick*s0*s0 );
    DataT a = ((t + fMass2 * fQp0 * qp0t) * radThick * s0 * s0);

    DataT D   = (fDownstream) ? DataT(1.) : DataT(-1.);
    DataT T23 = (thickness * thickness) / DataT(3.0);
    DataT T2  = thickness / DataT(2.0);

    fTr.C00() += kf::utils::iif(fMask, txtx1 * a * T23, DataT(0.));
    fTr.C10() += kf::utils::iif(fMask, tx * ty * a * T23, DataT(0.));
    fTr.C20() += kf::utils::iif(fMask, txtx1 * a * D * T2, DataT(0.));
    fTr.C30() += kf::utils::iif(fMask, tx * ty * a * D * T2, DataT(0.));

    fTr.C11() += kf::utils::iif(fMask, (kONE + tyty) * a * T23, DataT(0.));
    fTr.C21() += kf::utils::iif(fMask, tx * ty * a * D * T2, DataT(0.));
    fTr.C31() += kf::utils::iif(fMask, (kONE + tyty) * a * D * T2, DataT(0.));

    fTr.C22() += kf::utils::iif(fMask, txtx1 * a, DataT(0.));
    fTr.C32() += kf::utils::iif(fMask, tx * ty * a, DataT(0.));
    fTr.C33() += kf::utils::iif(fMask, (kONE + tyty) * a, DataT(0.));
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::EnergyLossCorrection(DataT radThick, FitDirection direction)
  {
    cnst qp2cut(1. / (10. * 10.));  // 10 GeV cut
    cnst qp02 = kf::utils::max(fQp0 * fQp0, qp2cut);
    cnst p2   = DataT(1.) / qp02;
    cnst E2   = fMass2 + p2;

    cnst bethe = ApproximateBetheBloch(p2 / fMass2);

    DataT tr = sqrt(DataT(1.f) + fTr.Tx() * fTr.Tx() + fTr.Ty() * fTr.Ty());

    DataT dE = bethe * radThick * tr * 2.33f * 9.34961f;

    if (direction == FitDirection::kDownstream) dE = -dE;

    cnst ECorrected  = sqrt(E2) + dE;
    cnst E2Corrected = ECorrected * ECorrected;

    DataT corr   = sqrt(p2 / (E2Corrected - fMass2));
    DataTmask ok = (corr == corr) && fMask;
    corr         = kf::utils::iif(ok, corr, DataT(1.));

    fQp0 *= corr;
    fTr.Qp() *= corr;
    fTr.C40() *= corr;
    fTr.C41() *= corr;
    fTr.C42() *= corr;
    fTr.C43() *= corr;
    fTr.C44() *= corr * corr;
    fTr.C54() *= corr;
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::EnergyLossCorrection(int atomicZ, DataTscal atomicA, DataTscal rho, DataTscal radLen,
                                                      DataT radThick, FitDirection direction)
  {
    cnst qp2cut(1. / (10. * 10.));  // 10 GeV cut
    cnst qp02 = kf::utils::max(fQp0 * fQp0, qp2cut);
    cnst p2   = DataT(1.) / qp02;
    cnst E2   = fMass2 + p2;

    DataT i;
    if (atomicZ < 13) {
      i = (12. * atomicZ + 7.) * 1.e-9;
    }
    else {
      i = (9.76 * atomicZ + 58.8 * std::pow(atomicZ, -0.19)) * 1.e-9;
    }

    cnst bethe = ApproximateBetheBloch(p2 / fMass2, rho, 0.20, 3.00, i, atomicZ / atomicA);

    DataT tr = sqrt(DataT(1.f) + fTr.Tx() * fTr.Tx() + fTr.Ty() * fTr.Ty());

    DataT dE = bethe * radThick * tr * radLen * rho;

    if (direction == FitDirection::kDownstream) dE = -dE;

    cnst ECorrected  = (sqrt(E2) + dE);
    cnst E2Corrected = ECorrected * ECorrected;

    DataT corr   = sqrt(p2 / (E2Corrected - fMass2));
    DataTmask ok = (corr == corr) && fMask;
    corr         = kf::utils::iif(ok, corr, DataT(1.));

    fQp0 *= corr;
    fTr.Qp() *= corr;

    DataT P(sqrt(p2));  // GeV

    DataT Z(atomicZ);
    DataT A(atomicA);
    DataT RHO(rho);

    DataT STEP = radThick * tr * radLen;
    DataT EMASS(0.511 * 1e-3);  // GeV

    DataT BETA  = P / ECorrected;
    DataT GAMMA = ECorrected / fMass;

    // Calculate xi factor (KeV).
    DataT XI = (DataT(153.5) * Z * STEP * RHO) / (A * BETA * BETA);

    // Maximum energy transfer to atomic electron (KeV).
    DataT ETA   = BETA * GAMMA;
    DataT ETASQ = ETA * ETA;
    DataT RATIO = EMASS / fMass;
    DataT F1    = DataT(2.) * EMASS * ETASQ;
    DataT F2    = DataT(1.) + DataT(2.) * RATIO * GAMMA + RATIO * RATIO;
    DataT EMAX  = DataT(1e6) * F1 / F2;

    DataT DEDX2 = XI * EMAX * (DataT(1.) - (BETA * BETA / DataT(2.))) * DataT(1e-12);

    DataT P2    = P * P;
    DataT SDEDX = (E2 * DEDX2) / (P2 * P2 * P2);

    //   T.fTr.C40() *= corr;
    //   T.fTr.C41() *= corr;
    //   T.fTr.C42() *= corr;
    //   T.fTr.C43() *= corr;
    // T.fTr.C44() *= corr*corr;
    fTr.C44() += kf::utils::fabs(SDEDX);
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::GetExtrapolatedXYline(DataT z, const kf::FieldRegion<DataT>& F, DataT& extrX,
                                                       DataT& extrY,
                                                       std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jx,
                                                       std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jy) const
  {
    // extrapolate track assuming it is straight (qp==0)
    // return the extrapolated X, Y and the derivatives of the extrapolated X and Y

    cnst c_light(0.000299792458);

    cnst tx  = fTr.GetTx();
    cnst ty  = fTr.GetTy();
    DataT dz = z - fTr.GetZ();

    DataT xx = tx * tx;
    DataT yy = ty * ty;
    DataT xy = tx * ty;

    DataT Ay = -xx - DataT(1.);
    DataT Bx = yy + DataT(1.);

    DataT ct = c_light * sqrt(DataT(1.) + xx + yy);


    DataT Sx, Sy, Sz;
    std::tie(Sx, Sy, Sz) = F.GetDoubleIntegrals(fTr.GetX(), fTr.GetY(), fTr.GetZ(),  //
                                                fTr.GetX() + dz * tx, fTr.GetY() + dz * ty, z);


    extrX = fTr.GetX() + tx * dz;
    extrY = fTr.GetY() + ty * dz;

    Jx.fill(DataT(0.));  // for a case
    Jy.fill(DataT(0.));

    Jx[0] = DataT(1.);
    Jx[1] = DataT(0.);
    Jx[2] = dz;
    Jx[3] = DataT(0.);
    Jx[4] = ct * (Sx * xy + Sy * Ay + Sz * ty);
    Jx[5] = DataT(0.);
    Jx[6] = DataT(0.);

    Jy[0] = DataT(0.);
    Jy[1] = DataT(1.);
    Jy[2] = DataT(0.);
    Jy[3] = dz;
    Jy[4] = ct * (Sx * Bx - Sy * xy - Sz * tx);
    Jy[5] = DataT(0.);
    Jx[6] = DataT(0.);
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::FilterWithTargetAtLine(DataT targZ, const kf::MeasurementXy<DataT>& targXY,
                                                        const kf::FieldRegion<DataT>& F)
  {
    // Add the target constraint to a straight line track

    DataT eX, eY;
    std::array<DataT, kf::TrackParamV::kNtrackParam> Jx, Jy;
    GetExtrapolatedXYline(targZ, F, eX, eY, Jx, Jy);
    FilterExtrapolatedXY(targXY, eX, eY, Jx, Jy);
  }

  template<typename DataT>
  DataT TrackKalmanFilter<DataT>::ApproximateBetheBloch(DataT bg2)
  {
    //
    // This is the parameterization of the Bethe-Bloch formula inspired by Geant.
    //
    // bg2  - (beta*gamma)^2
    // kp0 - density [g/cm^3]
    // kp1 - density effect first junction point
    // kp2 - density effect second junction point
    // kp3 - mean excitation energy [GeV]
    // kp4 - mean Z/A
    //
    // The default values for the kp* parameters are for silicon.
    // The returned value is in [GeV/(g/cm^2)].
    //

    cnst kp0 = 2.33f;
    cnst kp1 = 0.20f;
    cnst kp2 = 3.00f;
    cnst kp3 = 173e-9f;
    cnst kp4 = 0.49848f;

    constexpr DataTscal mK   = 0.307075e-3f;  // [GeV*cm^2/g]
    constexpr DataTscal _2me = 1.022e-3f;     // [GeV/c^2]
    cnst rho                 = kp0;
    cnst x0                  = kp1 * 2.303f;
    cnst x1                  = kp2 * 2.303f;
    cnst mI                  = kp3;
    cnst mZA                 = kp4;
    cnst maxT                = _2me * bg2;  // neglecting the electron mass

    //*** Density effect
    DataT d2(0.f);
    cnst x    = 0.5f * log(bg2);
    cnst lhwI = log(28.816f * 1e-9f * sqrt(rho * mZA) / mI);

    DataTmask init = x > x1;
    d2             = kf::utils::iif(init, lhwI + x - 0.5f, DataT(0.));
    cnst r         = (x1 - x) / (x1 - x0);
    init           = (x > x0) & (x1 > x);
    d2             = kf::utils::iif(init, lhwI + x - 0.5f + (0.5f - lhwI - x0) * r * r * r, d2);

    return mK * mZA * (DataT(1.f) + bg2) / bg2
           * (0.5f * log(_2me * bg2 * maxT / (mI * mI)) - bg2 / (DataT(1.f) + bg2) - d2);
  }

  template<typename DataT>
  DataT TrackKalmanFilter<DataT>::ApproximateBetheBloch(DataT bg2, DataT kp0, DataT kp1, DataT kp2, DataT kp3,
                                                        DataT kp4)
  {
    //
    // This is the parameterization of the Bethe-Bloch formula inspired by Geant.
    //
    // bg2  - (beta*gamma)^2
    // kp0 - density [g/cm^3]
    // kp1 - density effect first junction point
    // kp2 - density effect second junction point
    // kp3 - mean excitation energy [GeV]
    // kp4 - mean Z/A
    //
    // The default values for the kp* parameters are for silicon.
    // The returned value is in [GeV/(g/cm^2)].
    //

    //   cnst &kp0 = 2.33f;
    //   cnst &kp1 = 0.20f;
    //   cnst &kp2 = 3.00f;
    //   cnst &kp3 = 173e-9f;
    //   cnst &kp4 = 0.49848f;

    constexpr DataTscal mK   = 0.307075e-3f;  // [GeV*cm^2/g]
    constexpr DataTscal _2me = 1.022e-3f;     // [GeV/c^2]
    DataT rho                = kp0;
    cnst x0                  = kp1 * 2.303f;
    cnst x1                  = kp2 * 2.303f;
    DataT mI                 = kp3;
    DataT mZA                = kp4;
    cnst maxT                = _2me * bg2;  // neglecting the electron mass

    //*** Density effect
    DataT d2(0.f);
    cnst x    = 0.5f * log(bg2);
    cnst lhwI = log(28.816f * 1e-9f * sqrt(rho * mZA) / mI);

    DataTmask init = x > x1;
    d2             = kf::utils::iif(init, lhwI + x - 0.5f, DataT(0.));
    cnst r         = (x1 - x) / (x1 - x0);
    init           = (x > x0) & (x1 > x);
    d2             = kf::utils::iif(init, lhwI + x - 0.5f + (0.5f - lhwI - x0) * r * r * r, d2);

    return mK * mZA * (DataT(1.f) + bg2) / bg2
           * (0.5f * log(_2me * bg2 * maxT / (mI * mI)) - bg2 / (DataT(1.f) + bg2) - d2);
  }


  template<typename DataT>
  std::tuple<DataT, DataT> TrackKalmanFilter<DataT>::GetChi2XChi2U(kf::MeasurementXy<DataT> m, DataT x, DataT y,
                                                                   DataT C00, DataT C10, DataT C11)
  {

    DataT chi2x{0.};

    {  // filter X measurement
      DataT zeta = x - m.X();

      // F = CH'
      DataT F0 = C00;
      DataT F1 = C10;

      DataT HCH = F0;

      DataT wi     = DataT(1.) / (m.Dx2() + HCH);
      DataT zetawi = zeta * wi;
      chi2x        = m.NdfX() * zeta * zetawi;

      DataT K1 = F1 * wi;

      x -= F0 * zetawi;
      y -= F1 * zetawi;

      C00 -= F0 * F0 * wi;
      C10 -= K1 * F0;
      C11 -= K1 * F1;
    }

    DataT chi2u{0.};

    {  // filter U measurement, we need only chi2 here
      DataT cosPhi = -m.Dxy() / m.Dx2();
      DataT u      = cosPhi * m.X() + m.Y();
      DataT du2    = m.Dy2() + cosPhi * m.Dxy();

      DataT zeta = cosPhi * x + y - u;

      // F = CH'
      DataT F0 = cosPhi * C00 + C10;
      DataT F1 = cosPhi * C10 + C11;

      DataT HCH = (F0 * cosPhi + F1);

      chi2u += m.NdfY() * zeta * zeta / (du2 + HCH);
    }

    return std::tuple<DataT, DataT>(chi2x, chi2u);
  }


  template<typename DataT>
  void TrackKalmanFilter<DataT>::GuessTrack(const DataT& trackZ, const DataT hitX[], const DataT hitY[],
                                            const DataT hitZ[], const DataT hitT[], const DataT By[],
                                            const DataTmask hitW[], const DataTmask hitWtime[], int NHits)
  {
    // gives nice initial approximation for x,y,tx,ty - almost same as KF fit. qp - is shifted by 4%, resid_ual - ~3.5% (KF fit resid_ual - 1%).

    const DataT c_light(0.000299792458), c_light_i(DataT(1.) / c_light);

    DataT A0 = 0., A1 = 0., A2 = 0., A3 = 0., A4 = 0., A5 = 0.;
    DataT a0 = 0., a1 = 0., a2 = 0.;
    DataT b0 = 0., b1 = 0., b2 = 0.;

    DataT time          = 0.;
    DataTmask isTimeSet = DataTmask(false);

    DataT prevZ = 0.;
    DataT sy = 0., Sy = 0.;  // field integrals

    for (int i = 0; i < NHits; i++) {

      DataT w = kf::utils::iif(hitW[i], DataT(1.), DataT(0.));

      DataTmask setTime = (!isTimeSet) && hitWtime[i] && hitW[i];
      time              = kf::utils::iif(setTime, hitT[i], time);
      isTimeSet         = isTimeSet || setTime;

      DataT x = hitX[i];
      DataT y = hitY[i];
      DataT z = hitZ[i] - trackZ;

      {
        DataT dZ = z - prevZ;
        Sy += w * dZ * sy + DataT(0.5) * dZ * dZ * By[i];
        sy += w * dZ * By[i];
        prevZ = kf::utils::iif(hitW[i], z, prevZ);
      }

      DataT S = Sy;

      DataT wz = w * z;
      DataT wS = w * S;

      A0 += w;
      A1 += wz;
      A2 += wz * z;
      A3 += wS;
      A4 += wS * z;
      A5 += wS * S;

      a0 += w * x;
      a1 += wz * x;
      a2 += wS * x;

      b0 += w * y;
      b1 += wz * y;
      b2 += wS * y;
    }

    DataT m00 = A0;
    DataT m01 = A1;
    DataT m02 = A3;

    DataT m11 = A2;
    DataT m12 = A4;

    DataT m22 = A5;

    DataT m21 = m12;

    // { m00 m01 m02 }       ( a0 )
    // { m01 m11 m12 } = x * ( a1 )
    // { m02 m21 m22 }       ( a2 )

    {  // triangulation row 0
      m11 = m00 * m11 - m01 * m01;
      m12 = m00 * m12 - m01 * m02;
      a1  = m00 * a1 - m01 * a0;

      m21 = m00 * m21 - m02 * m01;
      m22 = m00 * m22 - m02 * m02;
      a2  = m00 * a2 - m02 * a0;
    }

    {  // triangulation step row 1
      m22 = m11 * m22 - m21 * m12;
      a2  = m11 * a2 - m21 * a1;
    }

    DataT L = 0.;
    {  // diagonalization row 2
      L = kf::utils::iif((kf::utils::fabs(m22) > DataT(1.e-4)), a2 / m22, DataT(0.));
      a1 -= L * m12;
      a0 -= L * m02;
    }

    {  // diagonalization row 1
      fTr.Tx() = a1 / m11;
      a0 -= fTr.Tx() * m01;
    }

    {  // diagonalization row 0
      fTr.X() = a0 / m00;
    }

    DataT txtx1 = DataT(1.) + fTr.Tx() * fTr.Tx();
    L           = L / txtx1;
    DataT L1    = L * fTr.Tx();

    A1 = A1 + A3 * L1;
    A2 = A2 + (A4 + A4 + A5 * L1) * L1;
    b1 += b2 * L1;

    // { A0 A1 } = x * ( b0 )
    // { A1 A2 }       ( b1 )

    A2 = A0 * A2 - A1 * A1;
    b1 = A0 * b1 - A1 * b0;

    fTr.Ty() = b1 / A2;
    fTr.Y()  = (b0 - A1 * fTr.Ty()) / A0;

    fTr.Qp()   = -L * c_light_i / sqrt(txtx1 + fTr.Ty() * fTr.Ty());
    fTr.Time() = time;
    fTr.Z()    = trackZ;
    fTr.Vi()   = kf::defs::SpeedOfLightInv<DataT>;
    fQp0       = fTr.Qp();
  }

  template class TrackKalmanFilter<kf::fvec>;
  template class TrackKalmanFilter<float>;
  template class TrackKalmanFilter<double>;

}  // namespace cbm::algo::kf
