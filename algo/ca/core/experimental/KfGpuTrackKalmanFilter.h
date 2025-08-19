/* Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Maksym Zyzak */


/// \file KfGpuTrackKalmanFilter.h
/// \brief Track fit utilities for the CA tracking based on the Kalman filter
/// \since 10.02.2023
/// \author S.Gorbunov

#pragma once  // include this header only once per compilation unit

//#include "KfFieldRegion.h"
#include "CaGpuField.h"
#include "KfMeasurementTime.h"
#include "KfMeasurementU.h"
//#include "KfMeasurementXy.h"
#include "CaMeasurementXy.h"
//#include "KfSimd.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"
#include "KfUtils.h"

#include <type_traits>

#include <xpu/device.h>

namespace cbm::algo::kf
{
  class Hit;

  //  enum class FitDirection
  //  {
  //    kUpstream,
  //    kDownstream
  //  };

  //  inline FitDirection operator!(FitDirection d)
  //  {
  //    return d == FitDirection::kUpstream ? FitDirection::kDownstream : FitDirection::kUpstream;
  //  }

  /// Track fit utilities for the CA tracking based on the Kalman Filter
  ///
  template<typename DataT>
  class GpuTrackKalmanFilter {

   public:
    using DataTscal = kf::utils::scaltype<DataT>;
    using DataTmask = kf::utils::masktype<DataT>;

    XPU_D GpuTrackKalmanFilter()
      : fMask(true)
      , fTr()
      , fQp0(0.)
      , fMass(0.10565800)
      , fMass2(fMass * fMass)
      , fMaxExtraplationStep(50.)
      , fDoFitVelocity(false)
    {
    }

    GpuTrackKalmanFilter(const kf::TrackParam<DataT>& t) { SetTrack(t); }

    GpuTrackKalmanFilter(const DataTmask& m, bool fitV) : fMask(m), fDoFitVelocity(fitV) {}

    template<typename T>
    GpuTrackKalmanFilter(const kf::TrackParam<T>& t)
    {
      SetTrack(t);
    }

    XPU_D void SetMask(const DataTmask& m) { fMask = m; }

    template<typename T>
    XPU_D void SetTrack(const kf::TrackParam<T>& t)
    {
      fTr.SetOneGpu(t);
      fQp0 = fTr.GetQp();
    }

    XPU_D void SetQp0(DataT qp0) { fQp0 = qp0; }

    XPU_D kf::TrackParam<DataT>& Tr() { return fTr; }

    XPU_D DataT& Qp0() { return fQp0; }

    XPU_D void SetDoFitVelocity(bool v) { fDoFitVelocity = v; }

    void SetOneEntry(const int i0, const GpuTrackKalmanFilter& T1, const int i1);

    std::string ToString(int i = -1);

    ///--------------------------
    /// Fit utilities

    /// set particle mass for the fit
    XPU_D void SetParticleMass(DataT mass)
    {
      fMass  = mass;
      fMass2 = mass * mass;
    }

    /// get the particle mass
    DataT GetParticleMass() const { return fMass; }

    /// get the particle mass squared
    DataT GetParticleMass2() const { return fMass2; }

    /// set max extrapolation step [cm]
    void SetMaxExtrapolationStep(double step) { fMaxExtraplationStep = DataT(step); }

    /// get the particle mass
    DataT GetMaxExtrapolationStep() const { return fMaxExtraplationStep; }

    XPU_D inline float iif_mask(bool cond, float val_true, float val_false)
    {
      //      if constexpr (std::is_same_v<DataT, float>) {
      float mask = cond ? 1.0f : 0.0f;
      return mask * val_true + (1.0f - mask) * val_false;
      //      } else {
      //	return kf::utils::iif(cond, val_true, val_false);
      //      }
    }

    /// filter the track with the 1d measurement
    XPU_D void Filter1d(const kf::MeasurementU<DataT>& m)
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
      DataT w = m.Du2() + (doProtect ? (DataT(1.0000001) * HCH) : HCH);
      //      DataT wi = kf::utils::iif(fMask, DataT(1.) / w, DataT(0.));
      DataT wi = iif_mask(fMask, DataT(1.) / w, DataT(0.));

