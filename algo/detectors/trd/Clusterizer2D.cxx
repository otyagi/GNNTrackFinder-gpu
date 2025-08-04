/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#include "Clusterizer2D.h"

#include "AlgoFairloggerCompat.h"

#include <algorithm>

namespace cbm::algo::trd
{

  //_______________________________________________________________________________
  std::vector<Cluster2D> Clusterizer2D::operator()(const std::vector<std::pair<CbmTrdDigi, int32_t>>& inVec,
                                                   const uint64_t t0) const
  {
    const size_t numRows = fParams.rowPar.size();
    const size_t numCols = fParams.rowPar[0].padPar.size();

    std::vector<std::vector<Cluster2D*>> buffer(numRows);  //row-wise organized clusters
    std::vector<size_t> bufferPos(numRows);                //storage of starting positon in buffers
    std::vector<Cluster2D> clustersOut;                    //clusters to be returned
    std::vector<inputType> inputData;                      //formatted input

    // Apply input formatting
    for (auto& input : inVec) {
      const CbmTrdDigi* d = &input.first;
      const size_t index  = input.second;
      int pad             = d->GetAddressChannel();
      uint16_t chT        = pad << 1;
      uint16_t chR        = chT + 1;
      int dtime;
      double t;
      double r      = d->GetCharge(t, dtime);  //subtract timeslice start time. maybe not needed anymore?
      int tm        = d->GetTimeDAQ() - t0;
      const int row = pad / numCols;
      const int col = pad % numCols;
      if (dtime < 0) tm += dtime;  // correct for the difference between tilt and rect
      if (r < 1 && !fParams.rowPar[row].padPar[col].chRMasked) chR = 0;
      if (t < 1 && !fParams.rowPar[row].padPar[col].chTMasked) chT = 0;
      inputData.emplace_back(chT, chR, tm, row, index, d);
    }

    // Sort input by time. TO DO: Use insert-sort like in TOF hitfinder
    std::sort(inputData.begin(), inputData.end(),
              [](const auto& a, const auto& b) -> bool { return std::get<2>(a) < std::get<2>(b); });

    // Fill row buffers
    for (auto& in : inputData) {
      auto& [chT, chR, tm, row, id, digi] = in;

      // check for saved
      if (buffer[row].empty()) {
        buffer[row].push_back(new Cluster2D(fParams.address, id, digi, chT, chR, row, tm));
        bufferPos[row] = 0;
        continue;
      }
      /////// To do: Check whether position storing truly helps, after Cluster2D::Merge() is optimized

      // Get stored starting position
      auto itc = (buffer[row].begin() + bufferPos[row]);

      //Update cluster
      for (; itc != buffer[row].end(); itc++) {
        if ((int64_t)(*itc)->GetStartTime() < tm - 5) {
          bufferPos[row]++;
          continue;
        }
        if ((*itc)->IsChannelInRange(chT, chR) == 0) {
          const uint tc = (*itc)->GetStartTime();
          (*itc)->AddDigi(id, digi, chT, chR, tc - tm);
          break;
        }
      }
      if (itc == buffer[row].end()) {
        buffer[row].push_back(new Cluster2D(fParams.address, id, digi, chT, chR, row, tm));
      }
    }

    // Build clusters
    for (auto& clRow : buffer) {

      // TO DO: Process the following remark from original code:
      // TODO look left and right for masked channels. If they exists add them to cluster.
      // if (AddClusterEdges(*itc0) && CWRITE(0)) L_(debug) << "  edge cl[0] : " << (*itc0)->ToString();

      // Phase 0 : try merge clusters if more than one on a row
      for (auto itc0 = clRow.begin(); itc0 != clRow.end(); itc0++) {
        if (*itc0 == nullptr) {
          continue;
        }
        for (auto itc1 = std::next(itc0); itc1 != clRow.end(); itc1++) {
          if (*itc1 == nullptr) {
            continue;
          }
          if ((*itc1)->GetStartTime() - (*itc0)->GetStartTime() > 50) {
            break;
          }
          if ((*itc0)->Merge((*itc1))) {
            delete (*itc1);
            *itc1 = nullptr;
            itc1  = itc0;
          }
        }
      }

      // Phase 1 : copy older clusters from the buffer to the module wise storage
      for (auto& pcluster : clRow) {
        if (pcluster == nullptr) {
          continue;
        }
        clustersOut.emplace_back(*pcluster);
        clustersOut.back().Finalize(fParams.rowPar[0].padPar.size());
        delete pcluster;
        pcluster = nullptr;
      }
    }

    return clustersOut;
  }

}  // namespace cbm::algo::trd
