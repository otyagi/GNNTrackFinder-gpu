/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaHitQaData.h
/// @date   01.09.2023
/// @brief  A helper class to store hit and MC-point parameter and calculate related quantities (header)
/// @author S.Zharko <s.zharko@gsi.de>

#ifndef CbmCaHitQaData_h
#define CbmCaHitQaData_h 1

#include <cmath>
#include <limits>
#include <tuple>

namespace cbm::ca
{
  /// @class HitQaData
  /// @brief Contains necessary data to calculate hit residuals and pulls
  class HitQaData {
   public:
    /// @brief Default constructor
    HitQaData() = default;

    /// @brief Destructor
    ~HitQaData() = default;

    /// @brief Copy constructor
    HitQaData(const HitQaData&) = default;

    /// @brief Move constructor
    HitQaData(HitQaData&&) = default;

    /// @brief Copy assignment operator
    HitQaData& operator=(const HitQaData&) = default;

    /// @brief Move assignment operator
    HitQaData& operator=(HitQaData&&) = default;

    /// @brief  Gets hit u-coordinate error
    /// @return Hit u-coordinate error [cm]
    double GetHitDu() const
    {
      auto cU = cos(fPhiU);
      auto sU = sin(fPhiU);
      return sqrt(cU * cU * GetHitDx() * GetHitDx() + 2. * sU * cU * GetHitDxy() + sU * sU * GetHitDy() * GetHitDy());
    }

    /// @brief  Gets hit v-coordinate error
    /// @return Hit v-coordinate error [cm]
    double GetHitDv() const
    {
      auto cV = cos(fPhiV);
      auto sV = sin(fPhiV);
      return sqrt(cV * cV * GetHitDx() * GetHitDx() + 2. * sV * cV * GetHitDxy() + sV * sV * GetHitDy() * GetHitDy());
    }

    /// @brief  Gets hit u- and v-coordinate covariance
    /// @return Hit u- and v-coordinate covariance [cm2]
    double GetHitDuv() const
    {
      auto cU = cos(fPhiU);
      auto sU = sin(fPhiU);
      auto cV = cos(fPhiV);
      auto sV = sin(fPhiV);
      return GetHitDx() * GetHitDx() * cU * cV + GetHitDxy() * (sU * cV + cU * sV) + GetHitDy() * GetHitDy() * sU * sV;
    }

    /// @brief  Gets hit x-coordinate error
    /// @return Hit x-coordinate error [cm]
    double GetHitDx() const { return fHitDx; }

    /// @brief  Gets hit x- and y-coordinate covariance
    /// @return Hit x- and y-coordinate covariance [cm2]
    double GetHitDxy() const { return fHitDxy; }

    /// @brief  Gets hit y-coordinate error
    /// @return Hit y-coordinate error [cm]
    double GetHitDy() const { return fHitDy; }

    /// @brief  Gets hit index
    /// @return Hit index
    int GetHitIndex() const { return fHitID; }

    /// @brief  Gets Pearson correlation coefficient for u- and v-coordinates
    /// @return Pearson correlation coefficient for u- and v-coordinates
    double GetHitRuv() const { return GetHitDuv() / GetHitDu() / GetHitDv(); }

    /// @brief  Gets hit time
    /// @return hit time [ns]
    double GetHitTime() const { return fHitTime; }

    /// @brief  Gets hit time error
    /// @return Hit time error [ns]
    double GetHitTimeError() const { return fHitTimeError; }

    /// @brief  Gets hit u-coordinate
    /// @return hit u-coordinate [cm]
    double GetHitU() const { return GetHitX() * cos(fPhiU) + GetHitY() * sin(fPhiU); }

    /// @brief  Gets hit v-coordinate
    /// @return hit v-coordinate [cm]
    double GetHitV() const { return GetHitX() * cos(fPhiV) + GetHitY() * sin(fPhiV); }

    /// @brief  Gets hit x-coordinate
    /// @return hit x-coordinate [cm]
    double GetHitX() const { return fHitX; }

    /// @brief  Gets hit y-coordinate
    /// @return hit y-coordinate [cm]
    double GetHitY() const { return fHitY; }

    /// @brief  Gets hit z-coordinate
    /// @return hit z-coordinate [cm]
    double GetHitZ() const { return fHitZ; }

