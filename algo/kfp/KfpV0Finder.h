/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0Finder.h
/// \date   01.02.2025
/// \brief  A V0 finding algorithm
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CbmEventTriggers.h"
#include "KFParticleTopoReconstructor.h"
#include "global/RecoResults.h"
#include "kfp/KfpV0FinderMonitor.h"

#include <limits>
#include <memory>


namespace cbm::algo::kfp
{
  /// \class V0Finder
  /// \brief A V0-finding algorithm
  class V0Finder {
   public:
    /// \struct ParticleInfo
    /// \brief  A structure to keep temporary PID information for tracks
    struct ParticleInfo {
      double fMass{std::numeric_limits<double>::quiet_NaN()};  //< Estimated mass of the particle [GeV/c2]
      double fDca{std::numeric_limits<double>::quiet_NaN()};   //< DCA to the origin [cm]
      double fBeta{std::numeric_limits<double>::quiet_NaN()};  //< Speed of the particle [c]
      double fQp{std::numeric_limits<double>::quiet_NaN()};    //< q/p of the particle [GeV^-1]
      int32_t fPdg{V0Finder::kUndefPdg};                       //< PDG code
      int32_t fCharge{0};                                      //< Charge of the particle
      bool fbSelected{false};                                  //< The track was selected
    };

    //* Framework and physical constants (public)
    static constexpr double kPionMass{0.13957039};      ///< Pion mass [GeV/c2]
    static constexpr double kProtonMass{0.938272088};   ///< Proton mass [GeV/c2]
    static constexpr double kSpeedOfLight{29.9792458};  ///< Speed of light [cm/ns]
    static constexpr int32_t kUndefPdg{-2};             ///< PDG for tracks, which PID cannot be inferred

    /// \brief Default constructor
    V0Finder() = default;

    /// \brief Copy constructor
    V0Finder(const V0Finder&) = delete;

    /// \brief Move constructor
    V0Finder(V0Finder&&) = delete;

    /// \brief Destructor
    ~V0Finder() = default;

    /// \brief Copy assignment operator
    V0Finder& operator=(const V0Finder&) = delete;

    /// \brief Move assignment operator
    V0Finder& operator=(V0Finder&&) = delete;

    /// \brief  Adds particle to reconstruction list
    /// \param  pdg  A PDG code of the particle to be reconstructed
    void AddDecayToReconstructionList(int pdg) { GetKFParticleFinder()->AddDecayToReconstructionList(pdg); }

    /// \brief  Gets monitor data
    const V0FinderMonitorData_t& GetEventMonitor() const { return fEventMonitor; }

    /// \brief  Gets origin
    const std::array<float, 3>& GetOrigin() const { return fOrigin; }

    /// \brief  Mutable access to the KfParticleFinder of the run topology reconstructor
    KFParticleFinder* GetKFParticleFinder() { return fpTopoReconstructor->GetKFParticleFinder(); }

    /// \brief  Constant access to the KfParticleFinder of the run topology reconstructor
    const KFParticleFinder* GetKFParticleFinder() const { return fpTopoReconstructor->GetKFParticleFinder(); }

    /// \brief  Gets a vector of particle info
    const std::vector<ParticleInfo>& GetParticleInfo() const { return fvParticleInfo; }

    /// \brief  Gets selected t0
    /// \note   NaN, if Lambda-candidate was not found
    double GetSelectedT0() const { return fSelectedT0; }

    /// \brief  Gets indices of selected tracks
    const std::vector<uint32_t>& GetSelectedTrackIds() const { return fvSelectedTrackIds; }

    /// \brief  Gets found t0s
    const std::vector<double>& GetT0s() const { return fvT0s; }

    /// \brief  Accessor to topology reconstructor
    const std::unique_ptr<KFParticleTopoReconstructor>& GetTopoReconstructor() const { return fpTopoReconstructor; }

    /// \brief  Gets track parameters
    const auto& GetTrackAssignedParams() const { return fvTrackParam; }

    /// \brief Initializes the instance (called in the beginning of the run)
    void Init();

    /// \brief  Processes a reconstructed data sample, returns a collection of fired triggers
    CbmEventTriggers Process(const RecoResults& recoEvent);

    /// \brief  Sets an address of a reference BMON diamond
    /// \param  iPartition  An index of the BMON hit partition: used to selected the hits from the particular config
    void SetBmonPartitionIndex(int iPartition) { fBmonPartitionIndex = iPartition; }