      //      DataT zetawi = zeta / (kf::utils::iif(maskDoFilter, m.Du2(), DataT(0.)) + HCH);
      DataT zetawi = zeta / (iif_mask(maskDoFilter, m.Du2(), DataT(0.)) + HCH);
      //      zetawi       = kf::utils::iif(fMask, zetawi, DataT(0.));
      zetawi = iif_mask(fMask, zetawi, DataT(0.));

      //      wi = kf::utils::iif(m.Du2() > DataT(0.), wi, DataT(0.));
      wi = iif_mask(m.Du2() > DataT(0.), wi, DataT(0.));

      fTr.ChiSq() += zeta * zeta * wi;
      //      fTr.Ndf() += kf::utils::iif(fMask, m.Ndf(), DataT(0.));
      fTr.Ndf() += iif_mask(fMask, m.Ndf(), DataT(0.));

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

    /// filter the track with the XY measurement
    //    XPU_D void FilterXY(const kf::MeasurementXy<DataT>& mxy, bool skipUnmeasuredCoordinates = false)ca::MeasurementXy<float>
    XPU_D void FilterXY(const ca::MeasurementXy<float>& mxy, bool skipUnmeasuredCoordinates = false)  //TODO: GPU only
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

    /// filter the track with the time measurement
    XPU_D void FilterTime(DataT t, DataT dt2, const DataTmask& timeInfo)
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

      //      DataT wi     = kf::utils::iif(mask, DataT(1.) / (dt2 + DataT(1.0000001) * HCH), DataT(0.));
      //      DataT zeta   = kf::utils::iif(mask, fTr.Time() - t, DataT(0.));
      //      DataT zetawi = kf::utils::iif(mask, zeta / (kf::utils::iif(maskDoFilter, dt2, DataT(0.)) + HCH), DataT(0.));
      DataT wi     = iif_mask(mask, DataT(1.) / (dt2 + DataT(1.0000001) * HCH), DataT(0.));
      DataT zeta   = iif_mask(mask, fTr.Time() - t, DataT(0.));
      DataT zetawi = iif_mask(mask, zeta / (iif_mask(maskDoFilter, dt2, DataT(0.)) + HCH), DataT(0.));

      //      fTr.ChiSqTime() += kf::utils::iif(maskDoFilter, zeta * zeta * wi, DataT(0.));
      //      fTr.NdfTime() += kf::utils::iif(mask, DataT(1.), DataT(0.));
      fTr.ChiSqTime() += iif_mask(maskDoFilter, zeta * zeta * wi, DataT(0.));
      fTr.NdfTime() += iif_mask(mask, DataT(1.), DataT(0.));

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

    /// filter the track with the time measurement
    //    void FilterTime(kf::MeasurementTime<DataT> mt) { FilterTime(mt.T(), mt.Dt2(), DataTmask(mt.NdfT() > DataT(0.))); }

    /// filter the inverse speed
    //    void FilterVi(DataT vi);

    /// measure the track velocity with the track Qp and the mass
    //    void MeasureVelocityWithQp();

    /// extrapolate the track to the given Z using the field F
    /// it can do several extrapolation steps if the Z is far away
    /// \param z_out - Z coordinate to extrapolate to
    /// \param F - field region
    XPU_D void Extrapolate(DataT z_out, const ca::GpuFieldRegion& F)
    {
      // use Q/p linearisation at fQp0
      //	if (F.GetFieldType() == kf::EFieldType::Null) {
      //	  ExtrapolateLineNoField(z_out);
      //	}
      //	else {
      DataT sgn = iif_mask(fTr.GetZ() < z_out, DataT(1.), DataT(-1.));
      while (xpu::abs(z_out - fTr.GetZ()) > 1.e-6f) {  //TODO: only for XPU
        //	        !kf::utils::isFull(kf::utils::iif(fMask, kf::utils::fabs(z_out - fTr.GetZ()), DataT(0.)) <= DataT(1.e-6))) {
        DataT zNew = fTr.GetZ() + sgn * fMaxExtraplationStep;  // max. 50 cm step
        zNew       = iif_mask(sgn * (z_out - zNew) <= DataT(0.), z_out, zNew);
        ExtrapolateStep(zNew, F);
      }
      //	}
    }

    /// extrapolate the track to the given Z using the field F
    /// it does extrapolation in one step
    XPU_D void ExtrapolateStep(DataT zOut, const ca::GpuFieldRegion& Field)
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

