/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaToolsMCTrack.h
/// \brief  Class represents a MC track for CA tracking QA and performance (implementation)
/// \since  22.11.2022
/// \author S.Zharko <s.zharko@gsi.de> (based on CbmL1MCTrack class by I.Kisel and S.Gorbunov)

#ifndef CaToolsMCTrack_h
#define CaToolsMCTrack_h 1

#include "CaToolsDef.h"
#include "CaToolsLinkKey.h"
#include "CaVector.h"
#include "CbmL1Hit.h"
#include "TMath.h"

#include <functional>

class CbmL1HitDebugInfo;

namespace cbm::ca::tools
{
  class MCPoint;

  namespace constants = cbm::algo::ca::constants;

  class MCTrack {
   public:
    /// Default constructor
    MCTrack() = default;

    /// Destructor
    ~MCTrack() = default;

    /// Copy constructor
    MCTrack(const MCTrack&) = default;

    /// Move constructor
    MCTrack(MCTrack&&) = default;

    /// Copy assignment operator
    MCTrack& operator=(const MCTrack&) = default;

    /// Move assignment operator
    MCTrack& operator=(MCTrack&&) = default;

    /// Adds index of point, associated with this MC track
    /// \param  iP  Index of MC point in the container of internal MC points for this event/TS
    void AddPointIndex(int iP) { fvPointIndexes.push_back_no_warning(iP); }

    /// Adds index of hit, associated with this MC track
    /// \param  iH  Index of hit in the container of internal hits for this event/TS
    void AddHitIndex(int iH) { fvHitIndexes.push_back_no_warning(iH); }

    /// Adds index of reconstructed track, associated with this MC track
    /// \param  iTre  Index of track in the array of reconstructed tracks of the TS
    void AddRecoTrackIndex(int iTre) { fvRecoTrackIndexes.push_back_no_warning(iTre); }

    /// Adds index of reconstructed track, which is not associated with this MC track but contains some hits, produced
    /// by this MC track
    /// \param  iTre  Index of track in the array of reconstructed tracks of the TS
    void AddTouchTrackIndex(int iTre) { fvTouchTrackIndexes.push_back_no_warning(iTre); }

    /// Clears contents
    void Clear();

    /// Clears container of point indexes
    void ClearPointIndexes() { fvPointIndexes.clear(); }

    /// Clears container of hit indexes
    void ClearHitIndexes() { fvHitIndexes.clear(); }

    /// Clears container of reconstructed track indexes
    void ClearRecoTrackIndexes() { fvRecoTrackIndexes.clear(); }

    /// Clears container of indexes of reconstructed tracks, which are not assigned to this MC track, but contain some
    /// of its hits
    void ClearTouchTrackIndexes() { fvTouchTrackIndexes.clear(); }

    // *********************
    // **     Getters     **
    // *********************

    /// Gets index of the first particle in the decay chain
    int GetChainId() const { return fChainId; }

    /// Gets charge [e]
    double GetCharge() const { return fCharge; }

    /// Gets total energy [GeV]
    double GetE() const { return std::sqrt(GetP() * GetP() + fMass * fMass); }

    /// Gets kinetic energy [GeV]
    double GetEkin() const { return GetE() - fMass; }

    /// Gets pseudo-rapidity
    double GetEta() const { return -std::log(std::tan(GetTheta() * 0.5)); }

    /// Gets index of MC event containing this track in external data structures
    int GetEventId() const { return fLinkKey.fEvent; }

    /// Gets index of the track in the external data structures
    int GetExternalId() const { return fLinkKey.fIndex; }

    /// Gets index of MC file containing this track in external data structures
    int GetFileId() const { return fLinkKey.fFile; }

    /// Gets a reference to associated hit indexes
    const auto& GetHitIndexes() const { return fvHitIndexes; }

    /// Gets index of track
    int GetId() const { return fId; }

