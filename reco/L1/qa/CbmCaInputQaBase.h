/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaInputQaBase.h
/// @date   13.01.2023
/// \brief  Class providing basic functionality for CA input QA-tasks
/// @author S.Zharko <s.zharko@gsi.de>


#pragma once

#include "CaMonitor.h"
#include "CbmCaHitQaData.h"
#include "CbmCaInputQaBase.h"
#include "CbmL1DetectorID.h"
#include "CbmMCDataManager.h"
#include "CbmPixelHit.h"
#include "CbmQaTask.h"
#include "TMath.h"

#include <set>
#include <unordered_map>
#include <vector>

class CbmMatch;
class CbmMCEventList;
class CbmMCDataArray;
class CbmMCDataManager;
class CbmMCTrack;
class CbmStsHit;
class CbmStsPoint;
class CbmTrackingDetectorInterfaceBase;
class CbmTimeSlice;
class FairMCPoint;
class TClonesArray;
class TH1F;
class TH2F;
class TProfile;
class TProfile2D;

namespace
{
  using cbm::algo::ca::EDetectorID;
  using cbm::algo::ca::Monitor;
  using cbm::algo::yaml::Property;
}  // namespace

/// \class CbmCaInputQaBase
/// \tparam DetID  Detector ID
/// \brief A QA-task class, which provides assurance of MuCh hits and geometry
template<EDetectorID DetID>
class CbmCaInputQaBase : public CbmQaTask {
 protected:
  using Point_t = cbm::ca::PointTypes_t::at<DetID>;  ///< Point type for a given detector ID
  using Hit_t   = cbm::ca::HitTypes_t::at<DetID>;    ///< Hit type for a given detector ID

  /// \struct Config
  /// \brief  Specific configuration for the CA input QA task
  struct Config {
    /// \struct McTrackCut
    /// \brief  MC track selection criteria
    struct McTrackCuts {
      double fMinMom   = 0.1;   ///< Track momentum on the exit of the station [GeV/c]
      double fMaxTheta = 60.;   ///< Track theta on the exit of the station [grad]
      bool fbPrimary   = true;  ///< Must the track come from the primary vertex
      /* clang-format off */
      CBM_YAML_PROPERTIES(
        Property(&McTrackCuts::fMinMom, "MinMom", "Minimal momentum at the exit of the station [GeV/c]"),
        Property(&McTrackCuts::fMaxTheta, "MaxTheta", "Maximal theta at the exit of the station [grad]"),
        Property(&McTrackCuts::fbPrimary, "IsPrimary", "Is track primary")
      );
      /* clang-format on */
    };

    McTrackCuts fMcTrackCuts;      ///< MC-track selection cuts
    double fPullMeanThrsh  = 0.1;  ///< Maximum allowed deviation of pull mean from zero
    double fPullWidthThrsh = 2.0;  ///< Maximum allowed deviation of pull width from unity
    double fEffThrsh       = 0.5;  ///< Threshold for hit efficiency in the selected range
    double fMaxDiffZStHit  = 1.0;  ///< Maximum allowed difference between z-position of hit and station [cm]
    /* clang-format off */
    CBM_YAML_PROPERTIES(
      Property(&Config::fMcTrackCuts, "McTrackCuts", "MC track cuts"),
      Property(&Config::fPullMeanThrsh, "PullMeanThrsh", "Pull mean threshold"),
      Property(&Config::fPullWidthThrsh, "PullWidthThrsh", "Pull width threshold"),
      Property(&Config::fEffThrsh, "EffThrsh", "Efficiency threshold"),
      Property(&Config::fMaxDiffZStHit, "MaxDiffZStHit", "Max difference between station and hit z")
    );
    /* clang-format on */
  };

 public:
  /// \brief Constructor from parameters
  /// \param  name      Name of the task
  /// \param  verbose   Verbose level
  /// \param  isMCUsed  Flag, whether MC information is available for this task
  CbmCaInputQaBase(const char* name, int verbose, bool isMCUsed);

  /// \brief Destructor
  virtual ~CbmCaInputQaBase() = default;

 protected:
  // ********************************************
  // ** Virtual method override from CbmQaTask **
  // ********************************************

  /// \brief  Method to check, if the QA results are acceptable
  /// \return Map of checks: key - name of the check, value - result of the check
  void Check() override;

  /// \brief Creates summary cavases, tables etc.
  void CreateSummary() override;

  /// \brief Fills histograms per hit
  virtual void FillHistogramsPerHit() {}

