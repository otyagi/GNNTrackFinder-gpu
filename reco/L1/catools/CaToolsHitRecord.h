/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaHitRecord.h
/// \brief  A header for cbm::ca::HitRecord structure
/// \since  15.05.2023
/// \author S.Zharko <s.zharko@gsi.de>

#ifndef CbmCaToolsHitRecord_h
#define CbmCaToolsHitRecord_h 1

#include <cstdint>
#include <string>

namespace cbm::ca::tools
{
  /// @brief A helper structure to store hits information from different detectors in a uniform manner
  ///
  /// @note  This structure is to be used ONLY at the TS/event initialization stage to fill hit arrays of different
  ///        types.
  ///
  struct HitRecord {
    double fX           = -1.;  ///< x component of hit position [cm]
    double fY           = -1.;  ///< y component of hit position [cm]
    double fZ           = -1.;  ///< z component of hit position [cm]
    double fT           = -1.;  ///< time of hit [ns]
    double fDx2         = -1.;  ///< error of x component of hit position [cm]
    double fDy2         = -1.;  ///< error of y component of hit position [cm]
    double fDxy         = -1.;  ///< correlation between x and y components [cm]
    double fDt2         = -1.;  ///< time error of hit [ns]
    double fRangeX      = -1.;  ///< range of x [cm]
    double fRangeY      = -1.;  ///< range of y [cm]
    double fRangeT      = -1.;  ///< range of t [ns]
    int64_t fDataStream = -1;   ///< Global index of detector module
    int fPointId        = -1;   ///< index of MC point
    int fExtId          = -2;   ///< external index of hit
    int fStaId          = -2;   ///< index of active tracking station
    int fStripF         = -2;   ///< index of front strip
    int fStripB         = -2;   ///< index of back strip
    int fDet            = -2;   ///< detector ID

    /// @brief  Tests hit quality (for example, if all the quantities are not nan)
    /// @return true   Hit is accepted
    /// @return false  Hit is discarded
    static constexpr bool kVerboseAccept = true;
    bool Accept() const;

    /// @brief  Converts hit record to a string
    std::string ToString() const;
  };
}  // namespace cbm::ca::tools

#endif  // CbmCaToolsHitRecord_h
