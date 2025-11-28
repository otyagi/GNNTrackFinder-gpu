/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GraphConstructor.cxx
/// \brief Functions for running GNN algorithm
/// \author Oddharak Tyagi

#include "GraphConstructor.h"

#include "CandClassifier.h"

namespace cbm::algo::ca
{

  GraphConstructor::GraphConstructor(const ca::InputData& input, WindowData& wData, TrackFitter& fTrackFitter)
    : frInput(input)
    , frWData(wData)
    , frTrackFitter(fTrackFitter)
  {
  }

  // @brief: for debugging save all found edges as tracks
  void GraphConstructor::SaveAllEdgesAsTracks()
  {
    tracks.clear();
    for (int istal = 0; istal < NStations; istal++) {
      for (const auto& edge : edges[istal]) {
        tracks.push_back(std::vector<int>{edge.first, edge.second});
      }
    }
    LOG(info) << "Num tracks (doublets as tracks): " << tracks.size();
  }

  // @brief: for debugging. saves triplets as tracks
  void GraphConstructor::SaveAllTripletsAsTracks()
  {
    tracks.clear();
    for (const auto& triplet : triplets_) {
      tracks.push_back(triplet);
    }
    LOG(info) << "Num tracks(triplets as tracks): " << tracks.size();
  }

  void GraphConstructor::FindFastPrim(const int mode)
  {

    CreateMetricLearningDoublets(0);

    const float margin = 2.0f;  // 2.0f gives extremely good results
    // fill edges
    int edgeIndex   = 0;
    int nEdgesFound = 0;
    float y1, z1, y2, z2, slope;
    for (int istal = 0; istal < NStations; istal++) {
      std::vector<std::pair<int, int>> edgesSta;
      edgesSta.reserve(maxNeighOrderPrim_ * doublets[istal].size());
      for (int iel = 0; iel < (int) doublets[istal].size(); iel++) {
        for (int iem = 0; iem < (int) doublets[istal][iel].size(); iem++) {
          int ihitl = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits
          y1        = frWData.Hit(ihitl).Y();
          z1        = frWData.Hit(ihitl).Z() + 44.0f;
          y2        = frWData.Hit(doublets[istal][iel][iem]).Y();
          z2        = frWData.Hit(doublets[istal][iel][iem]).Z() + 44.0f;
          slope     = (y2 - y1) / (z2 - z1);
          if (std::abs(y1 - slope * z1) > margin) {
            edgeIndex++;
            continue;
          }
          else {
            edgesSta.push_back(std::make_pair(ihitl, (int) doublets[istal][iel][iem]));
            nEdgesFound++;
          }
          edgeIndex++;
        }
      }
      edges.push_back(edgesSta);
    }
    LOG(info) << "Num true edges after removing displaced edges: " << nEdgesFound;

    // To save doublets as tracks
    // SaveAllEdgesAsTracks();
    // return;

    const float YZCut = 0.1;  // (radians) def - 0.1 from distributions
    const float XZCut = 0.1;  // def - 0.1 from distributions
    // create triplets with edges with shared hits and prepare for input to triplet classifier
    std::vector<std::vector<int>> allTripletsIndex;  // index in fWindowHits
    std::vector<float> tripletScores;
    for (int istal = 0; istal < NStations - 2; istal++) {
      for (int iel = 0; iel < (int) edges[istal].size(); iel++) {
        for (int iem = 0; iem < (int) edges[istal + 1].size(); iem++) {
          if (edges[istal][iel].second == edges[istal + 1][iem].first) {  /// overlapping edges form triplet
            std::vector<int> triplet;
            triplet.push_back(edges[istal][iel].first);
            triplet.push_back(edges[istal][iel].second);
            triplet.push_back(edges[istal + 1][iem].second);
            const auto& hit1 = frWData.Hit(triplet[0]);
            const auto& hit2 = frWData.Hit(triplet[1]);
            const auto& hit3 = frWData.Hit(triplet[2]);

            // Cuts on triplet. Limits come from distributions
            // YZ angle difference
            const float angle1YZ    = std::atan2(hit2.Y() - hit1.Y(), hit2.Z() - hit1.Z());
            const float angle2YZ    = std::atan2(hit3.Y() - hit2.Y(), hit3.Z() - hit2.Z());
            const float angleDiffYZ = angle1YZ - angle2YZ;
            if (angleDiffYZ < -YZCut || angleDiffYZ > YZCut) continue;

            // XZ slope and angle difference
            const float angle1XZ    = std::atan2(hit2.X() - hit1.X(), hit2.Z() - hit1.Z());
            const float angle2XZ    = std::atan2(hit3.X() - hit2.X(), hit3.Z() - hit2.Z());
            const float angleDiffXZ = angle1XZ - angle2XZ;
            if (angleDiffXZ < -XZCut || angleDiffXZ > XZCut) continue;

            allTripletsIndex.push_back(triplet);
            tripletScores.push_back(1.0f);  // dummy
          }
        }
      }
    }
    LOG(info) << "Number of triplets created from edges: " << allTripletsIndex.size();

    for (int i = 0; i < (int) allTripletsIndex.size(); i++) {
      triplets_.push_back(allTripletsIndex[i]);
      tripletScores_.push_back(tripletScores[i]);
    }

    FitTriplets(0);  // replaces dummy triplet score with KF chi2

    if (mode == 0) {
      SaveAllTripletsAsTracks();
    }
    else {
      CreateTracksTriplets(mode, 0);
    }
  }  // FindFastPrim

