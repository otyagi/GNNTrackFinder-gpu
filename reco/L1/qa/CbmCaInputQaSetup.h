/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaInputQaSetup.h
/// \brief  QA task for tracking detector interfaces
/// \since  28.08.2023
/// \author S.Zharko <s.zharko@gsi.de>


#pragma once

#include "CaParameters.h"
#include "CbmL1DetectorID.h"
#include "CbmMCDataArray.h"
#include "CbmMCDataObject.h"
#include "CbmMCEventList.h"
#include "CbmMuchPixelHit.h"
#include "CbmMuchPoint.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdHit.h"
#include "CbmMvdPoint.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmQaTask.h"
#include "CbmStsHit.h"
#include "CbmStsPoint.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTimeSlice.h"
#include "CbmTofHit.h"
#include "CbmTofPoint.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdHit.h"
#include "CbmTrdPoint.h"
#include "CbmTrdTrackingInterface.h"
#include "TClonesArray.h"

namespace cbm::ca
{
  /// \class InputQaSetup
  /// \brief A QA task to analyze hit and MC point occupancy distributions in different tracking stations
  class InputQaSetup : public CbmQaTask {
   public:
    /// \brief Constructor from parameters
    /// \param verbose   Verbosity level
    /// \param isMCUsed  Flag, if MC information is available for this task
    InputQaSetup(int verbose, bool isMCUsed);

    /// \brief Reads defined parameters object from file
    /// \param filename  Name of parameter file
    /// \note  TEMPORARY FUNCTION, A SEPARATE PARAMETERS INITIALIZATION CLASS IS TO BE USED
    void ReadParameters(const char* filename) { fsParametersFilename = filename; }

    /// \brief Checks results of the QA and returns a success flag
    void Check() override;

    /// \brief Creates summary cavases, tables etc.
    void CreateSummary() override;

    /// \brief Fills histograms
    void ExecQa() override;

    /// \brief Initializes QA-task
    InitStatus InitQa() override;


   private:
    /// \brief Check out initialized detectors
    void CheckoutDetectors();

    /// \brief Checks branches initialization
    void CheckInit() const;

    /// \brief Fill histograms for a given detector type
    template<ca::EDetectorID DetID>
    void FillHistogramsDet();


    // *******************************
    // **  Data branches and flags  **
    // *******************************
    /// \brief Pointers to the tracking detector interfaces for each subsystem
    DetIdArr_t<const CbmTrackingDetectorInterfaceBase*> fvpDetInterface = {{nullptr}};

    DetIdArr_t<bool> fvbUseDet              = {{false}};    ///< Detector subsystem usage flag
    DetIdArr_t<TClonesArray*> fvpBrHits     = {{nullptr}};  ///< Input branch for hits
    DetIdArr_t<CbmMCDataArray*> fvpBrPoints = {{nullptr}};  ///< Input branch for MC points

    CbmMCDataObject* fpMCEventHeader                    = nullptr;  ///< Pointer to MC event header
    CbmMCEventList* fpMCEventList                       = nullptr;  ///< Pointer to MC event list
    std::shared_ptr<ca::Parameters<float>> fpParameters = nullptr;  ///< Pointer to CA parameters object
    std::string fsParametersFilename                    = "";       ///< Filename for the tracking parameters

    DetIdArr_t<std::vector<double>> fvXmin;
    DetIdArr_t<std::vector<double>> fvXmax;
    DetIdArr_t<std::vector<double>> fvYmin;
    DetIdArr_t<std::vector<double>> fvYmax;
    DetIdArr_t<std::vector<double>> fvZmin;
    DetIdArr_t<std::vector<double>> fvZmax;

    // ******************
    // **  Histograms  **
    // ******************

    std::vector<TH2F*> fph_hit_xy   = {};  ///< Hit occupancy in x-y plane vs. station
    std::vector<TH2F*> fph_hit_xz   = {};  ///< Hit occupancy in x-z plane vs. station
    std::vector<TH2F*> fph_hit_yz   = {};  ///< Hit occupancy in y-z plane vs. station
    std::vector<TH2F*> fph_point_xy = {};  ///< MC point occupancy in x-z plane vs. station
    std::vector<TH2F*> fph_point_xz = {};  ///< MC point occupancy in x-z plane vs. station
    std::vector<TH2F*> fph_point_yz = {};  ///< MC point occupancy in y-z plane vs. station
  };

}  // namespace cbm::ca
