/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfField.h
/// \brief  Magnetic field representation in KF (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  22.07.2024

#pragma once

#include "KfDefs.h"
#include "KfFieldRegion.h"
#include "KfFieldSlice.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_free.hpp>

#include <array>
#include <optional>
#include <set>

namespace cbm::algo::kf
{
  namespace detail
  {
    /// \class  FieldBase
    /// \brief  Data members of the Field class (primary template)
    /// \tparam T        Underlying floating point type (float/double/fvec)
    /// \tparam FldMode  Field type, defined by EFieldMode
    template<typename T, EFieldMode FldMode>
    class alignas(VcMemAlign) FieldBase {
    };

    /// \class  FieldBase<T, Orig>
    /// \brief  FieldBase implementation for the original field
    /// \tparam T  Underlying floating point type (float/double/fvec)
    template<typename T>
    class alignas(VcMemAlign) FieldBase<T, EFieldMode::Orig> {
      template<typename, EFieldMode>
      friend class FieldBase;

     public:
      /// \brief Default constructor
      FieldBase() = default;

      /// \brief  Copy constructor
      /// \tparam I  Underlying floating type of the source
      template<typename I>
      FieldBase(const FieldBase<I, EFieldMode::Orig>& other) : fFieldFn(other.fFieldFn)
      {
        fvFieldSliceZ.reserve(other.fvFieldSliceZ.size());
        for (const auto& slice : other.fvFieldSliceZ) {
          fvFieldSliceZ.emplace_back(utils::simd::Cast<I, T>(slice));
        }
      }

      /// \brief  Copy assignment operator
      FieldBase& operator=(const FieldBase& other)
      {
        if (this != &other) {
          fvFieldSliceZ = other.fvFieldSliceZ;
          fFieldFn      = other.fFieldFn;
        }
        return *this;
      }

      /// \brief Sets a field slice (z-ref)
      /// \param fieldSliceZref  Reference z-coordinate of a field slice
      void AddFieldSlice(const T& fieldSliceZref) { fvFieldSliceZ.push_back(fieldSliceZref); }

      /// \brief Creates field value object
      /// \param sliceID  Index of slice
      /// \param x        x-coordinate of the point [cm]
      /// \param y        y-coordinate of the point [cm]
      FieldValue<T> GetFieldValue(int sliceID, const T& x, const T& y) const
      {
        return GlobalField::GetFieldValue(fFieldFn, x, y, fvFieldSliceZ[sliceID]);
      }

      /// \brief Gets field function
      const FieldFn_t& GetFieldFunction() const { return fFieldFn; }

      /// \brief Gets number of field slices in the instance
      int GetNofFieldSlices() const { return fvFieldSliceZ.size(); }

      /// \brief Removes a field slice
      /// \param iLayer  Index of field slice
      void RemoveSlice(int iLayer) { fvFieldSliceZ.erase(fvFieldSliceZ.begin() + iLayer); }

      /// \brief Sets magnetic field function
      /// \param fieldFn  Magnetic field function (KF-format)
      void SetFieldFunction(const FieldFn_t& fieldFn) { fFieldFn = fieldFn; }

      /// \brief String representation of the class
      /// \param indentLevel  Indent level of the string output
      /// \param verbose      Verbosity level
      std::string ToString(int indentLevel, int verbose) const;

     private:
      std::vector<T> fvFieldSliceZ{};         ///< z-positions of field slices to emulate functionality
      FieldFn_t fFieldFn{defs::ZeroFieldFn};  ///< Field function: (x,y,z) [cm] -> (Bx,By,Bz) [kG]
    };


    /// \class  FieldBase<T, EFieldMode::Intrpl>
    /// \brief  Data members of the Field class with the interpolation of the magnetic field
    /// \tparam T  Underlying floating point type (float/double/fvec)
    template<typename T>
    class alignas(VcMemAlign) FieldBase<T, EFieldMode::Intrpl> {
      template<typename, EFieldMode>
      friend class FieldBase;

      using SlicesContainer_t = std::vector<FieldSlice<T>>;

     public:
      /// \brief Default constructor
      FieldBase() = default;