  /// Slow primary and jump triplet
  void GraphConstructor::FindSlowPrimJump(const int mode)
  {
    CreateMetricLearningDoubletsJump(1);

    const float margin = 5.0f;  // def - 5
    // fill edges
    edges.clear();
    int edgeIndex   = 0;
    int nEdgesFound = 0;
    float y1, z1, y2, z2, slope;
    for (int istal = 0; istal < NStations - 1; istal++) {
      std::vector<std::pair<int, int>> edgesSta;
      edgesSta.reserve(2 * maxNeighOrderAllPrim_ * doublets[istal].size());
      for (int iel = 0; iel < (int) doublets[istal].size(); iel++) {
        for (int iem = 0; iem < (int) doublets[istal][iel].size(); iem++) {
          int ihitl = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits
          y1        = frWData.Hit(ihitl).Y();
          z1        = frWData.Hit(ihitl).Z() + 44.0f;
          y2        = frWData.Hit(doublets[istal][iel][iem]).Y();
          z2        = frWData.Hit(doublets[istal][iel][iem]).Z() + 44.0f;
          slope     = (y2 - y1) / (z2 - z1);
          if (std::abs(y1 - slope * z1) > margin) {
            edgeIndex++;
            continue;
          }
          else {
            edgesSta.push_back(std::make_pair(ihitl, (int) doublets[istal][iel][iem]));
            nEdgesFound++;
          }
          edgeIndex++;
        }
      }
      edges.push_back(edgesSta);
    }
    LOG(info) << "Num true edges after removing displaced edges: " << nEdgesFound;

    // saveAllEdgesAsTracks();
    // return;

    // consecutive edge difference cuts
    const float YZCut_Cons = 0.4;  // def - 0.4 in radians from distributions
    const float XZCut_Cons = 0.8;  // def - 0.8 in radians
    // jump edge difference cuts
    const float YZCut_Jump     = 0.2;   // def - 0.2
    const float XZCut_Jump     = 0.4;   // def - 0.4
    const float jump_margin_yz = 0.5f;  // def - 0.5
    float y3, z3;

    // create triplets with edges with shared hits and prepare for input to triplet classifier
    constexpr unsigned int N_TRIPLETS_SEC_MAX = 500000;
    std::vector<std::vector<int>> allTripletsIndex;  // index in fWindowHits
    allTripletsIndex.reserve(N_TRIPLETS_SEC_MAX);

    for (int istal = 0; istal < NStations - 2; istal++) {
      if (edges[istal].size() == 0) continue;  // skip empty edges
      for (int iel = 0; iel < (int) edges[istal].size(); iel++) {
        const int sta1 = frWData.Hit(edges[istal][iel].first).Station();
        const int sta2 = frWData.Hit(edges[istal][iel].second).Station();

        if ((sta2 - sta1) == 1) {                      // triplet type: [1 2 3/4]. First consecutive edge
          if (edges[istal + 1].size() == 0) continue;  // skip empty edges
          for (int iem = 0; iem < (int) edges[istal + 1].size(); iem++) {
            const int sta3 = frWData.Hit(edges[istal + 1][iem].second).Station();
            if ((sta3 - sta2) == 1) {                                         // triplet types: [1 2 3]. No jump
              if (edges[istal][iel].second == edges[istal + 1][iem].first) {  // adjacent edges form triplet
                std::vector<int> triplet;
                triplet.push_back(edges[istal][iel].first);
                triplet.push_back(edges[istal][iel].second);
                triplet.push_back(edges[istal + 1][iem].second);
                const auto& hit1 = frWData.Hit(triplet[0]);
                const auto& hit2 = frWData.Hit(triplet[1]);
                const auto& hit3 = frWData.Hit(triplet[2]);

                // Cuts on triplet. Limits come from distributions
                // YZ angle difference
                const float angle1YZ = std::atan2(hit2.Y() - hit1.Y(), hit2.Z() - hit1.Z());
                const float angle2YZ = std::atan2(hit3.Y() - hit2.Y(), hit3.Z() - hit2.Z());
                float angleDiffYZ    = angle1YZ - angle2YZ;
                if (angleDiffYZ < -YZCut_Cons || angleDiffYZ > YZCut_Cons) continue;

                // XZ slope and angle difference
                const float angle1XZ = std::atan2(hit2.X() - hit1.X(), hit2.Z() - hit1.Z());
                const float angle2XZ = std::atan2(hit3.X() - hit2.X(), hit3.Z() - hit2.Z());
                float angleDiffXZ    = angle1XZ - angle2XZ;
                if (angleDiffXZ < -XZCut_Cons || angleDiffXZ > XZCut_Cons) continue;

                allTripletsIndex.push_back(triplet);
              }
            }
            else if ((sta3 - sta2) == 2) {            // triplet type: [1 2 4]. First edge consecutive, second edge jump
              if (sta1 >= 9 || sta2 >= 10) continue;  // skip triplet with jump edge to last station
              if (edges[istal][iel].second == edges[istal + 1][iem].first) {  // adjacent to jump
                std::vector<int> triplet;
                triplet.push_back(edges[istal][iel].first);
                triplet.push_back(edges[istal][iel].second);
                triplet.push_back(edges[istal + 1][iem].second);
                const auto& hit1 = frWData.Hit(triplet[0]);
                const auto& hit2 = frWData.Hit(triplet[1]);
                const auto& hit3 = frWData.Hit(triplet[2]);

                // jump triplet must be primary
                y1    = hit1.Y();
                z1    = hit1.Z() + 44.0f;
                y3    = hit3.Y();
                z3    = hit3.Z() + 44.0f;
                slope = (y3 - y1) / (z3 - z1);
                if (std::abs(y1 - slope * z1) > jump_margin_yz) continue;

                // Cuts on triplet internal angle
                // YZ angle difference
                const float angle1YZ = std::atan2(hit2.Y() - hit1.Y(), hit2.Z() - hit1.Z());
                const float angle2YZ = std::atan2(hit3.Y() - hit2.Y(), hit3.Z() - hit2.Z());
                float angleDiffYZ    = angle1YZ - angle2YZ;
                if (angleDiffYZ < -YZCut_Jump || angleDiffYZ > YZCut_Jump) continue;

                // XZ slope and angle difference
                const float angle1XZ = std::atan2(hit2.X() - hit1.X(), hit2.Z() - hit1.Z());
                const float angle2XZ = std::atan2(hit3.X() - hit2.X(), hit3.Z() - hit2.Z());
                float angleDiffXZ    = angle1XZ - angle2XZ;
                if (angleDiffXZ < -XZCut_Jump || angleDiffXZ > XZCut_Jump) continue;

                allTripletsIndex.push_back(triplet);
              }
            }
          }
        }
        else if ((sta2 - sta1) == 2) {            // triplet types: [1 3 4]. First edge jump, second edge consecutive
          if (sta1 >= 9 || sta2 >= 11) continue;  // skip triplet with jump edge to last station
          if (edges[istal + 2].size() == 0) continue;  // skip empty edges
          for (int iem = 0; iem < (int) edges[istal + 2].size(); iem++) {
            if (edges[istal][iel].second == edges[istal + 2][iem].first) {  // jump to adjacent edge
              std::vector<int> triplet;
              triplet.push_back(edges[istal][iel].first);
              triplet.push_back(edges[istal][iel].second);
              triplet.push_back(edges[istal + 2][iem].second);
              const auto& hit1 = frWData.Hit(triplet[0]);
              const auto& hit2 = frWData.Hit(triplet[1]);
              const auto& hit3 = frWData.Hit(triplet[2]);

              // jump triplet must be primary
              y1    = hit1.Y();
              z1    = hit1.Z() + 44.0f;
              y3    = hit3.Y();
              z3    = hit3.Z() + 44.0f;
              slope = (y3 - y1) / (z3 - z1);
              if (std::abs(y1 - slope * z1) > jump_margin_yz) continue;

              // Cuts on triplet internal angle
              // YZ angle difference
              const float angle1YZ = std::atan2(hit2.Y() - hit1.Y(), hit2.Z() - hit1.Z());
              const float angle2YZ = std::atan2(hit3.Y() - hit2.Y(), hit3.Z() - hit2.Z());
              float angleDiffYZ    = angle1YZ - angle2YZ;
              if (std::abs(angleDiffYZ) > YZCut_Jump) continue;

              // XZ slope and angle difference
              const float angle1XZ = std::atan2(hit2.X() - hit1.X(), hit2.Z() - hit1.Z());
              const float angle2XZ = std::atan2(hit3.X() - hit2.X(), hit3.Z() - hit2.Z());
              float angleDiffXZ    = angle1XZ - angle2XZ;
              if (std::abs(angleDiffXZ) > XZCut_Jump) continue;

              allTripletsIndex.push_back(triplet);
            }
          }
        }
      }
    }
    if (allTripletsIndex.size() == 0) {
      LOG(info) << "No triplets found. Exiting.";
      return;
    }
    LOG(info) << "Number of triplets created from edges: " << allTripletsIndex.size();

    for (int i = 0; i < (int) allTripletsIndex.size(); i++) {
      triplets_.push_back(allTripletsIndex[i]);
    }

    FitTriplets(1);

    if (mode == 0) {
      // saveAllTripletsAsTracks();
    }
    else {
      CreateTracksTriplets(mode, 1);
    }
  }  // FindSlowPrimJump

