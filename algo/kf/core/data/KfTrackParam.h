/* Copyright (C) 2007-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer], Maksym Zyzak, Sergei Zharko */


/// \file   CaTrackParam.h
/// \brief  header file for the ca::TrackParam class
/// \since  02.06.2022
/// \author Sergey Gorbunov

#ifndef CA_CORE_CaTrackParam_h
#define CA_CORE_CaTrackParam_h 1

#include "KfDefs.h"
#include "KfSimd.h"
#include "KfUtils.h"

#include <boost/serialization/access.hpp>

#include <string>

// #if !defined(NO_ROOT) && !XPU_IS_HIP_CUDA
// #include <Rtypes.h>  // for ClassDef
// #endif

#include <xpu/device.h>

namespace cbm::algo::kf
{
  /// \class cbm::algo::kf::TrackParamBase
  /// \brief It is a technical base class of kf::TrackParam
  /// \brief that represents fitted track parameters at given z
  ///
  /// This is a template that can be instantiated for different floating point types.
  /// Track parameters: variable parameterts {x, y, tx, ty, q/p, t, vi} at fixed z
  /// Covariation matrix: low-diagopnal C[28]
  ///
  template<typename T>
  class alignas(VcMemAlign) TrackParamBase {

   public:
    friend class boost::serialization::access;

    static constexpr int kNtrackParam{7};  ///<  N of variable track parameters: {x, y, tx, ty, q/p, t, vi}
    static constexpr int kNcovParam{(kNtrackParam) * (kNtrackParam + 1) / 2};  ///< N of covariance matrix parameters

    typedef std::array<T, kNcovParam> CovMatrix_t;  ///< covariance matrix type

    TrackParamBase() = default;

    template<typename T1>
    TrackParamBase(const TrackParamBase<T1>& tr)
    {
      Set(tr);
    }

    /// Set all SIMD entries from all SIMD entries of the other class
    /// It works for scalar and fvec types,
    /// except of the case when T is scalar and TdataB is fvec.
    template<typename T1>
    void Set(const TrackParamBase<T1>& Tb)
    {
      CopyBase<T1, true, true>(0, Tb, 0);
    }

    /// Set all SIMD entries from one SIMD entry of the other class
    /// It also works when T is scalar
    void Set(const TrackParamBase<fvec>& Tb, const int ib) { CopyBase<fvec, true, false>(0, Tb, ib); }

    XPU_D inline void SetOneGpu(const TrackParamBase<float>& other)
    {
        fX = other.GetX();
        fY = other.GetY();
        fTx = other.GetTx();
        fTy = other.GetTy();
        fQp = other.GetQp();
        fZ = other.GetZ();
        fT = other.GetTime();
        fVi = other.GetVi();

        for (int i = 0; i < kNcovParam; ++i)
            fCovMatrix[i] = other.GetCovMatrix()[i];

        fChiSq     = other.GetChiSq();
        fNdf       = other.GetNdf();
        fChiSqTime = other.GetChiSqTime();
        fNdfTime   = other.GetNdfTime();
    }

    /// Set one SIMD entry from one SIMD entry of the other class
    /// It only works when T is fvec, TdataB is scalar
    template<typename T1>
    void SetOneEntry(const int ia, const TrackParamBase<T1>& Tb)
    {
      CopyBase<T1, false, true>(ia, Tb, 0);
    }

    /// Set one SIMD entry from one SIMD entry of the other class
    /// It only works when T is fvec, TdataB is fvec
    void SetOneEntry(const int ia, const TrackParamBase<fvec>& Tb, const int ib)
    {
      CopyBase<fvec, false, false>(ia, Tb, ib);
    }

    /// ---------------------------------------------------------------------------------------------------------------------
    /// Getters without 'Get' prefix to ease the math formulae
    ///

    /// \brief Gets z position [cm]
    T Z() const { return fZ; }

    /// \brief Gets x position [cm]
    T X() const { return fX; }

    /// \brief Gets y position [cm]
    T Y() const { return fY; }

    /// \brief Gets slope along x-axis
    T Tx() const { return fTx; }

