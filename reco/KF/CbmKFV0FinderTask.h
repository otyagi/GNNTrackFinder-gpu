/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmV0FinderTask.h
/// \brief  A FairTask for V0 candidates finding in mCBM
/// \since  10.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

// TODO: Provide a configurator to the class

#pragma once

#include "CbmEnumArray.h"
#include "CbmKFParticleFinder.h"  // for the CbmKFParticleFinder::InverseChi2Prob()
#include "CbmKFV0FinderQa.h"
#include "CbmKFVertex.h"
#include "CbmVertex.h"
#include "FairTask.h"
#include "KFParticleTopoReconstructor.h"

#include <memory>

class CbmEvent;
class CbmGlobalTrack;
class CbmStsTrack;
class CbmTofHit;
class TClonesArray;
class FairTrackParam;
class KFPTrackVector;
class KFVertex;
class KFParticleFinder;
class CbmEventTriggers;

namespace cbm::kfp
{

  /// \class  V0FinderTask
  /// \brief  A class to find V0 candidates in mCBM
  class V0FinderTask : public FairTask {
   public:
    /// \enum   Counter
    /// \brief  Enumeration of counters (for the entire run)
    enum class ECounter : uint8_t
    {
      TracksTotal,                //< Total number of tracks
      TracksSelected,             //< Tracks, which satisfy topology PID applicability
      TracksInfiniteParam,        //< Tracks, which have infinite parameters
      TracksWoTofHits,            //< Tracks, which have no TOF hits
      TracksWNegativeTofHitTime,  //< Tracks, the last TOF hit of which has a negative time (it's time is less then the t0)
      TracksWoStsHits,            //< Tracks, which have no STS hits
      TracksWoPid,                //< Tracks, which has undefined PID
      TracksWoMomentum,           //< Tracks, which has undefined PID
      TracksWAtLeastOneTofHit,   //< Tracks with at least one tof hit
      TracksWAtLeastTwoTofHits,  //< Tracks with at least two tof hits
      TracksWithUnphysicalBeta,  //< Tracks with beta > 1
      TracksW2Sts2Tof,           //< Tracks with at least two STS hits and two TOF hits
      Pions,                     //< Number of pion-candidates
      Protons,                   //< Number of proton-candidates
      EventsTotal,               //< Total number of events
      EventsWoTzero,             //< Number of events with undefined t-zero
      EventsLambdaCand,          //< Events with at least one pion and one proton candidate
      KfpEventsLambdaCand,       //< Events with lambda-candidates in KF-particle
      KfpLambdaCandidates,       //< Number of lambda-candidates
      END
    };

    /// \enum  EPidApproach
    /// \brief PID approach used
    enum class EPidApproach : uint8_t
    {
      Topo,  //< Topological PID (DCA calculation for a track)
      Mc     //< MC-truth PID
    };

    /// \enum  EProcessingMode
    /// \brief Data processing mode
    enum class EProcessingMode : uint8_t
    {
      TimeBased,  //< Time based simulation or real data
      EventBased  //< Event based simulation
    };

    /// \enum   EPvUsageMode
    /// \brief  Primary vertex finding/handling mode
    enum class EPvUsageMode : uint8_t
    {
      Target,               //< Use target center as a primary vertex
      Reconstructed,        //< Use reconstructed primary vertex
      ReconstructSingle,    //< Reconstruct a single PV from given track vectors
      ReconstructMultiple,  //< Reconstruct multiple PVs (in case of pile-up)
      Mc                    //< Use MC-true primary vertex
    };

    /// \struct QpAndBeta
    /// \brief  Qp and beta container
    struct QpAndBeta {
      double fQp{std::numeric_limits<double>::signaling_NaN()};
      double fQpVar{std::numeric_limits<double>::signaling_NaN()};
      double fBeta{std::numeric_limits<double>::signaling_NaN()};
      double fBetaVar{std::numeric_limits<double>::signaling_NaN()};
    };

