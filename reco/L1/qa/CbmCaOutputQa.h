/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaOutputQa.h
/// \brief  Tracking output QA-task (header)
/// \since  24.02.2023
/// \author Sergei Zharko <s.zharko@gsi.de>


#pragma once

#include "CaMonitor.h"
#include "CaParameters.h"
#include "CaToolsDebugger.h"
#include "CaVector.h"
#include "CbmCaMCModule.h"
#include "CbmCaTimeSliceReader.h"
#include "CbmCaTrackTypeQa.h"
#include "CbmL1DetectorID.h"
#include "CbmL1Hit.h"
#include "CbmQaCmpDrawer.h"
#include "CbmQaTask.h"

#include <array>
#include <memory>

namespace cbm::ca
{
  /// \brief Enumeration fors track category
  enum ETrackType
  {
    kPrimLongFast,  ///< primary long tracks (all stations in set)
    kPrimLong,      ///< long primary
    kAll,           ///< all tracks
    kGhost,         ///< ghost tracks (no MC is used)
    kPrim,          ///< primary
    kPrimFast,      ///< primary fast
    kSec,           ///< secondary
    kSecFast,       ///< secondary fast
    kAllE,          ///< all e-/e+
    kPrimE,         ///< primary e-/e+
    kPrimEP,        ///< primary e+
    kPrimEM,        ///< primary e-
    kSecE,          ///< secondary e-/e+
    kSecEP,         ///< secondary e+
    kSecEM,         ///< secondary e-
    kAllMU,         ///< all mu+/mu-
    kPrimMU,        ///< primary mu+/mu-
    kPrimMUP,       ///< primary mu+
    kPrimMUM,       ///< primary mu-
    kSecMU,         ///< secondary mu+/mu-
    kSecMUP,        ///< secondary mu+
    kSecMUM,        ///< secondary mu-
    kAllPI,         ///< all pi+/pi-
    kPrimPI,        ///< primary pi+/pi-
    kPrimPIP,       ///< primary pi+
    kPrimPIM,       ///< primary pi-
    kSecPI,         ///< secondary pi+/pi-
    kSecPIP,        ///< secondary pi+
    kSecPIM,        ///< secondary pi-
    kAllK,          ///< all K+/K-
    kPrimK,         ///< primary K+/K-
    kPrimKP,        ///< primary K+
    kPrimKM,        ///< primary K-
    kSecK,          ///< secondary K+/K-
    kSecKP,         ///< secondary K+
    kSecKM,         ///< secondary K-
    kAllPPBAR,      ///< all p/p-bar
    kPrimPPBAR,     ///< primary p/p-bar
    kPrimP,         ///< primary p
    kPrimPBAR,      ///< primary p-bar
    kSecPPBAR,      ///< secondary p/p-bar
    kSecP,          ///< secondary p
    kSecPBAR,       ///< secondary p-bar
    // kPrimD,
    // kPrimDBAR,
    // kSecD,
    // kSecDBAR,
    // kPrimT,
    // kPrimTBAR,
    // kSecT,
    // kSecTBAR,
    // kPrim3HE,
    // kPrim3HEBAR,
    // kSec3HE,
    // kSec3HEBAR,
    // kPrim4HE,
    // kPrim4HEBAR,
    // kSec4HE,
    // kSec4HEBAR,
    END
  };

  /// \brief  QA-task for CA tracking output results
  ///
  class OutputQa : public CbmQaTask {
    // WIP: Temporary flag to switch between two different approaches of filling track type histograms
    //
    // 1) Experimental approach (kEXPTRACKFILL = true) utilizes unified filling based on defined reco and MC cuts.
    // 2) Feature is to be studied more precisely (descrepancy in primary/secondary track with a standard approach)
    // 3) Experimental approach runs in ~10% slower, then the standard
    static constexpr bool kEXPTRACKFILL = false;

    /// Array for track type properties
    template<typename T>
    using TTypeArr_t = std::array<T, ETrackType::END>;

    /// \brief Structure to keep drawing attributes of histograms
    struct DrawAtt {
      Color_t fColor  = 1;  ///< Marker and line color
      Style_t fMarker = 1;  ///< Marker style
    };

   public:
    /// \brief  Constructor from parameters
    /// \param  verbose   Verbosity level
    /// \param  isMCUsed  Flag, if MC information is available for this task
    /// \param  recoMode  Reconstruction mode (see documentation for the CbmQaTask::SetRecoMode function)
    OutputQa(int verbose, bool isMCUsed, ECbmRecoMode recoMode = ECbmRecoMode::EventByEvent);