  /// create all secondary and jump triplets
  void GraphConstructor::FindAllSecJump(const int mode)
  {
    CreateMetricLearningDoubletsJump(3);

    const float outer_margin = 100.0f;  // def - 100.0f cm in YZ plane.
    // fill edges
    edges.clear();
    int edgeIndex   = 0;
    int nEdgesFound = 0;
    float y1, z1, y2, z2, slope, abs_intercept;
    for (int istal = 0; istal < NStations - 1; istal++) {
      std::vector<std::pair<int, int>> edgesSta;
      edgesSta.reserve(2 * maxNeighOrderSec_ * doublets[istal].size());
      for (int iel = 0; iel < (int) doublets[istal].size(); iel++) {
        for (int iem = 0; iem < (int) doublets[istal][iel].size(); iem++) {
          int ihitl     = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits
          y1            = frWData.Hit(ihitl).Y();
          z1            = frWData.Hit(ihitl).Z() + 44.0f;
          y2            = frWData.Hit(doublets[istal][iel][iem]).Y();
          z2            = frWData.Hit(doublets[istal][iel][iem]).Z() + 44.0f;
          slope         = (y2 - y1) / (z2 - z1);
          abs_intercept = std::abs(y1 - slope * z1);
          if (abs_intercept > outer_margin) {
            edgeIndex++;
            continue;
          }
          else {
            edgesSta.push_back(std::make_pair(ihitl, (int) doublets[istal][iel][iem]));
            nEdgesFound++;
          }
          edgeIndex++;
        }
      }
      edges.push_back(edgesSta);
    }
    LOG(info) << "Num true edges after removing displaced edges: " << nEdgesFound;

    // saveAllEdgesAsTracks();
    // return;

    // consecutive edge difference cuts
    const float YZCut_Cons = 0.4;  // def - 0.4 in radians from distributions
    const float XZCut_Cons = 0.8;  // def - 0.8 in radians
    // jump edge difference cuts
    const float YZCut_Jump     = 0.1;    // def - 0.1
    const float XZCut_Jump     = 0.2;    // def - 0.2
    const float jump_margin_yz = 10.0f;  // def -
    float y3, z3;

    // create triplets with edges with shared hits and prepare for input to triplet classifier
    constexpr unsigned int N_TRIPLETS_SEC_MAX = 500000;
    std::vector<std::vector<int>> allTripletsIndex;  // index in fWindowHits
    allTripletsIndex.reserve(N_TRIPLETS_SEC_MAX);

    for (int istal = 0; istal < NStations - 2; istal++) {
      if (edges[istal].size() == 0) continue;  // skip empty edges
      for (int iel = 0; iel < (int) edges[istal].size(); iel++) {
        const int sta1 = frWData.Hit(edges[istal][iel].first).Station();
        const int sta2 = frWData.Hit(edges[istal][iel].second).Station();

        if ((sta2 - sta1) == 1) {                      // triplet types: [1 2 .]. First consecutive edge
          if (edges[istal + 1].size() == 0) continue;  // skip empty edges
          for (int iem = 0; iem < (int) edges[istal + 1].size(); iem++) {
            const int sta3 = frWData.Hit(edges[istal + 1][iem].second).Station();
            if ((sta3 - sta2) == 1) {                                         // triplet types: [1 2 3]. No jump
              if (edges[istal][iel].second == edges[istal + 1][iem].first) {  // adjacent edges form triplet
                std::vector<int> triplet;
                triplet.push_back(edges[istal][iel].first);
                triplet.push_back(edges[istal][iel].second);
                triplet.push_back(edges[istal + 1][iem].second);
                const auto& hit1 = frWData.Hit(triplet[0]);
                const auto& hit2 = frWData.Hit(triplet[1]);
                const auto& hit3 = frWData.Hit(triplet[2]);

                // Cuts on triplet. Limits come from distributions
                // YZ angle difference
                const float angle1YZ = std::atan2(hit2.Y() - hit1.Y(), hit2.Z() - hit1.Z());
                const float angle2YZ = std::atan2(hit3.Y() - hit2.Y(), hit3.Z() - hit2.Z());
                float angleDiffYZ    = angle1YZ - angle2YZ;
                if (angleDiffYZ < -YZCut_Cons || angleDiffYZ > YZCut_Cons) continue;

                // XZ slope and angle difference
                const float angle1XZ = std::atan2(hit2.X() - hit1.X(), hit2.Z() - hit1.Z());
                const float angle2XZ = std::atan2(hit3.X() - hit2.X(), hit3.Z() - hit2.Z());
                float angleDiffXZ    = angle1XZ - angle2XZ;
                if (angleDiffXZ < -XZCut_Cons || angleDiffXZ > XZCut_Cons) continue;

                allTripletsIndex.push_back(triplet);
              }
            }
            else if ((sta3 - sta2) == 2) {  // triplet type: [1 2 4]. First edge consecutive, second edge jump
              if (sta1 >= 9) continue;      // skip triplet with jump edge to last station
              if (edges[istal][iel].second == edges[istal + 1][iem].first) {  // adjacent to jump
                std::vector<int> triplet;
                triplet.push_back(edges[istal][iel].first);
                triplet.push_back(edges[istal][iel].second);
                triplet.push_back(edges[istal + 1][iem].second);
                const auto& hit1 = frWData.Hit(triplet[0]);
                const auto& hit2 = frWData.Hit(triplet[1]);
                const auto& hit3 = frWData.Hit(triplet[2]);

                // jump triplet must be primary
                y1    = hit1.Y();
                z1    = hit1.Z() + 44.0f;
                y3    = hit3.Y();
                z3    = hit3.Z() + 44.0f;
                slope = (y3 - y1) / (z3 - z1);
                if (std::abs(y1 - slope * z1) > jump_margin_yz) continue;

                // Cuts on triplet internal angle
                // YZ angle difference
                const float angle1YZ = std::atan2(hit2.Y() - hit1.Y(), hit2.Z() - hit1.Z());
                const float angle2YZ = std::atan2(hit3.Y() - hit2.Y(), hit3.Z() - hit2.Z());
                float angleDiffYZ    = angle1YZ - angle2YZ;
                if (angleDiffYZ < -YZCut_Jump || angleDiffYZ > YZCut_Jump) continue;

                // XZ slope and angle difference
                const float angle1XZ = std::atan2(hit2.X() - hit1.X(), hit2.Z() - hit1.Z());
                const float angle2XZ = std::atan2(hit3.X() - hit2.X(), hit3.Z() - hit2.Z());
                float angleDiffXZ    = angle1XZ - angle2XZ;
                if (angleDiffXZ < -XZCut_Jump || angleDiffXZ > XZCut_Jump) continue;

                allTripletsIndex.push_back(triplet);
              }
            }
          }
        }
        else if ((sta2 - sta1) == 2) {  // triplet types: [1 3 4]. First edge jump, second edge consecutive
          if (sta1 >= 9) continue;      // skip triplet with jump edge to last station
          if (edges[istal + 2].size() == 0) continue;  // skip empty edges
          for (int iem = 0; iem < (int) edges[istal + 2].size(); iem++) {
            if (edges[istal][iel].second == edges[istal + 2][iem].first) {  // jump to adjacent edge
              std::vector<int> triplet;
              triplet.push_back(edges[istal][iel].first);
              triplet.push_back(edges[istal][iel].second);
              triplet.push_back(edges[istal + 2][iem].second);
              const auto& hit1 = frWData.Hit(triplet[0]);
              const auto& hit2 = frWData.Hit(triplet[1]);
              const auto& hit3 = frWData.Hit(triplet[2]);

              // jump triplet must be primary
              y1    = hit1.Y();
              z1    = hit1.Z() + 44.0f;
              y3    = hit3.Y();
              z3    = hit3.Z() + 44.0f;
              slope = (y3 - y1) / (z3 - z1);
              if (std::abs(y1 - slope * z1) > jump_margin_yz) continue;

              // Cuts on triplet internal angle
              // YZ angle difference
              const float angle1YZ = std::atan2(hit2.Y() - hit1.Y(), hit2.Z() - hit1.Z());
              const float angle2YZ = std::atan2(hit3.Y() - hit2.Y(), hit3.Z() - hit2.Z());
              float angleDiffYZ    = angle1YZ - angle2YZ;
              if (std::abs(angleDiffYZ) > YZCut_Jump) continue;

              // XZ slope and angle difference
              const float angle1XZ = std::atan2(hit2.X() - hit1.X(), hit2.Z() - hit1.Z());
              const float angle2XZ = std::atan2(hit3.X() - hit2.X(), hit3.Z() - hit2.Z());
              float angleDiffXZ    = angle1XZ - angle2XZ;
              if (std::abs(angleDiffXZ) > XZCut_Jump) continue;

              allTripletsIndex.push_back(triplet);
            }
          }
        }
      }
    }
    if (allTripletsIndex.size() == 0) {
      LOG(info) << "No triplets found. Exiting.";
      return;
    }
    LOG(info) << "Number of triplets created from edges: " << allTripletsIndex.size();

    for (int i = 0; i < (int) allTripletsIndex.size(); i++) {
      triplets_.push_back(allTripletsIndex[i]);
    }

    FitTriplets(3);

    if (mode == 0) {
      // saveAllTripletsAsTracks();
    }
    else {
      CreateTracksTriplets(mode, 3);
    }
  }  // FindAllSecJump

