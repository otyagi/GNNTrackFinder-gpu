/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   L1CAIteration.cxx
/// \brief  Definition of the L1CAIteration class methods
/// \since  05.02.2022
/// \author S.Zharko <s.zharko@gsi.de>

#include "CaIteration.h"

#include "CaDefs.h"

#include <limits>
#include <sstream>
#include <string_view>

using cbm::algo::ca::Iteration;
using cbm::algo::ca::Vector;

// ---------------------------------------------------------------------------------------------------------------------
//
Iteration::Iteration(const std::string& name) : fName(name) {}

// ---------------------------------------------------------------------------------------------------------------------
//
bool Iteration::Check() const
{
  using constants::size::MaxNstations;
  constexpr float kMaxFloat = std::numeric_limits<float>::max();
  bool res                  = true;

  auto CheckValueLimits = [&](std::string_view name, auto value, auto min, auto max) -> bool {
    if (value > max || value < min) {
      LOG(info) << "cbm::algo::ca::Iteration: parameter " << name << " = " << value << " runs out of range " << min
                << ", " << max;
      return false;
    }
    return true;
  };

  // TODO: SZh 06.10.2022: These values should be tuned. At the moment the std::numeric_limits<T>::max value is used for
  //                       debug purposes. In future, these values will be strengthened.
  res = CheckValueLimits("track_chi2_cut", fTrackChi2Cut, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("triplet_chi2_cut", fTripletChi2Cut, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("triplet_final_chi2_cut", fTripletFinalChi2Cut, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("doublet_chi2_cut", fDoubletChi2Cut, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("pick_gather", fPickGather, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("triplet_link_chi2", fTripletLinkChi2, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("max_qp", fMaxQp, 0.001f, kMaxFloat) && res;
  res = CheckValueLimits("max_slope_pv", fMaxSlopePV, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("max_slope", fMaxSlope, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("max_dz", fMaxDZ, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("min_n_hits", fMinNhits, 3, MaxNstations) && res;
  res = CheckValueLimits("min_n_hits_sta_0", fMinNhitsStation0, 3, MaxNstations) && res;
  res = CheckValueLimits("first_station_index", fFirstStationIndex, 0, MaxNstations) && res;
  res = CheckValueLimits("target_pos_sigma_x", fTargetPosSigmaX, 0.f, kMaxFloat) && res;
  res = CheckValueLimits("target_pos_sigma_y", fTargetPosSigmaY, 0.f, kMaxFloat) && res;
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Iteration::SetTargetPosSigmaXY(float sigmaX, float sigmaY)
{
  fTargetPosSigmaX = sigmaX;
  fTargetPosSigmaY = sigmaY;
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string Iteration::ToString(int) const
{
  std::vector<Iteration> vIter{*this};
  return Iteration::ToTableFromVector(vIter);
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string Iteration::ToTableFromVector(const Vector<Iteration>& vIterations)
{
  std::stringstream msg;
  msg << std::boolalpha;

  auto PutRow = [&](const std::string& name, std::function<void(const Iteration&)> fn) {
    msg << std::setw(40) << std::setfill(' ') << name << ' ';
    for (const auto& iter : vIterations) {
      msg << std::setw(12) << std::setfill(' ');
      fn(iter);
      msg << ' ';
    }
    msg << '\n';
  };

  PutRow("                                   ", [&](const Iteration& i) { msg << i.GetName(); });
  PutRow("Is primary                         ", [&](const Iteration& i) { msg << i.GetPrimaryFlag(); });
  PutRow("Is electron                        ", [&](const Iteration& i) { msg << i.GetElectronFlag(); });
  PutRow("If tracks created from triplets    ", [&](const Iteration& i) { msg << i.GetTrackFromTripletsFlag(); });
  PutRow("If tracks extended with unused hits", [&](const Iteration& i) { msg << i.GetExtendTracksFlag(); });
  PutRow("Triplets can jump over <=n stations", [&](const Iteration& i) { msg << i.GetMaxStationGap(); });
  PutRow("Min number of hits                 ", [&](const Iteration& i) { msg << i.GetMinNhits(); });
  PutRow("Min number of hits on station 0    ", [&](const Iteration& i) { msg << i.GetMinNhitsStation0(); });
  PutRow("Track chi2 cut                     ", [&](const Iteration& i) { msg << i.GetTrackChi2Cut(); });
  PutRow("Triplet chi2 cut                   ", [&](const Iteration& i) { msg << i.GetTripletChi2Cut(); });
  PutRow("Triplet final chi2 cut             ", [&](const Iteration& i) { msg << i.GetTripletFinalChi2Cut(); });
  PutRow("Doublet chi2 cut                   ", [&](const Iteration& i) { msg << i.GetDoubletChi2Cut(); });
  PutRow("Pick gather                        ", [&](const Iteration& i) { msg << i.GetPickGather(); });
  PutRow("Triplet link chi2                  ", [&](const Iteration& i) { msg << i.GetTripletLinkChi2(); });
  PutRow("Max q/p                            ", [&](const Iteration& i) { msg << i.GetMaxQp(); });
  PutRow("Max slope                          ", [&](const Iteration& i) { msg << i.GetMaxSlope(); });
  PutRow("Max slope at primary vertex        ", [&](const Iteration& i) { msg << i.GetMaxSlopePV(); });
  PutRow("Max DZ                             ", [&](const Iteration& i) { msg << i.GetMaxDZ(); });
  PutRow("Target position sigma X [cm]       ", [&](const Iteration& i) { msg << i.GetTargetPosSigmaX(); });
  PutRow("Target position sigma Y [cm]       ", [&](const Iteration& i) { msg << i.GetTargetPosSigmaY(); });
  PutRow("First tracking station index       ", [&](const Iteration& i) { msg << i.GetFirstStationIndex(); });

  return msg.str();
}
