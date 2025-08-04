/* Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Maksym Zyzak */


/// \file KfTrackKalmanFilter.h
/// \brief Track fit utilities for the CA tracking based on the Kalman filter
/// \since 10.02.2023
/// \author S.Gorbunov

#pragma once  // include this header only once per compilation unit

#include "KfFieldRegion.h"
#include "KfMeasurementTime.h"
#include "KfMeasurementU.h"
#include "KfMeasurementXy.h"
#include "KfSimd.h"
#include "KfTrackParam.h"
#include "KfUtils.h"

#include <type_traits>

namespace cbm::algo::kf
{
  class Hit;

  enum class FitDirection
  {
    kUpstream,
    kDownstream
  };

  inline FitDirection operator!(FitDirection d)
  {
    return d == FitDirection::kUpstream ? FitDirection::kDownstream : FitDirection::kUpstream;
  }

  /// Track fit utilities for the CA tracking based on the Kalman Filter
  ///
  template<typename DataT>
  class TrackKalmanFilter {

   public:
    using DataTscal = kf::utils::scaltype<DataT>;
    using DataTmask = kf::utils::masktype<DataT>;

    TrackKalmanFilter() = default;

    TrackKalmanFilter(const kf::TrackParam<DataT>& t) { SetTrack(t); }

    TrackKalmanFilter(const DataTmask& m, bool fitV) : fMask(m), fDoFitVelocity(fitV) {}

    template<typename T>
    TrackKalmanFilter(const kf::TrackParam<T>& t)
    {
      SetTrack(t);
    }

    void SetMask(const DataTmask& m) { fMask = m; }

    template<typename T>
    void SetTrack(const kf::TrackParam<T>& t)
    {
      fTr.Set(t);
      fQp0 = fTr.GetQp();
    }

    void SetQp0(DataT qp0) { fQp0 = qp0; }

    kf::TrackParam<DataT>& Tr() { return fTr; }

    DataT& Qp0() { return fQp0; }

    void SetDoFitVelocity(bool v) { fDoFitVelocity = v; }

    void SetOneEntry(const int i0, const TrackKalmanFilter& T1, const int i1);

    std::string ToString(int i = -1);

    ///--------------------------
    /// Fit utilities

    /// set particle mass for the fit
    void SetParticleMass(DataT mass)
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

    /// filter the track with the 1d measurement
    void Filter1d(const kf::MeasurementU<DataT>& m);

    /// filter the track with the XY measurement
    void FilterXY(const kf::MeasurementXy<DataT>& m, bool skipUnmeasuredCoordinates = false);

    /// filter the track with the time measurement
    void FilterTime(DataT t, DataT dt2, const DataTmask& m);

    /// filter the track with the time measurement
    void FilterTime(kf::MeasurementTime<DataT> mt) { FilterTime(mt.T(), mt.Dt2(), DataTmask(mt.NdfT() > DataT(0.))); }

    /// filter the inverse speed
    void FilterVi(DataT vi);

    /// measure the track velocity with the track Qp and the mass
    void MeasureVelocityWithQp();

    /// extrapolate the track to the given Z using the field F
    /// it can do several extrapolation steps if the Z is far away
    /// \param z - Z coordinate to extrapolate to
    /// \param F - field region
    void Extrapolate(DataT z, const kf::FieldRegion<DataT>& F);

    /// extrapolate the track to the given Z using the field F
    /// it does extrapolation in one step
    void ExtrapolateStep(DataT z_out, const kf::FieldRegion<DataT>& F);

    /// extrapolate the track to the given Z using linearization at the straight line
    void ExtrapolateLine(DataT z_out, const kf::FieldRegion<DataT>& F);

    /// extrapolate the track to the given Z assuming no magnetic field
    void ExtrapolateLineNoField(DataT z_out);

    /// apply energy loss correction to the track
    /// \param radThick - radiation length of the material
    /// \param direction - direction of the track
    void EnergyLossCorrection(DataT radThick, FitDirection direction);

    /// apply energy loss correction to the track
    /// more accurate formula using material atomic numbers
    /// \param atomicZ - atomic number of the material
    /// \param atomicA - atomic mass of the material
    /// \param rho - density of the material
    /// \param radLen - radiation length of the material
    /// \param radThick - radiation length of the material
    /// \param direction - direction of the track
    void EnergyLossCorrection(int atomicZ, DataTscal atomicA, DataTscal rho, DataTscal radLen, DataT radThick,
                              FitDirection direction);


    /// apply multiple scattering correction to the track with the given Qp0
    void MultipleScattering(DataT radThick, DataT tx0, DataT ty0, DataT qp0);

    /// apply multiple scattering correction to the track
    void MultipleScattering(DataT radThick) { MultipleScattering(radThick, fTr.GetTx(), fTr.GetTy(), fQp0); }

    /// apply multiple scattering correction in thick material to the track
    void MultipleScatteringInThickMaterial(DataT radThick, DataT thickness, bool fDownstream);

    ///------------------------------------------------------------------
    /// special utilities needed by the combinatorial track finder

    /// extrapolate track as a line, return the extrapolated X, Y and the Jacobians
    void GetExtrapolatedXYline(DataT z, const kf::FieldRegion<DataT>& F, DataT& extrX, DataT& extrY,
                               std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jx,
                               std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jy) const;