    /// \param  dca  DCA [cm]
    void SetMinPionDca(double dca) { fMinPionDca = dca; }

    /// \brief  Sets minimal proton DCA to primary vertex
    /// \param  dca  DCA [cm]
    void SetMinProtonDca(double dca) { fMinProtonDca = dca; }

    /// \brief  Sets origin
    /// \param  x  X-coordinate of the origin [cm]
    /// \param  y  Y-coordinate of the origin [cm]
    /// \param  z  Z-coordinate of the origin [cm]
    // FIXME: for now origin is defined as the target center, later it can be changed
    void SetOrigin(double x, double y, double z) { fOrigin = {float(x), float(y), float(z)}; }

    /// \brief  Sets minimal pion DCA to primary vertex
    /// \brief  Sets pion velocity range
    /// \param  vMin  Minimal velocity [cm/ns]
    /// \param  vMax  Maximal velocity [cm/ns]
    void SetPionVelocityRange(double vMin, double vMax)
    {
      fMinBetaPion = vMin / kSpeedOfLight;
      fMaxBetaPion = vMax / kSpeedOfLight;
    }

    /// \brief  Sets proton velocity range
    /// \param  vMin  Minimal velocity [cm/ns]
    /// \param  vMax  Maximal velocity [cm/ns]
    void SetProtonVelocityRange(double vMin, double vMax)
    {
      fMinBetaProton = vMin / kSpeedOfLight;
      fMaxBetaProton = vMax / kSpeedOfLight;
    }

    /// \brief  Sets the assigned PDG for primary particles
    /// \param  pdg  PDG code of the particle
    void SetPrimaryAssignedPdg(int pdg) { fPrimaryAssignedPdg = pdg; }

    /// \brief  Assignes an uncertainty to the momentum measurement
    /// \param  uncertainty  Relative uncertainty ( = sqrt(var(q/p)) / (q/p))
    void SetQpAssignedUncertainty(double uncertainty) { fQpAssignedUncertainty = uncertainty; }

    /// \brief  Sets an offset to t0
    /// \param  offset  An offset [ns]
    void SetTzeroOffset(double offset) { fTzeroOffset = offset; }

    /// \brief  Sets cut on the distance to the primary vertex from the decay vertex
    /// \param  cut  Cut value [cm]
    void SetLCut(float cut) { GetKFParticleFinder()->SetLCut(cut); }

    /// \brief  Sets cut on \f$\chi^2_{prim}\f$ of each track for 2-daughter decays
    /// \param  cut  Cut value
    void SetChiPrimaryCut2D(float cut) { GetKFParticleFinder()->SetChiPrimaryCut2D(cut); }

    /// \brief  Sets cut on \f$\chi^2_{geo}\f$ for 2-daughter decays
    /// \param  cut  Cut value
    void SetChi2Cut2D(float cut) { GetKFParticleFinder()->SetChi2Cut2D(cut); }

    /// \brief  Sets cut on \f$l/\Delta l\f$ for 2-daughter decays
    /// \param  cut  Cut value
    void SetLdLCut2D(float cut) { GetKFParticleFinder()->SetLdLCut2D(cut); }

   private:
    /// \brief  Assigns momentum based on the TOF measurement
    /// \param[in]  tofHits      Tof hits container
    /// \param[in]  tofHitIds    Tof hit indices, used by the track
    /// \param[in]  t0           A t0 value
    /// \param[inout]  pidInfo   PID information for the track
    /// \return  true  A physically reasonable momentum assigned
    /// \return  false Momentum was not assigned, because it was nonphysical
    /* clang-format off */
    bool AssignMomentum(const PartitionedVector<tof::Hit>& tofHits, 
                        const std::vector<RecoResults::HitId_t>& tofHitIds,
                        double t0, 
                        ParticleInfo& pidInfo);
    /* clang-format on */

    /// \brief  Assigns PID info based on the estimated DCA
    /// \param  dca  DCA of track to origin
    /// \return (mass, charge, pid)
    void AssignPid(ParticleInfo& info);

    /// \brief  Collects a vector of DCA
    /// \param  recoEvent  Instance of a reconstructed event
    void CollectDca(const RecoResults& recoEvent);

    /// \brief  Collects T0 values among the BMON hits
    /// \param  bmonHits  A span of BMON hits
    ///
    /// If multiple T0-s are found, the routine will run multiple times, until V0-candidates are found
    void CollectT0(gsl::span<const bmon::Hit> bmonHits);