    /// \struct DcaVector
    /// \brief  A vector representation of DCA to target
    struct DcaVector {
      double fAbs{std::numeric_limits<double>::signaling_NaN()};  ///< Absolute value
      double fX{std::numeric_limits<double>::signaling_NaN()};    ///< X-component of the unit-vector
      double fY{std::numeric_limits<double>::signaling_NaN()};    ///< Y-component of the unit-vector
    };

    /// \brief  Constructor
    /// \param  verbose  Verbosity of the task
    explicit V0FinderTask(int verbose = 1) : FairTask("V0FinderTask", verbose){};

    /// \brief  Destructor
    ~V0FinderTask() = default;

    // Eliminate copy and move
    V0FinderTask(const V0FinderTask&) = delete;
    V0FinderTask(V0FinderTask&&)      = delete;
    V0FinderTask& operator=(const V0FinderTask&) = delete;
    V0FinderTask& operator=(V0FinderTask&&) = delete;


    //* FairTask overriden methods

    /// \brief  Executes the task
    void Exec(Option_t*) override;

    /// \brief  Action on the end of the run
    void Finish() override;

    /// \brief  Initializes the task
    InitStatus Init() override;


    //* Accessors

    /// \brief  Mutable access to the KfParticleFinder of the run topology reconstructor
    KFParticleFinder* GetKFParticleFinder() { return fpTopoReconstructorRun->GetKFParticleFinder(); }

    /// \brief  Constant access to the KfParticleFinder of the run topology reconstructor
    const KFParticleFinder* GetKFParticleFinder() const { return fpTopoReconstructorRun->GetKFParticleFinder(); }

    /// \brief  Gets DCA to origin
    const auto& GetTrackDcaToOrigin() const { return fvTrackDca; }

    /// \brief  Accessor to the topology reconstructor
    const std::shared_ptr<const KFParticleTopoReconstructor> GetTopoReconstructor() const
    {
      return fpTopoReconstructorRun;
    }

    /// \brief  Accessor to the track DCA
    const std::vector<DcaVector>& GetTrackDca() { return fvTrackDca; }

    //* Task logic and preliminary selection cut setters

    /// \brief  Sets name of the configuration file
    /// \param  fileName  Name of the config file
    void SetConfigName(const TString& fileName) { fsConfigName = fileName; }

    /// \brief  Sets processing mode (time-based/event-based)
    void SetProcessingMode(EProcessingMode mode) { fProcessingMode = mode; }

    /// \brief  Sets PID approach
    void SetPidApproach(EPidApproach pid) { fPidApproach = pid; }

    /// \brief  Sets PV finding mode
    void SetPvFindingMode(EPvUsageMode mode) { fPvUsageMode = mode; }

    /// \brief  Sets the MC flag (if MC information required)
    void SetUseMc(bool bUseMc) { fbUseMc = bUseMc; }

    /// \brief  Sets the flag: if the QA should be executed
    void SetRunQa(bool bRunQa) { fbRunQa = bRunQa; }

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

    /// \brief  Sets minimal pion DCA to primary vertex
    /// \param  dca  DCA [cm]
    void SetMinPionDca(double dca) { fMinPionDca = dca; }

    /// \brief  Sets minimal proton DCA to primary vertex
    /// \param  dca  DCA [cm]
    void SetMinProtonDca(double dca) { fMinProtonDca = dca; }

    /// \brief  Sets a file name for the QA
    /// \param  fileName  Name of file
    void SetQaOutputFileName(const TString& fileName) { fsQaOutputName = fileName; }

    /// \brief  Assignes an uncertainty to the momentum measurement
    /// \param  uncertainty  Relative uncertainty ( = sqrt(var(q/p)) / (q/p))
    void SetQpAssignedUncertainty(double uncertainty) { fQpAssignedUncertainty = uncertainty; }