    /// \brief Gets slope along y-axis
    T Ty() const { return fTy; }

    /// \brief Gets charge over momentum [ec/GeV]
    T Qp() const { return fQp; }

    /// \brief Gets time [ns]
    T Time() const { return fT; }

    /// \brief Gets inverse velocity [ns/cm]
    T Vi() const { return fVi; }

    /// \brief Gets Chi-square of track fit model
    T ChiSq() const { return fChiSq; }

    /// \brief Gets NDF of track fit model
    T Ndf() const { return fNdf; }

    /// \brief Gets Chi-square of time measurements
    T ChiSqTime() const { return fChiSqTime; }

    /// \brief Gets NDF of time measurements
    T NdfTime() const { return fNdfTime; }

    /// \brief Get covariance matrix element
    /// \param i row
    /// \param j column
    /// \return covariance matrix element
    T C(int i, int j) const
    {
      int ind = (j <= i) ? i * (1 + i) / 2 + j : j * (1 + j) / 2 + i;
      return fCovMatrix[ind];
    }

    /// \brief Get covariance matrix element when indices are known at compile time
    /// \param i row
    /// \param j column
    /// \return matrix element
    template<int i, int j>
    T C() const
    {
      constexpr int ind = (j <= i) ? i * (1 + i) / 2 + j : j * (1 + j) / 2 + i;
      return fCovMatrix[ind];
    }

    /// \brief Individual getters for covariance matrix elements
    /// \return covariance matrix element
    ///
    T C00() const { return C<0, 0>(); }
    T C01() const { return C<0, 1>(); }
    T C02() const { return C<0, 2>(); }
    T C03() const { return C<0, 3>(); }
    T C04() const { return C<0, 4>(); }
    T C05() const { return C<0, 5>(); }
    T C06() const { return C<0, 6>(); }

    T C10() const { return C<1, 0>(); }
    T C11() const { return C<1, 1>(); }
    T C12() const { return C<1, 2>(); }
    T C13() const { return C<1, 3>(); }
    T C14() const { return C<1, 4>(); }
    T C15() const { return C<1, 5>(); }
    T C16() const { return C<1, 6>(); }

    T C20() const { return C<2, 0>(); }
    T C21() const { return C<2, 1>(); }
    T C22() const { return C<2, 2>(); }
    T C23() const { return C<2, 3>(); }
    T C24() const { return C<2, 4>(); }
    T C25() const { return C<2, 5>(); }
    T C26() const { return C<2, 6>(); }

    T C30() const { return C<3, 0>(); }
    T C31() const { return C<3, 1>(); }
    T C32() const { return C<3, 2>(); }
    T C33() const { return C<3, 3>(); }
    T C34() const { return C<3, 4>(); }
    T C35() const { return C<3, 5>(); }
    T C36() const { return C<3, 6>(); }

    T C40() const { return C<4, 0>(); }
    T C41() const { return C<4, 1>(); }
    T C42() const { return C<4, 2>(); }
    T C43() const { return C<4, 3>(); }
    T C44() const { return C<4, 4>(); }
    T C45() const { return C<4, 5>(); }
    T C46() const { return C<4, 6>(); }

    T C50() const { return C<5, 0>(); }
    T C51() const { return C<5, 1>(); }
    T C52() const { return C<5, 2>(); }
    T C53() const { return C<5, 3>(); }
    T C54() const { return C<5, 4>(); }
    T C55() const { return C<5, 5>(); }
    T C56() const { return C<5, 6>(); }

    T C60() const { return C<6, 0>(); }
    T C61() const { return C<6, 1>(); }
    T C62() const { return C<6, 2>(); }
    T C63() const { return C<6, 3>(); }
    T C64() const { return C<6, 4>(); }
    T C65() const { return C<6, 5>(); }
    T C66() const { return C<6, 6>(); }

    /// ---------------------------------------------------------------------------------------------------------------------
    ///  Getters with 'Get' prefix
    ///  Some of them involve calculations

    /// \brief Gets z position [cm]
    XPU_D XPU_H T GetZ() const { return fZ; }

    /// \brief Gets x position [cm]
    XPU_D XPU_H T GetX() const { return fX; }

