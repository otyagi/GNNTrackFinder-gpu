/* Copyright (C) 2021-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// @file L1Parameters.h
/// @brief Parameter container for the L1Algo library
/// @since 19.12.2021
/// @author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaIteration.h"
#include "CaSearchWindow.h"
#include "CaStation.h"
#include "CaVector.h"
#include "KfFieldRegion.h"
#include "KfSetup.h"

#include <boost/serialization/array.hpp>
#include <boost/serialization/utility.hpp>

#include <array>
#include <numeric>
#include <type_traits>
#include <utility>

namespace cbm::algo::ca
{
  class InitManager;
  enum class EDetectorID;

  using constants::Undef;

  /// Type definitions for used containers
  using IterationsContainer_t = Vector<Iteration>;
  template<typename DataT>
  using StationsContainer_t = std::array<ca::Station<DataT>, constants::size::MaxNstations>;

  enum TrackingMode
  {
    kSts,
    kGlobal,
    kMcbm
  };

  /// \class cbm::algo::ca::Parameters
  /// \brief A container for all external parameters of the CA tracking algorithm
  ///
  /// The class includes geometry parameters and physics cuts. The instance of the Parameters is constructed inside
  /// the InitManager class and then moved to the L1Algo instance.
  ///
  template<typename DataT>
  class alignas(constants::misc::Alignment) Parameters {
    /// \note The init manager is responsible for the formation of the Parameters block
    friend class InitManager;

    using DetectorID_t = std::underlying_type_t<EDetectorID>;
    template<typename T>
    using StationArray_t = std::array<T, constants::size::MaxNstations>;

   public:
    /// \brief Default constructor
    Parameters();

    /// \brief Destructor
    ~Parameters() noexcept = default;

    /// \brief Copy constructor
    Parameters(const Parameters& other) = default;

    /// \brief Copy constructor with type conversion
    template<typename DataIn>
    Parameters(const Parameters<DataIn>& other)
      : fGeometrySetup(other.GetGeometrySetup())
      , fActiveSetup(other.GetActiveSetup())
      , fMaxDoubletsPerSinglet(other.GetMaxDoubletsPerSinglet())
      , fMaxTripletPerDoublets(other.GetMaxTripletPerDoublets())
      , fCAIterations(other.GetCAIterations())
      , fVertexFieldValue(other.GetVertexFieldValue())
      , fVertexFieldRegion(other.GetVertexFieldRegion())
      , fvFirstGeoId(other.GetFirstGeoId())
      , fvLocalToGeoIdMap(other.GetLocalToGeoIdMap())
      , fvGeoToLocalIdMap(other.GetGeoToLocalIdMap())
      , fvGeoToActiveMap(other.GetGeoToActiveMap())
      , fvActiveToGeoMap(other.GetActiveToGeoMap())
      , fNstationsActiveTotal(other.GetNstationsActive())
      , fSearchWindows(other.GetSearchWindows())
      , fGhostSuppression(other.GetGhostSuppression())
      , fRandomSeed(other.GetRandomSeed())
      , fDefaultMass(other.GetDefaultMass())
      , fMisalignmentX(other.GetMisalignmentX())
      , fMisalignmentY(other.GetMisalignmentY())
      , fMisalignmentT(other.GetMisalignmentT())
      , fDevIsIgnoreHitSearchAreas(other.DevIsIgnoreHitSearchAreas())
      , fDevIsUseOfOriginalField(other.DevIsUseOfOriginalField())
      , fDevIsMatchDoubletsViaMc(other.DevIsMatchDoubletsViaMc())
      , fDevIsMatchTripletsViaMc(other.DevIsMatchTripletsViaMc())
      , fDevIsExtendTracksViaMc(other.DevIsExtendTracksViaMc())
      , fDevIsSuppressOverlapHitsViaMc(other.DevIsSuppressOverlapHitsViaMc())
      , fDevIsParSearchWUsed(other.DevIsParSearchWUsed())
    {
      fTargetPos[0] = kfutils::simd::Cast<DataIn, DataT>(other.GetTargetPositionX());
      fTargetPos[1] = kfutils::simd::Cast<DataIn, DataT>(other.GetTargetPositionY());
      fTargetPos[2] = kfutils::simd::Cast<DataIn, DataT>(other.GetTargetPositionZ());
      for (size_t i = 0; i < constants::size::MaxNstations; i++) {
        fStations[i] = other.GetStation(i);
      }
    }

    /// \brief Copy assignment operator
    Parameters& operator=(const Parameters& other) = default;

    /// \brief Move constructor
    Parameters(Parameters&& other) = default;

    /// \brief Move assignment operator
    Parameters& operator=(Parameters&& other) = default;

    /// \brief Prints configuration
    void Print(int verbosityLevel = 0) const;

    /// \brief String representation of the class contents
    /// \param verbosity    A verbose level for output
    /// \param indentLevel  Indent level of the string output
    std::string ToString(int verbosity = 0, int indentLevel = 0) const;

    /// \brief Gets upper-bound cut on max number of doublets per one singlet
    unsigned int GetMaxDoubletsPerSinglet() const { return fMaxDoubletsPerSinglet; }

    /// \brief Gets upper-bound cut on max number of triplets per one doublet
    unsigned int GetMaxTripletPerDoublets() const { return fMaxTripletPerDoublets; }

    /// \brief Gets total number of active stations
    int GetNstationsActive() const { return fNstationsActiveTotal; }

    /// \brief Gets number of active stations for given detector ID
    int GetNstationsActive(EDetectorID detectorID) const;

    /// \brief Gets total number of stations, provided by setup geometry
    int GetNstationsGeometry() const { return fvFirstGeoId.back(); }

    /// \brief Gets number of stations, provided by setup geometry for given detector ID
    int GetNstationsGeometry(EDetectorID detectorID) const
    {
      return fvFirstGeoId[static_cast<int>(detectorID) + 1] - fvFirstGeoId[static_cast<int>(detectorID)];
    }

    /// \brief Provides access to the first index of the station on each particular detector (const)
    const std::array<int, constants::size::MaxNdetectors + 1>& GetFirstGeoId() const { return fvFirstGeoId; }

    /// \brief  Gets local index of station
    /// \param  geoIndex  geometry index of the tracking station
    /// \return A pair (detectorID, local index of the tracking station)
    [[gnu::always_inline]] std::pair<EDetectorID, int> GetStationIndexLocal(int geoIndex) const
    {
      return fvGeoToLocalIdMap[geoIndex];
    }

    /// \brief Provides access to local indexes of stations (const)
    const StationArray_t<std::pair<EDetectorID, int>>& GetGeoToLocalIdMap() const { return fvGeoToLocalIdMap; }

    /// \brief Calculates global index of station among geometry (accounts for inactive stations)
    /// \param localIndex  local index of the tracking station (for a particular detector)
    /// \param detectorID  ID of the detector subsystem
    [[gnu::always_inline]] int GetStationIndexGeometry(int localIndex, EDetectorID detectorID) const
    {
      if (localIndex >= GetNstationsGeometry(detectorID)) {
        return -1;
      }
      return fvLocalToGeoIdMap[fvFirstGeoId[static_cast<int>(detectorID)] + localIndex];
    }

    /// \brief Provides access to global indexes of stations among geometry (const)
    const StationArray_t<int>& GetLocalToGeoIdMap() const { return fvLocalToGeoIdMap; }

    /// \brief Calculates global index of station used by track finder
    /// \param localIndex  local index of the tracking station (for a particular detector)
    /// \param detectorID  ID of the detector subsystem
    [[gnu::always_inline]] int GetStationIndexActive(int localIndex, EDetectorID detectorID) const
    {
      int geoIndex = GetStationIndexGeometry(localIndex, detectorID);
      return (geoIndex < 0) ? -1 : fvGeoToActiveMap[geoIndex];
    }

    /// \brief Provides access to global indexes of stations used by track finder (const)
    const StationArray_t<int>& GetGeoToActiveMap() const { return fvGeoToActiveMap; }

    /// \brief Provides access to the map of active to geo station indices (const)
    const StationArray_t<int>& GetActiveToGeoMap() const { return fvActiveToGeoMap; }

    /// \brief Provides access to L1CAIteration vector (const)
    const IterationsContainer_t& GetCAIterations() const { return fCAIterations; }

    /// \brief Provides number of iterations
    int GetNcaIterations() const { return fCAIterations.size(); }

    /// \brief Provides access to L1Stations container (const)
    const StationsContainer_t<DataT>& GetStations() const { return fStations; }

    /// \brief Gets reference to the particular station
    /// \param iStation  Index of station in the active stations container
    const Station<DataT>& GetStation(int iStation) const { return fStations[iStation]; }

    /// \brief Gets a search window for a selected station and track group
    /// \note  For a particular track finder iteration one can select a track group, which is defined by the minimal
    ///        momentum of tracks (fast tracks, all tracks), their vertex (primary or secondary tracks), or by particle
    ///        (electrons, muons, hadrons, etc.)
    /// \param iStation  Global index of active station
    /// \param iTrackGr  Index of a track group
    const SearchWindow& GetSearchWindow(int iStation, int iTrackGr) const
    {
      assert(iStation < fNstationsActiveTotal && iStation > 0);
      assert(iTrackGr < int(fCAIterations.size()));
      return fSearchWindows[iTrackGr * constants::size::MaxNstations + iStation];
    }

    /// \brief Provides access to the map of search windows vs. active station global index and tracks group (const)
    const std::array<SearchWindow, constants::size::MaxNstations * constants::size::MaxNtrackGroups>&
    GetSearchWindows() const
    {
      return fSearchWindows;
    }

    /// \brief Gets X component of target position
    DataT GetTargetPositionX() const { return fTargetPos[0]; }

    /// \brief Gets Y component of target position
    DataT GetTargetPositionY() const { return fTargetPos[1]; }

    /// \brief Gets Z component of target position
    DataT GetTargetPositionZ() const { return fTargetPos[2]; }

    /// \brief Gets kf::FieldRegion object at primary vertex
    const kf::FieldRegion<DataT>& GetVertexFieldRegion() const { return fVertexFieldRegion; }

    /// \brief Gets kf::FieldValue object at primary vertex
    const kf::FieldValue<DataT>& GetVertexFieldValue() const { return fVertexFieldValue; }

    /// \brief Gets random seed
    ///
    /// If random seed is zero, std::random_device is used to seed the random number generator.
    int GetRandomSeed() const { return fRandomSeed; }

    /// \brief Gets active setup
    const auto& GetActiveSetup() const { return fActiveSetup; }

    /// \brief Gets active setup
    const auto& GetGeometrySetup() const { return fGeometrySetup; }

    /// \brief Gets ghost suppression flag
    int GetGhostSuppression() const { return fGhostSuppression; }

    /// \brief Gets default mass
    float GetDefaultMass() const { return fDefaultMass; }

    /// \brief Gets misalignment of the detector systems in X, squared rms
    float GetMisalignmentXsq(EDetectorID detId) const
    {
      auto iDet = static_cast<DetectorID_t>(detId);
      return fMisalignmentX[iDet] * fMisalignmentX[iDet];
    }

    /// \brief Gets misalignment of the detector systems in Y, squared rms
    float GetMisalignmentYsq(EDetectorID detId) const
    {
      auto iDet = static_cast<DetectorID_t>(detId);
      return fMisalignmentY[iDet] * fMisalignmentY[iDet];
    }

    /// \brief Gets miscalibration of the detector systems in Time, squared rms
    float GetMisalignmentTsq(EDetectorID detId) const
    {
      auto iDet = static_cast<DetectorID_t>(detId);
      return fMisalignmentT[iDet] * fMisalignmentT[iDet];
    }

    /// \brief Provides access to the misalignment of the detector systems in X
    const std::array<float, constants::size::MaxNdetectors> GetMisalignmentX() const { return fMisalignmentX; }

    /// \brief Provides access to the misalignment of the detector systems in Y
    const std::array<float, constants::size::MaxNdetectors> GetMisalignmentY() const { return fMisalignmentY; }

    /// \brief Provides access to the misalignment of the detector systems in Time
    const std::array<float, constants::size::MaxNdetectors> GetMisalignmentT() const { return fMisalignmentT; }

    /// \brief Checks, if the detector subsystem active
    /// \param detId  Detector ID
    bool IsActive(EDetectorID detId) const { return GetNstationsActive(detId) != 0; }

    /// \brief Class invariant checker
    void CheckConsistency() const;

    // ***************************
    // ** Flags for development **
    // ***************************

    /// \brief Are the hit search areas ignored
    bool DevIsIgnoreHitSearchAreas() const { return fDevIsIgnoreHitSearchAreas; }

    /// \brief Is original field must be used instead of the approximated one
    bool DevIsUseOfOriginalField() const { return fDevIsUseOfOriginalField; }

    /// \brief Flag to match doublets using MC information
    bool DevIsMatchDoubletsViaMc() const { return fDevIsMatchDoubletsViaMc; }

    /// \brief Flag to match triplets using Mc information
    bool DevIsMatchTripletsViaMc() const { return fDevIsMatchTripletsViaMc; }

    /// \brief Flag to extend tracks using Mc information
    bool DevIsExtendTracksViaMc() const { return fDevIsExtendTracksViaMc; }

    /// \brief Flag to match hits in overlaps using Mc information
    bool DevIsSuppressOverlapHitsViaMc() const { return fDevIsSuppressOverlapHitsViaMc; }

    bool DevIsParSearchWUsed() const { return fDevIsParSearchWUsed; }

   private:
    /// \brief Geometrical KF-setup (including inactive stations)
    kf::Setup<DataT> fGeometrySetup{kf::EFieldMode::Intrpl};

    /// \brief Active KF-setup (every layer is an active tracking station)
    kf::Setup<DataT> fActiveSetup{kf::EFieldMode::Intrpl};

    unsigned int fMaxDoubletsPerSinglet{150};  ///< Upper-bound cut on max number of doublets per one singlet
    unsigned int fMaxTripletPerDoublets{15};   ///< Upper-bound cut on max number of triplets per one doublet

    alignas(constants::misc::Alignment) IterationsContainer_t fCAIterations{
      "ca::Parameters::fCAIterations"};  ///< L1 tracking iterations vector

    /*************************
     ** Geometry parameters **
     *************************/
    /// \brief Target position
    alignas(constants::misc::Alignment) std::array<DataT, 3> fTargetPos{Undef<float>, Undef<float>, Undef<float>};

    /// Field value object at primary vertex (between target and the first station)
    kf::FieldValue<DataT> fVertexFieldValue{};

    /// Field region object at primary vertex (between target and the first station)
    kf::FieldRegion<DataT> fVertexFieldRegion{};

    /// Array of stations
    alignas(constants::misc::Alignment) StationsContainer_t<DataT> fStations{};

    // ** Station layout arrays **
    /// \brief First index of the station on the particular detector
    ///
    /// The last element of the array corresponds to the total number of geometry stations
    alignas(constants::misc::Alignment) std::array<int, constants::size::MaxNdetectors + 1> fvFirstGeoId{};

    /// \brief Map of (local, det) to geo indices
    ///
    /// Usage:
    ///   iStGeo = fvLocaToGeoIdMap[fvFirstGeoId[iDet] + iStLocal];
    ///   geo index.
    alignas(constants::misc::Alignment) StationArray_t<int> fvLocalToGeoIdMap{};

    /// \brief Map of geo to (local, det) indices
    alignas(constants::misc::Alignment) StationArray_t<std::pair<EDetectorID, int>> fvGeoToLocalIdMap{};

    /// \brief Map of geo to active indices
    ///
    /// The vector maps actual station index (which is defined by ) to the index of station in tracking. If the station
    /// is inactive, its index is equal to -1.
    /// \example Let stations 1 and 4 be inactive. Then:
    ///   geometry index:  0  1  2  3  4  5  6  7  8  9  0  0  0  0
    ///   active index:    0 -1  1  2 -1  3  4  5  6  7  0  0  0  0
    alignas(constants::misc::Alignment) StationArray_t<int> fvGeoToActiveMap{};

    /// \brief Map of active to geo indices
    alignas(constants::misc::Alignment) StationArray_t<int> fvActiveToGeoMap{};


    alignas(constants::misc::Alignment) int fNstationsActiveTotal = -1;  ///< total number of active tracking stations


    /// \brief Map of search windows vs. active station global index and tracks group
    ///
    /// The tracks group can be defined by minimum momentum (fast/all tracks), origin (primary/secondary) and particle
    /// type (electron, muon, all). Other options also can be added
    alignas(constants::misc::Alignment)
      std::array<SearchWindow, constants::size::MaxNstations* constants::size::MaxNtrackGroups> fSearchWindows = {};

    int fGhostSuppression = 0;  ///< flag: if true, ghost tracks are suppressed
    int fRandomSeed       = 1;  ///< random seed
    float fDefaultMass    = constants::phys::MuonMass;

    /// misalignment of the detector systems in X
    std::array<float, constants::size::MaxNdetectors> fMisalignmentX{0.};

    /// misalignment of the detector systems in Y
    std::array<float, constants::size::MaxNdetectors> fMisalignmentY{0.};

    /// miscalibration of the detector systems in Time
    std::array<float, constants::size::MaxNdetectors> fMisalignmentT{0.};

    // ***************************
    // ** Flags for development **
    // ***************************

    bool fDevIsIgnoreHitSearchAreas{false};      ///< Process all hits on the station ignoring hit search area
    bool fDevIsUseOfOriginalField{false};        ///< Force use of original field
    bool fDevIsMatchDoubletsViaMc{false};        ///< Flag to match doublets using MC information
    bool fDevIsMatchTripletsViaMc{false};        ///< Flag to match triplets using Mc information
    bool fDevIsExtendTracksViaMc{false};         ///< Flag to extend tracks using Mc information
    bool fDevIsSuppressOverlapHitsViaMc{false};  ///< Flag to match hits in overlaps using Mc information

    bool fDevIsParSearchWUsed = false;  ///< Flag: when true, the parametrized search windows are used in track
                                        ///< finder; when false, the Kalman filter is used instead

    /// \brief Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int)
    {
      ar& fMaxDoubletsPerSinglet;
      ar& fMaxTripletPerDoublets;

      ar& fCAIterations;
      ar& fTargetPos;

      ar& fVertexFieldValue;
      ar& fVertexFieldRegion;

      ar& fStations;

      ar& fvFirstGeoId;
      ar& fvLocalToGeoIdMap;
      ar& fvGeoToLocalIdMap;
      ar& fvGeoToActiveMap;
      ar& fvActiveToGeoMap;
      ar& fNstationsActiveTotal;

      ar& fSearchWindows;

      ar& fGhostSuppression;
      ar& fRandomSeed;
      ar& fDefaultMass;

      ar& fMisalignmentX;
      ar& fMisalignmentY;
      ar& fMisalignmentT;

      ar& fDevIsIgnoreHitSearchAreas;
      ar& fDevIsUseOfOriginalField;
      ar& fDevIsMatchDoubletsViaMc;
      ar& fDevIsMatchTripletsViaMc;
      ar& fDevIsExtendTracksViaMc;
      ar& fDevIsSuppressOverlapHitsViaMc;
    }
  };
}  // namespace cbm::algo::ca