  /// combine overlapping triplets to form tracks. sort by length and score for competition
  void GraphConstructor::CreateTracksTriplets(const int mode, const int GNNIteration)
  {
    tracks.clear();
    tracks.reserve(10000000);  // optimized for track mode

    std::vector<std::vector<int>> tracklets;
    tracklets.reserve(10000000);
    std::vector<float> trackletScores;
    trackletScores.reserve(10000000);
    std::vector<std::vector<float>> trackletFitParams;  // store fit params of last triplet added to tracklet
    trackletFitParams.reserve(10000000);

    /// add all triplets as tracklets
    for (int i = 0; i < (int) triplets_.size(); i++) {
      tracklets.push_back(triplets_[i]);
      trackletScores.push_back(tripletScores_[i]);
      trackletFitParams.push_back(tripletFitParams_[i]);
    }

    /// organize triplets by station
    std::vector<std::vector<std::vector<int>>> tripletsByStation(NStations);
    std::vector<std::vector<float>> tripletsScore(NStations);
    std::vector<std::vector<std::vector<float>>> tripletsFitParams(NStations);
    for (int i = 0; i < (int) triplets_.size(); i++) {
      const int sta = frWData.Hit(triplets_[i][0]).Station();
      tripletsByStation[sta].push_back(triplets_[i]);
      tripletsScore[sta].push_back(tripletScores_[i]);
      tripletsFitParams[sta].push_back(tripletFitParams_[i]);
    }

    constexpr float degree_to_rad = 3.14159 / 180.0;
    double angle1YZ, angle2YZ, angleDiffYZ1, angle1XZ, angle2XZ, angleDiffXZ1;
    double angle3YZ, angle4YZ, angleDiffYZ2, angle3XZ, angle4XZ, angleDiffXZ2;
    double angleDiffYZ, angleDiffXZ;
    float YZ_cut, XZ_cut_neg_min, XZ_cut_neg_max, XZ_cut_pos_min, XZ_cut_pos_max;
    float YZ_cut_jump, XZ_cut_neg_min_jump, XZ_cut_neg_max_jump, XZ_cut_pos_min_jump, XZ_cut_pos_max_jump = 0.0f;
    // cuts from distribution figures for overlapping triplets
    switch (GNNIteration) {
      case 0:  // fastPrim
        YZ_cut         = 3.0f * degree_to_rad;
        XZ_cut_neg_min = -2.0f * degree_to_rad;
        XZ_cut_neg_max = 2.0f * degree_to_rad;
        XZ_cut_pos_min = -2.0f * degree_to_rad;
        XZ_cut_pos_max = 2.0f * degree_to_rad;
        break;
      case 1:  // AllPrim
        YZ_cut         = 1 * 20.0f * degree_to_rad;
        XZ_cut_neg_min = 1 * -10.0f * degree_to_rad;
        XZ_cut_neg_max = 1 * 10.0f * degree_to_rad;
        XZ_cut_pos_min = 1 * -10.0f * degree_to_rad;
        XZ_cut_pos_max = 1 * 10.0f * degree_to_rad;
        // jump cuts
        YZ_cut_jump         = 5.0f * degree_to_rad;   // def - 5
        XZ_cut_neg_min_jump = -5.0f * degree_to_rad;  // def - -5
        XZ_cut_neg_max_jump = 5.0f * degree_to_rad;   // def - 5
        XZ_cut_pos_min_jump = -5.0f * degree_to_rad;  // def - -5
        XZ_cut_pos_max_jump = 5.0f * degree_to_rad;   // def - 5
        break;
      case 3:  // sec
        /// loose cuts
        YZ_cut         = 1 * 20.0f * degree_to_rad;
        XZ_cut_neg_min = 1 * -10.0f * degree_to_rad;
        XZ_cut_neg_max = 1 * 10.0f * degree_to_rad;
        XZ_cut_pos_min = 1 * -10.0f * degree_to_rad;
        XZ_cut_pos_max = 1 * 10.0f * degree_to_rad;
        // jump cuts
        YZ_cut_jump         = 5.0f * degree_to_rad;   // def - 5
        XZ_cut_neg_min_jump = -5.0f * degree_to_rad;  // def - -5
        XZ_cut_neg_max_jump = 5.0f * degree_to_rad;   // def - 5
        XZ_cut_pos_min_jump = -5.0f * degree_to_rad;  // def - -5
        XZ_cut_pos_max_jump = 5.0f * degree_to_rad;   // def - 5
        break;
      default:
        LOG(info) << "[CreateTracksTriplets]Unknown iteration index: " << GNNIteration;
        YZ_cut         = 1 * 20.0f * degree_to_rad;
        XZ_cut_neg_min = 1 * -10.0f * degree_to_rad;
        XZ_cut_neg_max = 1 * 10.0f * degree_to_rad;
        XZ_cut_pos_min = 1 * -10.0f * degree_to_rad;
        XZ_cut_pos_max = 1 * 10.0f * degree_to_rad;
        break;
    }

    /// go over every tracklet and see if it can be extended with overlapping triplet
    for (int iTracklet = 0; iTracklet < (int) tracklets.size(); ++iTracklet) {
      // for (int iTracklet = 0; iTracklet < numTriplets; ++iTracklet) {

      const auto& tracklet = tracklets[iTracklet];
      int length           = tracklet.size();
      int middleSta        = frWData.Hit(tracklet[length - 2]).Station();
      const bool isJumpTripletLast =
        (frWData.Hit(tracklet[length - 1]).Station() - frWData.Hit(tracklet[length - 3]).Station()) == 3;

      for (int iTriplet = 0; iTriplet < (int) tripletsByStation[middleSta].size(); ++iTriplet) {
        // check overlapping triplet
        if (tracklet[length - 2] != tripletsByStation[middleSta][iTriplet][0]) continue;
        if (tracklet[length - 1] != tripletsByStation[middleSta][iTriplet][1]) continue;

        /// check difference of angle difference between triplets in XZ and YZ
        const auto& h1 = frWData.Hit(tracklet[length - 3]);
        const auto& h2 = frWData.Hit(tracklet[length - 2]);
        const auto& h3 = frWData.Hit(tracklet[length - 1]);
        const auto& h4 = frWData.Hit(tripletsByStation[middleSta][iTriplet][2]);

        // YZ angle 1
        angle1YZ     = std::atan2(h2.Y() - h1.Y(), h2.Z() - h1.Z());
        angle2YZ     = std::atan2(h3.Y() - h2.Y(), h3.Z() - h2.Z());
        angleDiffYZ1 = angle1YZ - angle2YZ;
        // XZ angle 1
        angle1XZ     = std::atan2(h2.X() - h1.X(), h2.Z() - h1.Z());
        angle2XZ     = std::atan2(h3.X() - h2.X(), h3.Z() - h2.Z());
        angleDiffXZ1 = angle1XZ - angle2XZ;
        // YZ angle 2
        angle3YZ     = std::atan2(h3.Y() - h2.Y(), h3.Z() - h2.Z());
        angle4YZ     = std::atan2(h4.Y() - h3.Y(), h4.Z() - h3.Z());
        angleDiffYZ2 = angle3YZ - angle4YZ;
        // XZ angle 2
        angle3XZ     = std::atan2(h3.X() - h2.X(), h3.Z() - h2.Z());
        angle4XZ     = std::atan2(h4.X() - h3.X(), h4.Z() - h3.Z());
        angleDiffXZ2 = angle3XZ - angle4XZ;

        angleDiffYZ = angleDiffYZ1 - angleDiffYZ2;
        angleDiffXZ = angleDiffXZ1 - angleDiffXZ2;

        /// atan2 returns result in radians!

        if (isJumpTripletLast) {  // last triplet of tracklet is jump triplet
          // YZ cut
          if (angleDiffYZ < -YZ_cut_jump || angleDiffYZ > YZ_cut_jump) continue;
          // positive particles curve -ve in XZ and -ve particles curve +ve in XZ
          if (angleDiffXZ1 < 0) {  // positive particles
            if (angleDiffXZ < XZ_cut_pos_min_jump || angleDiffXZ > XZ_cut_pos_max_jump) continue;
          }
          else {
            if (angleDiffXZ < XZ_cut_neg_min_jump || angleDiffXZ > XZ_cut_neg_max_jump) continue;
          }
        }
        else {  // not jump triplet
          // YZ cut
          if (angleDiffYZ < -YZ_cut || angleDiffYZ > YZ_cut) continue;
          // positive particles curve -ve in XZ and -ve particles curve +ve in XZ
          if (angleDiffXZ1 < 0) {  // positive particles
            if (angleDiffXZ < XZ_cut_pos_min || angleDiffXZ > XZ_cut_pos_max) continue;
          }
          else {
            if (angleDiffXZ < XZ_cut_neg_min || angleDiffXZ > XZ_cut_neg_max) continue;
          }
        }

        // check momentum compatibility of overlapping triplets
        const auto& oldFitParams = trackletFitParams[iTracklet];  // [chi2, qp, Cqp, Tx, C22, Ty, C33]
        const auto& newFitParams = tripletsFitParams[middleSta][iTriplet];
        // check qp compatibility
        float dqp = oldFitParams[1] - newFitParams[1];
        float Cqp = oldFitParams[2] + newFitParams[2];

        if (!std::isfinite(dqp)) continue;
        if (!std::isfinite(Cqp)) continue;

        float qpchi2Cut = 10.0f;                // def - 10.0f
        if (GNNIteration == 1) qpchi2Cut = 5;   // def - 5
        if (GNNIteration == 3) qpchi2Cut = 10;  // def - 10
        if (dqp * dqp > qpchi2Cut * Cqp) continue;

        /// new score should have component of how well the triplets match in momentum
        float newScore = trackletScores[iTracklet] + tripletsScore[middleSta][iTriplet];
        newScore += dqp * dqp / Cqp;  // add momentum chi2 to score

        // create new tracklet with last hit of triplet added
        std::vector<int> newTracklet = tracklet;
        newTracklet.push_back(tripletsByStation[middleSta][iTriplet][2]);
        tracklets.push_back(newTracklet);
        trackletScores.push_back(newScore);
        trackletFitParams.push_back(newFitParams);
      }
    }

    LOG(info) << "Num tracks constructed: " << tracklets.size();

    const int min_length = 4;
    // for iter 1. No fitting
    if (GNNIteration == 0 || GNNIteration == 1) {
      // min length condition
      for (int itracklet = 0; itracklet < (int) tracklets.size(); itracklet++) {
        if (tracklets[itracklet].size() >= min_length) {
          trackAndScores.push_back(std::make_pair(tracklets[itracklet], trackletScores[itracklet]));
        }
      }
      LOG(info) << "[iter 0] Num candidate tracks with length > 4 : " << trackAndScores.size();

      /// remove tracks with chi2 > max_chi2. where max_chi2 is 10*(2*hits - 5) //@TODO: check this
      float trackChi2Cut = 10.0f;                   // When not fitting candidates and using q/p proxy to chi2.
      if (GNNIteration == 0) trackChi2Cut = 10.0f;  // def - 10
      if (GNNIteration == 1) trackChi2Cut = 5.0f;   // def - 5
      for (int iTrack = 0; iTrack < (int) trackAndScores.size(); iTrack++) {
        if (trackAndScores[iTrack].second > trackChi2Cut * (trackAndScores[iTrack].first.size() - 2)) {
          trackAndScores.erase(trackAndScores.begin() + iTrack);
          iTrack--;
        }
      }
      LOG(info) << "[iter 0] Num tracks after tracks chi2 cut: " << trackAndScores.size();
    }
    else if (GNNIteration == 3) {  // iter 3
      // min length condition
      auto trackletsTmp = tracklets;
      tracklets.clear();
      tracklets.reserve(10000);
      auto trackletScoresTmp = trackletScores;
      trackletScores.clear();
      trackletScores.reserve(10000);
      for (int itracklet = 0; itracklet < (int) trackletsTmp.size(); itracklet++) {
        if (trackletsTmp[itracklet].size() >= min_length) {
          tracklets.push_back(trackletsTmp[itracklet]);
          trackletScores.push_back(trackletScoresTmp[itracklet]);
        }
      }
      LOG(info) << "[iter 3] Num candidate tracks with length > 4: " << tracklets.size();

      FitTracklets(tracklets, trackletScores, trackletFitParams);  // scores is chi2 here.

      if (useCandClassifier_) {
        LOG(info) << "[iter 3] Using candidate classifier...";
        std::vector<int> CandClassTopology = {13, 32, 32, 32, 1};
        CandClassifier CandFinder          = CandClassifier(CandClassTopology);
        CandFinder.setTestThreshold(CandClassifierThreshold_);

        std::string srcDir       = "/home/tyagi/cbmroot/NN/CandClassifier/";
        std::string fNameWeights = srcDir + "CandClassWeights_13.txt";
        std::string fNameBiases  = srcDir + "CandClassBiases_13.txt";
        CandFinder.loadModel(fNameWeights, fNameBiases);

        const float chi2Scaling = 50.0f;  // def - 50
        Matrix allCands_ndfSelected;
        std::vector<int> ndfSelected_id_in_tracklets;
        // input to candidate classifier [chi2, tx, ty, qp, C00, C11, C22, C33, C44, ndf, x, y, z]
        for (std::size_t iCand = 0; iCand < tracklets.size(); iCand++) {
          const auto& Cand          = tracklets[iCand];
          const auto& CandfitParams = trackletFitParams[iCand];
          if (Cand.size() > 6) {  // add to trackAndScores directly if ndf > 7
            trackAndScores.push_back(std::make_pair(Cand, trackletScores[iCand]));
          }
          else {  // pass to candidate classifier
            std::vector<float> cand{CandfitParams};
            cand[0] /= chi2Scaling;  // chi2 scaling
            cand[4] *= 1e5;
            cand[5] *= 1e3;
            cand[6] *= 1e2;
            cand[7] *= 1e2;
            cand[9] /= 10.0f;   // ndf
            cand[10] /= 50.0f;  // x
            cand[11] /= 50.0f;  // y
            cand[12] += 44.0f;  // z shift
            cand[12] /= 50.0f;  // z scale

            allCands_ndfSelected.push_back(cand);
            ndfSelected_id_in_tracklets.push_back(iCand);
          }
        }

        if (allCands_ndfSelected.size() == 0) {
          LOG(info) << "[iter 3] No candidate tracks to classify!";
          return;
        }
        std::vector<int> trueCandsIndex;    // index in allCands of true Candidates
        std::vector<float> trueCandsScore;  // score of true edges
        CandFinder.run(allCands_ndfSelected, trueCandsIndex, trueCandsScore);

        // add trueCandsIndex to trackAndScores
        for (std::size_t iCand = 0; iCand < trueCandsIndex.size(); iCand++) {
          const auto& Cand = tracklets[ndfSelected_id_in_tracklets[trueCandsIndex[iCand]]];
          float score      = trackletScores[ndfSelected_id_in_tracklets[trueCandsIndex[iCand]]];  // chi2
          // float score = trueCandsScore[iCand];  // classifier score
          trackAndScores.push_back(std::make_pair(Cand, score));
        }
        LOG(info) << "[iter 3] Num candidate tracks after fitting and classifier filtering: " << trackAndScores.size();
      }
      else {
        LOG(info) << "[iter 3] No candidate classifier used!";
        for (int itracklet = 0; itracklet < (int) tracklets.size(); itracklet++) {
          trackAndScores.push_back(std::make_pair(tracklets[itracklet], trackletScores[itracklet]));
        }
        LOG(info) << "[iter 3] Num candidate tracks after fitting: " << trackAndScores.size();
      }
    }

    if (mode == 2) {  // do track competition

      /// sort tracks by length and lower scores (chi2) first
      std::sort(trackAndScores.begin(), trackAndScores.end(),
                [](const std::pair<std::vector<int>, float>& a, const std::pair<std::vector<int>, float>& b) {
                  if (a.first.size() == b.first.size()) {
                    return a.second < b.second;
                  }
                  return a.first.size() > b.first.size();
                });
      LOG(info) << "Tracks sorted.";

      // #define STANDARD_COMP  // use for no jump iteration results
// #define BEGGAR_COMP  // also good.
#define ALTRUISTIC_COMP  // currently used

#ifdef STANDARD_COMP
      /// go from longest to shortest tracks. Mark used hit strips, remove tracks with used hit strips
      for (std::size_t iTrack = 0; iTrack < trackAndScores.size(); iTrack++) {
        /// check that all hits are not used
        const auto& track = trackAndScores[iTrack].first;
        bool remove       = false;
        for (const auto& hit : track) {
          if (fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].FrontKey()]
              || fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].BackKey()]) {
            trackAndScores.erase(trackAndScores.begin() + iTrack);
            iTrack--;
            remove = true;
            break;
          }
        }
        if (remove) continue;
        /// mark all hits as used
        for (const auto& hit : track) {
          fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].FrontKey()] = 1;
          fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].BackKey()]  = 1;
        }
      }
