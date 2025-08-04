/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfFieldRegion.h
/// \brief  Magnetic flux density interpolation along the track vs. z-coordinate (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  19.07.2024
///

#pragma once

#include "KfDefs.h"
#include "KfFieldValue.h"
#include "KfMath.h"
#include "KfUtils.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/split_free.hpp>

#include <array>
#include <optional>
#include <string>
#include <type_traits>

namespace cbm::algo::kf
{

  /// \class GlobalField
  /// \brief Handler for the global magnetic field related functions
  class GlobalField {
   public:
    /// \brief Global variable to store the fielf funciton (x,y,z)->(Bx,By,Bz)
    static FieldFn_t fgOriginalField;

    /// \brief Global field type
    static EFieldType fgOriginalFieldType;  ///< global field type

    /// \brief Undefined magnetic field function
    static std::tuple<double, double, double> UndefField(double, double, double);

    /// \brief Sets global field function
    /// \param fn  Field function of the FairRoot format
    static void SetFieldFunction(EFieldType fldType, const FieldFn_t& fn)
    {
      fgOriginalFieldType = fldType;
      fgOriginalField     = fn;
    }

    /// \brief Creates a field value object in a spactial point of a generic floating-point type
    /// \tparam T       Floating point data-type of the involved variables
    /// \param fieldFn  Field function (x,y,z)->(Bx,By,Bz)
    /// \param x        The point x-coordinate [cm]
    /// \param y        The point y-coordinate [cm]
    /// \param z        The point z-coordinate [cm]
    /// \return  FieldValue<T> object
    template<typename T>
    [[gnu::always_inline]] static FieldValue<T> GetFieldValue(const FieldFn_t& fieldFn, const T& x, const T& y,
                                                              const T& z);

    /// \brief Forces the use of the original field function
    static void ForceUseOfOriginalField(bool v = true) { fgForceUseOfOriginalField = v; }

    /// \brief Checks if the original field function is used
    static bool IsUsingOriginalFieldForced() { return fgForceUseOfOriginalField; }

   private:
    static bool fgForceUseOfOriginalField;  ///< Flag to force the use of the original field in all field classes
  };

  namespace detail
  {
    /// \class  FieldRegionBase
    /// \brief  Base class of the FieldRegion class (primary template)
    /// \tparam T          Underlying floating point type (float/double/fvec)
    /// \tparam OrigField  When true, the original CBM field function is used. When false, the polynomial
    ///                    interpolation is used instead.
    template<typename T, EFieldMode FldMode>
    class alignas(VcMemAlign) FieldRegionBase {
    };

    /// \class  FieldRegionBase<T, EFieldMode::Orig>
    /// \brief  Data members and specific methods of the FieldRegion with the original magnetic field
    /// \tparam T  Underlying floating point type (float/double/fvec)
    template<typename T>
    class alignas(VcMemAlign) FieldRegionBase<T, EFieldMode::Orig> {
      template<typename, EFieldMode>
      friend class FieldRegionBase;

     public:
      /// \brief Default constructor
      FieldRegionBase() = default;

      /// \brief Constructor
      FieldRegionBase(const FieldFn_t& fieldFn) : fFieldFn(fieldFn) {}

      /// \brief  Copy constructor
      /// \tparam I  Underlying floating point type of the source
      template<typename I>
      FieldRegionBase(const FieldRegionBase<I, EFieldMode::Orig>& other) : fFieldFn(other.fFieldFn)
      {
      }

      /// \brief Destructor
      ~FieldRegionBase() = default;

      /// \brief Copy assignment operator
      FieldRegionBase& operator=(const FieldRegionBase& other) = default;

      /// \brief Gets the field value in a spatial point on the track
      /// \param x  x-coordinate of the point [cm]
      /// \param y  y-coordinate of the point [cm]
      /// \param z  z-coordinate of the point [cm]
      /// \note  The x and y coordinates are ignored, if the interpolated field is used
      FieldValue<T> Get(const T& x, const T& y, const T& z) const
      {
        return GlobalField::GetFieldValue(fFieldFn, x, y, z);
      }

      /// \brief Gets the double integrals of the field along the track
      std::tuple<T, T, T> GetDoubleIntegrals(const T& /*x1*/, const T& /*y1*/, const T& /*z1*/,  //
                                             const T& /*x2*/, const T& /*y2*/, const T& /*z2*/) const
      {
        assert("cbm::algo::kf::FieldRegionBase<T, EFieldMode::Orig>::GetDoubleIntegrals() is not implemented");
        return {defs::Undef<T>, defs::Undef<T>, defs::Undef<T>};
      }

