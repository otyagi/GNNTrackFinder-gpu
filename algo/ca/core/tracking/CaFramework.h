/* Copyright (C) 2007-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak, Valentina Akishina, Igor Kulakov [committer], Sergei Zharko */

#pragma once  // include this header only once per compilation unit

#include "CaBranch.h"
#include "CaCloneMerger.h"
#include "CaDefs.h"
#include "CaGrid.h"
#include "CaGridEntry.h"
#include "CaHit.h"
#include "CaInputData.h"
#include "CaParameters.h"
#include "CaSimd.h"
#include "CaStation.h"
#include "CaTimer.h"
#include "CaTimesliceHeader.h"
#include "CaTrack.h"
#include "CaTrackFinder.h"
#include "CaTrackingMonitor.h"
#include "CaTriplet.h"
#include "CaVector.h"
#include "CaWindowData.h"
#include "KfFramework.h"

#include <array>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>

namespace cbm::algo::ca
{
  class TripletConstructor;
  class Track;

  //namespace
  //{
  //  using cbm::algo::ca::Track;      // TMP
  //  using cbm::algo::ca::Vector;     // TMP
  //  using cbm::algo::ca::Iteration;  // TMP
  //}  // namespace

  // *******************************
  // ** Types definition (global) **
  // *******************************

  using CaStationsArray_t = std::array<ca::Station<fvec>, constants::size::MaxNstations>;
  using Tindex            = int;  // TODO: Replace with ca::HitIndex_t, if suitable

  /// Main class of CA track finder algorithm
  /// Class implements a clones merger algorithm for the CA track finder
  ///
  class Framework {
   public:
    // **********************************
    // ** Member functions declaration **
    // **********************************

    // ** Constructors and destructor

    /// Constructor
    Framework(){};

    /// Copy constructor
    Framework(const Framework&) = delete;

    /// Move constructor
    Framework(Framework&&) = delete;

    /// Copy assignment operator
    Framework& operator=(const Framework&) = delete;

    /// Move assignment operator
    Framework& operator=(Framework&&) = delete;

    /// Destructor
    ~Framework() = default;

    /// Sets Framework parameters object
    /// \param other - reference to the Parameters object
    void SetParameters(const Parameters<fvec>& other) { fParameters = other; }
    // TODO: remove it (S.Zharko)

    /// Gets a pointer to the Framework parameters object
    const Parameters<fvec>& GetParameters() const { return fParameters; }

    /// Receives input data
    void ReceiveInputData(InputData&& inputData);

    /// Receives tracking parameters
    void ReceiveParameters(Parameters<fvec>&& parameters);

    /// Gets pointer to input data object for external access
    const InputData& GetInputData() const { return fInputData; }

    int PackIndex(const int& a, const int& b, const int& c);

    int UnPackIndex(const int& i, int& a, int& b, int& c);

    /// \brief Sets a default particle mass for the track fit
    ///
    /// The function is used during the reconstruction in order to estimate the multiple scattering and energy loss
    /// \param mass Default particle mass
    void SetDefaultParticleMass(fscal mass) { fDefaultMass = mass; }

    /// Gets default particle mass
    /// \return particle mass
    fscal GetDefaultParticleMass() const { return fDefaultMass; }

    /// Gets default particle mass squared
    /// \return particle mass squared
    fscal GetDefaultParticleMass2() const { return fDefaultMass * fDefaultMass; }

    /// \brief Gets timeslice header
    const TimesliceHeader& GetTsHeader() const { return fTsHeader; }

    /*********************************************************************************************/ /**
   *                             ------  FUNCTIONAL PART ------
   ************************************************************************************************/


    void Init(const TrackingMode mode);

    void Finish();

    void PrintHits();

    /// \brief Gets monitor data
    const TrackingMonitorData& GetMonitorData() const { return fMonitorData; }

    /// \brief Sets monitor data
    void SetMonitorData(const TrackingMonitorData& monitorData) { fMonitorData = monitorData; }

    TrackingMode GetTrackingMode() { return fpTrackFinder->GetTrackingMode(); }

    const std::vector<ca::WindowData>& GetWData() { return fpTrackFinder->GetWData(); }

   public:
    void FindTracks()
    {
      std::tie(fRecoTracks, fRecoHits) = fpTrackFinder->FindTracks(fInputData, fTsHeader);
      return;
    };

    /// Gets number of stations before the pipe (MVD stations in CBM)
    // int GetNstationsBeforePipe() const { return fNstationsBeforePipe; }

    /// Get mc track ID for a hit (debug tool)
    static int GetMcTrackIdForCaHit(int iHit);

    /// Get mc track ID for a hit (debug tool)
    static int GetMcTrackIdForWindowHit(int iGridHit);

    // const CbmL1MCTrack* GetMcTrackForWindowHit(int iHit) const;

    /// \brief Sets number of threads
    void SetNofThreads(int nThreads)
    {
      fNofThreads = nThreads;
      assert(nThreads > 0);
      LOG(info) << "ca::Framework: number of threads is set to " << fNofThreads;
    }

    /// \brief Gets number of threads
    int GetNofThreads() const { return fNofThreads; }

   private:
    int fNstationsBeforePipe{0};                    ///< number of stations before pipe (MVD stations in CBM)
    fscal fDefaultMass{constants::phys::MuonMass};  ///< mass of the propagated particle [GeV/c2]
    TimesliceHeader fTsHeader;                      ///< current timeslice header

    // ***************************
    // ** Member variables list **
    // ***************************

    Parameters<fvec> fParameters;  ///< Object of Framework parameters class
    InputData fInputData;          ///< Tracking input data

    Vector<unsigned char> fvHitKeyFlags{
      "Framework::fvHitKeyFlags"};  ///< List of key flags: has been this hit or cluster already used

    TrackingMonitorData fMonitorData{};  ///< Tracking monitor data (statistics per call)

    int fNofThreads = 1;  ///< Number of threads to execute the track-finder
                          ///
    bool checkTripletMatch(const ca::Triplet& l, const ca::Triplet& r, fscal& dchi2) const;

    std::unique_ptr<ca::TrackFinder> fpTrackFinder;  ///< Track finder steer class for the entire time slice

   public:
    //   Vector<CaHitTimeInfo> fHitTimeInfo;

    double fCaRecoTime{0.};  // time of the track finder + fitter

    Vector<Track> fRecoTracks{"Framework::fRecoTracks"};       ///< reconstructed tracks
    Vector<ca::HitIndex_t> fRecoHits{"Framework::fRecoHits"};  ///< packed hits of reconstructed tracks

    fvec EventTime{0.f};
    fvec Err{0.f};

   public:
  } _fvecalignment;

}  // namespace cbm::algo::ca