#endif  // STANDARD_COMP

#ifdef BEGGAR_COMP
      /// beggar method of track competition
      for (std::size_t iTrack = 0; iTrack < trackAndScores.size(); iTrack++) {
        /// check that all hits are not used
        const auto& track       = trackAndScores[iTrack].first;
        bool remove             = false;
        uint usedHits           = 0;
        int usedHitID           = -1;
        int usedHitIndexInTrack = -1;
        for (std::size_t iHit = 0; iHit < track.size(); iHit++) {
          int hit = track[iHit];
          if (fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].FrontKey()]
              || fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].BackKey()]) {
            usedHits++;
            usedHitID           = hit;
            usedHitIndexInTrack = iHit;
          }
        }
        if (usedHits == 1) {  // 'beg' for hit from longer accepted track
          bool begged = false;
          for (std::size_t iBeg = 0; iBeg < iTrack; iBeg++) {
            if (trackAndScores[iBeg].first.size() <= trackAndScores[iTrack].first.size())
              continue;                                        // only beg from longer tracks
            if (trackAndScores[iBeg].first.size() < 5) break;  // atleast 4 hits must be left after donation
            if (trackAndScores[iBeg].second < trackAndScores[iTrack].second)
              continue;  // dont donate to higher chi2 beggar

            auto& begTrack = trackAndScores[iBeg].first;
            for (std::size_t iBegHit = 0; iBegHit < begTrack.size(); iBegHit++) {
              const auto begHit = begTrack[iBegHit];
              if (begHit == usedHitID) continue;  // dont let exact hit be borrowed.
              if (fAlgo->fWindowHits[begHit].FrontKey() == fAlgo->fWindowHits[usedHitID].FrontKey()
                  || fAlgo->fWindowHits[begHit].BackKey()
                       == fAlgo->fWindowHits[usedHitID].BackKey()) {  // only one track will match
                // remove iBegHit from begTrack
                begTrack.erase(begTrack.begin() + iBegHit);
                // reset hit flags. Will be reset by beggar
                fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[begHit].FrontKey()] = 0;
                fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[begHit].BackKey()]  = 0;

                begged = true;  // stop begging.
                remove = false;
                break;
              }
            }
            if (begged) break;
            remove = true;  // remove track if begging not successful
          }
        }
        else if (usedHits > 1) {  // remove track with more than one hit used
          remove = true;
        }

        if (remove) {
          trackAndScores.erase(trackAndScores.begin() + iTrack);
          iTrack--;
          continue;
        }
        /// mark all hits as used
        for (const auto& hit : track) {
          fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].FrontKey()] = 1;
          fAlgo->fvHitKeyFlags[fAlgo->fWindowHits[hit].BackKey()]  = 1;
        }
      }