      //      cnst zMasked = kf::utils::iif(fMask, zOut, fTr.GetZ());
      cnst zMasked = iif_mask(fMask, zOut, fTr.GetZ());

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

        //        kf::FieldValue B = Field.Get(rstep[0], rstep[1], z);
        ca::GpuFieldValue B = Field.Get(rstep[0], rstep[1], z);  //TODO: GPU only
        DataT Bx            = B.x;
        DataT By            = B.y;
        DataT Bz            = B.z;
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

    /// extrapolate the track to the given Z using linearization at the straight line
    //    void ExtrapolateLine(DataT z_out, const kf::FieldRegion<DataT>& F);

    /// extrapolate the track to the given Z assuming no magnetic field
    //    void ExtrapolateLineNoField(DataT z_out);

    /// apply energy loss correction to the track
    /// \param radThick - radiation length of the material
    /// \param direction - direction of the track
    XPU_D void EnergyLossCorrection(DataT radThick, FitDirection direction)
    {
      cnst qp2cut(1. / (10. * 10.));  // 10 GeV cut
      cnst qp02 = xpu::max(fQp0 * fQp0, qp2cut);
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
      //      corr         = kf::utils::iif(ok, corr, DataT(1.));
      corr = iif_mask(ok, corr, DataT(1.));

      fQp0 *= corr;
      fTr.Qp() *= corr;
      //      fTr.C40() *= corr;
      //      fTr.C41() *= corr;
      //      fTr.C42() *= corr;
      //      fTr.C43() *= corr;
      //      fTr.C44() *= corr * corr;
      //      fTr.C54() *= corr;
      fTr.C(4, 0) *= corr;
      fTr.C(4, 1) *= corr;
      fTr.C(4, 2) *= corr;
      fTr.C(4, 3) *= corr;
      fTr.C(4, 4) *= corr * corr;
      fTr.C(5, 4) *= corr;
    }

    /// apply energy loss correction to the track
    /// more accurate formula using material atomic numbers
    /// \param atomicZ - atomic number of the material
    /// \param atomicA - atomic mass of the material
    /// \param rho - density of the material
    /// \param radLen - radiation length of the material
    /// \param radThick - radiation length of the material
    /// \param direction - direction of the track
    //    void EnergyLossCorrection(int atomicZ, DataTscal atomicA, DataTscal rho, DataTscal radLen, DataT radThick,
    //                              FitDirection direction);


    /// apply multiple scattering correction to the track with the given Qp0
    XPU_D void MultipleScattering(DataT radThick, DataT tx, DataT ty, DataT qp)
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
      DataT t     = xpu::sqrt(txtx1 + tyty);
      // DataT qpt   = qp * t;

      DataT lg = DataT(.0136) * (DataT(1.) + DataT(0.038) * log(radThick * t));
      //      lg       = kf::utils::iif(lg > DataT(0.), lg, DataT(0.));
      lg = iif_mask(lg > DataT(0.), lg, DataT(0.));

      DataT s0 = lg * qp * t;
      DataT a  = (DataT(1.) + fMass2 * qp * qp) * s0 * s0 * t * radThick;

      // Approximate formula

      // DataT h     = txtx + tyty;
      // DataT h2    = h * h;
      // cnst c1 = 0.0136f, c2 = c1 * 0.038f, c3 = c2 * 0.5f, c4 = -c3 / 2.0f, c5 = c3 / 3.0f, c6 = -c3 / 4.0f;
      // DataT s0 = (c1 + c2 * log(radThick) + c3 * h + h2 * (c4 + c5 * h + c6 * h2)) * qp0t;
      // DataT a = ( (kONE+mass2*qp0*qp0t)*radThick*s0*s0 );
      // DataT a = ((t + fMass2 * qp0 * qp0t) * radThick * s0 * s0);

      //      fTr.C22() += kf::utils::iif(fMask, txtx1 * a, DataT(0.));
      //      fTr.C32() += kf::utils::iif(fMask, tx * ty * a, DataT(0.));
      //      fTr.C33() += kf::utils::iif(fMask, tyty1 * a, DataT(0.));
      fTr.C(2, 2) += iif_mask(fMask, txtx1 * a, DataT(0.));
      fTr.C(3, 2) += iif_mask(fMask, tx * ty * a, DataT(0.));
      fTr.C(3, 3) += iif_mask(fMask, tyty1 * a, DataT(0.));
    }

