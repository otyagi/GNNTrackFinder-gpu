/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfTarget.h
/// @brief  A target layer in the KF-setup (header)
/// @since  18.07.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "KfDefs.h"
#include "KfMaterialMap.h"

#include <boost/serialization/access.hpp>

#include <string>

namespace cbm::algo::kf
{
  /// \class  Target
  /// \brief  A geometry layer in the target region
  /// \tparam T  Underlying floating-point type
  template<typename T>
  class alignas(VcMemAlign) Target {
    template<typename I>
    friend class Target;

   public:
    /// \brief Default constructor
    Target() = default;

    /// \brief Constructor from parameters
    /// \param x         x-coordinate of the nominal target center [cm]
    /// \param y         y-coordinate of the nominal target center [cm]
    /// \param z         z-coordinate of the nominal target center [cm]
    /// \param dz        half-thickness of the target [cm]
    /// \param r         size of target
    Target(double x, double y, double z, double dz, double r);

    /// \brief Copy constructor
    /// \tparam I  Underlying floating-point type of the source
    template<typename I>
    Target(const Target<I>& other)
      : fMaterial(other.fMaterial)
      , fX(utils::simd::Cast<I, T>(other.fX))
      , fY(utils::simd::Cast<I, T>(other.fY))
      , fZ(utils::simd::Cast<I, T>(other.fZ))
      , fDz(utils::simd::Cast<I, T>(other.fDz))
      , fR(utils::simd::Cast<I, T>(other.fR))
    {
    }

    /// \brief Destructor
    ~Target() = default;

    /// \brief  Copy assignment operator
    Target& operator=(const Target& other) = default;

    /// \brief Gets x-coordinate of the nominal target center
    const T& GetX() const { return fX; }

    /// \brief Gets y-coordinate of the nominal target center
    const T& GetY() const { return fY; }

    /// \brief Gets z-coordinate of the nominal target center
    const T& GetZ() const { return fZ; }

    /// \brief Gets target half-thickness
    const T& GetDz() const { return fDz; }

    /// \brief Gets transverse size of target
    const T& GetR() const { return fR; }

    /// \brief Gets material map
    const MaterialMap& GetMaterial() const { return fMaterial; }

    /// \brief Sets material map
    /// \param material  Material map
    void SetMaterial(const MaterialMap& material);

    /// \brief Sets material map (move semantics)
    /// \param material  Material map
    void SetMaterial(MaterialMap&& material);

    /// \brief Sets x-coordinate of the nominal target center
    /// \param x  x-coordinate [cm]
    void SetX(const T& x) { fX = x; }

    /// \brief Sets y-coordinate of the nominal target center
    /// \param y  y-coordinate [cm]
    void SetY(const T& y) { fY = y; }

    /// \brief Sets x-coordinate of the nominal target center
    /// \param z  x-coordinate [cm]
    void SetZ(const T& z) { fZ = z; }

    /// \brief Sets target half-thickness
    /// \param dz  half-thickness [cm]
    void SetDz(const T& dz) { fDz = dz; }

    /// \brief Sets target transverse size
    /// \brief Transverse size of target [cm]
    void SetR(const T& r) { fR = r; }

    /// \brief String representation of the class
    /// \param indentLevel  Indent level of the string output
    /// \param verbose      Verbosity level
    std::string ToString(int indentLevel = 0, int vebose = 1) const;

   private:
    /// \brief Serialization method
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fMaterial;
      ar& fX;
      ar& fY;
      ar& fZ;
      ar& fDz;
      ar& fR;
    }

    MaterialMap fMaterial{};  ///< Material map in the target region
    T fX{defs::Undef<T>};     ///< x-coordinate of the nominal target center [cm]
    T fY{defs::Undef<T>};     ///< y-coordinate of the nominal target center [cm]
    T fZ{defs::Undef<T>};     ///< z-coordinate of the nominal target center [cm]
    T fDz{defs::Undef<T>};    ///< Half-thickness of the target [cm]
    T fR{defs::Undef<T>};     ///< Half-thickness of the target [cm]
  };
}  // namespace cbm::algo::kf
