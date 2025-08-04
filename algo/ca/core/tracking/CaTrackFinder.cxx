/* Copyright (C) 2009-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Valentina Akishina, Ivan Kisel, Sergey Gorbunov [committer], Igor Kulakov, Sergei Zharko, Maksym Zyzak */

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

#include "CaTrackFinder.h"

#include "CaTimer.h"
#include "CaTrack.h"
#include "CaTriplet.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>


namespace cbm::algo::ca
{
  using constants::phys::ProtonMassD;
  using constants::phys::SpeedOfLightInv;
  using constants::phys::SpeedOfLightInvD;

  // -------------------------------------------------------------------------------------------------------------------
  //
  TrackFinder::TrackFinder(const ca::Parameters<fvec>& pars, const fscal mass, const ca::TrackingMode& mode,
                           TrackingMonitorData& monitorData, int nThreads, double& recoTime)
    : fParameters(pars)
    , fDefaultMass(mass)
    , fTrackingMode(mode)
    , fMonitorData(monitorData)
    , fvMonitorDataThread(nThreads)
    , fvWData(nThreads)
    , fNofThreads(nThreads)
    , fCaRecoTime(recoTime)
    , fvRecoTracks(nThreads)
    , fvRecoHitIndices(nThreads)
    , fWindowLength((ca::TrackingMode::kMcbm == mode) ? 500 : 10000)
  {
    assert(fNofThreads > 0);

    for (int iThread = 0; iThread < fNofThreads; ++iThread) {
      fvRecoTracks[iThread].SetName(std::string("TrackFinder::fvRecoTracks_") + std::to_string(iThread));
      fvRecoHitIndices[iThread].SetName(std::string("TrackFinder::fvRecoHitIndices_") + std::to_string(iThread));
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //CBM Level 1 4D Reconstruction
  //Finds tracks using the Cellular Automaton algorithm
  //
  TrackFinder::Output_t TrackFinder::FindTracks(const InputData& input, TimesliceHeader& tsHeader)
  {
    Output_t output;
    Vector<Track>& recoTracks        = output.first;   //reconstructed tracks
    Vector<ca::HitIndex_t>& recoHits = output.second;  //packed hits of reconstructed tracks

    if (input.GetNhits() < 1) {
      LOG(warn) << "No hits were passed to the ca::TrackFinder. Stopping the routine";
      return output;
    }

    //
    // The main CaTrackFinder routine
    // It splits the input data into sub-timeslices
    // and runs the track finder over the sub-slices
    //
    fMonitorData.StartTimer(ETimer::Tracking);
    fMonitorData.StartTimer(ETimer::PrepareTimeslice);
    fMonitorData.IncrementCounter(ECounter::TrackingCall);
    fMonitorData.IncrementCounter(ECounter::RecoHit, input.GetNhits());

    auto timerStart = std::chrono::high_resolution_clock::now();

    auto& wDataThread0 = fvWData[0];  // NOTE: Thread 0 must be always defined

    // ----- Reset data arrays -----------------------------------------------------------------------------------------

    wDataThread0.HitKeyFlags().reset(input.GetNhitKeys(), 0);

    fHitTimeInfo.reset(input.GetNhits());

    // TODO: move these values to Parameters namespace (S.Zharko)

    // length of sub-TS
    const fscal minProtonMomentum = 0.1;
    const fscal preFactor         = sqrt(1. + ProtonMassD * ProtonMassD / (minProtonMomentum * minProtonMomentum));
    const fscal targX             = fParameters.GetTargetPositionX()[0];
    const fscal targY             = fParameters.GetTargetPositionY()[0];
    const fscal targZ             = fParameters.GetTargetPositionZ()[0];

    fStatTsStart    = std::numeric_limits<fscal>::max();  // end time of the TS
    fStatTsEnd      = 0.;                                 // end time of the TS
    fStatNhitsTotal = 0;

    // calculate possible event time for the hits (fHitTimeInfo array)
    for (int iStream = 0; iStream < input.GetNdataStreams(); ++iStream) {

      fscal maxTimeBeforeHit = std::numeric_limits<fscal>::lowest();
      const int nStreamHits  = input.GetStreamNhits(iStream);
      fStatNhitsTotal += nStreamHits;

      for (int ih = 0; ih < nStreamHits; ++ih) {

        ca::HitIndex_t caHitId      = input.GetStreamStartIndex(iStream) + ih;
        const ca::Hit& h            = input.GetHit(caHitId);
        const ca::Station<fvec>& st = fParameters.GetStation(h.Station());
        const fscal dx              = h.X() - targX;
        const fscal dy              = h.Y() - targY;
        const fscal dz              = h.Z() - targZ;
        const fscal l               = sqrt(dx * dx + dy * dy + dz * dz);
        const fscal timeOfFlightMin = l * SpeedOfLightInv;
        const fscal timeOfFlightMax = 1.5 * l * preFactor * SpeedOfLightInvD;
        const fscal dt              = h.RangeT();
        // TODO: Is it possible, that the proton mass selection affects the search of heavier particles?

        CaHitTimeInfo& info = fHitTimeInfo[caHitId];
        info.fEventTimeMin  = st.timeInfo ? (h.T() - dt - timeOfFlightMax) : -1.e10;
        info.fEventTimeMax  = st.timeInfo ? (h.T() + dt - timeOfFlightMin) : 1.e10;

        // NOTE: if not a MT part, use wDataThread0.IsHitKeyUsed, it will be later copied to other threads
        if (info.fEventTimeMin > 500.e6 || info.fEventTimeMax < -500.) {  // cut hits with bogus start time > 500 ms
          wDataThread0.IsHitKeyUsed(h.FrontKey()) = 1;
          wDataThread0.IsHitKeyUsed(h.BackKey())  = 1;
          LOG(error) << "CATrackFinder: skip bogus hit " << h.ToString();
          continue;
        }
        maxTimeBeforeHit       = std::max(maxTimeBeforeHit, info.fEventTimeMax);
        info.fMaxTimeBeforeHit = maxTimeBeforeHit;
        fStatTsStart           = std::min(fStatTsStart, info.fEventTimeMax);
        fStatTsEnd             = std::max(fStatTsEnd, info.fEventTimeMin);
      }

      fscal minTimeAfterHit = std::numeric_limits<fscal>::max();
      // loop in the reverse order to fill CaHitTimeInfo::fMinTimeAfterHit fields

      for (int ih = nStreamHits - 1; ih >= 0; --ih) {
        ca::HitIndex_t caHitId = input.GetStreamStartIndex(iStream) + ih;
        const ca::Hit& h       = input.GetHit(caHitId);
        if (wDataThread0.IsHitKeyUsed(h.FrontKey()) || wDataThread0.IsHitKeyUsed(h.BackKey())) {
          continue;
        }  // the hit is skipped
        CaHitTimeInfo& info   = fHitTimeInfo[caHitId];
        minTimeAfterHit       = std::min(minTimeAfterHit, info.fEventTimeMin);
        info.fMinTimeAfterHit = minTimeAfterHit;
      }

      if (0) {
        static int tmp = 0;
        if (tmp < 10000) {
          tmp++;
          LOG(warning) << "\n\n stream " << iStream << " hits " << nStreamHits << "\n\n";
          for (int ih = 0; (ih < nStreamHits) && (tmp < 10000); ++ih) {
            ca::HitIndex_t caHitId = input.GetStreamStartIndex(iStream) + ih;
            const ca::Hit& h       = input.GetHit(caHitId);
            if (wDataThread0.IsHitKeyUsed(h.FrontKey()) || wDataThread0.IsHitKeyUsed(h.BackKey())) {
              continue;
            }  // the hit is skipped
            CaHitTimeInfo& info = fHitTimeInfo[caHitId];
            if (h.Station() < 4) {
              tmp++;
              LOG(warning) << " hit sta " << h.Station() << " stream " << iStream << " time " << h.T() << " event time "
                           << info.fEventTimeMin << " .. " << info.fEventTimeMax << " max time before hit "
                           << info.fMaxTimeBeforeHit << " min time after hit " << info.fMinTimeAfterHit;
            }
          }
        }
      }
    }
    // all hits belong to one sub-timeslice; 1 s is the maximal length of the TS
    fStatTsEnd = std::clamp(fStatTsEnd, fStatTsStart, fStatTsStart + 1.e9f);

    LOG(debug) << "CA tracker process time slice " << fStatTsStart * 1.e-6 << " -- " << fStatTsEnd * 1.e-6
               << " [ms] with " << fStatNhitsTotal << " hits";

    int nWindows = static_cast<int>((fStatTsEnd - fStatTsStart) / fWindowLength) + 1;
    if (nWindows < 1) {  // Situation, when fStatTsEnd == fStatTsStart
      nWindows = 1;
    }

    // int nWindowsThread = nWindows / fNofThreads;
    // LOG(info) << "CA: estimated number of time windows: " << nWindows;

    std::vector<std::pair<fscal, fscal>> vWindowRangeThread(fNofThreads);
    {  // Estimation of number of hits in time windows
      //Timer time;
      //time.Start();
      const HitIndex_t nHitsTot = input.GetNhits();
      const int nSt             = fParameters.GetNstationsActive();

      // Count number of hits per window and station
      std::vector<HitIndex_t> nHitsWindowSta(nWindows * nSt, 0);

      for (HitIndex_t iHit = 0; iHit < nHitsTot; ++iHit) {
        const auto& hit  = input.GetHit(iHit);
        const auto& info = fHitTimeInfo[iHit];
        int iWindow      = static_cast<int>((info.fEventTimeMin - fStatTsStart) / fWindowLength);
        if (iWindow < 0) {
          iWindow = 0;
        }
        if (iWindow >= nWindows) {
          LOG(error) << "ca: Hit out of range: iHit = " << iHit << ", min. event time = " << info.fEventTimeMin * 1.e-6
                     << " ms, window = " << iWindow;
          continue;
        }
        ++nHitsWindowSta[hit.Station() + iWindow * nSt];
      }

      // Remove hits from the "monster events"
      if (ca::TrackingMode::kMcbm == fTrackingMode) {
        const auto maxNofHitsSta = static_cast<HitIndex_t>(50 * fWindowLength / 1.e3);
        for (auto& content : nHitsWindowSta) {
          if (content > maxNofHitsSta) {
            content = 0;
          }
        }
      }

      // Integrate number of hits per window
      std::vector<HitIndex_t> nHitsWindow;
      nHitsWindow.reserve(nWindows);
      HitIndex_t nHitsCollected = 0;
      for (auto it = nHitsWindowSta.begin(); it != nHitsWindowSta.end(); it += nSt) {
        nHitsCollected = nHitsWindow.emplace_back(std::accumulate(it, it + nSt, nHitsCollected));
      }

      // Get time range for threads
      const HitIndex_t nHitsPerThread = nHitsCollected / fNofThreads;
      auto windowIt                   = nHitsWindow.begin();
      vWindowRangeThread[0].first     = fStatTsStart;
      for (int iTh = 1; iTh < fNofThreads; ++iTh) {
        windowIt                           = std::lower_bound(windowIt, nHitsWindow.end(), iTh * nHitsPerThread);
        const size_t iWbegin               = std::distance(nHitsWindow.begin(), windowIt) + 1;
        vWindowRangeThread[iTh].first      = fStatTsStart + iWbegin * fWindowLength;
        vWindowRangeThread[iTh - 1].second = vWindowRangeThread[iTh].first;
      }
      vWindowRangeThread[fNofThreads - 1].second = fStatTsEnd;

      //time.Stop();
      //LOG(info) << "Thread boarders estimation time: " << time.GetTotalMs() << " ms";
      LOG(debug) << "Fraction of hits from monster events: "
                 << static_cast<double>(nHitsTot - nHitsCollected) / nHitsTot;
    }

    // cut data into sub-timeslices and process them one by one
    //for (int iThread = 0; iThread < fNofThreads; ++iThread) {
    //  vWindowStartThread[iThread] = fStatTsStart + iThread * nWindowsThread * fWindowLength;
    //  vWindowEndThread[iThread] =
    //    vWindowStartThread[iThread] + nWindowsThread * fWindowLength;
    //}

    for (int iThread = 0; iThread < fNofThreads; ++iThread) {
      auto& entry  = vWindowRangeThread[iThread];
      double start = entry.first * 1.e-6;
      double end   = entry.second * 1.e-6;
      LOG(debug) << "Thread: " << iThread << " from " << start << " ms  to " << end << " ms (delta = " << end - start
                 << " ms)";
    }

    // Statistics for monitoring
    std::vector<int> vStatNwindows(fNofThreads), vStatNhitsProcessed(fNofThreads);

    fMonitorData.StopTimer(ETimer::PrepareTimeslice);
    // Save tracks
    if (fNofThreads == 1) {
      this->FindTracksThread(input, 0, std::ref(vWindowRangeThread[0]), std::ref(vStatNwindows[0]),
                             std::ref(vStatNhitsProcessed[0]));
      fMonitorData.StartTimer(ETimer::StoreTracksFinal);
      recoTracks = std::move(fvRecoTracks[0]);
      recoHits   = std::move(fvRecoHitIndices[0]);
      fMonitorData.StopTimer(ETimer::StoreTracksFinal);
    }
    else {
      std::vector<std::thread> vThreadList;
      vThreadList.reserve(fNofThreads);
      for (int iTh = 0; iTh < fNofThreads; ++iTh) {
        vThreadList.emplace_back(&TrackFinder::FindTracksThread, this, std::ref(input), iTh,
                                 std::ref(vWindowRangeThread[iTh]), std::ref(vStatNwindows[iTh]),
                                 std::ref(vStatNhitsProcessed[iTh]));
      }
      for (auto& th : vThreadList) {
        if (th.joinable()) {
          th.join();
        }
      }
      fMonitorData.StartTimer(ETimer::StoreTracksFinal);
      auto Operation  = [](size_t acc, const auto& v) { return acc + v.size(); };
      int nRecoTracks = std::accumulate(fvRecoTracks.begin(), fvRecoTracks.end(), 0, Operation);
      int nRecoHits   = std::accumulate(fvRecoHitIndices.begin(), fvRecoHitIndices.end(), 0, Operation);
      recoTracks.reserve(nRecoTracks);
      recoHits.reserve(nRecoHits);
      for (int iTh = 0; iTh < fNofThreads; ++iTh) {
        recoTracks.insert(recoTracks.end(), fvRecoTracks[iTh].begin(), fvRecoTracks[iTh].end());
        recoHits.insert(recoHits.end(), fvRecoHitIndices[iTh].begin(), fvRecoHitIndices[iTh].end());
      }
      fMonitorData.StopTimer(ETimer::StoreTracksFinal);
    }

    fMonitorData.IncrementCounter(ECounter::RecoTrack, recoTracks.size());
    fMonitorData.IncrementCounter(ECounter::RecoHitUsed, recoHits.size());

    auto timerEnd = std::chrono::high_resolution_clock::now();
    fCaRecoTime   = (double) (std::chrono::duration<double>(timerEnd - timerStart).count());

    // Add thread monitors to the main monitor
    for (auto& monitor : fvMonitorDataThread) {
      fMonitorData.AddMonitorData(monitor, true);
      //fMonitorData.AddMonitorData(monitor);
      monitor.Reset();
    }

    const int statNhitsProcessedTotal = std::accumulate(vStatNhitsProcessed.begin(), vStatNhitsProcessed.end(), 0);
    const int statNwindowsTotal       = std::accumulate(vStatNwindows.begin(), vStatNwindows.end(), 0);

    // Filling TS headear
    tsHeader.Start() = fStatTsStart;
    tsHeader.End()   = fStatTsEnd;

    fMonitorData.StopTimer(ETimer::Tracking);

    LOG(debug) << "CA tracker: time slice finished. Reconstructed " << recoTracks.size() << " tracks with "
               << recoHits.size() << " hits. Processed " << statNhitsProcessedTotal << " hits in " << statNwindowsTotal
               << " time windows. Reco time " << fCaRecoTime / 1.e9 << " s";
    return output;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void TrackFinder::FindTracksThread(const InputData& input, int iThread, std::pair<fscal, fscal>& windowRange,
                                     int& statNwindows, int& statNhitsProcessed)
  {
    //std::stringstream filename;
    //filename << "./dbg_caTrackFinder::FindTracksThread_" << iThread << ".txt";
    //std::ofstream out(filename.str());
    Timer timer;
    timer.Start();

    auto& monitor = fvMonitorDataThread[iThread];
    monitor.StartTimer(ETimer::TrackingThread);
    monitor.StartTimer(ETimer::PrepareThread);

    // Init vectors
    auto& tracks     = fvRecoTracks[iThread];
    auto& hitIndices = fvRecoHitIndices[iThread];
    auto& wData      = fvWData[iThread];
    {
      const int nStations          = fParameters.GetNstationsActive();
      const size_t nHitsTot        = input.GetNhits();
      const size_t nHitsExpected   = 2 * nHitsTot;
      const size_t nTracksExpected = 2 * nHitsTot / nStations;
      tracks.clear();
      tracks.reserve(nTracksExpected / fNofThreads);
      hitIndices.clear();
      hitIndices.reserve(nHitsExpected / fNofThreads);
      if (iThread != 0) {
        wData.HitKeyFlags() = fvWData[0].HitKeyFlags();
      }
      for (int iS = 0; iS < nStations; ++iS) {
        wData.TsHitIndices(iS).clear();
        wData.TsHitIndices(iS).reserve(nHitsTot);
      }
    }

    // Begin and end index of hit-range for streams
    std::vector<std::pair<HitIndex_t, HitIndex_t>> streamHitRanges(input.GetNdataStreams(), {0, 0});

    // Define first hit, skip all the hits, which are before the first window
    for (size_t iStream = 0; iStream < streamHitRanges.size(); ++iStream) {
      auto& range  = streamHitRanges[iStream];
      range.first  = input.GetStreamStartIndex(iStream);
      range.second = input.GetStreamStopIndex(iStream);

      for (HitIndex_t caHitId = range.first; caHitId < range.second; ++caHitId) {
        const ca::Hit& h = input.GetHit(caHitId);
        if (wData.IsHitKeyUsed(h.FrontKey()) || wData.IsHitKeyUsed(h.BackKey())) {
          continue;
        }
        const CaHitTimeInfo& info = fHitTimeInfo[caHitId];
        if (info.fMaxTimeBeforeHit < windowRange.first) {
          range.first = caHitId + 1;
        }
        if (info.fMinTimeAfterHit > windowRange.second) {
          range.second = caHitId;
        }
      }
    }

    int statLastLogTimeChunk = -1;

    // Track finder algorithm for the time window
    ca::TrackFinderWindow trackFinderWindow(fParameters, fDefaultMass, fTrackingMode, monitor);
    trackFinderWindow.InitTimeslice(input.GetNhitKeys());

    monitor.StopTimer(ETimer::PrepareThread);

    while (true) {
      monitor.IncrementCounter(ECounter::SubTS);
      // select the sub-slice hits
      for (int iS = 0; iS < fParameters.GetNstationsActive(); ++iS) {
        wData.TsHitIndices(iS).clear();
      }
      bool areUntouchedDataLeft = false;  // is the whole TS processed

      // TODO: SG: skip empty regions and start the subslice with the earliest hit

      statNwindows++;
      //out << statNwindows << ' ';

      monitor.StartTimer(ETimer::PrepareWindow);

      for (auto& range : streamHitRanges) {
        for (HitIndex_t caHitId = range.first; caHitId < range.second; ++caHitId) {
          const ca::Hit& h = input.GetHit(caHitId);
          if (wData.IsHitKeyUsed(h.FrontKey()) || wData.IsHitKeyUsed(h.BackKey())) {
            // the hit is already reconstructed
            continue;
          }
          const CaHitTimeInfo& info = fHitTimeInfo[caHitId];
          if (info.fEventTimeMax < windowRange.first) {
            // the hit belongs to previous sub-slices
            continue;
          }
          if (info.fMinTimeAfterHit > windowRange.first + fWindowLength) {
            // this hit and all later hits are out of the sub-slice
            areUntouchedDataLeft = true;
            break;
          }
          if (info.fEventTimeMin > windowRange.first + fWindowLength) {
            // the hit is too late for the sub slice
            areUntouchedDataLeft = true;
            continue;
          }

          // the hit belongs to the sub-slice
          wData.TsHitIndices(h.Station()).push_back(caHitId);
          if (info.fMaxTimeBeforeHit < windowRange.first + fWindowLength) {
            range.first = caHitId + 1;  // this hit and all hits before are before the overlap
          }
        }
      }

      //out << statNwindowHits << ' ';
      //if (statNwindowHits == 0) {  // Empty window
      //  monitor.StopTimer(ETimer::PrepareWindow);
      //  out << 0 << ' ' << 0 << ' ' << 0 << '\n';
      //  continue;
      //}

      if (ca::TrackingMode::kMcbm == fTrackingMode) {
        // cut at 50 hits per station per 1 us.
        int maxStationHits = (int) (50 * fWindowLength / 1.e3);
        for (int ista = 0; ista < fParameters.GetNstationsActive(); ++ista) {
          int nHitsSta = static_cast<int>(wData.TsHitIndices(ista).size());
          if (nHitsSta > maxStationHits) {
            wData.TsHitIndices(ista).clear();
          }
        }
      }

      int statNwindowHits = 0;
      for (int ista = 0; ista < fParameters.GetNstationsActive(); ++ista) {
        statNwindowHits += wData.TsHitIndices(ista).size();
      }
      statNhitsProcessed += statNwindowHits;

      // print the LOG for every 10 ms of data processed
      if constexpr (0) {
        int currentChunk = (int) ((windowRange.first - fStatTsStart) / 10.e6);
        if (!areUntouchedDataLeft || currentChunk > statLastLogTimeChunk) {
          statLastLogTimeChunk = currentChunk;
          double dataRead = 100. * (windowRange.first + fWindowLength - fStatTsStart) / (fStatTsEnd - fStatTsStart);
          if (dataRead > 100.) {
            dataRead = 100.;
          }
          LOG(debug) << "CA tracker process sliding window N " << statNwindows << ": time " << windowRange.first / 1.e6
                     << " ms + " << fWindowLength / 1.e3 << " us) with " << statNwindowHits << " hits. "
                     << " Processing " << dataRead << " % of the TS time and "
                     << 100. * statNhitsProcessed / fStatNhitsTotal << " % of TS hits."
                     << " Already reconstructed " << tracks.size() << " tracks on thread #" << iThread;
        }
      }

      //out << statNwindowHits << ' ';
      monitor.StopTimer(ETimer::PrepareWindow);

      //Timer trackingInWindow;  //DBG
      //trackingInWindow.Start();
      monitor.StartTimer(ETimer::TrackingWindow);
      trackFinderWindow.CaTrackFinderSlice(input, wData);
      monitor.StopTimer(ETimer::TrackingWindow);
      //trackingInWindow.Stop();
      //out << trackingInWindow.GetTotalMs() << ' ';

      // save reconstructed tracks with no hits in the overlap region
      //if (windowRange.first > 13.23e6 && windowRange.first < 13.26e6) {
      windowRange.first += fWindowLength;
      // we should add hits from reconstructed but not stored tracks to the new sub-timeslice
      // we do it in a simple way by extending the tsStartNew
      // TODO: only add those hits from the region before tsStartNew that belong to the not stored tracks
      //out << fvWData[iThread].RecoHitIndices().size() << ' ';
      //out << fvWData[iThread].RecoTracks().size() << '\n';

      monitor.StartTimer(ETimer::StoreTracksWindow);
      auto trackFirstHit = wData.RecoHitIndices().begin();

      for (const auto& track : wData.RecoTracks()) {

        const bool isTrackCompletelyInOverlap =
          std::all_of(trackFirstHit, trackFirstHit + track.fNofHits, [&](int caHitId) {
            CaHitTimeInfo& info = fHitTimeInfo[caHitId];
            return info.fEventTimeMax >= windowRange.first;
          });

        // Don't save tracks from the overlap region, since they might have additional hits in the next subslice.
        // Don't reject tracks in the overlap when no more data are left
        const bool useFlag = !isTrackCompletelyInOverlap || !areUntouchedDataLeft;

        for (int i = 0; i < track.fNofHits; i++) {
          const int caHitId                = *(trackFirstHit + i);
          const auto& h                    = input.GetHit(caHitId);
          wData.IsHitKeyUsed(h.FrontKey()) = static_cast<int>(useFlag);
          wData.IsHitKeyUsed(h.BackKey())  = static_cast<int>(useFlag);

          if (useFlag) {
            hitIndices.push_back(caHitId);
          }
        }
        if (useFlag) {
          tracks.push_back(track);
        }

        trackFirstHit += track.fNofHits;
      }  // sub-timeslice tracks
      monitor.StopTimer(ETimer::StoreTracksWindow);

      if (windowRange.first > windowRange.second) {
        break;
      }
      if (!areUntouchedDataLeft) {
        break;
      }
    }  // while(true)
    monitor.StopTimer(ETimer::TrackingThread);
    //timer.Stop();
    //LOG(info) << "CA: finishing tracking on thread " << iThread << " (time: " << timer.GetTotalMs() << " ms, "
    //          << "hits processed: " << statNhitsProcessed << ", "
    //          << "hits used: " << hitIndices.size() << ')';
  }
}  // namespace cbm::algo::ca