    /// apply multiple scattering correction to the track
    XPU_D void MultipleScattering(DataT radThick) { MultipleScattering(radThick, fTr.GetTx(), fTr.GetTy(), fQp0); }

    /// apply multiple scattering correction in thick material to the track
    //    void MultipleScatteringInThickMaterial(DataT radThick, DataT thickness, bool fDownstream);

    ///------------------------------------------------------------------
    /// special utilities needed by the combinatorial track finder

    /// extrapolate track as a line, return the extrapolated X, Y and the Jacobians
    //    void GetExtrapolatedXYline(DataT z, const kf::FieldRegion<DataT>& F, DataT& extrX, DataT& extrY,
    //                               std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jx,
    //                               std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jy) const;

    /// filter the track with the XY measurement placed at different Z
    /// \param m - measurement
    /// \param extrX - extrapolated X of the track
    /// \param extrY - extrapolated Y of the track
    /// \param Jx - Jacobian of the extrapolated X
    /// \param Jy - Jacobian of the extrapolated Y
    //    void FilterExtrapolatedXY(const kf::MeasurementXy<DataT>& m, DataT extrX, DataT extrY,
    //                              const std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jx,
    //                              const std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jy);

    /// extrapolate the track to the given Z using linearization at the straight line,
    /// \param z_out - Z coordinate to extrapolate to
    /// \return pair of the extrapolated X, and dX2 - the rms^2 of the extrapolated x
    //    std::pair<DataT, DataT> ExtrapolateLineXdX2(DataT z_out) const;

    /// extrapolate the track to the given Z using linearization at the straight line,
    /// \param z_out - Z coordinate to extrapolate to
    /// \return pair of the extrapolated Y, and dY2 - the rms^2 of the extrapolated y
    //    std::pair<DataT, DataT> ExtrapolateLineYdY2(DataT z_out) const;

    /// extrapolate the track to the given Z using linearization at the straight line,
    /// \param z_out - Z coordinate to extrapolate to
    /// \return extrapolated correlation cov<x,y>
    //    DataT ExtrapolateLineDxy(DataT z_out) const;

    /// add target measuremet to the track using linearisation at a straight line
    //    void FilterWithTargetAtLine(DataT targZ, const kf::MeasurementXy<DataT>& targXYInfo,
    //                                const kf::FieldRegion<DataT>& F);

    /// \brief Approximate mean energy loss with Bethe-Bloch formula
    /// \param bg2 (beta*gamma)^2
    /// \return mean energy loss
    XPU_D DataT ApproximateBetheBloch(DataT bg2)
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
      //      d2             = kf::utils::iif(init, lhwI + x - 0.5f, DataT(0.));
      d2     = iif_mask(init, lhwI + x - 0.5f, DataT(0.));
      cnst r = (x1 - x) / (x1 - x0);
      init   = (x > x0) & (x1 > x);
      //      d2             = kf::utils::iif(init, lhwI + x - 0.5f + (0.5f - lhwI - x0) * r * r * r, d2);
      d2 = iif_mask(init, lhwI + x - 0.5f + (0.5f - lhwI - x0) * r * r * r, d2);