      /// \brief  Copy constructor
      /// \tparam I  Underlying floating type of the source
      template<typename I>
      FieldBase(const FieldBase<I, EFieldMode::Intrpl>& other)
      {
        fvFieldSlices.reserve(other.fvFieldSlices.size());
        for (const auto& slice : other.fvFieldSlices) {
          fvFieldSlices.emplace_back(FieldSlice<T>(slice));
        }
      }

      /// \brief  Copy assignment operator
      FieldBase& operator=(const FieldBase& other)
      {
        if (this != &other) {
          fvFieldSlices = other.fvFieldSlices;
        }
        return *this;
      }

      /// \brief Sets a field slice
      /// \param fieldSlice  Field slice object
      void AddFieldSlice(const FieldSlice<T>& fieldSlice) { fvFieldSlices.push_back(fieldSlice); }

      /// \brief Gets field slice
      /// \param sliceID  Index of slice
      const auto& GetFieldSlice(int sliceID) const { return fvFieldSlices[sliceID]; }

      /// \brief Creates field value object
      /// \param sliceID  Index of slice
      /// \param x        x-coordinate of the point [cm]
      /// \param y        y-coordinate of the point [cm]
      FieldValue<T> GetFieldValue(int sliceID, const T& x, const T& y) const
      {
        return fvFieldSlices[sliceID].GetFieldValue(x, y);
      }

      /// \brief Gets number of field slices in the instance
      int GetNofFieldSlices() const { return fvFieldSlices.size(); }

      /// \brief Removes a field slice
      /// \param iLayer  Index of field slice
      void RemoveSlice(int iLayer) { fvFieldSlices.erase(fvFieldSlices.begin() + iLayer); }

      /// \brief String representation of the class
      /// \param indentLevel  Indent level of the string output
      /// \param verbose      Verbosity level
      std::string ToString(int indentLevel, int verbose) const;

     protected:
      SlicesContainer_t fvFieldSlices{};  ///< Array of field slices

     private:
      /// \brief Serialization function
      friend class boost::serialization::access;
      template<class Archive>
      void serialize(Archive& ar, const unsigned int)
      {
        ar& fvFieldSlices;
      }
    };
  }  // namespace detail


  /// \class  Field
  /// \brief  Magnetic field manager class
  /// \tparam T  Underlying floating point type (float/double/fvec)
  template<typename T>
  class alignas(VcMemAlign) Field {
    template<typename>
    friend class Field;
    friend class FieldFactory;
    friend class boost::serialization::access;

   public:
    /// \brief Constructor
    /// \param fldMode  Field mode
    Field(EFieldMode fldMode, EFieldType fldType)
      : foFldIntrpl(fldMode == EFieldMode::Intrpl ? std::make_optional(detail::FieldBase<T, EFieldMode::Intrpl>())
                                                  : std::nullopt)
      , foFldOrig(fldMode == EFieldMode::Orig ? std::make_optional(detail::FieldBase<T, EFieldMode::Orig>())
                                              : std::nullopt)
      , fPrimVertexField(FieldRegion<T>(fldMode, fldType))
      , fFieldType(fldType)
      , fFieldMode(fldMode)
    {
    }

    /// \brief Copy constructor
    template<typename I>
    Field(const Field<I>& other)
      : foFldIntrpl(other.foFldIntrpl.has_value()
                      ? std::make_optional(detail::FieldBase<T, EFieldMode::Intrpl>(*other.foFldIntrpl))
                      : std::nullopt)
      , foFldOrig(other.foFldOrig.has_value()
                    ? std::make_optional(detail::FieldBase<T, EFieldMode::Orig>(*other.foFldOrig))
                    : std::nullopt)
      , fPrimVertexField(FieldRegion<I>(other.fPrimVertexField))
      , fFieldType(other.fFieldType)
      , fFieldMode(other.fFieldMode)
    {
    }

    /// \brief Destructor
    ~Field() = default;

    /// \brief Copy assignment operator
    Field& operator=(const Field& other)
    {
      if (this != &other) {
        fPrimVertexField = other.fPrimVertexField;
        foFldIntrpl      = other.foFldIntrpl;
        foFldOrig        = other.foFldOrig;
        fFieldType       = other.fFieldType;
        fFieldMode       = other.fFieldMode;
      }
      return *this;
    }