#endif  // BEGGAR_COMP

#ifdef ALTRUISTIC_COMP
      /// --- TEST ---
      for (std::size_t iTrack = 0; iTrack < trackAndScores.size(); iTrack++) {
        /// check that all hits are not used
        auto& track    = trackAndScores[iTrack].first;
        bool remove    = false;
        uint nUsedHits = 0;
        std::vector<int> usedHitIDs;
        std::vector<int> usedHitIndexesInTrack;
        for (std::size_t iHit = 0; iHit < track.size(); iHit++) {
          const ca::Hit& hit = frWData.Hit(track[iHit]);
          if (frWData.IsHitKeyUsed(hit.FrontKey()) || frWData.IsHitKeyUsed(hit.BackKey())) {
            nUsedHits++;
            usedHitIDs.push_back(track[iHit]);
            usedHitIndexesInTrack.push_back(iHit);
          }
        }
        if (nUsedHits == 0) {  // clean tracks
          /// mark all hits as used
          for (const auto& hit : track) {
            frWData.IsHitKeyUsed(frWData.Hit(hit).FrontKey()) = 1;
            frWData.IsHitKeyUsed(frWData.Hit(hit).BackKey())  = 1;
          }
          continue;  // go next track
        }
        else if (nUsedHits > 0) {  // some hits used but still >=4 hits left
          if (track.size() - nUsedHits >= 4) {
            // remove used hits.
            // read usedHitIndexes in reverse order
            std::sort(usedHitIndexesInTrack.begin(), usedHitIndexesInTrack.end(), std::greater<int>());
            for (const auto usedHitIndex : usedHitIndexesInTrack) {
              track.erase(track.begin() + usedHitIndex);
            }
            // mark remaining hits as used
            for (const auto& hit : track) {
              frWData.IsHitKeyUsed(frWData.Hit(hit).FrontKey()) = 1;
              frWData.IsHitKeyUsed(frWData.Hit(hit).BackKey())  = 1;
            }
            continue;  // go next track
          }
          else {
            remove = true;  // remove track if begging not successful
          }
        }

        if (remove
            && (track.size() - nUsedHits == 3)) {  // 'beg' for hit from longer accepted track only if one hit required
          for (std::size_t iBeg = 0; iBeg < iTrack; iBeg++) {  // track to beg from
            if (trackAndScores[iBeg].first.size() <= trackAndScores[iTrack].first.size())
              continue;                                        // only beg from longer tracks
            if (trackAndScores[iBeg].first.size() < 5) break;  // atleast 4 hits must be left after donation
            if (trackAndScores[iBeg].second < trackAndScores[iTrack].second)
              continue;  // dont donate to higher chi2 beggar

            auto& begTrack = trackAndScores[iBeg].first;
            for (std::size_t iBegHit = 0; iBegHit < begTrack.size(); iBegHit++) {
              const auto begHit = begTrack[iBegHit];
              if (begHit == usedHitIDs[0]) continue;  // dont let exact hit be borrowed.
              if (frWData.Hit(begHit).FrontKey() == frWData.Hit(usedHitIDs[0]).FrontKey()
                  || frWData.Hit(begHit).BackKey()
                       == frWData.Hit(usedHitIDs[0]).BackKey()) {  // only one track will match
                // remove iBegHit from begTrack
                begTrack.erase(begTrack.begin() + iBegHit);
                // reset hit flags. Will be reset by beggar
                frWData.IsHitKeyUsed(frWData.Hit(begHit).FrontKey()) = 0;
                frWData.IsHitKeyUsed(frWData.Hit(begHit).BackKey())  = 0;

                remove = false;
                break;
              }
            }
          }
        }

        if (remove) {
          trackAndScores.erase(trackAndScores.begin() + iTrack);
          iTrack--;
          continue;
        }
        /// mark all hits as used
        for (const auto& hit : track) {
          frWData.IsHitKeyUsed(frWData.Hit(hit).FrontKey()) = 1;
          frWData.IsHitKeyUsed(frWData.Hit(hit).BackKey())  = 1;
        }
      }
