/* Copyright (C) 2021-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaInitManager.h
/// \brief  Input data management class for the CA tracking algorithm (header)
/// \since  24.12.2021
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaIteration.h"
#include "CaObjectInitController.h"
#include "CaParameters.h"
#include "CaSimd.h"
#include "CaStationInitializer.h"
#include "KfFieldRegion.h"

#include <array>
#include <bitset>
#include <memory>  //unique_ptr
#include <numeric>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace cbm::algo::ca
{
  /// \enum cbm::algo::ca::EDetectorID
  /// \brief Forward declaration of the tracking detectors scoped enumeration
  ///
  /// Concrete realization of this enumeration must be determined in the concrete setup class (i.e. CbmL1)
  enum class EDetectorID;
}  // namespace cbm::algo::ca

/// \brief Hash function definition for EDetectorID
template<>
struct std::hash<cbm::algo::ca::EDetectorID> {
  int operator()(cbm::algo::ca::EDetectorID t) const { return static_cast<int>(t); }
};

namespace cbm::algo::ca
{
  /// \brief Underlying integer type for the DetectorID
  using DetectorID_t = std::underlying_type_t<EDetectorID>;

  /// \class cbm::algo::ca::InitManager
  /// \brief A CA Parameters object initialization class
  ///
  /// This class provides an interface to form a solid Parameters object, which is used by the CA algorithm and
  /// the related routines. The initialization can be performed either from the detector-specific interfaces or by
  /// reading the already prepared binary wile with extention ca.par TODO:.... continue
  ///
  class InitManager {
   private:
    /// \brief Init-controller key set
    enum class EInitKey
    {
      // NOTE: Please, keep the numbers of the enumeration items in the existing order: it helps to debug the
      //       initialization with this->GetObjectInitController().ToString() method call (S.Zharko)
      kFieldFunction,                 ///< 0) If magnetic field getter function is set
      kTargetPos,                     ///< 1) If target position was defined
      kPrimaryVertexField,            ///< 2) If magnetic field value and region defined at primary vertex
      kStationsInfo,                  ///< 3) If all the planned stations were added to the manager
      kCAIterationsNumberCrosscheck,  ///< 4) If the number of CA track finder is initialized
      kCAIterations,                  ///< 5) If the CA track finder iterations were initialized
      kSearchWindows,                 ///< 6) If the hit search windows were initialized
      kGhostSuppression,              ///< 7)
      kRandomSeed,                    ///< 8) If the random seed is provided
      kStationLayoutInitialized,      ///< 9) If stations layout is initialized
      kSetupInitialized,              ///< 10) If KF-setup initialized
      kEnd                            ///< 11) [technical] number of entries in the enumeration
    };

    using DetectorIDIntMap_t = std::unordered_map<EDetectorID, int>;
    using DetectorIDSet_t    = std::set<EDetectorID>;
    using FieldFunction_t    = std::function<void(const double (&xyz)[3], double (&B)[3])>;
    using InitController_t   = ObjectInitController<static_cast<int>(EInitKey::kEnd), EInitKey>;
    template<typename T>
    using DetectorIDArr_t = std::array<T, constants::size::MaxNdetectors>;

   public:
    /// \brief Default constructor
    InitManager() = default;

    /// \brief Destructor
    ~InitManager() = default;

    /// \brief Copy constructor is forbidden
    InitManager(const InitManager& /*other*/) = delete;

    /// \brief Move constructor is forbidden
    InitManager(InitManager&& /*other*/) = delete;

    /// \brief Copy assignment operator is forbidden
    InitManager& operator=(const InitManager& /*other*/) = delete;

    /// \brief Move assignment operator is forbidden
    InitManager& operator=(InitManager&& /*other*/) = delete;

    /// \brief Adds a tracking station to the geometry
    ///
    /// \note The added station stays uninitialized until the Parameters object is formed
    void AddStation(const StationInitializer& station);

    /// \brief Adds tracking stations from the parameters file
    /// \param par  A valid instance of parameters, created for ACTIVE + INACTIVE tracking stations
    ///
    // TODO: Probably, we have to remove stations from the parameters and place them into a class ca::Geometery
    //       together with instances of active and geometry kf::Setup. ca::Parameters should contain only tracking
    //       parameters, which should be fully defined from the config. Search windows should be stored as an
    //       independent object, which depends on the ca::Parameters(iterations) and ca::Geometry(active station
    //       layout).
    void AddStations(const Parameters<fvec>& par);

    /// \brief Provides final checks of the parameters object
    void CheckInit();

    /// \brief Clears vector of CA track finder iterations
    void ClearCAIterations();

    /// \brief Clears vector of base setup
    void ClearSetupInfo();

    /// \brief  Forms parameters container
    /// \return Success flag: true - the container is formed, false - error while forming the container occured
    bool FormParametersContainer();

    /// \brief  Gets name of the detector
    /// \param  detId Index of the detector
    /// \return Name of the detector
    const std::string& GetDetectorName(EDetectorID detId) const { return fvDetectorNames[static_cast<int>(detId)]; }

    /// \brief Gets ghost suppression flag
    int GetGhostSuppression() const { return fParameters.fGhostSuppression; }

    /// \brief Gets a name of the main input configuration file
    const std::string& GetInputConfigMain() const { return fsConfigInputMain; }

    /// \brief Gets a name of the user input configuration file
    const std::string& GetInputConfigUser() const { return fsConfigInputMain; }

    /// \brief Gets a const reference to ca::ObjectInitController
    const InitController_t& GetInitController() const { return fInitController; }

    /// \brief Gets total number of active stations
    int GetNstationsActive() const;

    /// \brief Gets number of active stations for given detector ID
    int GetNstationsActive(EDetectorID detectorID) const;

    /// \brief Gets total number of stations, provided by setup geometry
    int GetNstationsGeometry() const;

    /// \brief Gets number of stations, provided by setup geometry for given detector ID
    int GetNstationsGeometry(EDetectorID detectorID) const;

    /// \brief Gets a name of the output configuration file
    const std::string& GetOutputConfigName() const { return fConfigOutputName; }

    /// \brief Gets a reference to the stations array
    std::vector<StationInitializer>& GetStationInfo();

    /// \brief Initializes station layout
    ///
    /// This function is to be called after all the tracking stations (StationInitializer objects) are added to the
    /// InitManager instance. After the initialization the vector of the tracking stations is sorted by z-positions
    /// and is available for modifications.
    void InitStationLayout();

    /// \brief Calculates kf::FieldValue and L1FieldReference values for a selected step in z-axis from the target position
    /// \param zStep step between nodal points
    void InitTargetField(double zStep);

    /// \brief Checks, if the detector is active
    bool IsActive(EDetectorID detectorID) const { return GetNstationsActive(detectorID) != 0; }

    /// \brief Checks, if the detector is present in the geometry
    bool IsPresent(EDetectorID detectorID) const { return GetNstationsGeometry(detectorID) != 0; }

    /// \brief Pushes an CA track finder iteration into a sequence of iteration using reference
    void PushBackCAIteration(const Iteration& iteration);

    /// \brief Pushes an CA track finder iteration into a sequence of iteration using raw pointer
    void PushBackCAIteration(const Iteration* pIteration) { PushBackCAIteration(*pIteration); }

    /// \brief Pushes an CA track finder iteration into a sequence of iteration using std::unique_ptr
    void PushBackCAIteration(const std::unique_ptr<Iteration>& puIteration) { PushBackCAIteration(*puIteration); }

    /// \brief Reads main and user parameters configs
    void ReadInputConfigs();

    /// \brief Reads geometry setup from file
    /// \param fileName  Name of input file
    void ReadGeometrySetup(const std::string& fileName);

    /// \brief Reads parameters object from boost-serialized binary file
    /// \param  fileName  Name of input file
    void ReadParametersObject(const std::string& fileName);

    /// \brief Reads search windows from file
    /// \param  fileName  Name of input file
    void ReadSearchWindows(const std::string& fileName);

    /// \brief Sets a number of CA track finder iterations to provide initialization cross-check
    // TODO: remove this method
    void SetCAIterationsNumberCrosscheck(int nIterations);

    /// \brief Sets base configuration file
    /// \param mainConfig  Path to main configuration file
    /// \note  The base configuraiton file is mandatory until the tracking configuration is initialized from
    ///        beforehand created Parameters file.
    void SetConfigMain(const std::string& mainConfig) { fsConfigInputMain = mainConfig; }

    /// \brief Sets user configuration file
    /// \param userConfig  Path to user configuration file
    /// \note  The user configuraiton file is optional
    void SetConfigUser(const std::string& userConfig) { fsConfigInputUser = userConfig; }

    /// \brief Sets detector names
    /// \param container  Container of the detector names
    template<size_t Size>
    void SetDetectorNames(const std::array<const char*, Size>& container)
    {
      static_assert(Size <= constants::size::MaxNdetectors,
                    "Please, be ensured that the constants::size::MaxNdetectors is not lower then the "
                    "EDetectorID::kEND value, provided by your setup");
      std::copy(container.begin(), container.end(), fvDetectorNames.begin());
    }

    /// Sets a magnetic field function, which will be applied for all the stations
    void SetFieldFunction(const FieldFunction_t& fieldFcn);

    /// \brief Sets the flag to enable/disable the ghost suppression routine
    void SetGhostSuppression(int ghostSuppression);

    /// \brief Sets a name of the output configuration file
    /// \param  filename  Name of the output CA parameters configuration
    ///
    /// The output file is created from the fields, saved in the resulted Parameters object.
    void SetOutputConfigName(const std::string& filename) { fConfigOutputName = filename; }

    /// \brief Sets pseudo-random numbers generator seed
    /// \param seed  Seed value
    /// \note  The default seed is 1
    void SetRandomSeed(unsigned int seed);

    // TODO: Use kf::Setup instead
    /// \brief Sets target position
    /// \param  x  Position X component [cm]
    /// \param  y  Position Y component [cm]
    /// \param  z  Position Z component [cm]
    void SetTargetPosition(double x, double y, double z);

    /// \brief Sets upper-bound cut on max number of doublets per one singlet
    void SetMaxDoubletsPerSinglet(unsigned int value) { fParameters.fMaxDoubletsPerSinglet = value; }

    /// \brief Sets upper-bound cut on max number of triplets per one doublet
    void SetMaxTripletPerDoublets(unsigned int value) { fParameters.fMaxTripletPerDoublets = value; }

    /// \brief Sets setup
    /// \tparam  Underlying type of the setup
    template<typename DataT>
    void SetGeometrySetup(const cbm::algo::kf::Setup<DataT>& setup)
    {
      if (!fInitController.GetFlag(EInitKey::kStationLayoutInitialized)) {
        std::stringstream msg;
        msg << "ca::InitManager: setup cannot be set until the station layout is initialized";
        throw std::runtime_error(msg.str());
      }
      fParameters.fGeometrySetup = kf::Setup<fvec>(setup);
      fParameters.fActiveSetup   = fParameters.fGeometrySetup;
      // A sequence of the last inactive materials will be anyway thrown away, so it is more effective to
      // loop over stations downstream
      for (int iStGeo = setup.GetNofLayers() - 1; iStGeo >= 0; --iStGeo) {
        auto [detID, locID] = fParameters.GetStationIndexLocal(iStGeo);
        int iStActive       = fParameters.GetStationIndexActive(locID, detID);
        if (iStActive < 0) {
          fParameters.fActiveSetup.DisableLayer(detID, locID);
        }
      }
      LOG(info) << "Geometry setup:" << fParameters.fGeometrySetup.ToString(1);
      LOG(info) << "Active setup:" << fParameters.fActiveSetup.ToString(1);
      fInitController.SetFlag(EInitKey::kSetupInitialized, true);
    }

    /// \brief Sets misalignment parameters in X direction
    /// \param  detectorId  Index of the detector system
    /// \param  x  Misalignment tolerance in x [cm]
    /// \param  y  Misalignment tolerance in y [cm]
    /// \param  t  Misalignment tolerance in t [ns]
    void SetMisalignmentTolerance(EDetectorID detectorId, double x, double y, double t)
    {
      fParameters.fMisalignmentX[static_cast<int>(detectorId)] = x;
      fParameters.fMisalignmentY[static_cast<int>(detectorId)] = y;
      fParameters.fMisalignmentT[static_cast<int>(detectorId)] = t;
    }

    /// \brief Sets default fitter mass
    /// \param mass Particle mass [GeV/c2]
    void SetDefaultMass(double mass) { fParameters.fDefaultMass = mass; }

    /// \brief  Takes parameters object from the init-manager instance
    /// \return A parameter object
    Parameters<fvec>&& TakeParameters();

    /// \brief Writes parameters object from boost-serialized binary file
    /// \param  fileName  Name of input file
    void WriteParametersObject(const std::string& fileName) const;

    // ***************************
    // ** Flags for development **
    // ***************************

    /// \brief Ignore hit search areas
    void DevSetIgnoreHitSearchAreas(bool value = true) { fParameters.fDevIsIgnoreHitSearchAreas = value; }

    /// \brief Force use of the original field (not approximated)
    void DevSetUseOfOriginalField(bool value = true) { fParameters.fDevIsUseOfOriginalField = value; }

    /// \brief Flag to match doublets using MC information
    void DevSetIsMatchDoubletsViaMc(bool value = true) { fParameters.fDevIsMatchDoubletsViaMc = value; }

    /// \brief Flag to match triplets using Mc information
    void DevSetIsMatchTripletsViaMc(bool value = true) { fParameters.fDevIsMatchTripletsViaMc = value; }

    /// \brief Flag to match triplets using Mc information
    void DevSetIsExtendTracksViaMc(bool value = true) { fParameters.fDevIsExtendTracksViaMc = value; }

    /// \brief Flag to match triplets using Mc information
    void DevSetIsSuppressOverlapHitsViaMc(bool value = true) { fParameters.fDevIsSuppressOverlapHitsViaMc = value; }

    /// \brief Flag to use estimated hit search windows
    /// \param true   estimated search windows will be used in track finder
    /// \param false  the Kalman filter is be used in track finder
    void DevSetIsParSearchWUsed(bool value = true) { fParameters.fDevIsParSearchWUsed = value; }

   private:
    /// \brief Checker for Iteration container initialization (sets EInitKey::kCAIterations)
    /// \return true If all Iteration objects were initialized properly
    void CheckCAIterationsInit();

    /// \brief Checker for StationInitializer set initialization (sets EInitKey::kStationsInfo)
    /// \return true If all StationInitializer objects were initialized properly. Similar effect can be achieved by
    void CheckStationsInfoInit();

    /// \brief Returns station layout into undefined condition
    void ClearStationLayout();

    InitController_t fInitController{};              ///< Initialization flags
    DetectorIDArr_t<std::string> fvDetectorNames{};  ///< Names of the detectors

    double fTargetZ{0.};  ///< Target position z component in double precision

    std::vector<StationInitializer> fvStationInfo{};  ///< Vector of StationInitializer objects (active + inactive)

    /// A function which returns magnetic field vector B in a radius-vector xyz
    FieldFunction_t fFieldFunction{[](const double (&)[3], double (&)[3]) {}};
    // NOTE: Stations of the detectors which are not assigned as active, are not included in the tracking!

    // TODO: remove
    int fCAIterationsNumberCrosscheck{-1};  ///< Number of iterations to be passed (must be used for cross-checks)

    Parameters<fvec> fParameters{};  ///< CA parameters object
    // TODO: With a separate KF-framework instance we need to figure it out, how to store and read the corresponding
    //       parameters (essential for the online reconstruction!!!)

    std::string fsConfigInputMain = "";  ///< name for the input configuration file
    std::string fsConfigInputUser = "";  ///< name for the input configuration file
    std::string fConfigOutputName = "";  ///< name for the output configuration file

    bool fbConfigIsRead       = false;  ///< Flag, if configuration file was read
    bool fbGeometryConfigLock = false;  ///< Lock geometry initialization
  };


}  // namespace cbm::algo::ca