    /// \brief Adds track type
    /// \param type  Track type
    /// \param flag  Flag: true/false
    ///
    /// Adds a track type for building distributions. By default, only all, primary, secondary and ghost track
    /// distributions are booked.
    void AddTrackType(ETrackType type, bool flag = true) { fvbTrackTypeOn[type] = flag; }

    /// \brief Enables debugger
    /// \param filename  Name of output ROOT file
    ///
    /// Creates a debugger and enables its usage inside a QA task
    void EnableDebugger(const char* filename)
    {
      if (!fpDebugger.get()) {
        fpDebugger = std::make_shared<tools::Debugger>(filename);
      }
    }

    /// \brief Reads defined parameters object from file
    /// \param filename  Name of parameter file
    /// \note  TEMPORARY FUNCTION, A SEPARATE PARAMETERS INITIALIZATION CLASS IS TO BE USED
    void ReadParameters(const char* filename) { fsParametersFilename = filename; }

    /// \brief Sets event display
    /// \param flag         On/off
    /// \param minNofPoints Minimum number of points to draw the event
    ///
    /// The event display draws xz- and yz-projection of the event snapshot, which includes MC and Reco tracks,
    /// hits and MC-points
    void SetEventDisplay(bool bDrawEvents, int minNofPoints = 2)
    {
      fbDrawEvents            = bDrawEvents;
      fEvtDisplayMinNofPoints = minNofPoints;
    }

    /// \brief Sets MVD use flag
    /// \param flag  Boolean flag: true - detector subsystem is used, false - detector subsystem is ignored
    void SetUseMvd(bool flag = true) { fbUseMvd = flag; }

    /// \brief Sets STS use flag
    /// \param flag  Boolean flag: true - detector subsystem is used, false - detector subsystem is ignored
    void SetUseSts(bool flag = true) { fbUseSts = flag; }

    /// \brief Sets MuCh use flag
    /// \param flag  Boolean flag: true - detector subsystem is used, false - detector subsystem is ignored
    void SetUseMuch(bool flag = true) { fbUseMuch = flag; }

    /// \brief Sets TRD use flag
    /// \param flag  Boolean flag: true - detector subsystem is used, false - detector subsystem is ignored
    void SetUseTrd(bool flag = true) { fbUseTrd = flag; }

    /// \brief Sets TOF use flag
    /// \param flag  Boolean flag: true - detector subsystem is used, false - detector subsystem is ignored
    void SetUseTof(bool flag = true) { fbUseTof = flag; }

    /// \brief Sets STS tracking mode
    void SetStsTrackingMode() { fTrackingMode = ECbmCaTrackingMode::kSTS; }

    /// \brief Sets mCBM global tracking mode
    void SetMcbmTrackingMode() { fTrackingMode = ECbmCaTrackingMode::kMCBM; }

    /// \brief Sets performance mode
    /// \param mode  Performance mode (1 is default: TODO - test)
    void SetPerformanceMode(int mode) { fPerformanceMode = mode; }

    /// \brief Set track type list for summary table
    /// \param trackTypes  A  of track type set
    void SetTrackTypeListForSummary(std::set<ETrackType>&& trackTypes)
    {
      fmSummaryTableEntries = std::move(trackTypes);
    }

   protected:
    /// \brief  Method to check, if the QA results are acceptable
    void Check() override;

    /// \brief Creates summary cavases, tables etc.
    void CreateSummary() override;

    /// \brief De-initializes histograms
    void DeInit() override {}

    /// \brief Fills histograms for a given track types
    void FillTrackTypeHistograms();

    /// \brief Initialises the QA-task
    InitStatus InitQa() override;

    /// \brief Fills histograms from time slice or event
    void ExecQa() override;

   private:
    /// \brief Fills reconstructed track by its index
    /// \param type      Track type
    /// \param iTrkReco  Index of reconstructed track
    [[gnu::always_inline]] void FillRecoTrack(ETrackType type, int iTrkReco)
    {
      if (fvbTrackTypeOn[type]) {
        fvpTrackHistograms[type]->FillRecoTrack(iTrkReco);
      }
    }

    /// \brief Fills MC track by its index
    /// \param type      Track type
    /// \param iTrkReco  Index of MC track
    void FillMCTrack(ETrackType type, int iTrkMC);

    /// \brief Draws event
    void DrawEvent();

