/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaStationInitializer.cxx
/// \brief
/// \author S.Zharko <s.zharko@gsi.de>
/// \since  18.01.2022
///

#include "CaStationInitializer.h"

#include "AlgoFairloggerCompat.h"
#include "CaDefs.h"

#include <iomanip>
#include <sstream>
#include <utility>

using cbm::algo::ca::EDetectorID;
using cbm::algo::ca::fvec;
using cbm::algo::ca::Station;
using cbm::algo::ca::StationInitializer;

// ---------------------------------------------------------------------------------------------------------------------
//
StationInitializer::StationInitializer(EDetectorID detectorID, int stationID) noexcept
  : fDetectorID(detectorID)
  , fStationID(stationID)
{
  fInitController.SetFlag(EInitKey::kDetectorID);
  fInitController.SetFlag(EInitKey::kStationID);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::Reset()
{
  StationInitializer other;
  this->Swap(other);
}

// ---------------------------------------------------------------------------------------------------------------------
//
const Station<fvec>& StationInitializer::GetStation() const
{
  if (!fInitController.IsFinalized()) {
    std::stringstream msg;
    msg << "StationInitializer::GetStation: attempt to get a Station object from uninitialized L1BaseStation with "
        << "stationID = " << fStationID << " and detectorID = " << static_cast<int>(fDetectorID);
    LOG(fatal) << msg.str();
  }
  return fStation;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetDetectorID(EDetectorID inID)
{
  if (!fInitController.GetFlag(EInitKey::kDetectorID)) {
    fDetectorID = inID;
    fInitController.SetFlag(EInitKey::kDetectorID);
  }
  else {
    LOG(warn) << "StationInitializer::SetDetectorID: Attempt of detector ID redifinition";
  }
}


// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetFieldFunction(
  const std::function<void(const double (&xyz)[3], double (&B)[3])>& getFieldValue)
{
  if (fInitController.GetFlag(EInitKey::kFieldSlice)) {
    LOG(warn) << "StationInitializer::SetFieldSlice: Attempt to redifine field slice for station with detectorID = "
              << static_cast<int>(fDetectorID) << " and stationID = " << fStationID << ". Redifinition ignored";
    return;
  }

  if (!fInitController.GetFlag(EInitKey::kZref)) {
    LOG(fatal) << "Attempt to set magnetic field slice before setting z position of the station";
  }
  if (!fInitController.GetFlag(EInitKey::kXmax)) {
    LOG(fatal) << "Attempt to set magnetic field slice before Xmax size of the station";
  }
  if (!fInitController.GetFlag(EInitKey::kYmax)) {
    LOG(fatal) << "Attempt to set magnetic field slice before Ymax size of the station";
  }

  /// \brief Magnetic field function type
  /// Signature: tuple<Bx, By, Bz>(x, y, z);
  auto glambda = [&](double x, double y, double z) -> std::tuple<double, double, double> {
    double xyz[3] = {x, y, z};
    double B[3]   = {};
    getFieldValue(xyz, B);
    return std::tuple<double, double, double>(B[0], B[1], B[2]);
  };

  fStation.fieldSlice = kf::FieldSlice<fvec>(glambda, fXmax, fYmax, fZref);

  fInitController.SetFlag(EInitKey::kFieldSlice);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetFieldStatus(int fieldStatus)
{
  fStation.fieldStatus = fieldStatus;
  fInitController.SetFlag(EInitKey::kFieldStatus);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetGeoLayerID(int geoLayerID)
{
  fStation.geoLayerID = geoLayerID;
  fInitController.SetFlag(EInitKey::kGeoLayerID);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetStationID(int inID)
{
  if (!fInitController.GetFlag(EInitKey::kStationID)) {
    fStationID = inID;
    fInitController.SetFlag(EInitKey::kStationID);
  }
  else {
    LOG(warn) << "StationInitializer::SetStationID: Attempt of station ID redifinition";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetStationType(int inType)
{
  if (!fInitController.GetFlag(EInitKey::kType)) {
    fStation.type = inType;
    fInitController.SetFlag(EInitKey::kType);
  }
  else {
    LOG(warn) << "StationInitializer::SetStationType: Attempt of station type redifinition";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetXmax(double aSize)
{
  fXmax         = aSize;
  fStation.Xmax = aSize;
  fInitController.SetFlag(EInitKey::kXmax);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetYmax(double aSize)
{
  fYmax         = aSize;
  fStation.Ymax = aSize;
  fInitController.SetFlag(EInitKey::kYmax);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetTimeInfo(int inTimeInfo)
{
  fStation.timeInfo = inTimeInfo;
  fInitController.SetFlag(EInitKey::kTimeInfo);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetTrackingStatus(bool flag)
{
  fTrackingStatus = flag;
  fInitController.SetFlag(EInitKey::kTrackingStatus);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetZref(double inZ)
{
  fStation.fZ = inZ;  // setting simd vector of single-precision floats, which is passed to high performanced L1Algo
  fZref       = inZ;  // setting precised value to use in field approximation etc
  fInitController.SetFlag(EInitKey::kZref);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetZmin(double inZ)
{
  fZmin = inZ;  // setting precised value to use in field approximation etc
  fInitController.SetFlag(EInitKey::kZmin);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::SetZmax(double inZ)
{
  fZmax = inZ;  // setting precised value to use in field approximation etc
  fInitController.SetFlag(EInitKey::kZmax);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void StationInitializer::Swap(StationInitializer& other) noexcept
{
  std::swap(fDetectorID, other.fDetectorID);
  std::swap(fStationID, other.fStationID);
  std::swap(fTrackingStatus, other.fTrackingStatus);
  std::swap(fXmax, other.fXmax);
  std::swap(fYmax, other.fYmax);
  std::swap(fZref, other.fZref);
  std::swap(fZmin, other.fZmin);
  std::swap(fZmax, other.fZmax);
  std::swap(fStation, other.fStation);
  std::swap(fInitController, other.fInitController);
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string StationInitializer::ToString(int verbosityLevel, int indentLevel) const
{
  std::stringstream aStream{};
  constexpr char indentChar = '\t';
  std::string indent(indentLevel, indentChar);

  if (verbosityLevel == 0) {
    aStream << indent << "StationInitializer object: {stationID, detectorID, z, address} = {" << fStationID << ", "
            << static_cast<int>(fDetectorID) << ", " << fZref << ", " << this << '}';
  }
  else if (verbosityLevel > 0) {
    aStream << indent << "StationInitializer object: at " << this << '\n';
    aStream << indent << indentChar << "Station ID:              " << fStationID << '\n';
    aStream << indent << indentChar << "Detector ID:             " << static_cast<int>(fDetectorID) << '\n';
    aStream << indent << indentChar << "ca::Station object:" << '\n';
    aStream << fStation.ToString(verbosityLevel - 1, indentLevel + 1) << '\n';
    aStream << indent << indentChar << "Additional fields:\n";
    aStream << indent << indentChar << indentChar << "Zmin:                    " << fZmin << '\n';
    aStream << indent << indentChar << indentChar << "Zmax:                    " << fZmax << '\n';
    aStream << indent << indentChar << indentChar << "Xmax:                    " << fXmax << '\n';
    aStream << indent << indentChar << indentChar << "Ymax:                    " << fYmax << '\n';
  }
  return aStream.str();
}