    /// filter the track with the XY measurement placed at different Z
    /// \param m - measurement
    /// \param extrX - extrapolated X of the track
    /// \param extrY - extrapolated Y of the track
    /// \param Jx - Jacobian of the extrapolated X
    /// \param Jy - Jacobian of the extrapolated Y
    void FilterExtrapolatedXY(const kf::MeasurementXy<DataT>& m, DataT extrX, DataT extrY,
                              const std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jx,
                              const std::array<DataT, kf::TrackParam<DataT>::kNtrackParam>& Jy);

    /// extrapolate the track to the given Z using linearization at the straight line,
    /// \param z_out - Z coordinate to extrapolate to
    /// \return pair of the extrapolated X, and dX2 - the rms^2 of the extrapolated x
    std::pair<DataT, DataT> ExtrapolateLineXdX2(DataT z_out) const;

    /// extrapolate the track to the given Z using linearization at the straight line,
    /// \param z_out - Z coordinate to extrapolate to
    /// \return pair of the extrapolated Y, and dY2 - the rms^2 of the extrapolated y
    std::pair<DataT, DataT> ExtrapolateLineYdY2(DataT z_out) const;

    /// extrapolate the track to the given Z using linearization at the straight line,
    /// \param z_out - Z coordinate to extrapolate to
    /// \return extrapolated correlation cov<x,y>
    DataT ExtrapolateLineDxy(DataT z_out) const;

    /// add target measuremet to the track using linearisation at a straight line
    void FilterWithTargetAtLine(DataT targZ, const kf::MeasurementXy<DataT>& targXYInfo,
                                const kf::FieldRegion<DataT>& F);

    /// \brief Approximate mean energy loss with Bethe-Bloch formula
    /// \param bg2 (beta*gamma)^2
    /// \return mean energy loss
    static DataT ApproximateBetheBloch(DataT bg2);

    /// \brief Approximate mean energy loss with Bethe-Bloch formula
    /// \param bg2 (beta*gamma)^2
    /// \param kp0 density [g/cm^3]
    /// \param kp1 density effect first junction point
    /// \param kp2 density effect second junction point
    /// \param kp3 mean excitation energy [GeV]
    /// \param kp4 mean Z/A
    /// \return mean energy loss
    static DataT ApproximateBetheBloch(DataT bg2, DataT kp0, DataT kp1, DataT kp2, DataT kp3, DataT kp4);

    /// \brief git two chi^2 components of the track fit to measurement
    /// \param m - measurement
    /// \param x - track X
    /// \param y - track Y
    /// \param C00 - track covariance C00
    /// \param C10 - track covariance C10
    /// \param C11 - track covariance C11
    /// \return pair of (chi^2_x, chi^2_u) components of the chi^2.
    ///         chi^2_u is calculated after track is fit to the X measurement
    static std::tuple<DataT, DataT> GetChi2XChi2U(kf::MeasurementXy<DataT> m, DataT x, DataT y, DataT C00, DataT C10,
                                                  DataT C11);

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
    void GuessTrack(const DataT& trackZ, const DataT hitX[], const DataT hitY[], const DataT hitZ[], const DataT hitT[],
                    const DataT By[], const DataTmask hitW[], const DataTmask hitWtime[], int NHits);

   private:
    typedef const DataT cnst;

    ///--------------------------
    /// Data members

    DataTmask fMask{true};  ///< mask of active elements in simd vectors

    kf::TrackParam<DataT> fTr{};  ///< track parameters
    DataT fQp0{0.};

    DataT fMass{0.10565800};      ///< particle mass (muon mass by default)
    DataT fMass2{fMass * fMass};  ///< mass squared

    DataT fMaxExtraplationStep{50.};  ///< max extrapolation step [cm]

    bool fDoFitVelocity{0};  // should the track velocity be fitted as an independent parameter

  } _fvecalignment;

  // =============================================================================================

  template<typename DataT>
  inline std::string TrackKalmanFilter<DataT>::ToString(int i)
  {
    return fTr.ToString(i);
  }


  template<typename DataT>
  inline void TrackKalmanFilter<DataT>::SetOneEntry(const int i0, const TrackKalmanFilter& T1, const int i1)
  {
    fTr.SetOneEntry(i0, T1.fTr, i1);
    kf::utils::VecCopy<DataT, DataT, false, false>::CopyEntries(fQp0, i0, T1.fQp0, i1);
  }

  template<typename DataT>
  inline std::pair<DataT, DataT> TrackKalmanFilter<DataT>::ExtrapolateLineXdX2(DataT z_out) const
  {
    DataT dz = (z_out - fTr.GetZ());
    return std::pair(fTr.GetX() + fTr.GetTx() * dz, fTr.C00() + dz * (2 * fTr.C20() + dz * fTr.C22()));
  }

  template<typename DataT>
  inline std::pair<DataT, DataT> TrackKalmanFilter<DataT>::ExtrapolateLineYdY2(DataT z_out) const
  {
    DataT dz = (z_out - fTr.GetZ());
    return std::pair(fTr.GetY() + fTr.GetTy() * dz, fTr.C11() + dz * (DataT(2.) * fTr.C31() + dz * fTr.C33()));
  }

  template<typename DataT>
  inline DataT TrackKalmanFilter<DataT>::ExtrapolateLineDxy(DataT z_out) const
  {
    DataT dz = (z_out - fTr.GetZ());
    return fTr.C10() + dz * (fTr.C21() + fTr.C30() + dz * fTr.C32());
  }

}  // namespace cbm::algo::kf
