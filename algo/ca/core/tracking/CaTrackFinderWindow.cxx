/* Copyright (C) 2009-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Valentina Akishina, Ivan Kisel, Sergey Gorbunov [committer], Igor Kulakov, Maksym Zyzak */

/*
 *=====================================================
 *
 *  CBM Level 1 4D Reconstruction
 *
 *  Authors: V.Akishina, I.Kisel,  S.Gorbunov, I.Kulakov, M.Zyzak
 *  Documentation: V.Akishina
 *
 *  e-mail : v.akishina@gsi.de
 *
 *=====================================================
 *
 *  Finds tracks using the Cellular Automaton algorithm
 *
 */


#include "CaTrackFinderWindow.h"

#include "AlgoFairloggerCompat.h"
#include "CaBranch.h"
#include "CaFramework.h"
#include "CaGrid.h"
#include "CaGridEntry.h"
#include "CaTrack.h"
#include "CaTripletConstructor.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>

// #include "CaToolsDebugger.h"

namespace cbm::algo::ca
{
  // -------------------------------------------------------------------------------------------------------------------
  TrackFinderWindow::TrackFinderWindow(const ca::Parameters<fvec>& pars, const fscal mass, const ca::TrackingMode& mode,
                                       ca::TrackingMonitorData& monitorData)
    : fParameters(pars)
    , fDefaultMass(mass)
    , fTrackingMode(mode)
    , frMonitorData(monitorData)
    , fTrackExtender(pars, mass)
    , fCloneMerger(pars, mass)
    , fTrackFitter(pars, mass, mode)
  {
  }

  // -------------------------------------------------------------------------------------------------------------------
  bool TrackFinderWindow::checkTripletMatch(const ca::Triplet& l, const ca::Triplet& r, fscal& dchi2,
                                            WindowData& wData) const
  {
    dchi2 = 1.e20;

    if (r.GetMHit() != l.GetRHit()) return false;
    if (r.GetLHit() != l.GetMHit()) return false;

    if (r.GetMSta() != l.GetRSta()) return false;
    if (r.GetLSta() != l.GetMSta()) return false;

    const fscal tripletLinkChi2 = wData.CurrentIteration()->GetTripletLinkChi2();
    if (r.IsMomentumFitted()) {
      assert(l.IsMomentumFitted());

      fscal dqp = l.GetQp() - r.GetQp();
      fscal Cqp = l.GetCqp() + r.GetCqp();

      if (!std::isfinite(dqp)) return false;
      if (!std::isfinite(Cqp)) return false;

      if (dqp * dqp > tripletLinkChi2 * Cqp) {
        return false;  // bad neighbour // CHECKME why do we need recheck it?? (it really change result)
      }
      dchi2 = dqp * dqp / Cqp;
    }
    else {
      fscal dtx = l.GetTx() - r.GetTx();
      fscal Ctx = l.GetCtx() + r.GetCtx();

      fscal dty = l.GetTy() - r.GetTy();
      fscal Cty = l.GetCty() + r.GetCty();

      // it shouldn't happen, but happens sometimes

      if (!std::isfinite(dtx)) return false;
      if (!std::isfinite(dty)) return false;
      if (!std::isfinite(Ctx)) return false;
      if (!std::isfinite(Cty)) return false;

      if (dty * dty > tripletLinkChi2 * Cty) return false;
      if (dtx * dtx > tripletLinkChi2 * Ctx) return false;

      //dchi2 = 0.5f * (dtx * dtx / Ctx + dty * dty / Cty);
      dchi2 = 0.;
    }

    if (!std::isfinite(dchi2)) return false;

    return true;
  }


