/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Etienne Bechtel, Florian Uhlig */

#include "Clusterizer.h"

#include <algorithm>
#include <unordered_map>

namespace cbm::algo::trd
{

  //_______________________________________________________________________________
  std::vector<Cluster> Clusterizer::operator()(const std::vector<std::pair<CbmTrdDigi, int32_t>>& inVec) const
  {
    std::vector<inputType> inputData;  //digis storage;
    std::vector<Cluster> clustersOut;
    std::vector<inputType*> digiPtrVec;                //digis pointer storage (kSelf only);
    std::vector<std::vector<inputType*>> digiMapSelf;  //channel-wise sorted input
    std::vector<std::vector<inputType*>> digiMapNeig;  //channel-wise sorted input

    const size_t numCols = fParams.rowPar[0].padPar.size();
    const size_t numRows = fParams.rowPar.size();
    const size_t numChan = numCols * numRows;

    for (auto& input : inVec) {
      inputData.push_back(std::make_tuple(input.second, &input.first, input.first.GetTime()));
    }
    ///// DRAW PICTURE HERE (LOGIC OF MAPS)

    // Initialize the digi maps
    digiMapSelf.resize(numChan);
    digiMapNeig.resize(numChan);

    for (auto& digidata : inputData) {
      const CbmTrdDigi* digi = (const CbmTrdDigi*) std::get<1>(digidata);
      if (!digi) continue;  // Skip invalid digis
      const CbmTrdDigi::eTriggerType type = static_cast<CbmTrdDigi::eTriggerType>(digi->GetTriggerType());
      const int channel                   = digi->GetAddressChannel();
      if (type == CbmTrdDigi::eTriggerType::kSelf) {
        digiMapSelf[channel].push_back(&digidata);
        digiPtrVec.push_back(&digidata);
      }
      if (type == CbmTrdDigi::eTriggerType::kNeighbor) {
        digiMapNeig[channel].push_back(&digidata);
      }
    }

    //Store last position in input channels to avoid unnecessary checks.
    std::vector<std::vector<inputType*>::iterator> lastChanPosSelf, lastChanPosNeig;
    for (size_t chan = 0; chan < numChan; chan++) {
      lastChanPosSelf.push_back(digiMapSelf[chan].begin());
      lastChanPosNeig.push_back(digiMapNeig[chan].begin());
    }

    // search for an unprocessed main triggered digi and then start a subloop to
    // directly construct the cluster  (search for main-trigger then add the neighbors)
    for (auto mainit = digiPtrVec.begin(); mainit != digiPtrVec.end(); mainit++) {

      // Skip if digi already processed
      const CbmTrdDigi* digi = (const CbmTrdDigi*) std::get<1>(**mainit);
      if (digi == nullptr) continue;

      // get digi time
      const double time = std::get<2>(**mainit);

      // variety of neccessary address information; uses the "combiId" for the
      // comparison of digi positions
      const int digiId  = std::get<0>(**mainit);
      const int channel = digi->GetAddressChannel();
      const int ncols   = fParams.rowPar[0].padPar.size();

      // some logic information which is used to process and find the clusters
      int lowcol  = channel;
      int highcol = channel;

      // the "seal" bools register when the logical end of the cluster was found
      bool sealtopcol = false;
      bool sealbotcol = false;

      // already seal the cluster if the main trigger is already at the right or left padrow border
      if (channel % ncols == ncols - 1) {
        sealtopcol = true;
      }
      if (channel % ncols == 0) {
        sealbotcol = true;
      }

      // //vector which contains the actual cluster
      std::vector<std::pair<int, const CbmTrdDigi*>> cluster;
      cluster.emplace_back(digiId, digi);
      std::get<1>(**mainit) = nullptr;

      // loop to find the other pads corresponding to the main trigger
      // is exited either if the implemented trigger logic is fullfilled
      // or if there are no more adjacend pads due to edges,etc.
      while (true) {

        // counter which is used to easily break clusters which are at the edge and
        // therefore do not fullfill the classical look
        const size_t oldSize = cluster.size();

        const int col = channel % ncols;
        if (col == ncols - 1) sealtopcol = true;
        if (col == 0) sealbotcol = true;

        if (!sealbotcol && 0 <= lowcol - 1) {
          if (TryAddDigi(&digiMapSelf, lowcol - 1, &lastChanPosSelf, &cluster, time)) {
            lowcol--;
          }
        }
        if (!sealtopcol && highcol + 1 < numChan) {
          if (TryAddDigi(&digiMapSelf, highcol + 1, &lastChanPosSelf, &cluster, time)) {
            highcol++;
          }
        }
        if (!sealbotcol && 0 <= lowcol - 1) {
          if (TryAddDigi(&digiMapNeig, lowcol - 1, &lastChanPosNeig, &cluster, time)) {
            sealbotcol = true;
          }
        }
        if (!sealtopcol && highcol + 1 < numChan) {
          if (TryAddDigi(&digiMapNeig, highcol + 1, &lastChanPosNeig, &cluster, time)) {
            sealtopcol = true;
          }
        }

        ////// Row merging was formerly here. To do: Implement as post-processing

        // some finish criteria
        if (cluster.size() - oldSize == 0) break;
        if (sealbotcol && sealtopcol) break;
      }  //!  while (true)

      addClusters(cluster, &clustersOut);
    }  //! for (auto mainit = inputData.begin(); mainit != inputData.end(); mainit++)

    return clustersOut;
  }