      /// Consistency checker
      void CheckConsistency() const;

      /// \brief String representation of the class content
      /// \param intdentLevel  Indent level
      /// \param verbose       Verbosity level
      std::string ToString(int indentLevel = 0, int verbose = 1) const;

     protected:
      FieldFn_t fFieldFn{defs::ZeroFieldFn};  ///< Field function: (x,y,z) [cm] -> (Bx,By,Bz) [kG]
    };


    /// \class  FieldRegionBase<T, Intrpl>
    /// \brief  Data members and specific mehtods of the FieldRegion class with the interpolated magnetic field
    /// \tparam T  Underlying floating  point type (float/double/fvec)
    template<typename T>
    class alignas(VcMemAlign) FieldRegionBase<T, EFieldMode::Intrpl> {
      template<typename, EFieldMode>
      friend class FieldRegionBase;

      using CoeffArray_t = std::array<std::array<T, 3>, 3>;  ///< Approximation coefficients array

     public:
      using FieldValue_t = FieldValue<T>;

      /// \brief Default constructor
      FieldRegionBase() = default;

      /// \brief Constructor from parameters
      /// \param b0  Field value in the first node [kG]
      /// \param z0  First node z-coordinate [cm]
      /// \param b1  Field value in the first node [kG]
      /// \param z1  Second node z-coordinate [cm]
      /// \param b2  Field value in the first node [kG]
      /// \param z2  Third node z-coordinate [cm]
      FieldRegionBase(const FieldValue_t& b0, const T& z0, const FieldValue_t& b1, const T& z1, const FieldValue_t& b2,
                      const T& z2)
      {
        this->Set(b0, z0, b1, z1, b2, z2);
      }

      /// \brief Copy constructor
      template<typename I>
      FieldRegionBase(const FieldRegionBase<I, EFieldMode::Intrpl>& other)
        : fZfirst(utils::simd::Cast<I, T>(other.fZfirst))
      {
        for (size_t iDim = 0; iDim < fCoeff.size(); ++iDim) {
          for (size_t iPow = 0; iPow < fCoeff[iDim].size(); ++iPow) {
            fCoeff[iDim][iPow] = utils::simd::Cast<I, T>(other.fCoeff[iDim][iPow]);
          }
        }
      }

      /// \brief Destructor
      ~FieldRegionBase() = default;

      /// \brief Copy assignment operator
      FieldRegionBase& operator=(const FieldRegionBase& other) = default;

      /// \brief Gets the field value in a spatial point on the track
      /// \param x  x-coordinate of the point [cm]
      /// \param y  y-coordinate of the point [cm]
      /// \param z  z-coordinate of the point [cm]
      FieldValue<T> Get(const T& x, const T& y, const T& z) const;

      /// \brief Gets the double integrals of the field along the track
      std::tuple<T, T, T> GetDoubleIntegrals(const T& x1, const T& y1, const T& z1,  //
                                             const T& x2, const T& y2, const T& z2) const;

      /// \brief Gets the first z-coordinate of the interpolation
      const T& GetZfirst() const { return fZfirst; }

      /// \brief Gets the coefficients of the polynomial approximation
      const auto& GetCoefficients() const { return fCoeff; }

      /// \brief Interpolates the field by three nodal points with the second order polynomial
      /// \note  The interpolation is carried out vs. z-coordinates of the space points of the field values
      /// \param b0  Field value at z0 [kG]
      /// \param z0  First nodal point in z-axis direction [cm]
      /// \param b1  Field value at z1 [kG]
      /// \param z1  Second nodal point in z-axis direction [cm]
      /// \param b2  Field value at z2 [kG]
      /// \param z2  Third nodal point in z-axis direction [cm]
      void Set(const FieldValue<T>& b0, const T& z0, const FieldValue<T>& b1, const T& z1, const FieldValue<T>& b2,
               const T& z2);

      /// \brief Set the coefficients of the polynomial approximation
      void Set(const CoeffArray_t& coeff, const T& z0)
      {
        fCoeff  = coeff;
        fZfirst = z0;
      }

      /// \brief Shifts the coefficients to another first z-coordinate
      /// \param z  new z-coordinate [cm]
      void Shift(const T& z);

