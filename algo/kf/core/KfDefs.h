/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfDefs.h
/// @brief  Common constant definitions for the Kalman Filter library
/// @since  28.03.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#ifndef KF_CORE_KfDefs_h
#define KF_CORE_KfDefs_h 1

#include "KfSimd.h"

#include <array>
#include <functional>
#include <limits>
#include <tuple>

namespace cbm::algo::kf
{
  // ---- Enumerations -------------------------------------------------------------------------------------------------
  //
  /// \enum  EFieldMode
  /// \brief Enumiration for the magnetic field representation variants in the track fitting algorithm
  enum class EFieldMode
  {
    Intrpl,  ///< Interpolated magnetic field
    Orig     ///< Original magnetic field function
  };

  /// \enum  EFieldType
  /// \brief Magnetic field type in different setup regions
  // TODO: SZh 21.08.2024: Develope the concept of field types at different setup regions. At the moment the only
  //                       two global options are available.
  enum class EFieldType
  {
    Normal,  ///< Field near the tracker subsystem
    Null     ///< No magnetic field
  };

  /// \struct Literal
  /// \brief  Replaces the type T with the Literal::type to handle the constant expressions for different constants
  template<typename T>
  struct Literal {
    using type = T;
  };

  template<>
  struct Literal<fvec> {
    using type = fscal;
  };

  template<typename T>
  using Literal_t = typename Literal<T>::type;


  // ---- Common data types --------------------------------------------------------------------------------------------
  //
  /// \brief Geometry (spatial) vector
  template<class T>
  using GeoVector_t = std::array<T, 3>;

  /// \brief Magnetic field function type
  /// Signature: tuple<Bx, By, Bz>(x, y, z);
  using FieldFn_t = std::function<std::tuple<double, double, double>(double, double, double)>;
}  // namespace cbm::algo::kf

namespace cbm::algo::kf::defs
{
  // ----- Array sizes -------------------------------------------------------------------------------------------------
  //constexpr int MaxNofFieldSlices    = 64;  ///< Max number of field slices
  //constexpr int MaxNofMaterialLayers = 32;  ///< Max number of the material layers
  // NOTE: max uint8_t is assumed in order to satisfy fles::Subsystem
  constexpr int MaxNofDetSubsystems = 256;  ///< Max number of detector types (STS, TRD, RICH,...)
  constexpr int MaxNofDetComponents = 128;  ///< Max number of detector components (stations, layers, ...)

  // ----- Control -----------------------------------------------------------------------------------------------------
  constexpr int DebugLvl     = 0;     ///< Level of debug output
  constexpr bool GetterCheck = true;  ///< Bound check in getters


  // ----- Math --------------------------------------------------------------------------------------------------------
  template<class T = double>
  constexpr auto Pi = Literal_t<T>(3.14159265358979323846L);  ///< Value of PI, used in ROOT TMath

  // ----- Physics -----------------------------------------------------------------------------------------------------
  template<class T = double>
  constexpr auto MuonMass = Literal_t<T>(0.105658375523L);  ///< Muon mass [GeV/c2]

  template<class T = double>
  constexpr auto PionMass = Literal_t<T>(0.1395703918L);  ///< Pion mass [GeV/c2]

  template<class T = double>
  constexpr auto KaonMass = Literal_t<T>(0.493677L);  ///< Kaon mass [GeV/c2] (PDG 22.08.2023)

  template<class T = double>
  constexpr auto ElectronMass = Literal_t<T>(0.0005109989500015L);  ///< Electron mass [GeV/c2]

  template<class T = double>
  constexpr auto ProtonMass = Literal_t<T>(0.93827208816L);  ///< Proton mass [GeV/c2] (PDG 11.08.2022)

  template<class T = double>
  constexpr auto SpeedOfLight = Literal_t<T>(29.9792458L);  ///< Speed of light [cm/ns]

  template<class T = double>
  constexpr auto SpeedOfLightInv = Literal_t<T>(1.L) / SpeedOfLight<T>;  ///< Inverse speed of light [ns/cm]

  template<class T = double>
  constexpr auto MinField = Literal_t<T>(1.e-4L);  ///< Minimal (negligible) magnetic field value [kG]

  // ----- Undefined values --------------------------------------------------------------------------------------------
  /// \brief Undefined values
  template<typename T1, typename T2 = T1>
  constexpr T2 Undef;

  template<>
  inline constexpr int Undef<int> = std::numeric_limits<int>::min();

  template<>
  inline constexpr unsigned Undef<unsigned> = std::numeric_limits<unsigned>::max();

  template<>
  inline constexpr float Undef<float> = std::numeric_limits<float>::signaling_NaN();

  template<>
  inline constexpr double Undef<double> = std::numeric_limits<double>::signaling_NaN();

  template<>
  inline constexpr fscal Undef<fvec> = std::numeric_limits<fscal>::signaling_NaN();

  // ----- Other -------------------------------------------------------------------------------------------------------
  /// \brief Zero magnetic field function
  constexpr auto ZeroFieldFn = [](double, double, double) constexpr { return std::make_tuple(0., 0., 0.); };

  // ----- Debug flags (NOTE: to be modified only by the experts) ------------------------------------------------------
  //
  namespace dbg
  {
  }  // namespace dbg
}  // namespace cbm::algo::kf::defs

#endif  // KF_CORE_KfDefs_h
