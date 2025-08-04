/* Copyright (C) 2007-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Igor Kulakov, Sergei Zharko */

/// \file KfMeasurementU
/// \brief Definition of the KfMeasurementU class

#pragma once  // include this header only once per compilation unit


#include "KfDefs.h"
#include "KfUtils.h"

#include <boost/serialization/access.hpp>

#include <string>

#include <xpu/device.h>

namespace cbm::algo::kf
{
  /// \brief The class describes a 1D - measurement U in XY coordinate system
  ///
  /// The measurement is defined as
  ///
  /// u = x * cos(phi) + y * sin(phi)
  ///
  /// where phi is the azimuthal angle of the measurement direction
  /// and x, y are the coordinates of the measurement
  /// The measurement may be scaled, i.e. cos^2 + sin^2 must not necessarily be equal to 1
  /// The measurement has a finite resolution, i.e. the measurement is not a point, but a distribution
  /// with a certain rms.
  /// The measurement may be used in the chi2 calculation or not
  /// The measurement may be a SIMD vector of values, when DataT is fvec type
  ///
  template<typename DataT>
  class alignas(VcMemAlign) MeasurementU {

   public:
    friend class boost::serialization::access;

    /// default constructor
    XPU_D MeasurementU() = default;

    /// constructor
    /// \param cosPhi   cos(phi)
    /// \param sinPhi   sin(phi)
    /// \param u        measurement, u = x * cos(phi) + y * sin(phi)
    /// \param du2      rms^2 of the measurement
    /// \param ndf      number of degrees of freedom (used for chi2 calculation)
    ///                 if ndf == 1, the measurement is used in the chi2 calculation
    ///                 if ndf == 0, the measurement is not used in the chi2 calculation
    XPU_D MeasurementU(DataT cosPhi, DataT sinPhi, DataT u, DataT du2, DataT ndf)
      : fCosPhi(cosPhi)
      , fSinPhi(sinPhi)
      , fU(u)
      , fDu2(du2)
      , fNdf(ndf)
    {
    }

    ///------------------------------
    /// Setters and getters

    XPU_D void SetCosPhi(DataT cosPhi) { fCosPhi = cosPhi; }
    XPU_D void SetSinPhi(DataT sinPhi) { fSinPhi = sinPhi; }
    XPU_D void SetU(DataT u) { fU = u; }
    XPU_D void SetDu2(DataT du2) { fDu2 = du2; }
    XPU_D void SetNdf(DataT ndf) { fNdf = ndf; }

    XPU_D DataT CosPhi() const { return fCosPhi; }
    XPU_D DataT SinPhi() const { return fSinPhi; }
    XPU_D DataT U() const { return fU; }
    XPU_D DataT Du2() const { return fDu2; }
    XPU_D DataT Ndf() const { return fNdf; }

    ///------------------------------
    /// Methods for debugging

    /// String representation of class contents
    /// \param indentLevel      number of indent characters in the output
    std::string ToString(int indentLevel = 0) const;

    /// Checks, if all fields are finite
    bool IsFinite() const
    {
      return (utils::IsFinite(CosPhi()) && utils::IsFinite(SinPhi()) && utils::IsFinite(U()) && utils::IsFinite(Du2())
              && utils::IsFinite(Ndf()));
    }

    /// Checks, if some fields are undefined
    bool IsUndefined() const
    {
      return (utils::IsUndefined(CosPhi()) || utils::IsUndefined(SinPhi()) || utils::IsUndefined(U())
              || utils::IsUndefined(Du2()) || utils::IsUndefined(Ndf()));
    }

    /// Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fCosPhi;
      ar& fSinPhi;
      ar& fU;
      ar& fDu2;
      ar& fNdf;
    }

   private:
    ///------------------------------
    /// Data members

    DataT fCosPhi = defs::Undef<DataT>;  ///< cos(phi)
    DataT fSinPhi = defs::Undef<DataT>;  ///< sin(phi)
    DataT fU      = defs::Undef<DataT>;  ///< measurement, u = x * cos(phi) + y * sin(phi)
    DataT fDu2    = defs::Undef<DataT>;  ///< rms^2 of the measurement

    /// \brief number of degrees of freedom (used for chi2 calculation)
    /// if ndf == 1, the measurement is used in the chi2 calculation
    /// if ndf == 0, the measurement is not used in the chi2 calculation
    DataT fNdf = defs::Undef<DataT>;
  } _fvecalignment;

}  // namespace cbm::algo::kf