    /// Gets link key
    LinkKey GetLinkKey() const { return fLinkKey; }

    /// Gets particle mass [GeV/c2]
    double GetMass() const { return fMass; }

    /// Gets max number of hits within a station
    int GetMaxNofHitsOnStation() const { return fMaxNofHitsOnStation; }

    /// Gets max number of points within a station
    int GetMaxNofPointsOnStation() const { return fMaxNofPointsOnStation; }

    /// Gets index of mother track in CA internal data structures
    int GetMotherId() const { return fMotherId; }

    /// Gets number of clones
    int GetNofClones() const { return (fvRecoTrackIndexes.size() > 0 ? fvRecoTrackIndexes.size() - 1 : 0); }

    /// Gets number of consecutive stations with hits
    int GetNofConsStationsWithHit() const { return fNofConsStationsWithHit; }

    /// Gets number of consecutive stations with MC points
    int GetNofConsStationsWithPoint() const { return fNofConsStationsWithPoint; }

    /// Gets number of hits
    int GetNofHits() const { return fvHitIndexes.size(); }

    /// Gets number of points
    int GetNofPoints() const { return fvPointIndexes.size(); }

    /// Gets number of assigned reconstructed tracks
    int GetNofRecoTracks() const { return fvRecoTrackIndexes.size(); }

    /// Gets number of reconstructed tracks, which contain hits from this MC track
    int GetNofTouchTracks() const { return fvTouchTrackIndexes.size(); }

    /// Gets absolute momentum [GeV/c]
    double GetP() const { return std::sqrt(fMom[0] * fMom[0] + fMom[1] * fMom[1] + fMom[2] * fMom[2]); }

    /// Gets PDG encoding
    int GetPdgCode() const { return fPdgCode; }

    /// Gets azimuthal angle [rad]
    double GetPhi() const { return std::atan2(fMom[1], fMom[0]); }

    /// Gets a reference to associated point indexes
    const auto& GetPointIndexes() const { return fvPointIndexes; }

    /// Gets process ID
    unsigned GetProcessId() const { return fProcId; }

    /// Gets transverse momentum [GeV/c]
    double GetPt() const { return std::sqrt(fMom[0] * fMom[0] + fMom[1] * fMom[1]); }

    /// Gets x component of momentum [GeV/c]
    double GetPx() const { return fMom[0]; }

    /// Gets x component of momentum [GeV/c]
    double GetPy() const { return fMom[1]; }

    /// Gets x component of momentum [GeV/c]
    double GetPz() const { return fMom[2]; }

    /// Gets rapidity
    double GetRapidity() const { return 0.5 * std::log((GetE() + fMom[2]) / (GetE() - fMom[2])); }

    /// Gets a reference to vector associated reconstructed track indexes
    const auto& GetRecoTrackIndexes() const { return fvRecoTrackIndexes; }

    /// Gets particle speed [c]
    double GetSpeed() const { return GetP() / GetE(); }

    /// Gets time of the track vertex [ns]
    double GetStartT() const { return fTime; }

    /// Gets x component of the track vertex [cm]
    double GetStartX() const { return fPos[0]; }

    /// Gets y component of the track vertex [cm]
    double GetStartY() const { return fPos[1]; }

    /// Gets z component of the track vertex [cm]
    double GetStartZ() const { return fPos[2]; }

    /// @brief Gets track polar angle
    double GetTheta() const { return std::acos(fMom[2] / GetP()); }

    /// Gets total number of stations with hits
    int GetTotNofStationsWithHit() const { return fTotNofStationsWithHit; }

    /// Gets total number of stations with MC points
    int GetTotNofStationsWithPoint() const { return fTotNofStationsWithPoint; }

    /// Gets track slope along x-axis
    double GetTx() const { return fMom[0] / fMom[2]; }

    /// Gets track slope along y-axis
    double GetTy() const { return fMom[1] / fMom[2]; }