    /// \brief Gets x position error [cm]
    XPU_D XPU_H T GetXError() const { return sqrt(C00()); }

    /// \brief Gets y position [cm]
    XPU_D XPU_H T GetY() const { return fY; }

    /// \brief Gets y position error [cm]
    XPU_D XPU_H T GetYError() const { return sqrt(C11()); }

    /// \brief Gets slope along x-axis
    XPU_D XPU_H T GetTx() const { return fTx; }

    /// \brief Gets error of slope along x-axis
    XPU_D XPU_H T GetTxError() const { return sqrt(C22()); }

    /// \brief Gets slope along y-axis
    XPU_D XPU_H T GetTy() const { return fTy; }

    /// \brief Gets error of slope along y-axis
    XPU_D XPU_H T GetTyError() const { return sqrt(C33()); }

    /// \brief Gets charge over momentum [ec/GeV]
    XPU_D XPU_H T GetQp() const { return fQp; }

    /// \brief Gets error of charge over momentum [ec/GeV]
    XPU_D XPU_H T GetQpError() const { return sqrt(C44()); }

    /// \brief Gets time [ns]
    XPU_D XPU_H T GetTime() const { return fT; }

    /// \brief Gets time error [ns]
    XPU_D XPU_H T GetTimeError() const { return sqrt(C55()); }

    /// \brief Gets inverse velocity [ns/cm] in downstream direction
    XPU_D XPU_H T GetVi() const { return fVi; }

    /// \brief Gets inverse velocity error [ns/cm]
    XPU_D XPU_H T GetViError() const { return sqrt(C66()); }

    /// \brief Gets covariance matrix
    XPU_D XPU_H const CovMatrix_t& GetCovMatrix() const { return fCovMatrix; }

    /// \brief Get covariance matrix element
    /// \param i row
    /// \param j column
    /// \return covariance matrix element
    XPU_D XPU_H T GetCovariance(int i, int j) const { return C(i, j); }

    /// Gets Chi-square of track fit model
    XPU_D XPU_H T GetChiSq() const { return fChiSq; }

    /// Gets NDF of track fit model
    XPU_D XPU_H T GetNdf() const { return fNdf; }

    /// Gets Chi-square of time measurements
    XPU_D XPU_H T GetChiSqTime() const { return fChiSqTime; }

    /// Gets NDF of time measurements
    XPU_D XPU_H T GetNdfTime() const { return fNdfTime; }

    /// Gets charge
    XPU_D XPU_H T GetCharge() const { return utils::iif(GetQp() > T(0.), T(1.), T(-1.)); }

    /// \brief Gets azimuthal angle [rad]
    XPU_D XPU_H T GetPhi() const { return atan2(GetTy(), GetTx()); }

    /// \brief Gets azimuthal angle error [rad]
    XPU_D XPU_H T GetPhiError() const;

    /// \brief Gets polar angle [rad]
    XPU_D XPU_H T GetTheta() const { return atan(sqrt(GetTx() * GetTx() + GetTy() * GetTy())); }

    /// \brief Gets polar angle error [rad]
    XPU_D XPU_H T GetThetaError() const;

    /// Gets momentum [GeV/ec]. For the straight tracks returns 1.e4 [GeV/ec]
    XPU_D XPU_H T GetP() const { return utils::iif(utils::fabs(GetQp()) > T(1.e-4), T(1.) / utils::fabs(GetQp()), T(1.e4)); }

    /// Gets z-component of the momentum [GeV/ec]
    XPU_D XPU_H T GetPz() const { return GetP() / sqrt(T(1.) + GetTx() * GetTx() + GetTy() * GetTy()); }

    /// Gets x-component of the momentum [GeV/ec]
    XPU_D XPU_H T GetPx() const { return GetPz() * GetTx(); }

    /// Gets y-component of the momentum [GeV/ec]
    XPU_D XPU_H T GetPy() const { return GetPz() * GetTy(); }

    /// Gets transverse momentum
    XPU_D XPU_H T GetPt() const
    {
      T t2 = GetTx() * GetTx() + GetTy() * GetTy();
      return GetP() * sqrt(t2 / (T(1.) + t2));
    }