  bool Clusterizer::TryAddDigi(std::vector<std::vector<inputType*>>* digimap, int chan,
                               std::vector<std::vector<inputType*>::iterator>* lastPos,
                               std::vector<std::pair<int, const CbmTrdDigi*>>* cluster, const double digiTime) const
  {
    const double interval = CbmTrdDigi::Clk(CbmTrdDigi::eCbmTrdAsicType::kSPADIC);

    // find the FN digis of main trigger or adjacent main triggers
    for (auto FNit = (*lastPos)[chan]; FNit != (*digimap)[chan].end(); FNit++) {

      // update starting time
      const double newtime = std::get<2>(**FNit);
      if (newtime < digiTime - interval) {
        (*lastPos)[chan]++;
        continue;
      }

      // break if out of range
      if (newtime > digiTime + interval) break;

      // Skip already processed digis
      const CbmTrdDigi* d = (const CbmTrdDigi*) std::get<1>(**FNit);
      if (d == nullptr) continue;

      // add digi to cluster
      const int digiid = std::get<0>(**FNit);
      cluster->emplace_back(digiid, d);
      std::get<1>(**FNit) = nullptr;
      return true;
    }
    return false;
  }

  //_____________________________________________________________________
  void Clusterizer::addClusters(std::vector<std::pair<int, const CbmTrdDigi*>> cluster,
                                std::vector<Cluster>* clustersOut) const
  {
    // create vector for indice matching
    std::vector<int> digiIndices;
    digiIndices.reserve(cluster.size());

    // add digi ids to vector
    std::transform(cluster.begin(), cluster.end(), std::back_inserter(digiIndices),
                   [](const auto& pair) { return pair.first; });

    // create vector for pointers
    std::vector<const CbmTrdDigi*> digis;
    digis.reserve(cluster.size());

    // add digi ids to vector
    std::transform(cluster.begin(), cluster.end(), std::back_inserter(digis),
                   [](const auto& pair) { return pair.second; });

    // Count rows and columns (TO DO: needs to be re-computed / moved if row merging is implemented)
    std::unordered_map<int, bool> cols, rows;
    for (const auto& pair : cluster) {
      const int ncols = fParams.rowPar[0].padPar.size();
      int digiChannel = pair.second->GetAddressChannel();
      int colId       = digiChannel % ncols;
      int globalRow   = digiChannel / ncols;
      int combiId     = globalRow * ncols + colId;
      cols[combiId]   = true;
      rows[globalRow] = true;
    }

    // add the cluster to the Array
    clustersOut->emplace_back(digiIndices, digis, fParams.address, cols.size(), rows.size());
  }

}  // namespace cbm::algo::trd