    /// \brief Creates an MC point from the track vertex
    MCPoint GetVertexPoint() const;

    /// Gets a reference to vector of reconstructed track indexes, not associated with this MC track but containing some
    /// hits, produced by this MC track
    const auto& GetTouchTrackIndexes() const { return fvTouchTrackIndexes; }

    /// \brief Initializes information about MC track hits arrangement within stations
    /// Defines:
    ///   #1) Number of stations with hits
    ///   #2) Maximal number of hits within one station
    ///   #3) Number of consecutive stations with a hit in MC track
    /// \param  vHits  Vector of hits for a given TS
    void InitHitsInfo(const ca::Vector<CbmL1HitDebugInfo>& vHits);

    /// \brief Initializes information about MC track points arrangement within stations
    /// Defines:
    ///   #1) Number of stations with points
    ///   #2) Maximal number of points within one station
    ///   #3) Maximal number of points within one sensor (with same z-position)
    ///   #4) Number of consecutive stations with a point in MC track
    /// \param  vPoints  Vector of points for a given TS
    void InitPointsInfo(const ca::Vector<MCPoint>& vPoints);


    // *******************
    // **     Flags     **
    // *******************

    /// Returns .... TODO
    bool IsAdditional() const { return fIsAdditional; }

    /// Returns true, if this MC track
    bool IsDisturbed() const { return fvTouchTrackIndexes.size(); }

    /// Returns flag, if the track is primary (process ID is 0 either mother ID is -1)
    bool IsPrimary() const { return fProcId == 0; }

    /// Returns flag, if track is reconstructable. The definition of particle reconstructability depends on the
    /// performance mode and should be determined in the experiment-specific MC module
    bool IsReconstructable() const { return fIsReconstructable; }

    bool IsReconstructed() const { return fvRecoTrackIndexes.size(); }

    /// Returns flag, if the track comes from a real particle (true), or from generated noise (false)
    bool IsSignal() const { return fIsSignal; }


    // *********************
    // **     Setters     **
    // *********************

    /// Sets index of the first particle in the decay chain
    void SetChainId(int chainId) { fChainId = chainId; }

    /// Sets charge [e]
    void SetCharge(double q) { fCharge = q; }

    /// Sets index of MC event containing this track in external data structures
    void SetEventId(int iEvent) { fLinkKey.fEvent = iEvent; }

    /// Sets index of track in external data structures
    /// \note This index should be used to navigate through links
    void SetExternalId(int id) { fLinkKey.fIndex = id; }

    /// Sets index of MC file containing this track in external data structures
    void SetFileId(int iFile) { fLinkKey.fFile = iFile; }

    /// Sets index of track in the CA internal data structure (within event/TS)
    void SetId(int id) { fId = id; }

    /// Sets index of mother track
    /// \note Mother ID should refer to the internal index of track
    void SetMotherId(int motherId) { fMotherId = motherId; }

    /// Sets particle mass [GeV/c2]
    void SetMass(double mass) { fMass = mass; }

    /// Sets PDG encoding
    void SetPdgCode(int pdg) { fPdgCode = pdg; }

    /// Sets process ID
    void SetProcessId(unsigned procId) { fProcId = procId; }

    /// Sets x component of momentum [GeV/c]
    void SetPx(double px) { fMom[0] = px; }

    /// Sets x component of momentum [GeV/c]
    void SetPy(double py) { fMom[1] = py; }

    /// Sets x component of momentum [GeV/c]
    void SetPz(double pz) { fMom[2] = pz; }

    /// Sets flag, if the track is additional (not reconstructable but still interesting)
    void SetFlagAdditional(bool isAdditional) { fIsAdditional = isAdditional; }

    /// Sets flag, if the track is reconstructable
    void SetFlagReconstructable(bool isReconstructable) { fIsReconstructable = isReconstructable; }

    /// Sets flag, if the track comes from signal
    void SetFlagSignal(bool isSignal) { fIsSignal = isSignal; }

