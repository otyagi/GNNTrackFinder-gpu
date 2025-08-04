/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Dominik Smith */

#include "V0Trigger.h"

#include <iterator>
#include <sstream>

#include <xpu/host.h>

namespace cbm::algo::evbuild
{


  V0Trigger::Result V0Trigger::operator()(const TrackVector& tracks, const V0TriggerConfig& config) const
  {

    xpu::push_timer("V0Trigger");
    xpu::t_add_bytes(tracks.size() * sizeof(cbm::algo::ca::Track));


    Result result;

    size_t numTracksUsed = 0;
    for (auto trackIter1 = tracks.begin(); trackIter1 != tracks.end(); trackIter1++) {
      if (!Select(*trackIter1, config)) continue;
      numTracksUsed++;
      for (auto trackIter2 = std::next(trackIter1); trackIter2 != tracks.end(); trackIter2++) {
        if (!Select(*trackIter2, config)) continue;

        // Check track time difference
        float time1 = trackIter1->fParPV.GetTime();
        float time2 = trackIter2->fParPV.GetTime();

        if (fpQa->IsActive()) {
          fpQa->fphPairDeltaT->Fill(time2 - time1);
        }

        if (time2 < time1) {
          result.second.errTracksUnsorted++;
          continue;
        }
        result.second.numTrackPairs++;
        if (time2 - time1 > config.PairDeltaT_max()) break;
        result.second.numTrackPairsAfterTimeCut++;

        // Check PCA cuts
        auto [zVertex, dist] = CalcPCA(trackIter1->fParPV, trackIter2->fParPV);
        if (fpQa->IsActive()) {
          fpQa->fphPairZVertex->Fill(zVertex);
          fpQa->fphPairDca->Fill(dist);
        }

        if (dist < config.PairDist_max()) {
          result.second.numTrackPairsAfterDistCut++;
          if (zVertex >= config.PairZ_min() && zVertex <= config.PairZ_max()) {
            result.second.numTrackPairsAfterZCut++;
            double tVertex = 0.5 * (time1 + time2);
            result.first.push_back(tVertex);
          }
        }
      }
    }

    result.second.time = xpu::pop_timer();
    L_(info) << "V0Trigger: tracks " << tracks.size() << ", unsorted " << result.second.errTracksUnsorted
             << ", used tracks " << numTracksUsed << ", track pairs " << result.second.numTrackPairs
             << ", after time cut " << result.second.numTrackPairsAfterTimeCut << ", after dist cut "
             << result.second.numTrackPairsAfterDistCut << ", after z cut " << result.second.numTrackPairsAfterZCut;
    return result;
  };


  std::pair<double, double> V0Trigger::CalcPCA(const TrackParam& track1, const TrackParam& track2) const
  {

    // Start point and direction of first track at z = 0
    const double ax = track1.GetX() - track1.GetTx() * track1.GetZ();
    const double ay = track1.GetY() - track1.GetTy() * track1.GetZ();
    const double ux = track1.GetTx();
    const double uy = track1.GetTy();

    // Start point and direction of second track at z = 0
    const double bx = track2.GetX() - track2.GetTx() * track2.GetZ();
    const double by = track2.GetY() - track2.GetTy() * track2.GetZ();
    const double vx = track2.GetTx();
    const double vy = track2.GetTy();

    // Difference vectors
    const double cx = ax - bx;
    const double cy = ay - by;
    const double wx = ux - vx;
    const double wy = uy - vy;

    // z coordinate at closest approach in the x-y plane
    const double z = -1. * (cx * wx + cy * wy) / (wx * wx + wy * wy);

    // Distance at closest approach in the x-y plane
    const double dx   = cx + z * wx;
    const double dy   = cy + z * wy;
    const double dist = sqrt(dx * dx + dy * dy);

    return std::make_pair(z, dist);
  }


  bool V0Trigger::Select(const Track& track, const V0TriggerConfig& config) const
  {

    // Minimum z at first measurement
    if (!(track.fParFirst.Z() >= config.TrackStartZ_min())) return false;

    // Maximum z at first measurement
    if (!(track.fParFirst.Z() <= config.TrackStartZ_max())) return false;

    // Minimum z at last measurement
    if (!(track.fParLast.Z() >= config.TrackEndZ_min())) return false;

    // Reject primaries
    if (IsPrimary(track.fParPV, config)) return false;

    return true;
  };

  bool V0Trigger::IsPrimary(const TrackParam& track, const V0TriggerConfig& config) const
  {

    // x coordinate of impact at target
    if (track.X() < config.TrackImpactX_min()) return false;
    if (track.X() > config.TrackImpactX_max()) return false;

    // y coordinate of impact at target
    if (track.Y() < config.TrackImpactY_min()) return false;
    if (track.Y() > config.TrackImpactY_max()) return false;

    return true;
  }


  std::string V0Trigger::ToString() const
  {
    std::stringstream out;
    out << "--- Using V0Trigger ---";
    return out.str();
  }


}  // namespace cbm::algo::evbuild