    /// \brief Creates field value object
    /// \param sliceID  Index of slice
    /// \param x        x-coordinate of the point [cm]
    /// \param y        y-coordinate of the point [cm]
    FieldValue<T> GetFieldValue(int sliceID, const T& x, const T& y) const
    {
      return fFieldMode == EFieldMode::Intrpl ? foFldIntrpl->GetFieldValue(sliceID, x, y)
                                              : foFldOrig->GetFieldValue(sliceID, x, y);
    }

    /// \brief Gets field type
    EFieldType GetFieldType() const { return this->fFieldType; }

    /// \brief Gets field region near primary vertex
    const FieldRegion<T>& GetPrimVertexField() const { return fPrimVertexField; }

    /// \brief Makes field region object
    /// \param b0  Field value in the first node [kG]
    /// \param z0  First node z-coordinate [cm]
    /// \param b1  Field value in the first node [kG]
    /// \param z1  Second node z-coordinate [cm]
    /// \param b2  Field value in the first node [kG]
    /// \param z2  Third node z-coordinate [cm]
    /// \note  Parameters b0-b2, z0-z2 are ignored, if fFieldMode == EFieldMode::Orig
    FieldRegion<T> GetFieldRegion(const FieldValue<T>& b0, const T& z0, const FieldValue<T>& b1, const T& z1,
                                  const FieldValue<T>& b2, const T& z2) const
    {
      return (fFieldMode == EFieldMode::Intrpl ? FieldRegion<T>(b0, z0, b1, z1, b2, z2)
                                               : FieldRegion<T>(fFieldType, foFldOrig->GetFieldFunction()));
    }

    /// \brief Removes a field slice
    /// \param iLayer  Index of field slice
    void RemoveSlice(int iLayer);

    /// \brief String representation of the class
    /// \param indentLevel  Indent level of the string output
    /// \param verbose      Verbosity level
    std::string ToString(int indentLevel, int verbose) const;

   private:
    /// \brief Default constructor
    Field() = default;

    /// \brief Serialization load method
    template<class Archive>
    void load(Archive& ar, const unsigned int /*version*/)
    {
      auto field = detail::FieldBase<T, EFieldMode::Intrpl>{};
      ar >> field;
      foFldIntrpl = std::make_optional(field);
      ar >> fPrimVertexField;
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
        ar << fPrimVertexField;
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

    std::optional<detail::FieldBase<T, EFieldMode::Intrpl>> foFldIntrpl{std::nullopt};  ///< Interpolated field
    std::optional<detail::FieldBase<T, EFieldMode::Orig>> foFldOrig{std::nullopt};      ///< Original field
    FieldRegion<T> fPrimVertexField;            ///< Field region near primary vertex (constant field is assumed)
    EFieldType fFieldType{EFieldType::Normal};  ///< Field type
    EFieldMode fFieldMode;                      ///< Field mode
  };


  /// \class FieldFactory
  /// \brief A helper class to instantiate a Field object
  class FieldFactory {
    /// \struct SliceRef
    /// \brief  A helper structure for field slices initialization
    struct SliceRef {
      double fHalfSizeX{defs::Undef<double>};  ///< Half-size of the slice in x-direction [cm]
      double fHalfSizeY{defs::Undef<double>};  ///< Half-size of the slice in y-direction [cm]
      double fRefZ{defs::Undef<double>};       ///< Reference z-position of the slice [cm]

      /// \brief Constructor
      /// \param halfX  Half-size of the slice in x-direction [cm]
      /// \param halfY  Half-size of the slice in y-direction [cm]
      /// \param refZ   Reference z-position of the slice [cm]
      SliceRef(double halfX, double halfY, double refZ) : fHalfSizeX(halfX), fHalfSizeY(halfY), fRefZ(refZ) {}

      /// \brief Comparision operator
      bool operator<(const SliceRef& r) const { return fRefZ < r.fRefZ; }
    };

   public:
    /// \brief Default constructor
    FieldFactory() = default;

    /// \brief Copy constructor
    FieldFactory(const FieldFactory&) = default;

    /// \brief Destructor
    ~FieldFactory() = default;

    /// \brief Copy assignment operator
    FieldFactory& operator=(const FieldFactory&) = default;

    /// \brief Adds a slice reference
    /// \param halfSizeX  Half-size of the slice in x-direction [cm]
    /// \param halfSizeY  Half-size of the slice in y-direction [cm]
    /// \param refZ       Reference z-position of the slice [cm]
    void AddSliceReference(double halfSizeX, double halfSizeY, double refZ);