    /// @brief  Gets Flag: if track has hits
    /// @return Flag: if track has hits
    bool GetIfTrackHasHits() const { return fbTrackHasHits; }

    /// @brief  Gets Flag: if track selected
    /// @return Flag: if track selected
    bool GetIfTrackSelected() const { return fbTrackSelected; }

    /// @brief  Gets residual of time
    /// @return Residual of time [ns]
    double GetResidualTime() const { return GetHitTime() - GetPointTime(); }

    /// @brief  Gets residual of u-coordinate
    /// @return Residual of u-coordinate [cm]
    double GetResidualU() const { return GetHitU() - GetPointU(); }

    /// @brief  Gets residual of v-coordinate
    /// @return Residual of v-coordinate [cm]
    double GetResidualV() const { return GetHitV() - GetPointV(); }

    /// @brief  Gets residual of x-coordinate
    /// @return Residual of x-coordinate [cm]
    double GetResidualX() const { return GetHitX() - GetPointX(); }

    /// @brief  Gets residual of y-coordinate
    /// @return Residual of y-coordinate [cm]
    double GetResidualY() const { return GetHitY() - GetPointY(); }

    /// @brief  Gets front strips stereo angle
    /// @return Front strips stereo angle [rad]
    double GetPhiU() const { return fPhiU; }

    /// @brief  Gets back strips stereo angle
    /// @return Back strips stereo angle [rad]
    double GetPhiV() const { return fPhiV; }

    /// @brief  Gets point index
    /// @return A tuple (pointID, eventID, fileID)
    std::tuple<int, int, int> GetPointID() const { return std::make_tuple(fPointID, fMCEventID, fMCFileID); }

    /// @brief  Gets point time
    /// @return Point time [ns]
    double GetPointTime() const { return fPointTime; }

    /// @brief  Gets point u-coordinate
    /// @return Point u-coordinate [cm]
    double GetPointU() const { return GetPointX() * cos(fPhiU) + GetPointY() * sin(fPhiU); }

    /// @brief  Gets point v-coordinate
    /// @return Point v-coordinate [cm]
    double GetPointV() const { return GetPointX() * cos(fPhiV) + GetPointY() * sin(fPhiV); }

    /// @brief  Gets point x-coordinate
    /// @return Point x-coordinate [cm]
    double GetPointX() const { return fPointX; }

    /// @brief  Gets point y-coordinate
    /// @return Point y-coordinate [cm]
    double GetPointY() const { return fPointY; }

    /// @brief Gets point z-coordinate
    /// @param pointZ Point z-coordinate [cm]
    double GetPointZ() const { return fPointZ; }

    /// @brief  Gets pull of time
    /// @return Pull of time
    double GetPullTime() const { return GetResidualTime() / GetHitTimeError(); }

    /// @brief  Gets pull of u-coordinate
    /// @return Pull of u-coordinate
    double GetPullU() const { return GetResidualU() / GetHitDu(); }

    /// @brief  Gets pull of v-coordinate
    /// @return Pull of v-coordinate
    double GetPullV() const { return GetResidualV() / GetHitDv(); }

    /// @brief  Gets pull of x-coordinate
    /// @return Pull of x-coordinate
    double GetPullX() const { return GetResidualX() / GetHitDx(); }

    /// @brief  Gets pull of y-coordinate
    /// @return Pull of y-coordinate
    double GetPullY() const { return GetResidualY() / GetHitDy(); }

    /// @brief  Gets station local index
    /// @return Station local index
    int GetStationID() const { return fStationID; }

    /// @brief Resets data fields
    void Reset() { this->operator=(HitQaData()); }

    /// @brief Sets hit x-coordinate error
    /// @param hitDx Hit x-coordinate error [cm]
    void SetHitDx(double hitDx) { fHitDx = hitDx; }

    /// @brief Sets hit x- and y-coordinate covariance
    /// @param hitDxy Hit x- and y-coordinate covariance [cm2]
    void SetHitDxy(double hitDxy) { fHitDxy = hitDxy; }

    /// @brief Sets hit y-coordinate error
    /// @param hitDy Hit y-coordinate error [cm]
    void SetHitDy(double hitDy) { fHitDy = hitDy; }

    /// @brief Sets hit index
    /// @param hitID  Hit index
    void SetHitIndex(int hitID) { fHitID = hitID; }

    /// @brief Sets hit time
    /// @param hitTime Hit time [ns]
    void SetHitTime(double hitTime) { fHitTime = hitTime; }