    /// ---------------------------------------------------------------------------------------------------------------------
    /// Parameter setters
    ///

    /// \brief Sets z position [cm]
    XPU_D void SetZ(T v) { fZ = v; }

    /// \brief Sets x position [cm]
    XPU_D void SetX(T v) { fX = v; }

    /// \brief Sets y position [cm]
    XPU_D void SetY(T v) { fY = v; }

    /// \brief Sets slope along x-axis
    XPU_D void SetTx(T v) { fTx = v; }

    /// \brief Sets slope along y-axis
    XPU_D void SetTy(T v) { fTy = v; }

    /// \brief Sets charge over momentum [ec/GeV]
    XPU_D void SetQp(T v) { fQp = v; }

    /// \brief Sets time [ns]
    XPU_D void SetTime(T v) { fT = v; }

    /// \brief Sets inverse velocity [ns/cm]
    XPU_D void SetVi(T v) { fVi = v; }

    /// \brief Sets Chi-square of track fit model
    XPU_D void SetChiSq(T v) { fChiSq = v; }

    /// \brief Sets NDF of track fit model
    XPU_D void SetNdf(T v) { fNdf = v; }

    /// \brief Sets Chi-square of time measurements
    XPU_D void SetChiSqTime(T v) { fChiSqTime = v; }

    /// \brief Sets NDF of time measurements
    XPU_D void SetNdfTime(T v) { fNdfTime = v; }

    /// \brief Sets covariance matrix
    XPU_D void SetCovMatrix(const CovMatrix_t& val) { fCovMatrix = val; }

    /// \brief Sets all elements of the covariance matrix to zero
    XPU_D void ResetCovMatrix() { memset(fCovMatrix.begin(), 0., kNcovParam * sizeof(T)); }

    /// \brief Get covariance matrix element
    /// \param i row
    /// \param j column
    /// \param val covariance matrix element
    void SetCovariance(int i, int j, T val)
    {
      int ind         = (j <= i) ? i * (1 + i) / 2 + j : j * (1 + j) / 2 + i;
      fCovMatrix[ind] = val;
    }

    /// \brief Get covariance matrix element when indices are known at compile time
    /// \param i row
    /// \param j column
    /// \param val matrix element
    template<int i, int j>
    XPU_D void SetCovariance(T val)
    {
      constexpr int ind = (j <= i) ? i * (1 + i) / 2 + j : j * (1 + j) / 2 + i;
      fCovMatrix[ind]   = val;
    }

    /// \brief Individual setters for covariance matrix elements
    ///
    XPU_D void SetC00(T val) { SetCovariance<0, 0>(val); }
    XPU_D void SetC10(T val) { SetCovariance<1, 0>(val); }
    XPU_D void SetC11(T val) { SetCovariance<1, 1>(val); }
    XPU_D void SetC20(T val) { SetCovariance<2, 0>(val); }
    XPU_D void SetC21(T val) { SetCovariance<2, 1>(val); }
    XPU_D void SetC22(T val) { SetCovariance<2, 2>(val); }
    XPU_D void SetC30(T val) { SetCovariance<3, 0>(val); }
    XPU_D void SetC31(T val) { SetCovariance<3, 1>(val); }
    XPU_D void SetC32(T val) { SetCovariance<3, 2>(val); }
    XPU_D void SetC33(T val) { SetCovariance<3, 3>(val); }
    XPU_D void SetC40(T val) { SetCovariance<4, 0>(val); }
    XPU_D void SetC41(T val) { SetCovariance<4, 1>(val); }
    XPU_D void SetC42(T val) { SetCovariance<4, 2>(val); }
    XPU_D void SetC43(T val) { SetCovariance<4, 3>(val); }
    XPU_D void SetC44(T val) { SetCovariance<4, 4>(val); }
    XPU_D void SetC50(T val) { SetCovariance<5, 0>(val); }
    XPU_D void SetC51(T val) { SetCovariance<5, 1>(val); }
    XPU_D void SetC52(T val) { SetCovariance<5, 2>(val); }
    XPU_D void SetC53(T val) { SetCovariance<5, 3>(val); }
    XPU_D void SetC54(T val) { SetCovariance<5, 4>(val); }
    XPU_D void SetC55(T val) { SetCovariance<5, 5>(val); }
    XPU_D void SetC60(T val) { SetCovariance<6, 0>(val); }
    XPU_D void SetC61(T val) { SetCovariance<6, 1>(val); }
    XPU_D void SetC62(T val) { SetCovariance<6, 2>(val); }
    XPU_D void SetC63(T val) { SetCovariance<6, 3>(val); }
    XPU_D void SetC64(T val) { SetCovariance<6, 4>(val); }
    XPU_D void SetC65(T val) { SetCovariance<6, 5>(val); }
    XPU_D void SetC66(T val) { SetCovariance<6, 6>(val); }

