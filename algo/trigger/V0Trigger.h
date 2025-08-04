/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#pragma once  // include this header only once per compilation unit

#include "CaTrack.h"
#include "CaVector.h"
#include "V0TriggerConfig.h"
#include "qa/trigger/V0TriggerQa.h"

#include <utility>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::evbuild
{


  /** @struct V0TriggerMoniData
   ** @brief Monitoring information for the algorithm V0Trigger
   **/
  struct V0TriggerMoniData {
    size_t errTracksUnsorted{0};
    size_t numTrackPairs{0};
    size_t numTrackPairsAfterTimeCut{0};
    size_t numTrackPairsAfterZCut{0};
    size_t numTrackPairsAfterDistCut{0};
    xpu::timings time;  ///< Time for trigger building
  };

  /** @class V0Trigger
  ** @brief Trigger class for secondary two-track vertices in mCBM
  ** @author Volker Friese <v.friese@gsi.de>
  ** @date 1 February 2024
  **
  ** The class takes an array of tracks and searches for track pairs close in time and with a point of closest approach (PCA)
  ** outside of the target area. Tracks are assumed to be straight lines, in the absence of a magnetic field as in mCBM.
  ** The class returns a list of trigger times corresponding to track pairs satisfying the selection criteria:
  ** Maximum time difference, minimum z of PCA, maximum distance at PCA.
  **/
  class V0Trigger {

   public:
    typedef std::pair<std::vector<double>, V0TriggerMoniData> Result;
    typedef cbm::algo::ca::Track Track;
    typedef cbm::algo::ca::Vector<cbm::algo::ca::Track> TrackVector;
    typedef cbm::algo::kf::TrackParamS TrackParam;

    /** @brief Constructor **/
    V0Trigger() = default;


    /** @brief Execution
     ** @param  tracks      Input track vector
     ** @param  config      Trigger configuration
     ** @return Vector of trigger times and monitoring data
     **/
    Result operator()(const TrackVector& tracks, const V0TriggerConfig& config) const;


    /** @brief Sets QA module
     ** @param pQa  Pointer to the QA module
     **/
    void SetQa(std::shared_ptr<V0TriggerQa> pQa) { fpQa = pQa; }


    /** @brief Info to string **/
    std::string ToString() const;


   private:
    /** @brief Calculation of closest approach of two tracks (straight lines)
     ** @param track1  Parameters of first track
     ** @param track2  Parameters of second track
     ** @return (z position, distance)
     ** 
     ** The closest approach is defined at the z position where the transverse distance of the tracks (in the x-y plane) is minimal.
     ** This is not strictly the minimal distance in 3-d space, which is mathematically and computationally more involved.
     ** It should be a good criterion for the purpose of finding displaced vertices.
     **/
    std::pair<double, double> CalcPCA(const TrackParam& track1, const TrackParam& track2) const;

    /** @brief Check track cuts 
     ** @param track  Track
     ** @return decision
     **/
    bool Select(const Track& track, const V0TriggerConfig& config) const;

    /** @brief Check if track is a priomary 
     ** @param track  Track parameters in target plane
     ** @return decision
     **/
    bool IsPrimary(const TrackParam& track, const V0TriggerConfig& config) const;

    std::shared_ptr<V0TriggerQa> fpQa{std::make_shared<V0TriggerQa>(nullptr)};  //! QA module
  };

}  // namespace cbm::algo::evbuild