    /// @brief Sets hit time error
    /// @param hitTimeError Hit time error [ns]
    void SetHitTimeError(double hitTimeError) { fHitTimeError = hitTimeError; }

    /// @brief Sets hit x-coordinate
    /// @param hitX Hit x-coordinate [cm]
    void SetHitX(double hitX) { fHitX = hitX; }

    /// @brief Sets hit y-coordinate
    /// @param hitY Hit y-coordinate [cm]
    void SetHitY(double hitY) { fHitY = hitY; }

    /// @brief Sets hit z-coordinate
    /// @param hitZ Hit z-coordinate [cm]
    void SetHitZ(double hitZ) { fHitZ = hitZ; }

    /// @brief Sets Flag: if track has hits
    /// @param ifTrackHasHits  Flag: if track has hits
    void SetIfTrackHasHits(bool ifTrackHasHits) { fbTrackHasHits = ifTrackHasHits; }

    /// @brief Sets Flag: if track selected
    /// @param ifTrackSelected  Flag: if track selected
    void SetIfTrackSelected(bool ifTrackSelected) { fbTrackSelected = ifTrackSelected; }

    /// @brief Sets front strips stereo angle
    /// @param phiU Front strips stereo angle [rad]
    void SetPhiU(double phiU) { fPhiU = phiU; }

    /// @brief Sets back strips stereo angle
    /// @param phiV Back strips stereo angle [rad]
    void SetPhiV(double phiV) { fPhiV = phiV; }

    /// @brief Sets point index
    /// @param pointID  Index of point
    /// @param eventID  Index of MC event
    /// @param fileID   Index of MC file
    void SetPointID(int pointID, int eventID, int fileID)
    {
      fPointID   = pointID;
      fMCEventID = eventID;
      fMCFileID  = fileID;
    }

    /// @brief Sets point time
    /// @param pointTime Point time [ns]
    void SetPointTime(double pointTime) { fPointTime = pointTime; }

    /// @brief Sets point x-coordinate
    /// @param pointX Point x-coordinate [cm]
    void SetPointX(double pointX) { fPointX = pointX; }

    /// @brief Sets point y-coordinate
    /// @param pointY Point y-coordinate [cm]
    void SetPointY(double pointY) { fPointY = pointY; }

    /// @brief Sets point z-coordinate
    /// @param pointZ Point z-coordinate [cm]
    void SetPointZ(double pointZ) { fPointZ = pointZ; }

    /// @brief Sets station local index
    /// @return Station local index
    void SetStationID(int iStLoc) { fStationID = iStLoc; }

   private:
    static constexpr double kNAN = std::numeric_limits<double>::signaling_NaN();

    double fPhiU         = kNAN;   ///< Stereo angle for front strips [rad]
    double fPhiV         = kNAN;   ///< Stereo anele for back strips [rad]
    double fHitX         = kNAN;   ///< Hit x-coordinate [cm]
    double fHitY         = kNAN;   ///< Hit y-coordinate [cm]
    double fHitZ         = kNAN;   ///< Hit z-coordinate [cm]
    double fHitTime      = kNAN;   ///< Hit time [ns]
    double fHitDx        = kNAN;   ///< Hit x-coordinate error [cm]
    double fHitDy        = kNAN;   ///< Hit y-coordinate error [cm]
    double fHitDxy       = kNAN;   ///< Hit x- and y-coordinate covariance [cm2]
    double fHitTimeError = kNAN;   ///< Hit time error [ns]
    double fPointX       = kNAN;   ///< Point x-coordinate [cm]
    double fPointY       = kNAN;   ///< Point y-coordinate [cm]
    double fPointZ       = kNAN;   ///< Point z-coordinate [cm]
    double fPointTime    = kNAN;   ///< Point time [ns]
    int fStationID       = -1;     ///< Local index of tracking station
    int fHitID           = -1;     ///< Index of hit
    int fPointID         = -1;     ///< Index of MC point
    int fMCEventID       = -1;     ///< Index of MC event
    int fMCFileID        = -1;     ///< Index of MC file id
    bool fbTrackSelected = false;  ///< Flag: if track selected
    bool fbTrackHasHits  = false;  ///< Flag: if track has hits
  };

}  // namespace cbm::ca

#endif  // CbmCaHitQaData_h