    /// \brief Utility function to draw a generic comparison of histograms from different track types
    /// \tparam TObj    Type of ROOT object
    /// \param  vTypes  Vector of types to draw
    /// \param  GetObj  Function, returning an object of a given type
    template<class TObj>
    void DrawSetOf(const std::vector<ETrackType>& vTypes, std::function<TObj*(ETrackType)> GetObj);


    /// \brief Defines drawing attributes for histograms of different track types
    void InitDrawingAttributes();

    // Flags for detector subsystems being used
    bool fbUseMvd               = false;  ///< is MVD used
    bool fbUseSts               = false;  ///< is STS used
    bool fbUseMuch              = false;  ///< is MuCh used
    bool fbUseTrd               = false;  ///< is TRD used
    bool fbUseTof               = false;  ///< is TOF used
    bool fbDrawEvents           = false;  ///< flag to draw events with the event display
    int fEvtDisplayMinNofPoints = 2;      ///< minimum number of MC points in the event display
    int fPerformanceMode        = 1;      ///< Performance mode

    ECbmCaTrackingMode fTrackingMode = ECbmCaTrackingMode::kSTS;  ///< Tracking mode

    std::string fsParametersFilename                    = "";
    std::unique_ptr<TimeSliceReader> fpTSReader         = nullptr;  ///< Reader of the time slice
    std::shared_ptr<MCModule> fpMCModule                = nullptr;  ///< MC module
    std::shared_ptr<ca::DataManager> fpDataManager      = nullptr;  ///< Data manager
    std::shared_ptr<tools::Debugger> fpDebugger         = nullptr;  ///< Debugger
    std::shared_ptr<ca::Parameters<float>> fpParameters = nullptr;  ///< Tracking parameters object

    ca::Vector<CbmL1HitId> fvHitIds{"CbmCaOutputQa::fvHitIds"};
    ca::Vector<CbmL1HitDebugInfo> fvHits{"CbmCaOutputQa::fvHits"};
    ca::Vector<CbmL1Track> fvRecoTracks{"CbmCaOutputQa::fvRecoTracks"};
    tools::MCData fMCData;  ///< Input MC data (points and tracks)

    /// \enum  EMonitorKey
    /// \brief QA monitor counters
    enum class EMonitorKey
    {
      kEvent,
      kTrack,
      kHit,
      kMcTrack,
      kMcPoint,
      END
    };

    ca::Monitor<EMonitorKey> fMonitor{"Output tracking QA"};

    std::set<ETrackType> fmSummaryTableEntries;  ///< Which track types should be listed in the summary table

    // *************************
    // **  List of histograms **
    // *************************

    TTypeArr_t<std::string> fvsTrackTypeName;                     ///< Array of track type unique names
    TTypeArr_t<std::unique_ptr<TrackTypeQa>> fvpTrackHistograms;  ///< Histogrammers for different track types
    TTypeArr_t<bool> fvbTrackTypeOn = {0};                        ///< Usage flag for different track types
    TTypeArr_t<DrawAtt> fvTrackDrawAtts;                          ///< Drawing attributes for track types

    // ************************************
    // ** Drawing options and properties **
    // ************************************

    static constexpr int kCXSIZEPX = 600;  ///< Canvas size along x-axis [px]
    static constexpr int kCYSIZEPX = 600;  ///< Canvas size along y-axis [px]

    ClassDefOverride(OutputQa, 0);
  };

}  // namespace cbm::ca


// **********************
// **  Implementation  **
// **********************

// ---------------------------------------------------------------------------------------------------------------------
//
template<class TObj>
void cbm::ca::OutputQa::DrawSetOf(const std::vector<ETrackType>& vTypes, std::function<TObj*(ETrackType)> GetObj)
{
  CbmQaCmpDrawer<TObj> drawer;
  for (auto type : vTypes) {
    if (!fvbTrackTypeOn[type] || !fvpTrackHistograms[type]->IsMCUsed()) {
      continue;
    }
    drawer.RegisterObject(GetObj(type), fvpTrackHistograms[type]->GetTitle());
  }
  if constexpr (std::is_same_v<TH1F, TObj>) {
    drawer.SetMinimum(1.e-1);
  }
  drawer.Draw("");
  drawer.Clear();
}

// ---------------------------------------------------------------------------------------------------------------------
//
/// \brief Fills MC track by its index
/// \param type      Track type
/// \param iTrkReco  Index of MC track
[[gnu::always_inline]] inline void cbm::ca::OutputQa::FillMCTrack(ETrackType type, int iTrkMC)
{
  // Fill histograms and efficiency profiles
  if (fvbTrackTypeOn[type]) {
    fvpTrackHistograms[type]->FillMCTrack(iTrkMC);
  }
}