  /// \brief Fills histograms per MC point
  virtual void FillHistogramsPerPoint() {}

  /// \brief De-initializes histograms
  void DeInit() override;

  /// \brief Defines parameter for a derived class
  virtual void DefineParameters() = 0;

  /// \brief Fills histograms per event or time-slice
  void ExecQa() override;

  /// \brief Initializes QA task
  InitStatus InitQa() override;

  /// \brief Checks ranges for mean and standard deviation
  /// \return  String with an error message: empty string is equivalent to success
  std::pair<std::string, bool> CheckRangePull(TH1* h) const;

  /// \brief Returns a pointer to current hit QA data object
  ///
  /// The hit QA data object is filled on each step for hit reconstruction and available inside
  /// functions FillHit() and FillMCPoint()
  const cbm::ca::HitQaData& GetHitQaData() const { return fHitQaData; }

  /// \brief checks if the track at mc point passes the track selection cuts
  /// \return true when selected
  bool IsTrackSelected(const CbmMCTrack* track, const Point_t* point) const;

  /// \brief Checks, if the hit is acceptable (has reasonable parameters)
  bool IsHitAcceptable(const CbmPixelHit* pHit) const;

  /// \struct ResidualFitResult
  /// \brief  Stores fit residuals result
  struct ResidualFitResult {
    double mean = 0;  ///< mean of the distribution
    double lo   = 0;  ///< lower limit for the mean
    double hi   = 0;  ///< higher limit for the mean
  };

  /// \enum EMonitorKey
  /// \brief QA monitor counters
  enum class EMonitorKey
  {
    kEvent,
    kHit,
    kHitAccepted,
    kMcPoint,
    kMcPointWrongStation,
    END
  };
  cbm::algo::ca::Monitor<EMonitorKey> fMonitor{};  ///< Monitor for the input QA

  Config fConfig;  ///< Task specific configuration parameters


  // ----- Input data branches

  CbmTrackingDetectorInterfaceBase* fpDetInterface = nullptr;  ///< Instance of detector interface

  CbmTimeSlice* fpTimeSlice = nullptr;  ///< Pointer to current time-slice

  TClonesArray* fpHits     = nullptr;  ///< Array of hits
  TClonesArray* fpClusters = nullptr;  ///< Array of hit clusters

  CbmMCDataManager* fpMCDataManager = nullptr;  ///< MC data manager
  CbmMCEventList* fpMCEventList     = nullptr;  ///< MC event list
  CbmMCDataArray* fpMCTracks        = nullptr;  ///< Array of MC tracks
  CbmMCDataArray* fpMCPoints        = nullptr;  ///< Array of MC points

  TClonesArray* fpHitMatches = nullptr;  ///< Array of hit matches

  // ******************
  // **  Parameters  **
  // ******************

  static constexpr double kNAN = std::numeric_limits<double>::signaling_NaN();

  // ----- Histogram binning parameters
  int fNbins  = 200;  ///< General number of bins
  int fNbinsZ = 800;  ///< Number of bins for z axis in overall views

  std::array<double, 2> fRHitDx = {kNAN, kNAN};  ///< Range for hit x coordinate error [cm]
  std::array<double, 2> fRHitDy = {kNAN, kNAN};  ///< Range for hit y coordinate error [cm]
  std::array<double, 2> fRHitDu = {kNAN, kNAN};  ///< Range for hit u coordinate error [cm]
  std::array<double, 2> fRHitDv = {kNAN, kNAN};  ///< Range for hit v coordinate error [cm]
  std::array<double, 2> fRHitDt = {kNAN, kNAN};  ///< Range for hit time error [ns]

  std::array<double, 2> fRResX = {kNAN, kNAN};  ///< Range for residual of x coordinate [cm]
  std::array<double, 2> fRResY = {kNAN, kNAN};  ///< Range for residual of y coordinate [cm]
  std::array<double, 2> fRResU = {kNAN, kNAN};  ///< Range for residual of u coordinate [cm]
  std::array<double, 2> fRResV = {kNAN, kNAN};  ///< Range for residual of v coordinate [cm]
  std::array<double, 2> fRResT = {kNAN, kNAN};  ///< Range for residual of time [ns]

  std::array<double, 2> fRangeDzHitPoint = {-0.05, 0.05};  ///< Range for z-pos difference of the hit and point [cm]

  // NOTE: Pull binning is fixed by convention, since it is used for hit finder calibrations. Please,
  //       do not modify!
  // TODO: 29.05.2024 SZh: Put to a tuple <double, int, int>
  static constexpr int kNbinsPull   = 200;
  static constexpr double kRPull[2] = {-10., 10.};  ///< Range for pull histograms coordinate