    /// Sets time of the track vertex [ns]
    void SetStartT(double t) { fTime = t; }

    /// Sets x component of the track vertex [cm]
    void SetStartX(double x) { fPos[0] = x; }

    /// Sets y component of the track vertex [cm]
    void SetStartY(double y) { fPos[1] = y; }

    /// Sets z component of the track vertex [cm]
    void SetStartZ(double z) { fPos[2] = z; }

    /// Sorts points inside track by a provided hit comparison function function
    /// \param  cmpFn  Functional object to compare mcPoints
    void SortPointIndexes(const std::function<bool(const int& lhs, const int& rhs)>& cmpFn);

    /// @brief Provides string representation of a track
    /// @param verbose  Verbosity level
    /// @param header   Flag: to print header or data
    /// @return String representation
    std::string ToString(int verbose = 1, bool header = false) const;

   private:
    // ****************************
    // **     Data variables     **
    // ****************************

    double fMass   = constants::Undef<double>;  ///< Particle mass [GeV/c2]
    double fCharge = constants::Undef<double>;  ///< Particle charge [e]
    double fTime   = constants::Undef<double>;  ///< Time of track [cm]

    std::array<double, 3> fPos = {constants::Undef<double>, constants::Undef<double>,
                                  constants::Undef<double>};  ///< Track vertex components [cm]
    std::array<double, 3> fMom = {constants::Undef<double>, constants::Undef<double>,
                                  constants::Undef<double>};  ///< Momentum components [GeV/c]

    int fPdgCode     = constants::Undef<int>;       ///< PDG encoding
    unsigned fProcId = constants::Undef<unsigned>;  ///< Process ID (from ROOT::TProcessID)

    // Track address
    int fId       = constants::Undef<int>;  ///< Index of MC track in internal container for TS/event
    int fMotherId = constants::Undef<int>;  ///< Index of mother MC track in the external tracks container
    int fChainId  = constants::Undef<int>;  ///< Index of the first particle in the decay chain

    LinkKey fLinkKey = {constants::Undef<int>, constants::Undef<int>,
                        constants::Undef<int>};  ///< A link key of this track in the external data structures

    bool fIsSignal          = false;  ///< If the track comes from signal
    bool fIsReconstructable = false;  ///< If track is reconstructable
    bool fIsAdditional      = false;  ///< If track is not reconstructable, but still interesting

    // Arrangement of hits and points within the stations
    int fNofConsStationsWithHit   = constants::Undef<int>;  ///< Number of consecutive stations with hits
    int fNofConsStationsWithPoint = constants::Undef<int>;  ///< Number of consecutive stations with points
    int fTotNofStationsWithHit    = constants::Undef<int>;  ///< Total number of stations with hits
    int fTotNofStationsWithPoint  = constants::Undef<int>;  ///< Total number of stations with MC points
    int fMaxNofPointsOnStation    = constants::Undef<int>;  ///< Max number of MC points on a station
    int fMaxNofPointsOnSensor = constants::Undef<int>;  ///< Max number of MC points with same Z (means on same sensor)
    int fMaxNofHitsOnStation  = constants::Undef<int>;  ///< Max number of hits on a station

    ca::Vector<int> fvPointIndexes      = {"ca::tools::fvPointIndexes"};      ///< Indexes of MC points in ext.container
    ca::Vector<int> fvHitIndexes        = {"ca::tools::fvHitIndexes"};        ///< Indexes of hits in int.container
    ca::Vector<int> fvRecoTrackIndexes  = {"ca::tools::fvRecoTrackIndexes"};  ///< Indexes of associated reco tracks
    ca::Vector<int> fvTouchTrackIndexes = {"ca::tools::fvTouchTrackIndexes"};  ///< Pointers to non-associated tracks,
                                                                               ///< which use hits from this track
  };

}  // namespace cbm::ca::tools

#endif  // CaToolsMCTrack_h