    /// \brief  Sets an offset to t0
    /// \param  offset  An offset [ns]
    void SetTzeroOffset(double offset) { fTzeroOffset = offset; }

    /// \brief  Special settings in the mixed-event analysis mode
    /// \param  bMixedEvent  Flag: true - run in the mixed-event mode
    // NOTE: The KFParticleFinder has its internal mixed-event analysis mode, but was not used in mCBM Lambda-analysis
    //       the internal mixed-event mode can be selected using the following method:
    //          fpTopoReconstructorRun->SetMixedEventAnalysis()
    void SetMixedEventMode(bool bMixedEvent) { fbMixedEventMode = bMixedEvent; }


    //* KFParticleFinder setters

    /// \brief  Adds particle to reconstruction list
    /// \param  pdg  A PDG code of the particle to be reconstructed
    void AddDecayToReconstructionList(int pdg) { GetKFParticleFinder()->AddDecayToReconstructionList(pdg); }

    /// \brief  Sets P-value cut to the topology reconstructor
    /// \param  pVal  P-value cut
    void SetPrimaryProbCut(double pVal)
    {
      constexpr int Ndf{2};
      fpTopoReconstructorRun->SetChi2PrimaryCut(CbmKFParticleFinder::InversedChi2Prob(pVal, Ndf));
    }

    /// \brief  Sets cut on the distance between secondary tracks at the DCA point
    /// \param  cut  Cut value [cm]
    void SetMaxDistanceBetweenParticlesCut(float cut) { GetKFParticleFinder()->SetMaxDistanceBetweenParticlesCut(cut); }

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

    /// \brief  Sets cuts on selection of secondary and primary candidates
    /// \param  sigmaMass  \f$\sigma_{M}\f$ [GeV/c2]
    /// \param  chi2Topo   \f$\chi^2_{topo}\f$
    /// \param  ldl        \f$l/\Delta l\f$
    void SetSecondaryCuts(float sigmaMass, float chi2Topo, float ldl)
    {
      GetKFParticleFinder()->SetSecondaryCuts(sigmaMass, chi2Topo, ldl);
    }

    /// \brief  Sets \f$l/\Delta l\f$ cut for \f$\Xi\f$ and \f$\Omega\f$
    /// \param  cut  Cut value
    void SetLdLCutXiOmega(float cut) { GetKFParticleFinder()->SetLdLCutXiOmega(cut); }

    /// \brief  Sets \f$\chi^2_{topo}\f$ cut for \f$\Xi\f$ and \f$\Omega\f$
    /// \param  cut  Cut value
    void SetChi2TopoCutXiOmega(float cut) { GetKFParticleFinder()->SetChi2TopoCutXiOmega(cut); }

    /// \brief  Sets \f$\chi^2_{geo}\f$ cut for \f$\Xi\f$ and \f$\Omega\f$
    /// \param  cut  Cut value
    void SetChi2CutXiOmega(float cut) { GetKFParticleFinder()->SetChi2CutXiOmega(cut); }

    /// \brief  Sets \f$\chi^2_{topo}\f$ cut for resonances
    /// \param  cut  Cut value
    void SetChi2TopoCutResonances(float cut) { GetKFParticleFinder()->SetChi2TopoCutResonances(cut); }

    /// \brief  Sets \f$\chi^2_{geo}\f$ cut for resonances
    /// \param  cut  Cut value
    void SetChi2CutResonances(float cut) { GetKFParticleFinder()->SetChi2CutResonances(cut); }

    /// \brief  Sets the cut on transverse momentum of each daughter track of low mass vector mesons
    /// \param  cut  Cut value [GeV/c]
    void SetPtCutLMVM(float cut) { GetKFParticleFinder()->SetPtCutLMVM(cut); }

    /// \brief  Sets the cut on momentum of each daughter track of low mass vector mesons in dimuon channel
    /// \param  cut  Cut value [GeV/c]
    void SetPCutLMVM(float cut) { GetKFParticleFinder()->SetPCutLMVM(cut); }