    ///---------------------------------------------------------------------------------------------------------------------
    /// References to parameters to ease the math formulae
    /// They are especially useful for masked SIMD operations like:  X()( mask ) = fvec(1.);
    ///

    /// \brief Reference to z position [cm]
    XPU_D T& Z() { return fZ; }

    /// \brief Reference to x position [cm]
    XPU_D T& X() { return fX; }

    /// \brief Reference to y position [cm]
    XPU_D T& Y() { return fY; }

    /// \brief Reference to slope along x-axis
    XPU_D T& Tx() { return fTx; }

    /// \brief Reference to slope along y-axis
    XPU_D T& Ty() { return fTy; }

    /// \brief Reference to charge over momentum [ec/GeV]
    XPU_D T& Qp() { return fQp; }

    /// \brief Reference to time [ns]
    XPU_D T& Time() { return fT; }

    /// \brief Reference to inverse velocity [ns/cm]
    XPU_D T& Vi() { return fVi; }

    /// \brief Reference to covariance matrix
    CovMatrix_t& CovMatrix() { return fCovMatrix; }

    /// \brief Reference to Chi-square of track fit model
    XPU_D T& ChiSq() { return fChiSq; }

    /// \brief Reference to NDF of track fit model
    XPU_D T& Ndf() { return fNdf; }

    /// \brief Reference to Chi-square of time measurements
    XPU_D T& ChiSqTime() { return fChiSqTime; }

    /// \brief Reference to NDF of time measurements
    XPU_D T& NdfTime() { return fNdfTime; }

    /// \brief Get a reference to the covariance matrix element
    /// \param i row
    /// \param j column
    /// \return matrix element
    XPU_D T& C(int i, int j)
    {
      int ind = (j <= i) ? i * (1 + i) / 2 + j : j * (1 + j) / 2 + i;
      return fCovMatrix[ind];
    }

    /// \brief Get a reference to the covariance matrix element when indices are known at compile time
    /// \param i row
    /// \param j column
    /// \param val matrix element
    template<int i, int j>
    XPU_D T& C()
    {
      constexpr int ind = (j <= i) ? i * (1 + i) / 2 + j : j * (1 + j) / 2 + i;
      return fCovMatrix[ind];
    }

    /// \brief Individual references to covariance matrix elements
    ///
    XPU_D T& C00() { return C<0, 0>(); }
    XPU_D T& C01() { return C<0, 1>(); }
    XPU_D T& C02() { return C<0, 2>(); }
    XPU_D T& C03() { return C<0, 3>(); }
    XPU_D T& C04() { return C<0, 4>(); }
    XPU_D T& C05() { return C<0, 5>(); }
    XPU_D T& C06() { return C<0, 6>(); }

    XPU_D T& C10() { return C<1, 0>(); }
    XPU_D T& C11() { return C<1, 1>(); }
    XPU_D T& C12() { return C<1, 2>(); }
    XPU_D T& C13() { return C<1, 3>(); }
    XPU_D T& C14() { return C<1, 4>(); }
    XPU_D T& C15() { return C<1, 5>(); }
    XPU_D T& C16() { return C<1, 6>(); }