      /// Consistency checker
      void CheckConsistency() const;

      /// \brief String representation of the class content
      /// \param intdentLevel  Indent level
      /// \param verbose       Verbosity level
      std::string ToString(int indentLevel = 0, int verbose = 1) const;

     protected:
      CoeffArray_t fCoeff{{T(0.)}};  ///< Approximation coefficients
      T fZfirst{Literal_t<T>()};     ///< Reference z-coordinate of the field region

     private:
      /// Serialization function
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive& ar, const unsigned int)
      {
        ar& fCoeff;
        ar& fZfirst;
      }
    };
  }  // namespace detail


  /// \class  FieldRegion
  /// \brief  Magnetic field region, corresponding to a hit triplet
  /// \tparam T        Underlying data-type
  template<typename T>
  class alignas(VcMemAlign) FieldRegion {
    template<typename>
    friend class FieldRegion;

   public:
    /// \brief  Constructor of the generic field
    FieldRegion(EFieldMode fldMode = EFieldMode::Intrpl, EFieldType fldType = EFieldType::Normal)
      : foFldIntrpl(fldMode == EFieldMode::Intrpl ? std::make_optional<detail::FieldRegionBase<T, EFieldMode::Intrpl>>()
                                                  : std::nullopt)
      , foFldOrig(fldMode == EFieldMode::Orig ? std::make_optional<detail::FieldRegionBase<T, EFieldMode::Orig>>()
                                              : std::nullopt)
      , fFieldType(fldType)
      , fFieldMode(fldMode)
    {
    }

    /// \brief  Copy constructor
    /// \tparam I  Underlying floating point type of the source
    template<typename I>
    FieldRegion(const FieldRegion<I>& other)
      : foFldIntrpl(other.foFldIntrpl.has_value()
                      ? std::make_optional(detail::FieldRegionBase<T, EFieldMode::Intrpl>(*other.foFldIntrpl))
                      : std::nullopt)
      , foFldOrig(other.foFldOrig.has_value()
                    ? std::make_optional(detail::FieldRegionBase<T, EFieldMode::Orig>(*other.foFldOrig))
                    : std::nullopt)
      , fFieldType(other.fFieldType)
      , fFieldMode(other.fFieldMode)
    {
    }

    /// \brief  Constructor for the interpolated field region
    FieldRegion(const FieldValue<T>& b0, const T& z0, const FieldValue<T>& b1, const T& z1, const FieldValue<T>& b2,
                const T& z2)
      : foFldIntrpl(std::make_optional<detail::FieldRegionBase<T, EFieldMode::Intrpl>>())
      , foFldOrig(std::nullopt)
      , fFieldType(EFieldType::Normal)
      , fFieldMode(EFieldMode::Intrpl)
    {
      // Set the field type automatically
      Set(b0, z0, b1, z1, b2, z2);
    }

    /// \brief  Constructor for the interpolated field region
    /// \param  fieldType  Type of the field
    FieldRegion(EFieldType fieldType, const detail::FieldRegionBase<T, EFieldMode::Intrpl>& fld)
      : foFldIntrpl(std::make_optional(fld))
      , foFldOrig(std::nullopt)
      , fFieldType(fieldType)
      , fFieldMode(EFieldMode::Intrpl)
    {
    }

    /// \brief  Constructor for the field region with the original field function
    /// \param  fieldType  Type of the field
    FieldRegion(EFieldType fieldType, const FieldFn_t& fieldFn)
      : foFldIntrpl(std::nullopt)
      , foFldOrig(std::make_optional<detail::FieldRegionBase<T, EFieldMode::Orig>>(fieldFn))
      , fFieldType(fieldType)
      , fFieldMode(EFieldMode::Orig)
    {
    }

    /// \brief Destructor
    ~FieldRegion() = default;

    /// \brief Copy assignment operator
    FieldRegion& operator=(const FieldRegion& other) = default;

    /// \brief Gets the field value in a spatial point on the track
    /// \param x  x-coordinate of the point [cm]
    /// \param y  y-coordinate of the point [cm]
    /// \param z  z-coordinate of the point [cm]
    /// \note  The x and y coordinates are ignored, if the interpolated field is used
    FieldValue<T> Get(const T& x, const T& y, const T& z) const
    {
      return fFieldMode == EFieldMode::Intrpl ? foFldIntrpl->Get(x, y, z) : foFldOrig->Get(x, y, z);
    }

    /// \brief Gets the double integrals of the field along the track
    std::tuple<T, T, T> GetDoubleIntegrals(const T& x1, const T& y1, const T& z1, const T& x2, const T& y2,
                                           const T& z2) const
    {
      return fFieldMode == EFieldMode::Intrpl ? foFldIntrpl->GetDoubleIntegrals(x1, y1, z1, x2, y2, z2)
                                              : foFldOrig->GetDoubleIntegrals(x1, y1, z1, x2, y2, z2);
    }

    /// \brief Gets field mode
    EFieldMode GetFieldMode() const { return fFieldMode; }

    /// \brief Gets the field type
    EFieldType GetFieldType() const { return fFieldType; }

    /// \brief Interpolates the field by three nodal points with the second order polynomial
    /// \note  The interpolation is carried out vs. z-coordinates of the space points of the field values
    /// \param b0  Field value at z0 [kG]
    /// \param z0  First nodal point in z-axis direction [cm]
    /// \param b1  Field value at z1 [kG]
    /// \param z1  Second nodal point in z-axis direction [cm]
    /// \param b2  Field value at z2 [kG]
    /// \param z2  Third nodal point in z-axis direction [cm]
    void Set(const FieldValue<T>& b0, const T& z0, const FieldValue<T>& b1, const T& z1, const FieldValue<T>& b2,
             const T& z2)
    {
      fFieldMode = EFieldMode::Intrpl;
      fFieldType = EFieldType::Normal;
      // Set the field type automatically
      if (b0.IsZero() && b1.IsZero() && b2.IsZero()) {
        fFieldType = EFieldType::Null;
      }
      foFldIntrpl->Set(b0, z0, b1, z1, b2, z2);
    }

    /// \brief Sets the field type
    void SetFieldType(EFieldType fldType) { fFieldType = fldType; }

    /// \brief Shifts the coefficients to another first z-coordinate
    /// \param z  new z-coordinate [cm]
    void Shift(const T& z);

    /// Consistency checker
    void CheckConsistency() const;

    /// \brief String representation of the class content
    /// \param intdentLevel  Indent level
    /// \param verbose       Verbosity level
    std::string ToString(int indentLevel = 0, int verbose = 1) const;

    const auto& GetIntrplField() const { return foFldIntrpl; }

   private:
    friend class boost::serialization::access;

    /// \brief Serialization load method
    template<class Archive>
    void load(Archive& ar, const unsigned int /*version*/)
    {
      auto field = detail::FieldRegionBase<T, EFieldMode::Intrpl>{};
      ar >> field;
      foFldIntrpl = std::make_optional(field);
      ar >> fFieldType;
      ar >> fFieldMode;
      foFldOrig = std::nullopt;  // Note: original field cannot be serialized
    }

    /// \brief Serialization save method
    template<class Archive>
    void save(Archive& ar, const unsigned int /*version*/) const
    {
      if (fFieldMode == EFieldMode::Intrpl) {
        ar << (*foFldIntrpl);
        ar << fFieldType;
        ar << fFieldMode;
      }
      else if (fFieldMode == EFieldMode::Orig) {
        throw std::logic_error("Attempt to serialize a kf::FieldRegion object with fFieldMode == EFieldMode::Orig");
      }
    }

    /// \brief Serialization method
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
      boost::serialization::split_member(ar, *this, version);
    }

    std::optional<detail::FieldRegionBase<T, EFieldMode::Intrpl>> foFldIntrpl{std::nullopt};
    std::optional<detail::FieldRegionBase<T, EFieldMode::Orig>> foFldOrig{std::nullopt};
    EFieldType fFieldType{EFieldType::Null};  ///< Field type in a given region
    EFieldMode fFieldMode;                    ///< Field mode
  };


  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  inline FieldValue<T> GlobalField::GetFieldValue(const FieldFn_t& fieldFn, const T& x, const T& y, const T& z)
  {
    FieldValue<T> B;
    using utils::simd::Cast;
    for (size_t i = 0; i < utils::simd::Size<T>(); ++i) {
      auto [bx, by, bz] = fieldFn(Cast<T, double>(x, i), Cast<T, double>(y, i), Cast<T, double>(z, i));
      B.SetSimdEntry(bx, by, bz, i);
    }
    return B;
  }

}  // namespace cbm::algo::kf