    /// \brief  Estimates speed of particle, using TOF measurement
    /// \param  tofHit   A TOF hit
    /// \param  t0       An t0 value
    /// \return Speed of particle [c]
    double EstimateBeta(const tof::Hit& tofHit, double t0) const;

    /// \brief  Estimate DCA of a track to origin
    /// \param  fst  first  STS hit
    /// \param  snd  second STS hit
    /// \return dca [cm]
    double EstimateDca(const sts::Hit& fst, const sts::Hit& snd) const;

    /// \brief  Tries to find V0-candidates for a given t0
    /// \param  recoEvent  Instance of a reconstructed event
    /// \param  t0         An estimated t0 value
    /// \return true  V0-candidates found
    /// \return false V0-candidates not found
    bool FindV0Candidates(const RecoResults& recoEvent, double t0);

    /// \brief  Initializes copies of track parameter vectors
    /// \param  tracks  A container of tracks
    void InitTrackParamVectors(const ca::Vector<ca::Track>& tracks);

    /// \brief   Makes a KF vertex
    /// \param   r  coordinates of PV [cm]
    static KFVertex MakeKfpPrimaryVertex(const std::array<float, 3>& r);

    /// \brief  Applies selection cut on the track
    /// \param  particleInfo  Particle collected information
    /// \return true  Track is selected
    /// \return false Track is rejected
    bool SelectTrack(const ParticleInfo& particleInfo) const;

    /// \brief  Sets KFP track parameters
    /// \param[inout]  kfpTrkVector  Reference to the KFP track vector
    /// \param[in]     iKfpTrk       Index of the KFP track
    /// \param[in]     iCaTrk        Index of the CA track
    /// \param[in]     trkParam      Track parameters
    /// \param[in]     particleInfo  Particle information
    void SetKfpTrackParameters(KFPTrackVector& kfpTrkVector, uint32_t iKfpTrk, uint32_t iCaTrk,
                               const ca::Track::TrackParam_t& trkParam, const ParticleInfo& particleInfo) const;


    //* Framework and physical constants (private)
    static constexpr bool kUseAverageSpeed{false};  ///< If an average speed of tof hits is used

    V0FinderMonitorData_t fEventMonitor;  ///< Main monitor data instance

    //* Different run-time cuts and flags (TODO: define in a config)
    double fTzeroOffset{0.};             ///< Offset for T0
    double fMinPionDca{1.5};             ///< Minimum DCA to PV for pions
    double fMinProtonDca{0.5};           ///< Minimum DCA to PV for protons
    double fQpAssignedUncertainty{0.1};  ///< Assigned relative uncertainty for q/p estimation
    double fMinBetaProton{0.};           ///< Minimal proton velocity (beta) [c]
    double fMaxBetaProton{1.};           ///< Maximal proton velocity (beta) [c]
    double fMinBetaPion{0.};             ///< Minimal proton velocity (beta) [c]
    double fMaxBetaPion{1.};             ///< Maximal proton velocity (beta) [c]
    int fPrimaryAssignedPdg{321};        ///< Assigned PDG hypothesis for primary particles

    //* Run-time variables, provided by framework
    int fBmonPartitionIndex{-1};  ///< Index of selected partition in BMON hit vector

    //* Temporary data arrays (NOTE: keep them here for QA)
    std::array<float, 3> fOrigin{0.f, 0.f, 0.f};                   ///< Coordinates of origin [cm]
    std::vector<double> fvT0s;                                     ///< Found t0s [ns] (in event)
    std::vector<ParticleInfo> fvParticleInfo;                      ///< PID info of tracks (in event)
    std::vector<uint32_t> fvSelectedTrackIds;                      ///< IDs of selected tracks (in event)
    double fSelectedT0{std::numeric_limits<double>::quiet_NaN()};  ///< A t0 value selected by the lambda-finder

    /// \brief A copy of track parameters (first, last)
    std::vector<std::pair<ca::Track::TrackParam_t, ca::Track::TrackParam_t>> fvTrackParam;

    //* Auxilary variables

    /// \brief An instance of the topology reconstructor
    std::unique_ptr<KFParticleTopoReconstructor> fpTopoReconstructor{std::make_unique<KFParticleTopoReconstructor>()};
  };
}  // namespace cbm::algo::kfp