    XPU_D T& C20() { return C<2, 0>(); }
    XPU_D T& C21() { return C<2, 1>(); }
    XPU_D T& C22() { return C<2, 2>(); }
    XPU_D T& C23() { return C<2, 3>(); }
    XPU_D T& C24() { return C<2, 4>(); }
    XPU_D T& C25() { return C<2, 5>(); }
    XPU_D T& C26() { return C<2, 6>(); }

    XPU_D T& C30() { return C<3, 0>(); }
    XPU_D T& C31() { return C<3, 1>(); }
    XPU_D T& C32() { return C<3, 2>(); }
    XPU_D T& C33() { return C<3, 3>(); }
    XPU_D T& C34() { return C<3, 4>(); }
    XPU_D T& C35() { return C<3, 5>(); }
    XPU_D T& C36() { return C<3, 6>(); }

    XPU_D T& C40() { return C<4, 0>(); }
    XPU_D T& C41() { return C<4, 1>(); }
    XPU_D T& C42() { return C<4, 2>(); }
    XPU_D T& C43() { return C<4, 3>(); }
    XPU_D T& C44() { return C<4, 4>(); }
    XPU_D T& C45() { return C<4, 5>(); }
    XPU_D T& C46() { return C<4, 6>(); }

    XPU_D T& C50() { return C<5, 0>(); }
    XPU_D T& C51() { return C<5, 1>(); }
    XPU_D T& C52() { return C<5, 2>(); }
    XPU_D T& C53() { return C<5, 3>(); }
    XPU_D T& C54() { return C<5, 4>(); }
    XPU_D T& C55() { return C<5, 5>(); }
    XPU_D T& C56() { return C<5, 6>(); }

    XPU_D T& C60() { return C<6, 0>(); }
    XPU_D T& C61() { return C<6, 1>(); }
    XPU_D T& C62() { return C<6, 2>(); }
    XPU_D T& C63() { return C<6, 3>(); }
    XPU_D T& C64() { return C<6, 4>(); }
    XPU_D T& C65() { return C<6, 5>(); }
    XPU_D T& C66() { return C<6, 6>(); }


    ///---------------------------------------------------------------------------------------------------------------------
    /// Other methods

    /// \brief Resets variances of track parameters and chi2, ndf values
    /// \param c00  Variance of x-position [cm2]
    /// \param c11  Variance of y-position [cm2]
    /// \param c22  Variance of slope along x-axis
    /// \param c33  Variance of slope along y-axis
    /// \param c44  Variance of charge over momentum [(ec/GeV)2]
    /// \param c55  Variance of time [ns2]
    /// \param c66  Variance of inverse velocity [1/c2]
    XPU_D void ResetErrors(T c00, T c11, T c22, T c33, T c44, T c55, T c66);

    /// \brief Prints parameters to a string
    /// \param i  Index of SIMD vector element (when i== -1, the entire vector is printed)
    std::string ToString(int i = -1) const;

    /// \brief Prints correlations to a string
    /// \param i  Index of SIMD vector element (when i== -1, the entire vector is printed)
    std::string ToStringCorrelations(int i = -1) const;

    /// \brief Checks whether some parameters are finite
    bool IsFinite(bool printWhenWrong) const;

    /// \brief Checks whether SIMD entry i is consistent
    bool IsEntryConsistent(bool printWhenWrong, int i) const;

    /// \brief Checks whether first nFilled SIMD entries are consistent
    bool IsConsistent(bool printWhenWrong, int nFilled) const;

    /// \brief Initializes inverse velocity range
    XPU_D void InitVelocityRange(fscal minP);


    ///---------------------------------------------------------------------------------------------------------------------
    /// Serialization
    ///
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fX;
      ar& fY;
      ar& fTx;
      ar& fTy;
      ar& fQp;
      ar& fZ;
      ar& fT;
      ar& fVi;
      ar& fCovMatrix;
      ar& fChiSq;
      ar& fNdf;
      ar& fChiSqTime;
      ar& fNdfTime;
    }

