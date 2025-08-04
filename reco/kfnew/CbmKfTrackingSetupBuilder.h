/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaTrackingSetupBuilder.h
/// \brief  Tracking setup initializer in CBM (source)
/// \since  28.08.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CbmDefs.h"
#include "CbmEnumArray.h"
#include "KfSetupBuilder.h"
#include "TString.h"

#include <mutex>
#include <tuple>

namespace cbm::kf
{
  /// \class TrackingSetupBuilder
  /// \brief Encapsulation of the kf::Setup initialization routines for CBM
  class TrackingSetupBuilder {
   public:
    /// \brief Instance access
    static TrackingSetupBuilder* Instance();

    /// \brief Gets a shared pointer to the geometry setup
    /// \note  The original magnetic field is defined.
    /// \note  Use-cases: precise fit in physical analyses and QA.
    std::shared_ptr<const cbm::algo::kf::Setup<double>> GetSharedGeoSetup();

    /// \brief Makes setup object
    /// \param fldMode  Field mode (kf::EFiledMode)
    template<typename T>
    cbm::algo::kf::Setup<T> MakeSetup(cbm::algo::kf::EFieldMode fldMode)
    {
      if (!fbInitialized) {
        this->Init();
      }
      return fBuilder.MakeSetup<T>(fldMode);
    }

    /// \brief  Checks, if a tracking detector is used (is in geometry and has hits)
    bool IsInGeometry(cbm::algo::ca::EDetectorID detID) const { return fvbDetInGeometry[detID]; }

    /// \brief  Checks, if a tracking detector has hits
    bool HasHits(cbm::algo::ca::EDetectorID detID) const { return fvbDetHasHits[detID]; }

    /// \brief  Sets hits ignoring (DEBUG FLAG)
    void SetIgnoreHitPresence(bool ok = true)
    {
      fbIgnoreHitPresence = ok;
      fbInitialized       = false;
      CheckDetectorPresence();
    }

    // Disable copy and move
    TrackingSetupBuilder(const TrackingSetupBuilder&) = delete;
    TrackingSetupBuilder(TrackingSetupBuilder&&)      = delete;
    TrackingSetupBuilder& operator=(const TrackingSetupBuilder&) = delete;
    TrackingSetupBuilder& operator=(TrackingSetupBuilder&&) = delete;

   private:
    template<typename T>
    using DetectorIDArray_t = cbm::core::EnumArray<cbm::algo::ca::EDetectorID, T>;

    /// \brief Hit branch names vs. cbm::algo::ca::EDetectorID
    static constexpr DetectorIDArray_t<const char*> kDetHitBrName{
      {"MvdHit", "StsHit", "MuchPixelHit", "TrdHit", "TofHit"}};

    /// \brief Default constructor
    TrackingSetupBuilder() = default;

    /// \brief Destructor
    ~TrackingSetupBuilder() = default;

    /// \brief  Check detector presence
    void CheckDetectorPresence();

    /// \brief Sets the material budget cache file name
    /// \param filename  Material budget cache file name
    /// \param geoHash   A hash of the geometry
    ///
    /// If provided, the instance will try to read the material budget maps from the file.
    /// If the file does not exist,or the geometry hash was changed since the last time (reference hash differs from
    /// the one read from the file), a warning will be produced, the material budget maps will be recreated on the fly
    /// and they will be stored again to the file (meaning a new cache file will be generated over the existing one).
    void SetMaterialCacheFile(const TString& filename, size_t geoHash)
    {
      fBuilder.SetMaterialCacheFile(filename.Data(), geoHash);
    }

    /// \brief  Initializes the instance
    /// \note   Is executed on the first call of MakeSetup function
    void Init();

    // Material map creator properties (TODO: Provide setters, if needed)
    static constexpr double kMatCreatorPitch{0.1};    ///< Material budget map minimal bin size [cm]
    static constexpr int kMatCreatorMaxNbins{100};    ///< Max number of bins in the material budget map in x(y) axis
    static constexpr int kMatCreatorNrays{3};         ///< Number of rays per dimension for the material budget
    static constexpr bool kMatCreatorSafeMode{true};  ///< Safe mode of the material map creation
    static constexpr double kTargFieldInitStep{2.5};  ///< Step between nodes in the target field initialization [cm]
    static constexpr double kTargMaterialOffset{1};   ///< Offset between target upper limit and its material zMax [cm]

    inline static TrackingSetupBuilder* fpInstance{nullptr};
    inline static std::mutex fMutex{};

    cbm::algo::kf::SetupBuilder fBuilder{};       ///< KF-setup builder
    DetectorIDArray_t<bool> fvbDetInGeometry{{false}};  ///< Is detector subsystem in geometry?
    DetectorIDArray_t<bool> fvbDetHasHits{{false}};     ///< Does detector subsystem have hits?

    /// \brief An instance of the tracking KF-setup in a double precision
    /// \note  The original magnetic field is defined.
    /// \note  Use-cases: precise fit in physical analyses and QA.
    std::shared_ptr<cbm::algo::kf::Setup<double>> fpGeoSetup{nullptr};

    /// \brief Checks, if the setup was already initialized
    /// \note  Each call of the setup initializer resets the setup builder, so the initialization is called
    ///        in the next MakeSetup call
    bool fbInitialized{false};        ///< Check, if the setup builder initialized
    bool fbIgnoreHitPresence{false};  ///< Ignores hit presence, when active subsystems are determined
  };


}  // namespace cbm::kf