    /// \brief  Sets the cut on transverse momentum of each daughter track of \f$J/\psi\f$
    /// \param  cut  Cut value [GeV/c]
    void SetPtCutJPsi(float cut) { GetKFParticleFinder()->SetPtCutJPsi(cut); }

    /// \brief  Sets the cut on transverse momentum of each daughter track of open charm particles
    /// \param  cut  Cut value [GeV/c]
    void SetPtCutCharm(float cut) { GetKFParticleFinder()->SetPtCutCharm(cut); }

    /// \brief  Sets cut on \f$\chi^2_{prim}\f$ of each track for open charm particles
    /// \param  cut  Cut value [GeV/c]
    void SetChiPrimaryCutCharm(float cut) { GetKFParticleFinder()->SetChiPrimaryCutCharm(cut); }

    /// \brief  Sets \f$l/\Delta l\f$ cut for open charm with >=3 daughters
    /// \param  cut  Cut value
    void SetLdLCutCharmManybodyDecays(float cut) { GetKFParticleFinder()->SetLdLCutCharmManybodyDecays(cut); }

    /// \brief  Sets \f$\chi^2_{topo}\f$ cut for open charm with >=3 daughters
    /// \param  cut  Cut value
    void SetChi2TopoCutCharmManybodyDecays(float cut) { GetKFParticleFinder()->SetChi2TopoCutCharmManybodyDecays(cut); }

    /// \brief  Sets \f$\chi^2_{geo}\f$ cut for open charm with >=3 daughters
    /// \param  cut  Cut value
    void SetChi2CutCharmManybodyDecays(float cut) { GetKFParticleFinder()->SetChi2CutCharmManybodyDecays(cut); }

    /// \brief  Sets \f$l/\Delta l\f$ cut for open charm with 2 daughters
    /// \param  cut  Cut value
    void SetLdLCutCharm2D(float cut) { GetKFParticleFinder()->SetLdLCutCharm2D(cut); }

    /// \brief  Sets \f$\chi^2_{topo}\f$ cut for open charm with 2 daughters
    /// \param  cut  Cut value
    void SetChi2TopoCutCharm2D(float cut) { GetKFParticleFinder()->SetChi2TopoCutCharm2D(cut); }

    /// \brief  Sets \f$\chi^2_{geo}\f$ cut for open charm with 2 daughters
    /// \param  cut  Cut value
    void SetChi2CutCharm2D(float cut) { GetKFParticleFinder()->SetChi2CutCharm2D(cut); }


    //* Static helper methods

    /// \brief  String representation of processing mode
    /// \param  mode  Data processing mode
    static std::string ToString(EProcessingMode mode);

    /// \brief  String representation of PID approach
    /// \param  pid  PID approach
    static std::string ToString(EPidApproach pid);

    /// \brief  String representation of the PV finding mode
    /// \param  pvMode  PV finding mode
    static std::string ToString(EPvUsageMode pvMode);

    /// \brief  String representation of
    /// \param  pParam  Parameters of the track
    static std::string ToString(const FairTrackParam* pParam);

    /// \brief   Checks track parameter validity
    /// \param   pParam  Pointer to a track parameter
    /// \return  true  The parameters are ok
    /// \return  false The parameters are invalid
    static bool CheckTrackParam(const FairTrackParam* pParam);

   private:
    //* Auxilary internal methods

    /// \brief   Applies configuration from fsConfigName
    void ApplyConfiguration();

    /// \brief   Assigns momentum to a global track
    /// \param   pTrack  Pointer to a global track
    /// \param   pdg     PID hypothesis
    /// \return  true    Momentum is set
    /// \return  false   Momentum is not set (the track must be rejected)
    bool AssignMomentum(CbmGlobalTrack* pTrack, int pdg);