   private:
    /// \brief Copies all/one entries from the other class
    /// \tparam TdataB  Type of the other class
    /// \tparam TDoAllA  If true, all entries of the current class must be set
    /// \tparam TDoAllB  If true, all entries of the other class must be used
    /// \param ia  Index of SIMD vector element of the current class
    /// \param Tb  Other class
    /// \param ib  Index of SIMD vector element of the other class
    template<typename T1, bool TDoAllA, bool TDoAllB>
    void CopyBase(const int ia, const TrackParamBase<T1>& Tb, const int ib);


   private:
    /// ---------------------------------------------------------------------------------------------------------------------
    /// Class members

    // Initializing parameters with NANs spoils the track fit where
    // the masked-out SIMD entries are suppressed by a multication by 0.
    // Therefore, we initialize the data members here with finite numbers.
    // For the numerical safety, with some reasonable numbers.

    // clang-format off
    CovMatrix_t fCovMatrix {1.,
                            0., 1.,
                            0., 0., 1.,
                            0., 0., 0., 1.,
                            0., 0., 0., 0., 1.,
                            0., 0., 0., 0., 0., 1.,
                            0., 0., 0., 0., 0., 0., 1.};  ///< covariance matrix
    // clang-format on

    T fX{0.};   ///< x-position [cm]
    T fY{0.};   ///< y-position [cm]
    T fZ{0.};   ///< z-position [cm]
    T fTx{0.};  ///< slope along x-axis
    T fTy{0.};  ///< slope along y-axis
    T fQp{0.};  ///< charge over momentum [ec/GeV]
    T fT{0.};   ///< time [ns]
    T fVi{0.};  ///< inverse velocity in downstream direction [ns/cm]

