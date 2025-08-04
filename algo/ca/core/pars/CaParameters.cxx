/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file L1Parameters.cxx
/// \brief Parameter container for the L1Algo library
/// \since 10.02.2022
/// \author S.Zharko <s.zharko@gsi.de>

#include "CaParameters.h"

#include "AlgoFairloggerCompat.h"

#include <iomanip>

using cbm::algo::ca::EDetectorID;
using cbm::algo::ca::Iteration;
using cbm::algo::ca::Parameters;
using cbm::algo::ca::kfutils::CheckSimdVectorEquality;

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename DataT>
Parameters<DataT>::Parameters()
{
  fvGeoToActiveMap.fill(-1);  // by default, all stations are inactive, thus all the IDs must be -1
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename DataT>
void Parameters<DataT>::CheckConsistency() const
{
  LOG(info) << "Consistency test for L1 parameters object... ";
  /*
   * Array of active station IDs
   *
   * In the array of active station IDs, 
   *   (i)   sum of elements, which are not equal -1, must be equal the number of stations,
   *   (ii)  subsequence of all the elements, which are not equal -1, must be a gapless subset of integer numbers starting with 0
   */

  auto filterInactiveStationIDs = [](int x) { return x != -1; };
  int nStationsCheck = std::count_if(fvGeoToActiveMap.cbegin(), fvGeoToActiveMap.cend(), filterInactiveStationIDs);
  if (nStationsCheck != fNstationsActiveTotal) {
    std::stringstream msg;
    msg << "ca::Parameters: invalid object condition: array of active station IDs is not consistent "
        << "with the total number of stations (" << nStationsCheck << " vs. " << fNstationsActiveTotal << ' '
        << "expected)";
    throw std::logic_error(msg.str());
  }

  // Check, if the subsequence of all the elements, which are not equal -1, must be a gapless subset of integer numbers
  // starting from 0. If it is, the testValue in the end must be equal the number of non -1 elements

  std::vector<int> idsCheck(
    nStationsCheck);  // temporary vector containing a sequence of active station IDs without -1 elements
  std::copy_if(fvGeoToActiveMap.cbegin(), fvGeoToActiveMap.cend(), idsCheck.begin(), filterInactiveStationIDs);
  bool isStationIDsOk = true;
  for (int id = 0; id < nStationsCheck; ++id) {
    isStationIDsOk = isStationIDsOk && idsCheck[id] == id;
  }
  if (!isStationIDsOk) {
    std::stringstream msg;
    msg << "ca::Parameters: invalid object condition: array of active station IDs is not a gapless subset "
        << "of integer numbers starting from 0:\n\t";
    for (auto id : fvGeoToActiveMap) {
      msg << std::setw(3) << std::setfill(' ') << id << ' ';
    }
    throw std::logic_error(msg.str());
  }

  /*
   * Check magnetic field flags of the stations
   *
   * In a current version of tracking there are three configurations possible to be proceeded:
   *  A. All the stations are inside magnetic field
   *  B. There is no magnetic field in a setup
   *  C. All the first stations are inside magnetic field, all the last stations are outside the field
   * In all the cases the fieldStatus flags should be sorted containing all non-zero elements in the beginning
   * (representing stations placed into magnetic field) and all zero elements in the end of z-axis.
   */
  bool ifFieldStatusFlagsOk = std::is_sorted(fStations.cbegin(), fStations.cbegin() + fNstationsActiveTotal,
                                             [&](const ca::Station<DataT>& lhs, const ca::Station<DataT>& rhs) {
                                               return bool(lhs.fieldStatus) > bool(rhs.fieldStatus);
                                             });

  if (!ifFieldStatusFlagsOk) {
    std::stringstream msg;
    msg << "ca::Parameters: invalid object condition: L1 tracking is impossible for a given field configuration:\n";
    for (int iSt = 0; iSt < fNstationsActiveTotal; ++iSt) {
      msg << "- station ID:  " << iSt << ",  field status: " << fStations[iSt].fieldStatus << '\n';
    }
    throw std::logic_error(msg.str());
  }

  /*
   * Check target position SIMD vector
   */

  kfutils::CheckSimdVectorEquality(fTargetPos[0], "L1Parameters: target position x");
  kfutils::CheckSimdVectorEquality(fTargetPos[1], "L1Parameters: target position y");
  kfutils::CheckSimdVectorEquality(fTargetPos[2], "L1Parameters: target position z");

  /*
   * Check vertex field region and value objects at primary vertex
   */

  fVertexFieldValue.CheckConsistency();
  fVertexFieldRegion.CheckConsistency();


  /*
   * Check if each station object is consistent itself, and if all of them are placed after the target
   * NOTE: If a station was not set up, it is accounted inconsistent (uninitialized). In the array of stations there are uninitialized
   *       stations possible (with id > NstationsActiveTotal), thus one should NOT run the loop above over all the stations in array
   *       but only until *(fNstationsActive.cend() - 1) (== NstationsActiveTotal).
   * TODO: Probably, we should introduce methods, which check the consistency of fully initialized objects such as ca::Station,
   *       L1MaterialInfo, etc. (S.Zharko)
   */

  for (int iSt = 0; iSt < fNstationsActiveTotal; ++iSt) {
    fStations[iSt].CheckConsistency();
    if (kfutils::simd::Cast<DataT, float>(fStations[iSt].fZ, 0) < kfutils::simd::Cast<DataT, float>(fTargetPos[2], 0)) {
      std::stringstream msg;
      msg << "ca::Parameters: station with global ID = " << iSt << " is placed before target "
          << "(z_st = " << kfutils::simd::Cast<DataT, float>(fStations[iSt].fZ, 0)
          << " [cm] < z_targ = " << kfutils::simd::Cast<DataT, float>(fTargetPos[2], 0) << " [cm])";
      throw std::logic_error(msg.str());
    }
  }

  /*
   * Check thickness maps
   * NOTE: If a ca::MaterialMap map was not set up, it is accounted inconsistent (uninitialized). In the array of thickness maps for each 
   *       there are uninitialized elements possible (with id > NstationsActiveTotal), thus one should NOT run the loop above over 
   *       all the stations in array but only until *(fNstationsActive.cend() - 1) (== NstationsActiveTotal).
   */
  // TODO: Provide these checks in the kf::Setup
  //for (int iSt = 0; iSt < fNstationsActiveTotal; ++iSt) {
  //  fThickMap[iSt].CheckConsistency();
  //}

  /*
   *  Check equality of station indices in setups:
   */
  {
    if (fActiveSetup.GetNofLayers() != this->GetNstationsActive()) {
      std::stringstream msg;
      msg << "ca::Parameters: number of stations in active kf setup (" << fActiveSetup.GetNofLayers()
          << ") differes from the actual number of active tracking stations (" << GetNstationsActive() << ')';
      throw std::logic_error(msg.str());
    }

    if (fGeometrySetup.GetNofLayers() != this->GetNstationsGeometry()) {
      std::stringstream msg;
      msg << "ca::Parameters: number of stations in geometry kf setup (" << fGeometrySetup.GetNofLayers()
          << ") differes from the actual number of geometry tracking stations (" << GetNstationsGeometry() << ')';
      throw std::logic_error(msg.str());
    }

    {
      std::stringstream msg;
      msg << "ca::Parameters: inconsistency in geometry station indexing in kf-setup and old init:";
      bool bConsistent = true;
      for (int iStGeo = 0; iStGeo < GetNstationsGeometry(); ++iStGeo) {
        auto [detId, locId]           = GetStationIndexLocal(iStGeo);
        auto [detIdSetup, locIdSetup] = fGeometrySetup.GetIndexMap().template GlobalToLocal<EDetectorID>(iStGeo);
        if (detId != detIdSetup || locId != locIdSetup) {
          bConsistent = false;
        }
        msg << "\n- (old) detId = " << static_cast<int>(detId) << ", locId = " << locId
            << " ---- (kf) detId = " << static_cast<int>(detIdSetup) << ", locId = " << locIdSetup;
      }
      if (!bConsistent) {
        throw std::logic_error(msg.str());
      }
    }

    {
      std::stringstream msg;
      msg << "ca::Parameters: inconsistency in active station indexing in kf-setup and old init:";
      bool bConsistent = true;
      for (int iStGeo = 0; iStGeo < GetNstationsGeometry(); ++iStGeo) {
        auto [detId, locId] = GetStationIndexLocal(iStGeo);
        int iStActive       = GetStationIndexActive(locId, detId);
        if (iStActive < 0) {
          continue;
        }
        auto [detIdSetup, locIdSetup] = fActiveSetup.GetIndexMap().template GlobalToLocal<EDetectorID>(iStActive);
        if (detId != detIdSetup || locId != locIdSetup) {
          bConsistent = false;
        }
        msg << "\n- (old) detId = " << static_cast<int>(detId) << ", locId = " << locId
            << " ---- (kf) detId = " << static_cast<int>(detIdSetup) << ", locId = " << locIdSetup;
      }
      if (!bConsistent) {
        throw std::logic_error(msg.str());
      }
    }
  }


  /*
   *  Check iterations sequence
   *  1. Number of iterations should be larger then zero
   *  2. Each iteration should contain values within predefined limits
   *  3. Number of iterations with TrackFromTriplets flag turned on no more then 1
   *  4. If the TrackFromTriplets iteration exists, it should be the last one in the sequence
   */
  {
    int nIterations = fCAIterations.size();
    if (!nIterations) {
      std::stringstream msg;
      msg << "L1Parameters: 0 track finder iterations were found. Please, define at least one iteration";
      throw std::logic_error(msg.str());
    }

    std::string names = "";
    for (const auto& iter : fCAIterations) {
      if (!iter.Check()) {
        names += iter.GetName() + " ";
      }
    }
    if (names.size()) {
      std::stringstream msg;
      msg << "L1Parameters: some parameters are out of range for the following iterations: " << names;
      throw std::logic_error(msg.str());
    }

    nIterations = std::count_if(fCAIterations.begin(), fCAIterations.end(),
                                [=](const Iteration& it) { return it.GetTrackFromTripletsFlag(); });
    if (nIterations > 1) {
      std::stringstream msg;
      msg << "L1Parameters: found " << nIterations << " iterations with GetTrackFromTripletsFlag() == true:\n";
      for (const auto& iter : fCAIterations) {
        if (iter.GetTrackFromTripletsFlag()) {
          msg << '\t' << "- " << iter.GetName() << '\n';
        }
      }
      msg << "Only the one iteration can have GetTrackFromTripletsFlag() == true, and this iteration should be ";
      msg << "the last one";
      throw std::logic_error(msg.str());
    }

    if (nIterations == 1 && !(fCAIterations.end() - 1)->GetTrackFromTripletsFlag()) {
      std::stringstream msg;
      msg << "L1Parameters: iteration with GetTrackFromTripletsFlag() == true is not the last in a sequence. ";
      msg << "The GetTrackFromTripletsFlag() value in the iterations sequence: \n";
      for (const auto& iter : fCAIterations) {
        msg << '\t' << "- " << std::setw(15) << std::setfill(' ') << iter.GetName() << ' ';
        msg << std::setw(6) << std::setfill(' ') << iter.GetTrackFromTripletsFlag() << '\n';
      }
      throw std::logic_error(msg.str());
    }
  }

  LOG(info) << "Consistency test for L1 parameters object... \033[1;32mpassed\033[0m";
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename DataT>
int Parameters<DataT>::GetNstationsActive(EDetectorID detectorID) const
{
  int nStations = 0;
  for (int iStLoc = 0; iStLoc < this->GetNstationsGeometry(detectorID); ++iStLoc) {
    int iStActive = this->GetStationIndexActive(iStLoc, detectorID);
    if (iStActive > -1) {
      ++nStations;
    }
  }
  return nStations;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename DataT>
void Parameters<DataT>::Print(int /*verbosityLevel*/) const
{
  LOG(info) << ToString();
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<typename DataT>
std::string Parameters<DataT>::ToString(int verbosity, int indentLevel) const
{
  using namespace constants;
  using std::setfill;
  using std::setw;
  std::stringstream msg{};
  if (verbosity < 1) {
    return msg.str();
  }

  constexpr char indentCh = '\t';
  std::string indent(indentLevel, indentCh);

  /// Sets red/green color dependently from flag
  auto ClrFlg = [&](bool flag) { return (flag ? clrs::GN : clrs::RD); };

  msg << " ----- CA parameters list -----\n";
  msg << indent << clrs::CLb << "COMPILE TIME CONSTANTS:\n" << clrs::CL;
  msg << indent << indentCh << "Bits to code one station:           " << constants::size::StationBits << '\n';
  msg << indent << indentCh << "Bits to code one triplet:           " << constants::size::TripletBits << '\n';
  msg << indent << indentCh << "Max number of detectors:            " << constants::size::MaxNdetectors << '\n';
  msg << indent << indentCh << "Max number of stations:             " << constants::size::MaxNstations << '\n';
  msg << indent << indentCh << "Max number of triplets:             " << constants::size::MaxNtriplets << '\n';
  msg << indent << clrs::CLb << "RUNTIME CONSTANTS:\n" << clrs::CL;
  msg << indent << indentCh << "Random seed:                        " << fRandomSeed << '\n';
  msg << indent << indentCh << "Max number of doublets per singlet: " << fMaxDoubletsPerSinglet << '\n';
  msg << indent << indentCh << "Max number of triplets per doublet: " << fMaxTripletPerDoublets << '\n';
  msg << indent << indentCh << "Ghost suppression:                   " << fGhostSuppression << '\n';
  msg << indent << clrs::CLb << "CA TRACK FINDER ITERATIONS:\n" << clrs::CL;
  msg << Iteration::ToTableFromVector(fCAIterations);
  msg << indent << clrs::CLb << "GEOMETRY:\n" << clrs::CL;
  msg << indent << indentCh << clrs::CLb << "TARGET:\n" << clrs::CL;
  msg << indent << indentCh << indentCh << "Position:\n";
  for (int dim = 0; dim < 3 /*nDimensions*/; ++dim) {
    msg << indent << indentCh << indentCh << indentCh << char(120 + dim) << " = "
        << kfutils::simd::Cast<DataT, float>(fTargetPos[dim], 0) << " cm\n";
  }
  msg << indent << indentCh << clrs::CLb << "FIELD:\n" << clrs::CL;
  msg << indent << indentCh << indentCh << "Target field:\n";
  msg << indent << indentCh << indentCh << indentCh
      << "Bx = " << kfutils::simd::Cast<DataT, float>(fVertexFieldValue.GetBx(), 0) << " Kg\n";
  msg << indent << indentCh << indentCh << indentCh
      << "By = " << kfutils::simd::Cast<DataT, float>(fVertexFieldValue.GetBy(), 0) << " Kg\n";
  msg << indent << indentCh << indentCh << indentCh
      << "Bz = " << kfutils::simd::Cast<DataT, float>(fVertexFieldValue.GetBz(), 0) << " Kg\n";


  msg << indent << indentCh << clrs::CLb << "NUMBER OF STATIONS:\n" << clrs::CL;
  msg << indent << indentCh << indentCh << "Number of stations (Geometry): ";
  for (int iDet = 0; iDet < constants::size::MaxNdetectors; ++iDet) {
    msg << setw(2) << this->GetNstationsGeometry(static_cast<EDetectorID>(iDet)) << ' ';
  }
  msg << " | total = " << setw(2) << this->GetNstationsGeometry();
  msg << '\n';
  msg << indent << indentCh << indentCh << "Number of stations (Active):   ";
  for (int iDet = 0; iDet < constants::size::MaxNdetectors; ++iDet) {
    msg << setw(2) << setfill(' ') << this->GetNstationsActive(static_cast<EDetectorID>(iDet)) << ' ';
  }
  msg << " | total = " << setw(2) << this->GetNstationsActive();
  msg << '\n' << indent << indentCh << indentCh << clrs::CL << "Geometry station indices: ";
  for (int iStGeo = 0; iStGeo < this->GetNstationsGeometry(); ++iStGeo) {
    bool isActive = fvGeoToActiveMap[iStGeo] != -1;
    msg << ClrFlg(isActive) << setw(3) << setfill(' ') << iStGeo << ' ';
  }
  msg << '\n' << indent << indentCh << indentCh << clrs::CL << "Local station indices:    ";
  for (int iStGeo = 0; iStGeo < this->GetNstationsGeometry(); ++iStGeo) {
    bool isActive = fvGeoToActiveMap[iStGeo] != -1;
    msg << ClrFlg(isActive) << setw(3) << setfill(' ') << static_cast<int>(fvGeoToLocalIdMap[iStGeo].second) << ' ';
  }
  msg << '\n' << indent << indentCh << indentCh << clrs::CL << "Detector indices:         ";
  for (int iStGeo = 0; iStGeo < this->GetNstationsGeometry(); ++iStGeo) {
    bool isActive = fvGeoToActiveMap[iStGeo] != -1;
    msg << ClrFlg(isActive) << setw(3) << setfill(' ') << static_cast<int>(fvGeoToLocalIdMap[iStGeo].first) << ' ';
  }
  msg << '\n' << indent << indentCh << indentCh << clrs::CL << "Active station indices:   ";
  for (int iStGeo = 0; iStGeo < this->GetNstationsGeometry(); ++iStGeo) {
    bool isActive = fvGeoToActiveMap[iStGeo] != -1;
    msg << ClrFlg(isActive) << setw(3) << setfill(' ') << fvGeoToActiveMap[iStGeo] << ' ';
  }
  msg << clrs::CL << '\n';

  msg << indent << indentCh << clrs::CLb << "STATIONS:\n" << clrs::CL;
  msg << indent << indentCh << setw(9) << "Active ID" << ' ';
  msg << fStations[0].ToString(1, 0, true) << '\n';
  for (int iStAct = 0; iStAct < this->GetNstationsActive(); ++iStAct) {
    msg << indent << indentCh << setw(9) << iStAct << ' ';
    msg << fStations[iStAct].ToString(verbosity, 0) << '\n';
  }

  msg << indent << indentCh << clrs::CLb << "MISALIGNMENT TOLERANCES:\n" << clrs::CL;
  msg << indent << indentCh;
  msg << setw(9) << "Active ID";
  msg << setw(9) << "dx[cm]";
  msg << setw(9) << "dy[cm]";
  msg << setw(9) << "dt[ns]" << '\n';

  for (int iDet = 0; iDet < constants::size::MaxNdetectors; ++iDet) {
    for (int iStLocal = 0; iStLocal < this->GetNstationsGeometry(static_cast<EDetectorID>(iDet)); ++iStLocal) {
      int iStActive = this->GetStationIndexActive(iStLocal, static_cast<EDetectorID>(iDet));
      if (iStActive < 0) {
        continue;
      }
      msg << indent << indentCh;
      msg << setw(9) << iStActive;
      msg << setw(9) << fMisalignmentX[iDet];
      msg << setw(9) << fMisalignmentY[iDet];
      msg << setw(9) << fMisalignmentT[iDet] << '\n';
    }
  }

  msg << indent << clrs::CLb << "DEV FLAGS:" << clrs::CL << " (for debug only)\n";
  msg << indent << indentCh << "Hits search area is ignored:     " << fDevIsIgnoreHitSearchAreas << '\n';
  msg << indent << indentCh << "Non-approx. field is used:       " << fDevIsMatchDoubletsViaMc << '\n';
  msg << indent << indentCh << "Doublets matching via MC:        " << fDevIsMatchDoubletsViaMc << '\n';
  msg << indent << indentCh << "Triplets matching via MC:        " << fDevIsMatchTripletsViaMc << '\n';
  msg << indent << indentCh << "Extend tracks with MC matching:  " << fDevIsExtendTracksViaMc << '\n';
  msg << indent << indentCh << "Overlap hits matching via MC:    " << fDevIsSuppressOverlapHitsViaMc << '\n';
  msg << indent << indentCh << "Use hit search windows:          " << fDevIsParSearchWUsed << '\n';

  if (fDevIsParSearchWUsed) {
    msg << indent << "SEARCH WINDOWS:\n";
    for (int iSt = 1; iSt < fNstationsActiveTotal; ++iSt) {
      for (int iIter = 0; iIter < (int) fCAIterations.size(); ++iIter) {
        msg << indent << "- station: " << iSt << ", iteration: " << iIter << '\n';
        msg << GetSearchWindow(iSt, iIter).ToString() << '\n';
      }
    }
  }

  msg << '\n';
  return msg.str();
}

namespace cbm::algo::ca
{
  template class Parameters<fvec>;
  template class Parameters<float>;
  template class Parameters<double>;
}  // namespace cbm::algo::ca