      return mK * mZA * (DataT(1.f) + bg2) / bg2
             * (0.5f * log(_2me * bg2 * maxT / (mI * mI)) - bg2 / (DataT(1.f) + bg2) - d2);
    }

    /// \brief Approximate mean energy loss with Bethe-Bloch formula
    /// \param bg2 (beta*gamma)^2
    /// \param kp0 density [g/cm^3]
    /// \param kp1 density effect first junction point
    /// \param kp2 density effect second junction point
    /// \param kp3 mean excitation energy [GeV]
    /// \param kp4 mean Z/A
    /// \return mean energy loss
    //    static DataT ApproximateBetheBloch(DataT bg2, DataT kp0, DataT kp1, DataT kp2, DataT kp3, DataT kp4);

    /// \brief git two chi^2 components of the track fit to measurement
    /// \param m - measurement
    /// \param x - track X
    /// \param y - track Y
    /// \param C00 - track covariance C00
    /// \param C10 - track covariance C10
    /// \param C11 - track covariance C11
    /// \return pair of (chi^2_x, chi^2_u) components of the chi^2.
    ///         chi^2_u is calculated after track is fit to the X measurement
    //    static std::tuple<DataT, DataT> GetChi2XChi2U(kf::MeasurementXy<DataT> m, DataT x, DataT y, DataT C00, DataT C10,
    //                                                  DataT C11);

    /// \brief fast guess of track parameterts based on its hits
    /// \param trackZ - Z coordinate of the track
    /// \param hitX - X coordinate of the hits
    /// \param hitY - Y coordinate of the hits
    /// \param hitZ - Z coordinate of the hits
    /// \param hitT - Time coordinate of the hits
    /// \param By - y component of the magnetic field
    /// \param hitW - hit weight
    /// \param hitWtime - hit weight for the time measurement
    /// \param NHits - number of hits
    XPU_D void GuessTrack(const DataT& trackZ, const DataT hitX[], const DataT hitY[], const DataT hitZ[],
                          const DataT hitT[], const DataT By[], const DataTmask hitW[], const DataTmask hitWtime[],
                          int NHits)
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

        DataT w = iif_mask(hitW[i], DataT(1.), DataT(0.));

        DataTmask setTime = (!isTimeSet) && hitWtime[i] && hitW[i];
        time              = iif_mask(setTime, hitT[i], time);
        isTimeSet         = isTimeSet || setTime;

        DataT x = hitX[i];
        DataT y = hitY[i];
        DataT z = hitZ[i] - trackZ;

        {
          DataT dZ = z - prevZ;
          Sy += w * dZ * sy + DataT(0.5) * dZ * dZ * By[i];
          sy += w * dZ * By[i];
          prevZ = iif_mask(hitW[i], z, prevZ);
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
        L = iif_mask((xpu::abs(m22) > DataT(1.e-4)), a2 / m22, DataT(0.));
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

   private:
    typedef const DataT cnst;

    ///--------------------------
    /// Data members

    DataTmask fMask;  ///< mask of active elements in simd vectors

    kf::TrackParam<DataT> fTr;  ///< track parameters
    DataT fQp0;

    DataT fMass;   ///< particle mass (muon mass by default)
    DataT fMass2;  ///< mass squared

    DataT fMaxExtraplationStep;  ///< max extrapolation step [cm]

    bool fDoFitVelocity;  // should the track velocity be fitted as an independent parameter
  };

  // =============================================================================================

  //  template<typename DataT>
  //  inline std::string GpuTrackKalmanFilter<DataT>::ToString(int i)
  //  {
  //    return fTr.ToString(i);
  //  }
  //
  //
  //  template<typename DataT>
  //  inline void GpuTrackKalmanFilter<DataT>::SetOneEntry(const int i0, const GpuTrackKalmanFilter& T1, const int i1)
  //  {
  //    fTr.SetOneEntry(i0, T1.fTr, i1);
  //    kf::utils::VecCopy<DataT, DataT, false, false>::CopyEntries(fQp0, i0, T1.fQp0, i1);
  //  }
  //
  //  template<typename DataT>
  //  inline std::pair<DataT, DataT> GpuTrackKalmanFilter<DataT>::ExtrapolateLineXdX2(DataT z_out) const
  //  {
  //    DataT dz = (z_out - fTr.GetZ());
  //    return std::pair(fTr.GetX() + fTr.GetTx() * dz, fTr.C00() + dz * (2 * fTr.C20() + dz * fTr.C22()));
  //  }
  //
  //  template<typename DataT>
  //  inline std::pair<DataT, DataT> GpuTrackKalmanFilter<DataT>::ExtrapolateLineYdY2(DataT z_out) const
  //  {
  //    DataT dz = (z_out - fTr.GetZ());
  //    return std::pair(fTr.GetY() + fTr.GetTy() * dz, fTr.C11() + dz * (DataT(2.) * fTr.C31() + dz * fTr.C33()));
  //  }
  //
  //  template<typename DataT>
  //  inline DataT GpuTrackKalmanFilter<DataT>::ExtrapolateLineDxy(DataT z_out) const
  //  {
  //    DataT dz = (z_out - fTr.GetZ());
  //    return fTr.C10() + dz * (fTr.C21() + fTr.C30() + dz * fTr.C32());
  //  }
  template class GpuTrackKalmanFilter<float>;

}  // namespace cbm::algo::kf