    T fChiSq{0.};      ///< chi^2 of track fit, spatial measurements
    T fNdf{0.};        ///< NDF of track fit, spatial measurements
    T fChiSqTime{0.};  ///< chi^2 of track fit, time measurements
    T fNdfTime{0.};    ///< NDF of track fit, time measurements

// #if !defined(NO_ROOT) && !XPU_IS_HIP_CUDA
//     ClassDefNV(TrackParamBase, 1);
// #endif

  };  // class TrackParamBase


  /// \class cbm::algo::ca:: TrackParamBaseScalar
  /// \brief Scalar version of TrackParamBase
  ///
  /// It contains extra methods that are difficult to implement in SIMD version
  template<typename T>
  class TrackParamBaseScalar : public TrackParamBase<T> {
   public:
    using TrackParamBase<T>::TrackParamBase;

    /// ---------------------------------------------------------------------------------------------------------------------using
    /// \brief Gets pseudo-rapidity
    T GetEta() const { return -log(tan(this->GetTheta() * T(0.5))); }

// #if !defined(NO_ROOT) && !XPU_IS_HIP_CUDA
//     ClassDefNV(TrackParamBaseScalar, 1);
// #endif
  };


  // ---------------------------------------------------------------------------------------------------------------------
  /// TrackParam classes of different types

  /// \class cbm::algo::kf::TrackParam
  /// \brief Class represents fitted track parameters at given z
  ///
  template<typename T>
  class TrackParam : public TrackParamBaseScalar<T> {
   public:
    using TrackParamBaseScalar<T>::TrackParamBaseScalar;

// #if !defined(NO_ROOT) && !XPU_IS_HIP_CUDA
//     ClassDefNV(TrackParam, 1);
// #endif
  };

  template<>
  class TrackParam<fvec> : public TrackParamBase<fvec> {
   public:
    using TrackParamBase<fvec>::TrackParamBase;

// #if !defined(NO_ROOT) && !XPU_IS_HIP_CUDA
//     ClassDefNV(TrackParam<fvec>, 1);
// #endif
  };


  typedef TrackParam<fvec> TrackParamV;
  typedef TrackParam<fscal> TrackParamS;
  typedef TrackParam<double> TrackParamD;


  /// ---------------------------------------------------------------------------------------------------------------------
  /// Inline and template function implementation


  /// ---------------------------------------------------------------------------------------------------------------------
  ///
  template<typename T>
  XPU_D XPU_H inline T TrackParamBase<T>::GetPhiError() const
  {
    // phi = atan( tx / ty ); }

    T phiDdenom = GetTx() * GetTx() + GetTy() * GetTy();
    T phiDTx    = -GetTy() / phiDdenom;  // partial derivative of phi over Tx
    T phiDTy    = +GetTx() / phiDdenom;  // partial derivative of phi over Ty

    T varTx   = C22();  // variance of Tx
    T varTy   = C33();  // variance of Ty
    T covTxTy = C32();  // covariance of Tx and Ty

    T varPhi = phiDTx * phiDTx * varTx + phiDTy * phiDTy * varTy + T(2.) * phiDTx * phiDTy * covTxTy;
    return sqrt(varPhi);
  }


  /// ---------------------------------------------------------------------------------------------------------------------
  ///
  template<typename T>
  XPU_D XPU_H inline T TrackParamBase<T>::GetThetaError() const
  {
    // theta = atan(sqrt( tx * tx + ty * ty) )

    T sumSqSlopes = GetTx() * GetTx() + GetTy() * GetTy();
    T thetaDdenom = sqrt(sumSqSlopes) * (T(1.) + sumSqSlopes);
    T thetaDTx    = GetTx() / thetaDdenom;
    T thetaDTy    = GetTy() / thetaDdenom;

    T varTx   = C22();  // variance of Tx
    T varTy   = C33();  // variance of Ty
    T covTxTy = C32();  // covariance of Tx and Ty

    T varTheta = thetaDTx * thetaDTx * varTx + thetaDTy * thetaDTy * varTy + T(2.) * thetaDTx * thetaDTy * covTxTy;

    return sqrt(varTheta);
  }


  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<typename TdataA>
  template<typename TdataB, bool TDoAllA, bool TDoAllB>
  inline void TrackParamBase<TdataA>::CopyBase(const int ia, const TrackParamBase<TdataB>& Tb, const int ib)
  {
    auto copy = [&](TdataA& a, const TdataB& b) {
      utils::VecCopy<TdataA, TdataB, TDoAllA, TDoAllB>::CopyEntries(a, ia, b, ib);
    };

    copy(fX, Tb.GetX());
    copy(fY, Tb.GetY());
    copy(fTx, Tb.GetTx());
    copy(fTy, Tb.GetTy());
    copy(fQp, Tb.GetQp());
    copy(fZ, Tb.GetZ());
    copy(fT, Tb.GetTime());
    copy(fVi, Tb.GetVi());

    for (int i = 0; i < kNcovParam; ++i) {
      copy(fCovMatrix[i], Tb.GetCovMatrix()[i]);
    }

    copy(fChiSq, Tb.GetChiSq());
    copy(fNdf, Tb.GetNdf());
    copy(fChiSqTime, Tb.GetChiSqTime());
    copy(fNdfTime, Tb.GetNdfTime());

  }  // CopyBase


  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  XPU_D inline void TrackParamBase<T>::ResetErrors(T c00, T c11, T c22, T c33, T c44, T c55, T c66)
  {
    //    fCovMatrix.fill(0.);
    for (auto& element : fCovMatrix) {
      element = 0.;
    }

    SetC00(c00);
    SetC11(c11);
    SetC22(c22);
    SetC33(c33);
    SetC44(c44);
    SetC55(c55);
    SetC66(c66);

    SetChiSq(0.);
    SetNdf(-5.);
    SetChiSqTime(0.);
    SetNdfTime(-2.);
  }

  // ---------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  XPU_D inline void TrackParamBase<T>::InitVelocityRange(fscal minP)
  {
    // initialise the velocity range with respect to the minimal momentum minP {GeV/c}
    using defs::ProtonMass;
    using defs::SpeedOfLightInv;
    fscal maxVi = sqrt(1. + (ProtonMass<double> / minP) * (ProtonMass<double> / minP)) * SpeedOfLightInv<double>;
    fscal minVi = SpeedOfLightInv<fscal>;
    fscal vmean = minVi + 0.4 * (maxVi - minVi);
    fscal dvi   = (maxVi - vmean) / 3.;
    SetVi(vmean);
    SetC66(dvi * dvi);
  }

}  // namespace cbm::algo::kf

#endif  // CA_CORE_CaTrackParam_h