  std::vector<double> frXmin;  ///< Range for hit x coordinate [cm]
  std::vector<double> frXmax;  ///< Range for hit x coordinate [cm]
  std::vector<double> frYmin;  ///< Range for hit y coordinate [cm]
  std::vector<double> frYmax;  ///< Range for hit y coordinate [cm]
  std::vector<double> frZmin;  ///< Range for hit z coordinate [cm]
  std::vector<double> frZmax;  ///< Range for hit z coordinate [cm]


  // ----- Histograms
  //
  // Hit occupancy
  std::vector<TH2F*> fvph_hit_xy;  ///< hit y vs x in different stations
  std::vector<TH2F*> fvph_hit_zx;  ///< hit x vs z in different stations
  std::vector<TH2F*> fvph_hit_zy;  ///< hit y vs z in different stations

  std::vector<TH1F*> fvph_hit_station_delta_z;  ///< Difference between station and hit z positions [cm]

  // Hit errors
  std::vector<TH1F*> fvph_hit_dx;
  std::vector<TH1F*> fvph_hit_dy;
  std::vector<TH1F*> fvph_hit_du;
  std::vector<TH1F*> fvph_hit_dv;
  std::vector<TH1F*> fvph_hit_kuv;
  std::vector<TH1F*> fvph_hit_dt;

  // MC points occupancy
  std::vector<TH1F*> fvph_n_points_per_hit;  ///< number of points per one hit in different stations

  std::vector<TH2F*> fvph_point_xy;  ///< point y vs x in different stations
  std::vector<TH2F*> fvph_point_zx;  ///< point x vs z in different stations
  std::vector<TH2F*> fvph_point_zy;  ///< point y vs z in different stations

  std::vector<TH1F*> fvph_point_hit_delta_z;  ///< difference between z of hit and MC point in different stations

  // Residuals
  std::vector<TH1F*> fvph_res_x;  ///< residual for x coordinate in different stations
  std::vector<TH1F*> fvph_res_y;  ///< residual for y coordinate in different stations
  std::vector<TH1F*> fvph_res_u;  ///< residual for u coordinate in different stations
  std::vector<TH1F*> fvph_res_v;  ///< residual for v coordinate in different stations
  std::vector<TH1F*> fvph_res_t;  ///< residual for t coordinate in different stations

  std::vector<TH2F*> fvph_res_x_vs_x;  ///< residual for x coordinate in different stations
  std::vector<TH2F*> fvph_res_y_vs_y;  ///< residual for y coordinate in different stations
  std::vector<TH2F*> fvph_res_u_vs_u;  ///< residual for u coordinate in different stations
  std::vector<TH2F*> fvph_res_v_vs_v;  ///< residual for v coordinate in different stations
  std::vector<TH2F*> fvph_res_t_vs_t;  ///< residual for t coordinate in different stations

  // Pulls
  std::vector<TH1F*> fvph_pull_x;  ///< pull for x coordinate in different stations
  std::vector<TH1F*> fvph_pull_y;  ///< pull for y coordinate in different stations
  std::vector<TH1F*> fvph_pull_u;  ///< pull for u coordinate in different stations
  std::vector<TH1F*> fvph_pull_v;  ///< pull for v coordinate in different stations
  std::vector<TH1F*> fvph_pull_t;  ///< pull for t coordinate in different stations

  std::vector<TH2F*> fvph_pull_x_vs_x;  ///< pull for x coordinate in different stations
  std::vector<TH2F*> fvph_pull_y_vs_y;  ///< pull for y coordinate in different stations
  std::vector<TH2F*> fvph_pull_u_vs_u;  ///< pull for u coordinate in different stations
  std::vector<TH2F*> fvph_pull_v_vs_v;  ///< pull for v coordinate in different stations
  std::vector<TH2F*> fvph_pull_t_vs_t;  ///< pull for t coordinate in different stations

  // Hit efficiencies
  std::vector<TProfile2D*> fvpe_reco_eff_vs_xy;  ///< Efficiency of hit reconstruction vs. x and y coordinates of MC
  std::vector<TH1F*> fvph_reco_eff;  ///< Distribution of hit reconstruction efficiency in bins of fvpe_reco_eff_vs_xy


  // FIXME: change to private
 protected:
  cbm::ca::HitQaData fHitQaData{};  ///< Current hit QA data object
};