#endif  // ALTRUISTIC_COMP

    }  // track competition

    // save tracks
    for (int iTrack = 0; iTrack < (int) trackAndScores.size(); iTrack++) {
      tracks.push_back(trackAndScores[iTrack].first);
    }

    LOG(info) << "Num tracks after cleaning: " << tracks.size();
  }  // CreateTracksTriplets

  void GraphConstructor::FitTriplets(const int GNNiteration)
  {
    Vector<int> selectedTripletIndexes;
    Vector<float> selectedTripletScores;
    Vector<Track> GNNTripletCandidates;
    Vector<HitIndex_t> GNNTripletHits;
    GNNTripletCandidates.reserve(triplets_.size());
    GNNTripletHits.reserve(triplets_.size() * 3);
    selectedTripletIndexes.reserve(triplets_.size());
    selectedTripletScores.reserve(triplets_.size());

    std::vector<std::vector<float>>
      selectedTripletFitParams;  // [chi2, qp, Cqp, T3.Tx()[0], T3.C22()[0], T3.Ty()[0], T3.C33()[0]]
    selectedTripletFitParams.reserve(triplets_.size());

    for (const auto& trip : triplets_) {
      for (const auto& hit : trip) {
        int hitID = frWData.Hit(hit).Id();  // index in InputData
        GNNTripletHits.push_back(hitID);
      }
      Track t;
      t.fNofHits = trip.size();
      GNNTripletCandidates.push_back(t);
    }
    frTrackFitter.FitGNNTriplets(frInput, frWData, GNNTripletCandidates, GNNTripletHits, selectedTripletIndexes,
                                 selectedTripletScores, selectedTripletFitParams, GNNiteration);
    LOG(info) << "Candidate triplets fitted with KF.";

    /// remove from tripletScores_ and triplets_, triplets that not selected by KF
    auto tripletsTmp = triplets_;
    triplets_.clear();
    tripletScores_.clear();
    triplets_.reserve(selectedTripletIndexes.size());
    tripletScores_.reserve(selectedTripletIndexes.size());
    tripletFitParams_.reserve(selectedTripletIndexes.size());
    for (std::size_t i = 0; i < selectedTripletIndexes.size(); ++i) {
      // replace NN score with KF chi2
      tripletScores_.push_back(selectedTripletScores[i]);
      tripletFitParams_.push_back(selectedTripletFitParams[i]);
      triplets_.push_back(tripletsTmp[selectedTripletIndexes[i]]);
    }
    LOG(info) << "Num triplets after KF fit: " << tripletScores_.size();
  }  // FitTriplets

  void GraphConstructor::FitTracklets(std::vector<std::vector<int>>& tracklets, std::vector<float>& trackletScores,
                                      std::vector<std::vector<float>>& trackletFitParams)
  {
    Vector<int> selectedTrackIndexes;
    Vector<float> selectedTrackScores;
    Vector<Track> GNNTrackCandidates;
    Vector<HitIndex_t> GNNTrackHits;
    GNNTrackCandidates.reserve(tracklets.size());
    GNNTrackHits.reserve(tracklets.size() * 10);
    selectedTrackIndexes.reserve(tracklets.size());
    selectedTrackScores.reserve(tracklets.size());

    std::vector<std::vector<float>>
      selectedTrackFitParams;  // [chi2, qp, Cqp, T3.Tx()[0], T3.C22()[0], T3.Ty()[0], T3.C33()[0]]
    selectedTrackFitParams.reserve(tracklets.size());

    for (const auto& trackCand : tracklets) {
      for (const auto& hit : trackCand) {
        const int hitID = frWData.Hit(hit).Id();  // get hit id in fInputData
        GNNTrackHits.push_back(hitID);
      }
      Track t;
      t.fNofHits = trackCand.size();
      GNNTrackCandidates.push_back(t);
    }

    frTrackFitter.FitGNNTracklets(frInput, frWData, GNNTrackCandidates, GNNTrackHits, selectedTrackIndexes, selectedTrackScores,
                                        selectedTrackFitParams, 3);
    LOG(info) << "Candidate tracks fitted with KF.";

    /// print track params of first 10 tracks
    // for (int i = 0; i < 10; i++) {
    //   std::cout << "Track " << i << " chi2 = " << TrackFitParams[i][0] << " qp = " << TrackFitParams[i][1]
    //             << " Cqp = " << TrackFitParams[i][2] << " Tx = " << TrackFitParams[i][3] << " C22 = " << TrackFitParams[i][4]
    //             << " Ty = " << TrackFitParams[i][5] << " C33 = " << TrackFitParams[i][6] << std::endl;
    // }

    /// remove from tracklets, tracks not selected by KF
    auto trackletsTmp = tracklets;
    tracklets.clear();
    tracklets.reserve(selectedTrackIndexes.size());
    trackletScores.clear();
    trackletScores.reserve(selectedTrackIndexes.size());
    trackletFitParams.clear();
    trackletFitParams.reserve(selectedTrackIndexes.size());
    for (std::size_t i = 0; i < selectedTrackIndexes.size(); ++i) {
      trackletScores.push_back(selectedTrackScores[i]);
      trackletFitParams.push_back(selectedTrackFitParams[i]);
      tracklets.push_back(trackletsTmp[selectedTrackIndexes[i]]);
    }
    LOG(info) << "Num tracks after KF fit: " << trackletScores.size();
  } // FitTracklets

  // @brief: add tracks and hits to final containers
  void GraphConstructor::PrepareFinalTracks()
  {
    if (tracks.size() == 0) {  // no tracks found
      return;
    }
    for (const auto& track : tracks) {
      for (const auto& iHit : track) {
        const ca::Hit& hit = frWData.Hit(iHit);
        // used strips are marked
        frWData.IsHitKeyUsed(hit.FrontKey()) = 1;
        frWData.IsHitKeyUsed(hit.BackKey())  = 1;
        frWData.RecoHitIndices().push_back(hit.Id());
      }
      Track t;
      t.fNofHits = track.size();
      frWData.RecoTracks().push_back(t);
    }
  }  // prepareFinalTracks

  void GraphConstructor::CreateMetricLearningDoublets(const int iter)
  {
    LOG(info) << std::string(50, '-');
    LOG(info) << "CreateMetricLearningDoublets";
    LOG(info) << std::string(50, '-');

    std::vector<std::vector<float>> embedCoord;
    embedCoord.reserve(10000);
    std::vector<std::vector<float>> NNinput;
    NNinput.reserve(10000);
    std::vector<std::vector<std::vector<float>>> NNinputFinal;
    std::vector<int> staStartIndex;
    staStartIndex.reserve(NStations);
    staStartIndex.push_back(0);
    // Step 1 - embed all hits in time slice
    {
      std::vector<int> EmbNetTopology_ = {3, 16, 16, 6};
      EmbedNet EmbNet_                 = EmbedNet(EmbNetTopology_);
      const std::string srcDir         = "/home/tyagi/cbmroot/NN/";
      if (iter == 0) {
        std::string fNameModel   = "embed/embed";
        std::string fNameWeights = srcDir + fNameModel + "Weights_11.txt";
        std::string fNameBiases  = srcDir + fNameModel + "Biases_11.txt";
        EmbNet_.loadModel(fNameWeights, fNameBiases);
      }
      else if (iter == 3) {
        std::string fNameModel   = "embed/embed";
        std::string fNameWeights = srcDir + fNameModel + "Weights_13.txt";
        std::string fNameBiases  = srcDir + fNameModel + "Biases_13.txt";
        EmbNet_.loadModel(fNameWeights, fNameBiases);
      }

      std::vector<fscal> input;
      for (int istal = 0; istal < NStations; istal++) {
        const int nGridEntriesL = frWData.Grid(istal).GetEntries().size();
        staStartIndex.push_back(staStartIndex.back() + nGridEntriesL);
        for (auto iel = 0; iel < nGridEntriesL; iel++) {
          ca::HitIndex_t ihitl = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits
          ca::Hit& hitl        = frWData.Hit(ihitl);
          fscal x              = hitl.X();
          fscal y              = hitl.Y();
          fscal z              = hitl.Z() + 44.0f;  // shift z to positive
          input.push_back(x);
          input.push_back(y);
          input.push_back(z);
          NNinput.push_back(input);
          input.clear();
        }
      }  // istal
      NNinputFinal.push_back(NNinput);
      EmbNet_.run(NNinputFinal);
      EmbNet_.getEmbeddedCoords(embedCoord, 0);  // only works for event one.
      LOG(info) << "EmbedCoord size: " << embedCoord.size();
    }  // step 1


    // Step 2 - use kNN to form doublets
    {
      // initialize doublets
      std::vector<HitIndex_t> dum;
      dum.push_back(0);
      for (int istal = 0; istal < NStations; istal++) {
        doublets[istal].clear();
        const int nGridEntriesL = frWData.Grid(istal).GetEntries().size();
        doublets[istal].reserve(nGridEntriesL);
        for (int iel = 0; iel < nGridEntriesL; iel++) {
          doublets[istal].push_back(dum);
        }
      }

      int kNNOrder = 25;
      switch (iter) {
        case 0: kNNOrder = maxNeighOrderPrim_; break;
        case 1: kNNOrder = maxNeighOrderAllPrim_; break;
        case 3: kNNOrder = maxNeighOrderSec_; break;
        default: break;
      }

      std::vector<HitIndex_t> closestHitsWindow;
      closestHitsWindow.reserve(kNNOrder + 1);
      std::vector<HitIndex_t> closestHits;  // for finding maxDist when closestHits.size > maxNeighOrder
      closestHits.reserve(kNNOrder + 1);
      float maxDist    = 0.0f;
      int maxDistIndex = 0;
      float d          = 0.0f;
      for (int istal = 0; istal < NStations; istal++) {
        const int nGridEntriesL = frWData.Grid(istal).GetEntries().size();
        for (int iel = 0; iel < nGridEntriesL; iel++) {
          ca::HitIndex_t ihitl = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits

          int nGridEntriesM = frWData.Grid(istal + 1).GetEntries().size();
          for (int iem = 0; iem < nGridEntriesM; iem++) {
            ca::HitIndex_t ihitm = frWData.Grid(istal + 1).GetEntries()[iem].GetObjectId();  // index in fvHits

            d = MLPutil::hitDistanceSq(embedCoord[staStartIndex[istal] + iel],
                                       embedCoord[staStartIndex[istal + 1] + iem]);
            if (closestHitsWindow.size() < kNNOrder) {
              closestHitsWindow.push_back(ihitm);
              closestHits.push_back(staStartIndex[istal + 1] + iem);
              if (d > maxDist) {
                maxDist      = d;
                maxDistIndex = closestHitsWindow.size() - 1;
              }
            }
            else if (d < maxDist) {
              closestHitsWindow.push_back(ihitm);
              closestHitsWindow.erase(closestHitsWindow.begin() + maxDistIndex);
              closestHits.push_back(staStartIndex[istal + 1] + iem);
              closestHits.erase(closestHits.begin() + maxDistIndex);
              maxDist = 0.0f;
              for (int i = 0; i < (int) closestHits.size(); i++) {
                d = MLPutil::hitDistanceSq(embedCoord[staStartIndex[istal] + iel], embedCoord[closestHits[i]]);
                if (d > maxDist) {
                  maxDist      = d;
                  maxDistIndex = i;
                }
              }
            }
          }
          doublets[istal][iel] = closestHitsWindow;
          closestHitsWindow.clear();
          closestHits.clear();
          maxDist      = 0.0f;
          maxDistIndex = 0;
        }
      }
    }

    /// count num of doublets
    int numDoublets = 0;
    for (int istal = 0; istal < NStations; istal++) {
      for (int ih = 0; ih < (int) doublets[istal].size(); ih++) {
        numDoublets += doublets[istal][ih].size();
      }
    }
    LOG(info) << "Total Number of doublets: " << numDoublets;

    LOG(info) << "End metric learning doublets";
  }

  ///@brief: Create also edges which skip one station
  void GraphConstructor::CreateMetricLearningDoubletsJump(const int iter)
  {
    LOG(info) << std::string(50, '-');
    LOG(info) << "CreateMetricLearningDoubletsJump";
    LOG(info) << std::string(50, '-');

    std::vector<std::vector<float>> embedCoord;
    embedCoord.reserve(10000);
    std::vector<std::vector<float>> NNinput;
    NNinput.reserve(10000);
    std::vector<std::vector<std::vector<float>>> NNinputFinal;
    std::vector<int> staStartIndex;
    staStartIndex.reserve(NStations);
    staStartIndex.push_back(0);
    // Step 1 - embed all hits in time slice
    {
      std::vector<int> EmbNetTopology_ = {3, 16, 16, 6};
      EmbedNet EmbNet_                 = EmbedNet(EmbNetTopology_);
      const std::string srcDir         = "/home/tyagi/cbmroot/NN/";
      if (iter == 0) {
        std::string fNameModel   = "embed/embed";
        std::string fNameWeights = srcDir + fNameModel + "Weights_11.txt";
        std::string fNameBiases  = srcDir + fNameModel + "Biases_11.txt";
        EmbNet_.loadModel(fNameWeights, fNameBiases);
      }
      else if (iter == 3 || iter == 1) {
        std::string fNameModel   = "embed/embed";
        std::string fNameWeights = srcDir + fNameModel + "Weights_13.txt";
        std::string fNameBiases  = srcDir + fNameModel + "Biases_13.txt";
        EmbNet_.loadModel(fNameWeights, fNameBiases);
      }

      std::vector<fscal> input;
      for (int istal = 0; istal < NStations; istal++) {
        const int nGridEntriesL = frWData.Grid(istal).GetEntries().size();
        staStartIndex.push_back(staStartIndex.back() + nGridEntriesL);
        for (auto iel = 0; iel < nGridEntriesL; ++iel) {
          ca::HitIndex_t ihitl = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits
          ca::Hit& hitl        = frWData.Hit(ihitl);
          fscal x              = hitl.X();
          fscal y              = hitl.Y();
          fscal z              = hitl.Z() + 44.0f;  // shift z to positive
          input.push_back(x);
          input.push_back(y);
          input.push_back(z);
          NNinput.push_back(input);
          input.clear();
        }  // hit on station
      }  // istal
      NNinputFinal.push_back(NNinput);
      EmbNet_.run(NNinputFinal);
      EmbNet_.getEmbeddedCoords(embedCoord, 0);  // only works for event one.
      LOG(info) << "EmbedCoord size: " << embedCoord.size();
    }  // step 1


    // Step 2 - use kNN to form doublets
    {
      // initialize doublets
      std::vector<HitIndex_t> dum;
      dum.push_back(0);
      for (int istal = 0; istal < NStations; istal++) {
        doublets[istal].clear();
        const int nGridEntriesL = frWData.Grid(istal).GetEntries().size();
        doublets[istal].reserve(nGridEntriesL);
        for (int iel = 0; iel < nGridEntriesL; iel++) {
          doublets[istal].push_back(dum);
        }
      }

      int kNNOrder_Jump = 10;  // jump doublets
      int kNNOrder      = 25;
      switch (iter) {
        case 0: kNNOrder = maxNeighOrderPrim_; break;
        case 1: {
          kNNOrder_Jump = maxNeighOrderAllPrimJump_;
          kNNOrder      = maxNeighOrderAllPrim_;
          break;
        }
        case 3: {
          kNNOrder_Jump = maxNeighOrderSecJump_;
          kNNOrder      = maxNeighOrderSec_;
          break;
        }
        default: break;
      }

      // Doublets without jump
      std::vector<HitIndex_t> closestHitsWindow;
      closestHitsWindow.reserve(kNNOrder + 1);
      std::vector<HitIndex_t> closestHits;  // for finding maxDist when closestHits.size > maxNeighOrder
      closestHits.reserve(kNNOrder + 1);
      float maxDist    = 0.0f;
      int maxDistIndex = 0;
      float d          = 0.0f;

      for (int istal = 0; istal < NStations; istal++) {
        const int nGridEntriesL = frWData.Grid(istal).GetEntries().size();
        for (int iel = 0; iel < nGridEntriesL; iel++) {
          ca::HitIndex_t ihitl = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits
          int nGridEntriesM    = frWData.Grid(istal + 1).GetEntries().size();
          for (int iem = 0; iem < nGridEntriesM; iem++) {
            ca::HitIndex_t ihitm = frWData.Grid(istal + 1).GetEntries()[iem].GetObjectId();  // index in fvHits
            d                    = MLPutil::hitDistanceSq(embedCoord[staStartIndex[istal] + iel],
                                                          embedCoord[staStartIndex[istal + 1] + iem]);
            if (closestHitsWindow.size() < kNNOrder) {
              closestHitsWindow.push_back(ihitm);
              closestHits.push_back(staStartIndex[istal + 1] + iem);
              if (d > maxDist) {
                maxDist      = d;
                maxDistIndex = closestHitsWindow.size() - 1;
              }
            }
            else if (d < maxDist) {
              closestHitsWindow.push_back(ihitm);
              closestHitsWindow.erase(closestHitsWindow.begin() + maxDistIndex);
              closestHits.push_back(staStartIndex[istal + 1] + iem);
              closestHits.erase(closestHits.begin() + maxDistIndex);
              maxDist = 0.0f;
              for (int i = 0; i < (int) closestHits.size(); i++) {
                d = MLPutil::hitDistanceSq(embedCoord[staStartIndex[istal] + iel], embedCoord[closestHits[i]]);
                if (d > maxDist) {
                  maxDist      = d;
                  maxDistIndex = i;
                }
              }
            }
          }
          doublets[istal][iel] = closestHitsWindow;
          closestHitsWindow.clear();
          closestHits.clear();
          maxDist      = 0.0f;
          maxDistIndex = 0;
        }
      }

      // Doublets with one station skipped
      closestHitsWindow.clear();
      closestHitsWindow.reserve(kNNOrder_Jump + 1);
      closestHits.clear();  // for finding maxDist when closestHits.size > maxNeighOrder
      closestHits.reserve(kNNOrder_Jump + 1);
      maxDist      = 0.0f;
      maxDistIndex = 0;
      d            = 0.0f;
      for (int istal = 0; istal < NStations - 1; istal++) {
        const int iStaNext      = istal + 2;  // skip one station
        const int nGridEntriesL = frWData.Grid(istal).GetEntries().size();
        for (int iel = 0; iel < nGridEntriesL; iel++) {
          ca::HitIndex_t ihitl = frWData.Grid(istal).GetEntries()[iel].GetObjectId();  // index in fvHits
          int nGridEntriesM    = frWData.Grid(iStaNext).GetEntries().size();
          for (int iem = 0; iem < nGridEntriesM; iem++) {
            ca::HitIndex_t ihitm = frWData.Grid(iStaNext).GetEntries()[iem].GetObjectId();
            d =
              MLPutil::hitDistanceSq(embedCoord[staStartIndex[istal] + iel], embedCoord[staStartIndex[iStaNext] + iem]);
            if (closestHitsWindow.size() < kNNOrder_Jump) {
              closestHitsWindow.push_back(ihitm);
              closestHits.push_back(staStartIndex[iStaNext] + iem);
              if (d > maxDist) {
                maxDist      = d;
                maxDistIndex = closestHitsWindow.size() - 1;
              }
            }
            else if (d < maxDist) {
              closestHitsWindow.push_back(ihitm);
              closestHitsWindow.erase(closestHitsWindow.begin() + maxDistIndex);
              closestHits.push_back(staStartIndex[iStaNext] + iem);
              closestHits.erase(closestHits.begin() + maxDistIndex);
              maxDist = 0.0f;
              for (int i = 0; i < (int) closestHits.size(); i++) {
                d = MLPutil::hitDistanceSq(embedCoord[staStartIndex[istal] + iel], embedCoord[closestHits[i]]);
                if (d > maxDist) {
                  maxDist      = d;
                  maxDistIndex = i;
                }
              }
            }
          }
          // insert closestHitsWindow to end of doublets[istal][iel]
          auto& temp = doublets[istal][iel];
          temp.insert(temp.end(), closestHitsWindow.begin(), closestHitsWindow.end());
          closestHitsWindow.clear();
          closestHits.clear();
          maxDist      = 0.0f;
          maxDistIndex = 0;
        }
      }
    }

    /// count num of doublets
    int numDoublets = 0;
    for (int istal = 0; istal < NStations; istal++) {
      for (int ih = 0; ih < (int) doublets[istal].size(); ih++) {
        numDoublets += doublets[istal][ih].size();
      }
    }
    LOG(info) << "Total Number of doublets with jump: " << numDoublets;

    LOG(info) << "End metric learning doublets with Jump";
  }  // CreateMetricLearningDoubletsJump


}  // namespace cbm::algo::ca