/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Oddharak Tyagi [committer] */

/// \file GraphConstructor.cxx
/// \brief Functions for running GNN algorithm
/// \author Oddharak Tyagi

#include "GraphConstructor.h"

namespace cbm::algo::ca
{

  GraphConstructor::GraphConstructor(WindowData& wData) : frWData(wData) {}

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

    // FitTriplets();  // replaces dummy triplet score with KF chi2

    if (mode == 0) {
      SaveAllTripletsAsTracks();
    }
    else {
      CreateTracksTriplets(mode);
    }
  }  // FindFastPrim

  /// combine overlapping triplets to form tracks. sort by length and score for competition
  void GraphConstructor::CreateTracksTriplets(const int mode) {}  // CreateTracksTriplets

  void GraphConstructor::FitTriplets() {}  // FitTriplets

  // @brief: add tracks and hits to final containers
  void GraphConstructor::PrepareFinalTracks()
  {
    if (tracks.size() == 0) {  // no tracks found
      return;
    }
    for (const auto& track : tracks) {
      for (const auto& iHit : track) {
        const ca::Hit& hit = frWData.Hit(iHit);
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

}  // namespace cbm::algo::ca