    /// \brief Gets field mode
    EFieldMode GetFieldMode() const { return fFieldMode; }

    /// \brief Gets field type
    EFieldType GetFieldType() const { return fFieldType; }

    /// \brief  Create field
    /// \tparam T        Underlying floating-point data type
    template<typename T>
    Field<T> MakeField() const;

    /// \brief Resets the instance
    void Reset() { *this = FieldFactory(); }

    /// \brief Resets slicer references
    void ResetSliceReferences() { fSliceReferences.clear(); }

    /// \brief Sets magnetic field function
    /// \param fieldFn  Magnetic field function (KF-format)
    void SetFieldFunction(const FieldFn_t& fieldFn, EFieldType fldType)
    {
      fFieldFn   = fieldFn;
      fFieldType = fldType;
    }

    /// \brief Sets field mode
    void SetFieldMode(EFieldMode fldMode) { fFieldMode = fldMode; }

    /// \brief Sets a step for the primary vertex field region estimation
    /// \param step  A step between nodal points in z-axis direction [cm]
    void SetStep(double step = 2.5) { fTargetStep = step; }

    /// \brief Sets target
    /// \param x  x-coordinate of the target position [cm]
    /// \param y  y-coordinate of the target position [cm]
    /// \param z  z-coordinate of the target position [cm]
    void SetTarget(double x, double y, double z) { fTarget = {x, y, z}; }

   private:
    std::set<SliceRef> fSliceReferences;      ///< Set of slice references
    FieldFn_t fFieldFn{defs::ZeroFieldFn};    ///< Field function (x, y, z) [cm] -> (Bx, By, Bz) [kG]
    double fTargetStep{2.5};                  ///< Step between nodal points for the primary vertex field estimation
    EFieldType fFieldType{EFieldType::Null};  ///< Field type
    EFieldMode fFieldMode{EFieldMode::Intrpl};  ///< FieldMode

    /// \brief Target position
    std::array<double, 3> fTarget{{defs::Undef<double>, defs::Undef<double>, defs::Undef<double>}};
  };

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<typename T>
  Field<T> FieldFactory::MakeField() const
  {
    auto field = Field<T>(fFieldMode, fFieldType);

    // Check initialization
    if (std::any_of(fTarget.begin(), fTarget.end(), [](double x) { return !utils::IsFinite(x); })) {
      throw std::logic_error("FieldFactory::MakeField: target is undefined");
    }
    if (!fSliceReferences.size()) {  // TODO: Remove requirement of slice references
      throw std::logic_error("FieldFactory::MakeField: no slice references were provided");
    }
    if (!fFieldFn) {
      throw std::logic_error("FieldFactory::CreateField: no field function is provided");
    }

    // Initialize the Field object
    if (fFieldMode == EFieldMode::Orig) {
      field.foFldOrig->SetFieldFunction(fFieldFn);
      field.fPrimVertexField = FieldRegion<T>(fFieldType, fFieldFn);
      for (const auto& sliceRef : fSliceReferences) {
        field.foFldOrig->AddFieldSlice(sliceRef.fRefZ);
      }
    }
    else if (fFieldMode == EFieldMode::Intrpl) {  // A version with the interpolated field
      for (const auto& sliceRef : fSliceReferences) {
        field.foFldIntrpl->AddFieldSlice(
          FieldSlice<T>(fFieldFn, sliceRef.fHalfSizeX, sliceRef.fHalfSizeY, sliceRef.fRefZ));
      }
      {
        // PV field initialization
        constexpr size_t nNodes = 3;
        double z                = fTarget[2];
        std::array<FieldValue<T>, nNodes> fld;
        std::array<T, nNodes> pos;
        for (size_t iNode = 0; iNode < nNodes; ++iNode) {
          pos[iNode] = utils::simd::Cast<double, T>(z);
          fld[iNode] = GlobalField::GetFieldValue<double>(fFieldFn, fTarget[0], fTarget[1], z);
          z += fTargetStep;
        }
        field.fPrimVertexField = FieldRegion<T>(fld[0], pos[0], fld[1], pos[1], fld[2], pos[2]);
        // TODO: Provide alternative method for Orig
      }
    }
    return field;
  }
}  // namespace cbm::algo::kf