    /// \brief  Estimates distance to the origin
    /// \param  pStsTrack  STS track
    /// \return Vector of the DCA {abs, ex, ey}
    DcaVector EstimateDcaToOrigin(const CbmStsTrack* pTrack) const;

    /// \brief  Estimates q/p of the track, using one TOF hit (Norbert's method)
    /// \param  pTofHit  Pointer to a TOF hit
    /// \param  pdg      PID hypothesis
    /// \note   Gives smaller statistical uncertainty, but has a systematic effect (to be studied with MC)
    QpAndBeta EstimateQp(const CbmTofHit* pTofHit, int pdg) const;

    /// \brief  Makes a KF-particle track vector
    /// \param  vpTracks       Vector of pointers to global tracks
    /// \param  vTrackIds      Vector of track indices
    /// \param  vChi2ToPv      Vector of chi2 of assigning a track as primary
    /// \param  bAtFirstPoint  true: the first track point is used, false: the last track point is used
    /// \note   No track selection is performed
    KFPTrackVector MakeKfpTrackVector(const std::vector<const CbmGlobalTrack*>& vpTracks,
                                      const std::vector<int>& vTrackIds, const std::vector<float>& vChi2ToPv,
                                      bool bAtFirstPoint) const;

    /// \brief   Makes a KF vertex
    /// \param   x  x-coordinate of PV [cm]
    /// \param   y  y-coordinate of PV [cm]
    /// \param   z  z-coordinate of PV [cm]
    KFVertex MakeKfpPrimaryVertex(float x, float y, float z) const;

    /// \brief  Infers PID hypothesis for a track using track topology
    /// \param  dca  An absolute value of DCA to the PV estimation
    /// \return PDG code of the particle
    /// \return -2  If the PID analysis cannot be performed for a track of a given quality
    int InferTrackPidTopo(double dca) const;

    /// \brief  Processes one event
    /// \tparam UseEvent  Explicit flag of using the CbmEvent pointer to access data
    /// \param  pEvent  Pointer to event
    /// \return true    Event is accepted for V0 reconstruction
    /// \return false   Event is rejected for V0 reconstruction
    /// \note   In the event-based mode, the ProcessEvent(nullptr) must be called explicitly
    template<bool UseEvent>
    bool ProcessEvent(const CbmEvent* pEvent);

    /// \brief  Selects/rejects a track
    /// \param  pTrack  A pointer to global track
    /// \param  iTrk    Global index of the track
    /// \return true:  Track is selected
    /// \return false: Track is rejected
    /// \note   Modifies the track, assigning momentum and PID hypothesis
    ///
    /// In this function the preliminary selection of tracks is performed. The selection criteria:
    ///  1)  the track has at least two STS hits for estimation of the PDG codes (pi-, proton, or primary (kaon PDG))
    ///  2)  the track has at least one TOF hit for estimation of its momentum
    ///      2.1) tracks with the TOF hit times (t - t0) < 0 are rejected
    ///      2.2) tracks with the beta > 1 are rejected
    ///  3)  the track has defined parameters in the first hit
    ///      NOTE: this cut was used, because the tracks with negative TOF times were not rejected. Now it seems,
    ///            that the track parameters are always defined.
    // TODO:  Replace with a functor (a function from a separate FairTask)
    bool SelectTrack(CbmGlobalTrack* pTrack, int iTrk);

    /// \brief  Shifts TOF hits to the t0 estimation
    /// \param  pEvent  Pointer to the event
    /// \return value of estimated t0 [ns]
    double ShiftTofHitsToTzero(const CbmEvent* pEvent);

    /// \brief  Stores particles, reconstructed in event to the run topology reconstructor
    void StoreParticles();


    //* Task constants (To be moved to a configuration)
    static constexpr int kPrimaryPdg{321};        ///< PID hypothesis of primary tracks (kaons?)
    static constexpr int kUndefPdg{-2};           ///< Undefined value, such tracks will be skipped
    static constexpr float kChi2PvPrimThrsh{3.};  ///< Chi2 threshold of assigning tracks as primaries in PV reco

