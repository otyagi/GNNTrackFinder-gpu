/* Copyright (C) 2010-2021 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer], Maksym Zyzak, Valentina Akishina, Ivan Kisel, Sergey Gorbunov, Sergei Zharko, Dominik Smith */

#include "CaFramework.h"

#include "CaGridEntry.h"
#include "CaTimer.h"
#include "CaTrack.h"
// #include "CaToolsDebugger.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

namespace cbm::algo::ca
{
  using constants::phys::ProtonMassD;
  using constants::phys::SpeedOfLightInv;
  using constants::phys::SpeedOfLightInvD;
  //using cbm::ca::tools::Debugger;

  // -------------------------------------------------------------------------------------------------------------------
  //
  void Framework::Init(const TrackingMode mode)
  {
    fpTrackFinder =
      std::make_unique<ca::TrackFinder>(fParameters, fDefaultMass, mode, fMonitorData, fNofThreads, fCaRecoTime);
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void Framework::Finish()
  {
    //Debugger::Instance().Write();
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void Framework::ReceiveInputData(InputData&& inputData)
  {
    // ----- Get input data --------------------------------------------------------------------------------------------
    fInputData = std::move(inputData);
  }


  // -------------------------------------------------------------------------------------------------------------------
  //
  void Framework::ReceiveParameters(Parameters<fvec>&& parameters)
  {
    fParameters          = std::move(parameters);
    fNstationsBeforePipe = fParameters.GetNstationsActive(static_cast<EDetectorID>(0));

    kf::GlobalField::ForceUseOfOriginalField(fParameters.DevIsUseOfOriginalField());
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  int Framework::GetMcTrackIdForCaHit(int /*iHit*/)
  {
    return -1;
    /*
    int hitId    = iHit;
    int iMcPoint = CbmL1::Instance()->GetHitBestMcRefs()[hitId];
    if (iMcPoint < 0) return -1;
    return CbmL1::Instance()->GetMcPoints()[iMcPoint].ID;
    */
  }

  int Framework::GetMcTrackIdForWindowHit(int /*iHit*/)
  {
    return -1;
    /*
    int hitId    = fWindowHits[iHit].Id();
    int iMcPoint = CbmL1::Instance()->GetHitBestMcRefs()[hitId];
    if (iMcPoint < 0) return -1;
    return CbmL1::Instance()->GetMcPoints()[iMcPoint].ID;
    */
  }
}  // namespace cbm::algo::ca
/*
const CbmL1MCTrack* Framework::GetMcTrackForWindowHit(int iHit) const
{
  return nullptr;
  int id = GetMcTrackIdForWindowHit(iHit);
  if (id < 0) return nullptr;
  return &CbmL1::Instance()->GetMcTracks()[id];
}
*/

//   bool Framework::SortTrip(TripSort const& a, TripSort const& b) {
//       return   ( a.trip.GetLevel() >  b.trip.GetLevel() );
// }
//
// bool Framework::SortCand(CandSort const& a, CandSort const& b) {
//     if (a.cand.Lengtha != b.cand.Lengtha) return (a.cand.Lengtha > b.cand.Lengtha);
//
//     if (a.cand.ista != b.cand.ista ) return (a.cand.ista  < b.cand.ista );
//
//     if (a.cand.chi2  != b.cand.chi2 )return (a.cand.chi2  < b.cand.chi2 );
//     //return (a->chi2  < b->chi2 );
//     //   return (a->CandIndex < b->CandIndex );
//    // return (a.cand.CandIndex > b.cand.CandIndex );
// }

//   inline int Framework::PackIndex(const int& a, const int& b, const int& c) {
//       return   (a) + ((b)*10000) + (c*100000000);
// }
//
//   inline int Framework::UnPackIndex(const int& i, int& a, int& b, int& c) {