  // **************************************************************************************************
  // *                                                                                                *
  // *                            ------ CATrackFinder procedure ------                               *
  // *                                                                                                *
  // **************************************************************************************************

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::CaTrackFinderSlice(const ca::InputData& input, WindowData& wData)
  {
    // Init windows
    frMonitorData.StartTimer(ETimer::InitWindow);
    ReadWindowData(input.GetHits(), wData);
    frMonitorData.StopTimer(ETimer::InitWindow);

    // Init grids
    frMonitorData.StartTimer(ETimer::PrepareGrid);
    PrepareGrid(input.GetHits(), wData);
    frMonitorData.StopTimer(ETimer::PrepareGrid);

    std::optional<ca::GpuTrackFinderSetup> gpuTrackFinderSetup;
    size_t iter_num = 0;
    if constexpr (constants::gpu::GpuTracking) {
      // XPU initialization
      // - temporary solution for tests only
      // - should not be initialized here
      setenv("XPU_PROFILE", "1", 1);
      xpu::settings settings;
      //      settings.device = "cpu0";
      settings.device = "hip0";
      //      settings.verbose = true;
      xpu::initialize(settings);

      // Set up environment for GPU tracking
      xpu::push_timer("gpuTFinit");
      gpuTrackFinderSetup.emplace(wData, fParameters);
      xpu::timings gpuTFinit = xpu::pop_timer();
      if constexpr (constants::gpu::GpuTimeMonitoring) {
        LOG(info) << "GPU tracking :: Initialization: " << gpuTFinit.wall() << " ms";
      }

      SetupGpuTrackFinder(gpuTrackFinderSetup.value());
    }

    if (constants::gpu::GNNTracking) {  // Run GNN tracking
      frMonitorData.StartTimer(ETimer::FindTracks);
      auto& caIterations = fParameters.GetCAIterations();
      for (auto iter = caIterations.begin(); iter != caIterations.end(); ++iter) {
        // ----- Prepare iteration
        frMonitorData.StartTimer(ETimer::PrepareIteration);
        PrepareCAIteration(*iter, wData, iter == caIterations.begin());
        frMonitorData.StopTimer(ETimer::PrepareIteration);

        GNNTrackFinder(wData, iter_num);

        iter_num++;

      }  // ---- Loop over Track Finder iterations: END ----//
      frMonitorData.StopTimer(ETimer::FindTracks);
    }
    else {  // Run CA iterations
      frMonitorData.StartTimer(ETimer::FindTracks);
      auto& caIterations = fParameters.GetCAIterations();
      for (auto iter = caIterations.begin(); iter != caIterations.end(); ++iter) {
        // ----- Prepare iteration
        frMonitorData.StartTimer(ETimer::PrepareIteration);
        PrepareCAIteration(*iter, wData, iter == caIterations.begin());
        frMonitorData.StopTimer(ETimer::PrepareIteration);

        if constexpr (constants::gpu::GpuTracking) {
          // ----- Triplets construction on GPU -----
          frMonitorData.StartTimer(ETimer::ConstructTriplets);
          ConstructTripletsGPU(wData, gpuTrackFinderSetup.value(), iter_num);
          iter_num++;
          frMonitorData.StopTimer(ETimer::ConstructTriplets);
        }
        else {
          // ----- Triplets construction -----
          frMonitorData.StartTimer(ETimer::ConstructTriplets);
          ConstructTriplets(wData);
          frMonitorData.StopTimer(ETimer::ConstructTriplets);
        }

        // ----- Search for neighbouring triplets -----
        frMonitorData.StartTimer(ETimer::SearchNeighbours);
        SearchNeighbors(wData);
        frMonitorData.StopTimer(ETimer::SearchNeighbours);

        // ----- Collect track candidates and create tracks
        frMonitorData.StartTimer(ETimer::CreateTracks);
        CreateTracks(wData, *iter, std::get<2>(fTripletData));
        frMonitorData.StopTimer(ETimer::CreateTracks);

        // ----- Suppress strips of suppressed hits
        frMonitorData.StartTimer(ETimer::SuppressHitKeys);
        for (unsigned int ih = 0; ih < wData.Hits().size(); ih++) {
          if (wData.IsHitSuppressed(ih)) {
            const ca::Hit& hit                 = wData.Hit(ih);
            wData.IsHitKeyUsed(hit.FrontKey()) = 1;
            wData.IsHitKeyUsed(hit.BackKey())  = 1;
          }
        }
        frMonitorData.StopTimer(ETimer::SuppressHitKeys);
      }  // ---- Loop over Track Finder iterations: END ----//
      frMonitorData.StopTimer(ETimer::FindTracks);
    }

    // Fit tracks
    frMonitorData.StartTimer(ETimer::FitTracks);
    fTrackFitter.FitCaTracks(input, wData);
    frMonitorData.StopTimer(ETimer::FitTracks);

    // Merge clones
    frMonitorData.StartTimer(ETimer::MergeClones);
    fCloneMerger.Exec(input, wData);
    frMonitorData.StopTimer(ETimer::MergeClones);

    // Fit tracks
    frMonitorData.StartTimer(ETimer::FitTracks);
    fTrackFitter.FitCaTracks(input, wData);
    frMonitorData.StopTimer(ETimer::FitTracks);
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::ReadWindowData(const Vector<Hit>& hits, WindowData& wData)
  {
    int nHits = 0;
    for (int iS = 0; iS < fParameters.GetNstationsActive(); iS++) {
      wData.HitStartIndexOnStation(iS) = nHits;
      wData.NofHitsOnStation(iS)       = wData.TsHitIndices(iS).size();
      nHits += wData.NofHitsOnStation(iS);
    }
    wData.HitStartIndexOnStation(fParameters.GetNstationsActive()) = nHits;
    wData.ResetHitData(nHits);

    for (int iS = 0; iS < fParameters.GetNstationsActive(); iS++) {
      int iFstHit = wData.HitStartIndexOnStation(iS);
      for (ca::HitIndex_t ih = 0; ih < wData.TsHitIndices(iS).size(); ++ih) {
        ca::Hit h = hits[wData.TsHitIndex(iS, ih)];  /// D.S. 29.7.24: There is a copy operation here. Can be avoided?
        h.SetId(wData.TsHitIndex(iS, ih));
        wData.Hit(iFstHit + ih) = h;
      }
    }

    if constexpr (fDebug) {
      LOG(info) << "===== Sliding Window hits: ";
      for (int i = 0; i < nHits; ++i) {
        LOG(info) << "    " << wData.Hit(i).ToString();
      }
      LOG(info) << "===== ";
    }

    wData.RecoTracks().clear();
    wData.RecoTracks().reserve(2 * nHits / fParameters.GetNstationsActive());

    wData.RecoHitIndices().clear();
    wData.RecoHitIndices().reserve(2 * nHits);
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::PrepareGrid(const Vector<Hit>& hits, WindowData& wData)
  {
    for (int iS = 0; iS < fParameters.GetNstationsActive(); ++iS) {

      fscal lasttime  = std::numeric_limits<fscal>::infinity();
      fscal starttime = -std::numeric_limits<fscal>::infinity();
      fscal gridMinX  = -0.1;
      fscal gridMaxX  = 0.1;
      fscal gridMinY  = -0.1;
      fscal gridMaxY  = 0.1;

      for (ca::HitIndex_t ih = 0; ih < wData.TsHitIndices(iS).size(); ++ih) {
        const ca::Hit& h = hits[wData.TsHitIndex(iS, ih)];

        gridMinX = std::min(gridMinX, h.X());
        gridMinY = std::min(gridMinY, h.Y());
        gridMaxX = std::max(gridMaxX, h.X());
        gridMaxY = std::max(gridMaxY, h.Y());

        const fscal time = h.T();
        assert(std::isfinite(time));
        lasttime  = std::min(lasttime, time);
        starttime = std::max(starttime, time);
      }

      // TODO: changing the grid also changes the result. Investigate why it happens.
      // TODO: find the optimal grid size

      const int nSliceHits = wData.TsHitIndices(iS).size();
      const fscal sizeY    = gridMaxY - gridMinY;
      const fscal sizeX    = gridMaxX - gridMinX;
      const int nBins2D    = 1 + nSliceHits;

      // TODO: SG: the coefficients should be removed
      const fscal scale    = fParameters.GetStation(iS).GetZ<fscal>() - fParameters.GetTargetPositionZ()[0];
      const fscal maxScale = 0.3 * scale;
      const fscal minScale = 0.01 * scale;

      fscal yStep = 0.3 * sizeY / sqrt(nBins2D);
      fscal xStep = 0.8 * sizeX / sqrt(nBins2D);
      yStep       = std::clamp(yStep, minScale, maxScale);
      xStep       = std::clamp(xStep, minScale, maxScale);

      auto& grid = wData.Grid(iS);
      grid.BuildBins(gridMinX, gridMaxX, gridMinY, gridMaxY, xStep, yStep);
      /* clang-format off */
      grid.StoreHits(wData.Hits(), 
                     wData.HitStartIndexOnStation(iS), 
                     wData.NofHitsOnStation(iS),
                     wData.HitKeyFlags());
      /* clang-format on */
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::PrepareCAIteration(const ca::Iteration& caIteration, WindowData& wData, const bool isFirst)
  {
    wData.SetCurrentIteration(&caIteration);

    // Check if it is not the first element
    if (!isFirst) {
      for (int ista = 0; ista < fParameters.GetNstationsActive(); ++ista) {
        wData.Grid(ista).RemoveUsedHits(wData.Hits(), wData.HitKeyFlags());
      }
    }

    // --> frAlgo.fIsWindowHitSuppressed.reset(frAlgo.fWindowHits.size(), 0);
    wData.ResetHitSuppressionFlags();  // TODO: ??? No effect?

    // --- SET PARAMETERS FOR THE ITERATION ---
    // define the target
    const fscal SigmaTargetX = caIteration.GetTargetPosSigmaX();
    const fscal SigmaTargetY = caIteration.GetTargetPosSigmaY();  // target constraint [cm]

    // Select magnetic field. For primary tracks - fVtxFieldValue, for secondary tracks - st.fieldSlice
    if (caIteration.GetPrimaryFlag()) {
      wData.TargB() = fParameters.GetVertexFieldValue();
    }
    else {
      wData.TargB() = fParameters.GetStation(0).fieldSlice.GetFieldValue(0, 0);
    }  // NOTE: calculates field frAlgo.fTargB in the center of 0th station

    wData.TargetMeasurement().SetX(fParameters.GetTargetPositionX());
    wData.TargetMeasurement().SetY(fParameters.GetTargetPositionY());
    wData.TargetMeasurement().SetDx2(SigmaTargetX * SigmaTargetX);
    wData.TargetMeasurement().SetDxy(0);
    wData.TargetMeasurement().SetDy2(SigmaTargetY * SigmaTargetY);
    wData.TargetMeasurement().SetNdfX(1);
    wData.TargetMeasurement().SetNdfY(1);
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::ConstructTriplets(WindowData& wData)
  {
    auto& [vHitFirstTriplet, vHitNofTriplets, vTriplets] = fTripletData;
    vHitFirstTriplet.reset(wData.Hits().size(), 0);  /// link hit -> first triplet { hit, *, *}
    vHitNofTriplets.reset(wData.Hits().size(), 0);   /// link hit ->n triplets { hit, *, *}

    ca::TripletConstructor constructor(fParameters, wData, fDefaultMass, fTrackingMode);

    // prepare triplet storage
    for (int j = 0; j < fParameters.GetNstationsActive(); j++) {
      const size_t nHitsStation = wData.TsHitIndices(j).size();
      vTriplets[j].clear();
      vTriplets[j].reserve(2 * nHitsStation);
    }

    // indices of the two neighbouring station, taking into account allowed gaps
    std::vector<std::pair<int, int>> staPattern;
    for (int gap = 0; gap <= wData.CurrentIteration()->GetMaxStationGap(); gap++) {
      for (int i = 0; i <= gap; i++) {
        staPattern.push_back(std::make_pair(1 + i, 2 + gap));
      }
    }
    for (int istal = fParameters.GetNstationsActive() - 2; istal >= wData.CurrentIteration()->GetFirstStationIndex();
         istal--) {
      // start with downstream chambers
      const auto& grid = wData.Grid(istal);
      for (auto& entry : grid.GetEntries()) {
        ca::HitIndex_t ihitl = entry.GetObjectId();
        const size_t oldSize = vTriplets[istal].size();
        for (auto& pattern : staPattern) {
          constructor.CreateTripletsForHit(fvTriplets, istal, istal + pattern.first, istal + pattern.second, ihitl);
          vTriplets[istal].insert(vTriplets[istal].end(), fvTriplets.begin(), fvTriplets.end());
        }
        vHitFirstTriplet[ihitl] = PackTripletId(istal, oldSize);
        vHitNofTriplets[ihitl]  = vTriplets[istal].size() - oldSize;
      }
    }  // istal
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::SearchNeighbors(WindowData& wData)
  {
    auto& [vHitFirstTriplet, vHitNofTriplets, vTriplets] = fTripletData;

    for (int istal = fParameters.GetNstationsActive() - 2; istal >= wData.CurrentIteration()->GetFirstStationIndex();
         istal--) {
      // start with downstream chambers

      for (ca::Triplet& tr : vTriplets[istal]) {
        unsigned int nNeighbours   = vHitNofTriplets[tr.GetMHit()];
        unsigned int neighLocation = vHitFirstTriplet[tr.GetMHit()];
        unsigned int neighStation  = TripletId2Station(neighLocation);
        unsigned int neighTriplet  = TripletId2Triplet(neighLocation);

        if (nNeighbours > 0) {
          assert((int) neighStation >= istal + 1
                 && (int) neighStation <= istal + 1 + wData.CurrentIteration()->GetMaxStationGap());
        }

        unsigned char level = 0;

        for (unsigned int iN = 0; iN < nNeighbours; ++iN, ++neighTriplet, ++neighLocation) {

          ca::Triplet& neighbour = vTriplets[neighStation][neighTriplet];

          fscal dchi2 = 0.;
          if (!checkTripletMatch(tr, neighbour, dchi2, wData)) continue;

          if (tr.GetFNeighbour() == 0) {
            tr.SetFNeighbour(neighLocation);
          }
          tr.SetNNeighbours(neighLocation - tr.GetFNeighbour() + 1);

          level = std::max(level, static_cast<unsigned char>(neighbour.GetLevel() + 1));
        }
        tr.SetLevel(level);
      }
      frMonitorData.IncrementCounter(ECounter::Triplet, vTriplets[istal].size());
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::CreateTracks(WindowData& wData, const ca::Iteration& caIteration, TripletArray_t& vTriplets)
  {
    // min level to start triplet. So min track length = min_level+3.
    const int min_level = wData.CurrentIteration()->GetTrackFromTripletsFlag()
                            ? 0
                            : std::min(caIteration.GetMinNhits(), caIteration.GetMinNhitsStation0()) - 3;

    // collect consequtive: the longest tracks, shorter, more shorter and so on
    for (int firstTripletLevel = fParameters.GetNstationsActive() - 3; firstTripletLevel >= min_level;
         firstTripletLevel--) {
      // choose length in triplets number - firstTripletLevel - the maximum possible triplet level among all triplets in the searched candidate
      CreateTrackCandidates(wData, vTriplets, min_level, firstTripletLevel);
      DoCompetitionLoop(wData);
      SelectTracks(wData);
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::CreateTrackCandidates(WindowData& wData, TripletArray_t& vTriplets, const int min_level,
                                                const int firstTripletLevel)
  {
    //  how many levels to check
    int nlevel = (fParameters.GetNstationsActive() - 2) - firstTripletLevel + 1;

    const unsigned char min_best_l =
      (firstTripletLevel > min_level) ? firstTripletLevel + 2 : min_level + 3;  // loose maximum

    // Uses persistent field to save memory allocations.
    // fNewTr is only used here!
    ca::Branch(&new_tr)[constants::size::MaxNstations] = fNewTr;

    fvTrackCandidates.clear();
    fvTrackCandidates.reserve(wData.Hits().size() / 10);

    for (const auto& h : wData.Hits()) {
      fvHitKeyToTrack[h.FrontKey()] = -1;
      fvHitKeyToTrack[h.BackKey()]  = -1;
    }

    //== Loop over triplets with the required level, find and store track candidates
    for (int istaF = wData.CurrentIteration()->GetFirstStationIndex();
         istaF <= fParameters.GetNstationsActive() - 3 - firstTripletLevel; ++istaF) {

      if (--nlevel == 0) break;  //TODO: SG: this is not needed

      for (ca::Triplet& first_trip : vTriplets[istaF]) {

        const auto& fstTripLHit = wData.Hit(first_trip.GetLHit());
        if (wData.IsHitKeyUsed(fstTripLHit.FrontKey()) || wData.IsHitKeyUsed(fstTripLHit.BackKey())) {
          continue;
        }

        //  skip track candidates that are too short

        int minNhits = wData.CurrentIteration()->GetMinNhits();

        if (fstTripLHit.Station() == 0) {
          minNhits = wData.CurrentIteration()->GetMinNhitsStation0();
        }
        if (wData.CurrentIteration()->GetTrackFromTripletsFlag()) {
          minNhits = 0;
        }

        if (3 + first_trip.GetLevel() < minNhits) {
          continue;
        }

        // Collect triplets, which can start a track with length equal to firstTipletLevel + 3. This cut suppresses
        // ghost tracks, but does not affect the efficiency
        if (first_trip.GetLevel() < firstTripletLevel) {
          continue;
        }

        ca::Branch curr_tr;
        curr_tr.AddHit(first_trip.GetLHit());
        curr_tr.SetChi2(first_trip.GetChi2());

        ca::Branch best_tr = curr_tr;

        /// reqursive func to build a tree of possible track-candidates and choose the best
        CAFindTrack(istaF, best_tr, &first_trip, curr_tr, min_best_l, new_tr, wData, vTriplets);

        if (best_tr.NofHits() < firstTripletLevel + 2) continue;  // loose maximum one hit

        if (best_tr.NofHits() < min_level + 3) continue;  // should find all hits for min_level

        if (best_tr.NofHits() < minNhits) {
          continue;
        }

        int ndf = best_tr.NofHits() * 2 - 5;

        // TODO: automatize the NDF calculation

        if (ca::TrackingMode::kGlobal == fTrackingMode || ca::TrackingMode::kMcbm == fTrackingMode) {
          ndf = best_tr.NofHits() * 2 - 4;
        }

        best_tr.SetChi2(best_tr.Chi2() / ndf);
        if (fParameters.GetGhostSuppression()) {
          if (3 == best_tr.NofHits()) {
            if (!wData.CurrentIteration()->GetPrimaryFlag() && (istaF != 0)) continue;  // too /*short*/ non-MAPS track
            if (wData.CurrentIteration()->GetPrimaryFlag() && (best_tr.Chi2() > 5.0)) continue;
          }
        }
        // NOTE: Temporarely disable the warnings, one has to provide more specific coefficient in the capacity
        //                   reservation
        fvTrackCandidates.push_back_no_warning(best_tr);
        ca::Branch& tr = fvTrackCandidates.back();
        tr.SetStation(istaF);
        tr.SetId(fvTrackCandidates.size() - 1);
        // Mark the candidate as dead. To became alive it should first pass the competition in DoCompetitionLoop
        tr.SetAlive(false);
        if constexpr (fDebug) {
          std::stringstream s;
          s << "iter " << wData.CurrentIteration()->GetName() << ", track candidate " << fvTrackCandidates.size() - 1
            << " found, L = " << best_tr.NofHits() << " chi2= " << best_tr.Chi2() << " hits: ";
          for (auto hitIdLoc : tr.Hits()) {
            const auto hitId = wData.Hit(hitIdLoc).Id();
            s << hitId << " (mc " << ca::Framework::GetMcTrackIdForCaHit(hitId) << ") ";
          }
          LOG(info) << s.str();
        }
      }  // itrip
    }  // istaF
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::DoCompetitionLoop(const WindowData& wData)
  {

    // look at the dead track candidates in fvTrackCandidates pool; select the best ones and make them alive

    for (int iComp = 0; (iComp < 100); ++iComp) {

      bool repeatCompetition = false;

      // == Loop over track candidates and mark their strips
      for (ca::Branch& tr : fvTrackCandidates) {
        if (tr.IsAlive()) {
          continue;
        }
        for (auto& hitId : tr.Hits()) {

          auto updateStrip = [&](int& strip) {
            if ((strip >= 0) && (strip != tr.Id())) {  // strip is used by other candidate
              const auto& other = fvTrackCandidates[strip];
              if (!other.IsAlive() && tr.IsBetterThan(other)) {
                strip = tr.Id();
              }
              else {
                return false;
              }
            }
            else {
              strip = tr.Id();
            }
            return true;
          };

          const ca::Hit& h = wData.Hit(hitId);
          if (!updateStrip(fvHitKeyToTrack[h.FrontKey()])) {  // front  strip
            break;
          }
          if (!updateStrip(fvHitKeyToTrack[h.BackKey()])) {  // back  strip
            break;
          }
        }  // loop over hits
      }  // itrack

      // == Check if some suppressed candidates are still alive

      for (ca::Branch& tr : fvTrackCandidates) {
        if (tr.IsAlive()) {
          continue;
        }

        tr.SetAlive(true);
        for (const auto& hitIndex : tr.Hits()) {
          if (!tr.IsAlive()) break;
          const ca::Hit& h = wData.Hit(hitIndex);
          tr.SetAlive((fvHitKeyToTrack[h.FrontKey()] == tr.Id()) && (fvHitKeyToTrack[h.BackKey()] == tr.Id()));
        }

        if (!tr.IsAlive()) {  // release strips
          for (auto hitId : tr.Hits()) {
            const ca::Hit& h = wData.Hit(hitId);
            if (fvHitKeyToTrack[h.FrontKey()] == tr.Id()) {
              fvHitKeyToTrack[h.FrontKey()] = -1;
            }
            if (fvHitKeyToTrack[h.BackKey()] == tr.Id()) {
              fvHitKeyToTrack[h.BackKey()] = -1;
            }
          }
        }
        else {
          repeatCompetition = true;
        }
      }  // itrack

      if (!repeatCompetition) break;
    }  // competitions
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::SelectTracks(WindowData& wData)
  {
    for (Tindex iCandidate = 0; iCandidate < (Tindex) fvTrackCandidates.size(); ++iCandidate) {
      ca::Branch& tr = fvTrackCandidates[iCandidate];

      if constexpr (fDebug) {
        LOG(info) << "iter " << wData.CurrentIteration()->GetName() << ", track candidate " << iCandidate
                  << ": alive = " << tr.IsAlive();
      }
      if (!tr.IsAlive()) continue;

      if (wData.CurrentIteration()->GetExtendTracksFlag()) {
        if (tr.NofHits() < fParameters.GetNstationsActive()) {
          fTrackExtender.ExtendBranch(tr, wData);
        }
      }

      for (auto iHit : tr.Hits()) {
        const ca::Hit& hit = wData.Hit(iHit);
        /// used strips are marked
        wData.IsHitKeyUsed(hit.FrontKey()) = 1;
        wData.IsHitKeyUsed(hit.BackKey())  = 1;
        wData.RecoHitIndices().push_back(hit.Id());
      }
      Track t;
      t.fNofHits = tr.NofHits();
      wData.RecoTracks().push_back(t);
      if (0) {  // SG debug
        std::stringstream s;
        s << "store track " << iCandidate << " chi2= " << tr.Chi2() << "\n";
        s << " hits: ";
        for (auto hitLoc : tr.Hits()) {
          auto hitId = wData.Hit(hitLoc).Id();
          s << ca::Framework::GetMcTrackIdForCaHit(hitId) << " ";
        }
        LOG(info) << s.str();
      }
    }  // tracks
  }

  /** *************************************************************
       *                                                              *
       *     The routine performs recursive search for tracks         *
       *                                                              *
       *     I. Kisel                                    06.03.05     *
       *     I.Kulakov                                    2012        *
       *                                                              *
       ****************************************************************/

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::CAFindTrack(int ista, ca::Branch& best_tr, const ca::Triplet* curr_trip, ca::Branch& curr_tr,
                                      unsigned char min_best_l, ca::Branch* new_tr, WindowData& wData,
                                      TripletArray_t& vTriplets)
  /// recursive search for tracks
  /// input: @ista - station index, @&best_tr - best track for the privious call
  /// output: @&NCalls - number of function calls
  {

    if (curr_trip->GetLevel() == 0)  // the end of the track -> check and store
    {
      // -- finish with current track
      // add rest of hits
      const auto& hitM = wData.Hit(curr_trip->GetMHit());
      const auto& hitR = wData.Hit(curr_trip->GetRHit());

      if (!(wData.IsHitKeyUsed(hitM.FrontKey()) || wData.IsHitKeyUsed(hitM.BackKey()))) {
        curr_tr.AddHit(curr_trip->GetMHit());
      }
      if (!(wData.IsHitKeyUsed(hitR.FrontKey()) || wData.IsHitKeyUsed(hitR.BackKey()))) {
        curr_tr.AddHit(curr_trip->GetRHit());
      }

      //if( curr_tr.NofHits() < min_best_l - 1 ) return; // suppouse that only one hit can be added by extender
      // TODO: SZh 21.08.2023: Replace hard-coded value with a parameter
      int ndf = curr_tr.NofHits() * 2 - 5;

      if (ca::TrackingMode::kGlobal == fTrackingMode || ca::TrackingMode::kMcbm == fTrackingMode) {
        ndf = curr_tr.NofHits() * 2 - 4;
      }
      if (curr_tr.Chi2() > wData.CurrentIteration()->GetTrackChi2Cut() * ndf) {
        return;
      }

      // -- select the best
      if ((curr_tr.NofHits() > best_tr.NofHits())
          || ((curr_tr.NofHits() == best_tr.NofHits()) && (curr_tr.Chi2() < best_tr.Chi2()))) {
        best_tr = curr_tr;
      }
      return;
    }
    else  //MEANS level ! = 0
    {
      int N_neighbour = (curr_trip->GetNNeighbours());

      for (Tindex in = 0; in < N_neighbour; in++) {

        unsigned int ID      = curr_trip->GetFNeighbour() + in;
        unsigned int Station = TripletId2Station(ID);
        unsigned int Triplet = TripletId2Triplet(ID);

        const ca::Triplet& new_trip = vTriplets[Station][Triplet];

        fscal dchi2 = 0.;
        if (!checkTripletMatch(*curr_trip, new_trip, dchi2, wData)) continue;

        const auto& hitL = wData.Hit(new_trip.GetLHit());                                 // left hit of new triplet
        if (wData.IsHitKeyUsed(hitL.FrontKey()) || wData.IsHitKeyUsed(hitL.BackKey())) {  //hits are used
          //  no used hits allowed -> compare and store track
          if ((curr_tr.NofHits() > best_tr.NofHits())
              || ((curr_tr.NofHits() == best_tr.NofHits()) && (curr_tr.Chi2() < best_tr.Chi2()))) {
            best_tr = curr_tr;
          }
        }
        else {  // hit is not used: add the left hit from the new triplet to the current track

          unsigned char new_L = curr_tr.NofHits() + 1;
          fscal new_chi2      = curr_tr.Chi2() + dchi2;

          if constexpr (0) {  //SGtrd2d debug!!
            int mc01 = ca::Framework::GetMcTrackIdForWindowHit(curr_trip->GetLHit());
            int mc02 = ca::Framework::GetMcTrackIdForWindowHit(curr_trip->GetMHit());
            int mc03 = ca::Framework::GetMcTrackIdForWindowHit(curr_trip->GetRHit());
            int mc11 = ca::Framework::GetMcTrackIdForWindowHit(new_trip.GetLHit());
            int mc12 = ca::Framework::GetMcTrackIdForWindowHit(new_trip.GetMHit());
            int mc13 = ca::Framework::GetMcTrackIdForWindowHit(new_trip.GetRHit());

            if ((mc01 == mc02) && (mc02 == mc03)) {
              LOG(info) << " sta " << ista << " mc0 " << mc01 << " " << mc02 << " " << mc03 << " mc1 " << mc11 << " "
                        << mc12 << " " << mc13 << " chi2 " << curr_tr.Chi2() / (2 * (curr_tr.NofHits() + 2) - 4)
                        << " new " << new_chi2 / (2 * (new_L + 2) - 4);
              LOG(info) << "   hits " << curr_trip->GetLHit() << " " << curr_trip->GetMHit() << " "
                        << curr_trip->GetRHit() << " " << new_trip.GetLHit();
            }
          }

          int ndf = 2 * (new_L + 2) - 5;

          if (ca::TrackingMode::kGlobal == fTrackingMode) {  //SGtrd2d!!!
            ndf = 2 * (new_L + 2) - 4;
          }
          else if (ca::TrackingMode::kMcbm == fTrackingMode) {
            ndf = 2 * (new_L + 2) - 4;
          }
          else {
            ndf = new_L;  // TODO: SG:  2 * (new_L + 2) - 5
          }

          if (new_chi2 > wData.CurrentIteration()->GetTrackChi2Cut() * ndf) continue;

          // add new hit
          new_tr[ista] = curr_tr;
          new_tr[ista].AddHit(new_trip.GetLHit());

          const int new_ista = ista + new_trip.GetMSta() - new_trip.GetLSta();
          new_tr[ista].SetChi2(new_chi2);

          CAFindTrack(new_ista, best_tr, &new_trip, new_tr[ista], min_best_l, new_tr, wData, vTriplets);
        }  // add triplet to track
      }  // for neighbours
    }  // level = 0
  }

  void TrackFinderWindow::SetupGpuTrackFinder(GpuTrackFinderSetup& gpuTrackFinderSetup)
  {
    xpu::push_timer("SetupParametersTime");
    gpuTrackFinderSetup.SetupParameters();
    xpu::timings SetupParametersTime = xpu::pop_timer();

    xpu::push_timer("SetInputDataTime");
    gpuTrackFinderSetup.SetInputData();
    xpu::timings SetInputDataTime = xpu::pop_timer();

    xpu::push_timer("SetupMaterialMapTime");
    gpuTrackFinderSetup.SetupMaterialMap();
    xpu::timings SetupMaterialMapTime = xpu::pop_timer();

    if constexpr (constants::gpu::GpuTimeMonitoring) {
      LOG(info) << "GPU tracking :: SetupParameters: " << SetupParametersTime.wall() << " ms";
      LOG(info) << "GPU tracking :: SetInputData: " << SetInputDataTime.wall() << " ms";
      LOG(info) << "GPU tracking :: SetupMaterialMap: " << SetupMaterialMapTime.wall() << " ms";
    }
  }

  void TrackFinderWindow::ConstructTripletsGPU(WindowData& wData, GpuTrackFinderSetup& gpuTrackFinderSetup,
                                               int iteration)
  {
    xpu::push_timer("SetupGridTime");
    gpuTrackFinderSetup.SetupGrid();
    xpu::timings SetupGridTime = xpu::pop_timer();

    xpu::push_timer("SetupIterationDataTime");
    gpuTrackFinderSetup.SetupIterationData(iteration);
    xpu::timings SetupIterationDataTime = xpu::pop_timer();

    xpu::push_timer("RunGpuTrackingSetup");
    gpuTrackFinderSetup.RunGpuTrackingSetup();
    xpu::timings RunGpuTrackingSetup = xpu::pop_timer();

    std::optional<std::vector<std::array<unsigned int, 2>>> triplet_sort;
    if constexpr (constants::gpu::CpuSortTriplets) {
      xpu::push_timer("SortTriplets");
      triplet_sort.emplace();
      triplet_sort.value().reserve(gpuTrackFinderSetup.GetNofTriplets());

      for (unsigned int itr = 0; itr < gpuTrackFinderSetup.GetNofTriplets(); itr++) {
        if (gpuTrackFinderSetup.GetTriplet(itr).GetChi2() <= 0.) continue;
        const auto& triplet = gpuTrackFinderSetup.GetTriplet(itr);
        triplet_sort.value().push_back({triplet.GetLHit(), itr});
      }
      std::sort(
        triplet_sort.value().begin(), triplet_sort.value().end(),
        [](const std::array<unsigned int, 2>& a, const std::array<unsigned int, 2>& b) { return (a[0] < b[0]); });
      xpu::timings SortTriplets = xpu::pop_timer();

      if constexpr (constants::gpu::GpuTimeMonitoring) {
        LOG(info) << "GPU tracking :: SetupGrid: " << SetupGridTime.wall() << " ms";
        LOG(info) << "GPU tracking :: SetupIterationData: " << SetupIterationDataTime.wall() << " ms";
        LOG(info) << "GPU tracking :: RunGpuTracking: " << RunGpuTrackingSetup.wall() << " ms";
        LOG(info) << "GPU tracking :: SortTripletsCPU: " << SortTriplets.wall() << " ms";
      }
    }

    int ihitl_0 = -1;

    auto& [vHitFirstTriplet, vHitNofTriplets, vTriplets] = fTripletData;
    vHitFirstTriplet.reset(wData.Hits().size(), 0);  /// link hit -> first triplet { hit, *, *}
    vHitNofTriplets.reset(wData.Hits().size(), 0);   /// link hit ->n triplets { hit, *, *}

    // prepare triplet storage
    for (int j = 0; j < fParameters.GetNstationsActive(); j++) {
      const size_t nHitsStation = wData.TsHitIndices(j).size();
      vTriplets[j].clear();
      vTriplets[j].reserve(2 * nHitsStation);
    }

    int nTriplets = gpuTrackFinderSetup.GetNofTriplets();
    if constexpr (constants::gpu::CpuSortTriplets) {
      nTriplets = triplet_sort.value().size();
    }
    for (size_t itr = 0; itr < nTriplets; itr++) {
      int iSta, ihitl;
      if constexpr (constants::gpu::CpuSortTriplets) {
        iSta  = gpuTrackFinderSetup.GetTriplet(std::get<1>(triplet_sort.value()[itr])).GetLSta();
        ihitl = gpuTrackFinderSetup.GetTriplet(std::get<1>(triplet_sort.value()[itr])).GetLHit();
      }
      else {
        iSta  = gpuTrackFinderSetup.GetTriplet(itr).GetLSta();
        ihitl = gpuTrackFinderSetup.GetTriplet(itr).GetLHit();
      }
      const size_t oldSize = vTriplets[iSta].size();

      if constexpr (constants::gpu::CpuSortTriplets) {
        vTriplets[iSta].emplace_back(gpuTrackFinderSetup.GetTriplet(std::get<1>(triplet_sort.value()[itr])));
        if (ihitl == ihitl_0) {
          vHitNofTriplets[ihitl]++;
        }
        else {
          vHitFirstTriplet[ihitl] = PackTripletId(iSta, oldSize);
          vHitNofTriplets[ihitl]  = vTriplets[iSta].size() - oldSize;
        }
        ihitl_0 = ihitl;
      }
      else {
        if (gpuTrackFinderSetup.GetTriplet(itr).GetChi2() > 0.)
          vTriplets[iSta].emplace_back(gpuTrackFinderSetup.GetTriplet(itr));
        vHitFirstTriplet[ihitl] = PackTripletId(iSta, oldSize);
        vHitNofTriplets[ihitl]  = vTriplets[iSta].size() - oldSize;
      }
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  void TrackFinderWindow::GNNTrackFinder(WindowData& wData, const int iteration)
  {
    GraphConstructor graphConstructor(wData);

    // Argument to run classifier is:
    // 0 - Triplets as tracks, 1 - Candidates, 2 - Tracks
    switch (iteration) {
      case 0: graphConstructor.FindFastPrim(2); break;
      default: LOG(info) << "Unexpected iteration index: " << iteration; break;
    }

    // Pass tracks to next stage in pipeline
    graphConstructor.PrepareFinalTracks();
  }


}  // namespace cbm::algo::ca