    //* Physical constants
    static constexpr double kPionMass{0.13957039};      ///< Pion mass [GeV/c2]
    static constexpr double kProtonMass{0.938272088};   ///< Proton mass [GeV/c2]
    static constexpr double kSpeedOfLight{29.9792458};  ///< Speed of light [cm/ns]

    //* Different run-time cuts and flags (TODO: define in a config)
    double fTzeroOffset{0.};             ///< Offset for T0
    double fMinPionDca{1.5};             ///< Minimum DCA to PV for pions
    double fMinProtonDca{0.5};           ///< Minimum DCA to PV for protons
    double fQpAssignedUncertainty{0.1};  ///< Assigned relative uncertainty for q/p estimation
    int fPrimaryAssignedPdg{321};        ///< Assigned PDG hypothesis for primary particles
    double fMinBetaProton{0.};           ///< Minimal proton velocity (beta) [c]
    double fMaxBetaProton{1.};           ///< Maximal proton velocity (beta) [c]
    double fMinBetaPion{0.};             ///< Minimal proton velocity (beta) [c]
    double fMaxBetaPion{1.};             ///< Maximal proton velocity (beta) [c]

    //* Input data branches
    TClonesArray* fpBrRecoEvents{nullptr};
    TClonesArray* fpBrGlobalTracks{nullptr};
    TClonesArray* fpBrStsTracks{nullptr};
    TClonesArray* fpBrTrdTracks{nullptr};
    TClonesArray* fpBrTofTracks{nullptr};
    TClonesArray* fpBrStsHits{nullptr};
    TClonesArray* fpBrTrdHits{nullptr};
    TClonesArray* fpBrTofHits{nullptr};
    CbmVertex* fpBrPrimaryVertex{nullptr};

    //* Output data branches
    std::vector<CbmEventTriggers>* fpBrEventTriggers{nullptr};  //< Branch for event triggers
    // TODO: in principle one can store the found KF particles and vertices as well

    //* Temporary data and auxilary variables (per FairTask::Exec() call)
    std::vector<DcaVector> fvTrackDca;  ///< Track DCA vector [n global tracks]
    std::unique_ptr<CbmVertex> fpOrigin{
      std::make_unique<CbmVertex>()};  ///< Origin (e.g., can be either reconstructed PV or target)

    //* KFParticleFinder utilities
    /// \brief Main topology reconstructor
    // Main topology reconstructor
    std::shared_ptr<KFParticleTopoReconstructor> fpTopoReconstructorRun{
      std::make_shared<KFParticleTopoReconstructor>()};
    std::unique_ptr<KFParticleTopoReconstructor> fpTopoReconstructorEvent{
      std::make_unique<KFParticleTopoReconstructor>()};

    //* Control flags
    EProcessingMode fProcessingMode{EProcessingMode::TimeBased};  ///< Processing mode
    EPidApproach fPidApproach{EPidApproach::Topo};                ///< PID approach used
    EPvUsageMode fPvUsageMode{EPvUsageMode::Target};              ///< Primary vertex mode
    bool fbMixedEventMode{false};                                 ///< Run in a mix-event mode
    bool fbUseMc{false};                                          ///< Run using MC-information
    bool fbRunQa{false};                                          ///< Run QA
    bool fbUsePvChi2Selection{false};                             ///< Select

    //* QA and monitoring variables
    cbm::core::EnumArray<ECounter, size_t> fCounters{{0}};  ///< Counters per run
    std::unique_ptr<V0FinderQa> fpQa{nullptr};              ///< If QA is processed
    TString fsQaOutputName{"./V0FinderQa.root"};            ///< Output QA name
    TString fsConfigName{""};                               ///< Name of the config


    ClassDefOverride(V0FinderTask, 0);
  };
}  // namespace cbm::kfp
