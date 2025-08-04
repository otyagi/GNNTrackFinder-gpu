/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmL1DetectorID.h TODO: Rename to CbmCaDefs.h !
/// @brief  Implementation of L1DetectorID enum class for CBM
/// @author S.Zharko
/// @since  01.12.2022

#pragma once

//#include "CaDefs.h"
#include "CbmDefs.h"
#include "CbmEnumArray.h"

#include <string>

/// *************************************************
/// **    Detector-dependent common definitions    **
/// *************************************************

class CbmMvdPoint;
class CbmStsPoint;
class CbmMuchPoint;
class CbmTrdPoint;
class CbmTofPoint;

class CbmMvdHit;
class CbmStsHit;
class CbmMuchPixelHit;
class CbmTrdHit;
class CbmTofHit;

namespace cbm::ca
{
  namespace ca = cbm::algo::ca;

  /// @brief  Alias to array, indexed by L1DetectorID enum
  /// @note   To be used only in CBM-specific code
  template<typename T>
  using DetIdArr_t = cbm::core::EnumArray<ca::EDetectorID, T>;

  /// @brief  List of

  /// @struct DetIdTypeArr_t
  /// @brief  Array of types, indexed by L1DetectorID enum
  ///
  /// The array of types allows to treat different types of detector in a uniform manner
  /// Example:
  ///   using HitTypes_t = DetIdTypeArr_t<CbmMvdHit, CbmStsHit, CbmMuchPixelHit, CbmTrdHit, CbmTofHit>;
  ///   ...
  ///   HitTypes_t::at<L1DetectorID::kSts> hit;  // Sts hit
  template<class... Types>
  struct DetIdTypeArr_t {
    template<ca::EDetectorID DetID>
    using at                          = std::tuple_element_t<static_cast<std::size_t>(DetID), std::tuple<Types...>>;
    static constexpr std::size_t size = sizeof...(Types);
  };

  /// @brief Names of detector subsystems
  /// @note  These names are used for data branches IO, thus any modification can lead to the
  ///        read-out corruption.
  constexpr DetIdArr_t<const char*> kDetName = {{"MVD", "STS", "MUCH", "TRD", "TOF"}};

  /// @brief Name of hit branches for each detector
  constexpr DetIdArr_t<const char*> kDetHitBrName = {{"MvdHit", "StsHit", "MuchPixelHit", "TrdHit", "TofHit"}};

  /// @brief Name of point branches for each detector
  constexpr DetIdArr_t<const char*> kDetPointBrName = {{"MvdPoint", "StsPoint", "MuchPoint", "TrdPoint", "TofPoint"}};

  /// @brief Data type of hits (for CbmEvent)
  /* clang-format off */
  constexpr DetIdArr_t<ECbmDataType> kCbmHitType = 
  {{
    ECbmDataType::kMvdHit, 
    ECbmDataType::kStsHit, 
    ECbmDataType::kMuchPixelHit, 
    ECbmDataType::kTrdHit, 
    ECbmDataType::kTofHit
  }};
  /* clang-format on */

  /// @brief Conversion map from ca::EDetectorID to ECbmModuleId
  constexpr DetIdArr_t<ECbmModuleId> kCbmModuleId = {
    {ECbmModuleId::kMvd, ECbmModuleId::kSts, ECbmModuleId::kMuch, ECbmModuleId::kTrd, ECbmModuleId::kTof}};

  /// @brief Name

  /// @brief Types of MC point objects for each detector
  using PointTypes_t = DetIdTypeArr_t<CbmMvdPoint, CbmStsPoint, CbmMuchPoint, CbmTrdPoint, CbmTofPoint>;

  /// @brief Types of hit objects for each detector
  using HitTypes_t = DetIdTypeArr_t<CbmMvdHit, CbmStsHit, CbmMuchPixelHit, CbmTrdHit, CbmTofHit>;

}  // namespace cbm::ca

/// @brief Enumeration for different tracking running modes
enum class ECbmCaTrackingMode
{
  kSTS,  ///< Local tracking in CBM (STS + MVD), results stored to the StsTrack branch
  kMCBM  ///< Global tracking in mCBM (STS, MuCh, TRD, TOF), results stored to GlobalTrack branch
};
