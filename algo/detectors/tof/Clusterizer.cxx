/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau */

#include "Clusterizer.h"

// TOF Classes and includes
#include "CbmTofDigi.h"

// C++ Classes and includes
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>

namespace cbm::algo::tof
{

  Clusterizer::resultType Clusterizer::operator()(const std::vector<std::pair<CbmTofDigi, int32_t>>& digisIn)
  {
    std::vector<inputType> input = chanSortDigis(digisIn);
    return buildClusters(input);
  }

  std::vector<Clusterizer::inputType>
  Clusterizer::chanSortDigis(const std::vector<std::pair<CbmTofDigi, int32_t>>& digisIn)
  {
    std::vector<inputType> result(fParams.fChanPar.size());  //[nbCh][nDigis]

    // Bucket-sort by channel
    for (size_t iDigi = 0; iDigi < digisIn.size(); iDigi++) {
      const CbmTofDigi* pDigi = &digisIn[iDigi].first;
      const int32_t index     = digisIn[iDigi].second;
      const double chan       = pDigi->GetChannel();
      result[chan].emplace_back(pDigi, index);
    }

    /// Sort channel-wise by time    // not needed if digis are pre-sorted
    //for (size_t chan = 0; chan < fParams.fChanPar.size(); chan++) {
    //  std::sort(
    //    result[chan].begin(), result[chan].end(),
    //    [](const std::pair<const CbmTofDigi*, int32_t>& a, const std::pair<const CbmTofDigi*, int32_t>& b) -> bool {
    //      return a.first->GetTime() < b.first->GetTime();
    //    });
    //}
    return result;
  }

  //Iterator-based version. Faster than index-based version.
  Clusterizer::resultType Clusterizer::buildClusters(std::vector<inputType>& input)
  {
    // Hit variables
    Hit cluster;

    // Output variables
    resultType result;
    auto& [clustersOut, chanSizes, chanAddresses, digiIndRef] = result;

    // Reference cell of a cluster
    TofCell* cell = nullptr;

    // Last Channel Temp variables
    int32_t lastChan = -1;
    double lastPosY  = 0.0;
    double lastTime  = 0.0;

    const size_t numChan = fParams.fChanPar.size();

    //Store last position in input channels to avoid unnecessary checks in AddNextChan().
    std::vector<inputType::iterator> lastChanPos;
    for (size_t chan = 0; chan < numChan; chan++) {
      lastChanPos.push_back(input[chan].begin());
    }

    for (int32_t chan = 0; (size_t) chan < numChan; chan++) {

      // Set partition vectors
      chanSizes.push_back(clustersOut.size());
      chanAddresses.push_back(fParams.fChanPar[chan].address);

      // skip over dead channels
      if (fParams.fDeadStrips & (1 << chan)) {
        chanSizes.back() = 0;
        continue;
      }
      inputType& storDigi = input[chan];
      auto digiIt         = storDigi.begin();

      while (1 < std::distance(digiIt, storDigi.end())) {
        while (digiIt->first->GetSide() == std::next(digiIt)->first->GetSide()) {  // Not one Digi of each end!
          digiIt++;
          if (2 > std::distance(digiIt, storDigi.end())) {
            break;
          }
        }
        if (2 > std::distance(digiIt, storDigi.end())) {
          break;
        }

        // 2 Digis = both sides present
        cell                     = &fParams.fChanPar[chan].cell;
        const CbmTofDigi* xDigiA = digiIt->first;
        const CbmTofDigi* xDigiB = std::next(digiIt)->first;

        // use local coordinates, (0,0,0) is in the center of counter  ?
        ROOT::Math::XYZVector pos(((-(double) numChan / 2. + (double) chan) + 0.5) * cell->sizeX, 0., 0.);

        const double timeDif = xDigiA->GetTime() - xDigiB->GetTime();

        pos.SetY(fParams.fSigVel * timeDif * 0.5);  // A is the top side, B is the bottom side
        if (xDigiA->GetSide() != 1.) {
          pos.SetY(-pos.Y());
        }  // B is the bottom side, A is the top side

        while (std::distance(digiIt, storDigi.end()) > 2 && std::abs(pos.Y()) > cell->sizeY * fParams.fPosYMaxScal) {

          const CbmTofDigi* xDigiC = std::next(digiIt, 2)->first;

          const double timeDifN = (xDigiC->GetSide() == xDigiA->GetSide()) ? xDigiC->GetTime() - xDigiB->GetTime()
                                                                           : xDigiC->GetTime() - xDigiA->GetTime();

          double posYN = fParams.fSigVel * timeDifN * 0.5;
          if (xDigiC->GetSide() != 1.) {
            posYN *= -1.;
          }

          if (std::abs(posYN) >= std::abs(pos.Y())) {
            break;
          }
          pos.SetY(posYN);

          if (xDigiC->GetSide() == xDigiA->GetSide()) {
            xDigiA = xDigiC;
          }
          else {
            xDigiB = xDigiC;
          }
          digiIt++;
        }

        if (std::abs(pos.Y()) > cell->sizeY * fParams.fPosYMaxScal) {  // remove both digis
          digiIt++;
          continue;
        }
        // The "Strip" time is the mean time between each end
        const double time = 0.5 * (xDigiA->GetTime() + xDigiB->GetTime());

        // Weight is the total charge => sum of both ends ToT
        const double totSum = xDigiA->GetTot() + xDigiB->GetTot();

        // Now check if a hit/cluster is already started
        if (0 < cluster.numChan()) {
          // a cluster is already started => check distance in space/time
          // For simplicity, just check along strip direction for now
          // and break cluster when a not fired strip is found
          if (!(std::abs(time - lastTime) < fParams.fdMaxTimeDist && lastChan == chan - 1
                && std::abs(pos.Y() - lastPosY) < fParams.fdMaxSpaceDist)) {

            cluster.normalize(fParams.fTimeRes);
            cluster.finalize(*cell, fParams);
            clustersOut.push_back(cluster);
            cluster.reset();
          }
        }
        cluster.add(pos, time, totSum, totSum);
        digiIndRef.push_back(digiIt->second);
        digiIndRef.push_back(std::next(digiIt)->second);
        digiIt += 2;

        lastChan = chan;
        lastPosY = pos.Y();
        lastTime = time;
        AddNextChan(input, lastChan, cluster, clustersOut, digiIndRef, &lastChanPos);
      }  // while( 1 < storDigi.size() )

      // Apply subtraction such that chanSize constains hit count per channel
      chanSizes.back() = clustersOut.size() - chanSizes.back();

      // In rare cases, a single digi remains and is deleted here.
      storDigi.clear();
    }  // for( int32_t chan = 0; chan < iNbCh; chan++ )

    // Now check if another hit/cluster is started
    // and save it if it's the case.
    if (0 < cluster.numChan()) {
      cluster.normalize(fParams.fTimeRes);
      cluster.finalize(*cell, fParams);
      clustersOut.push_back(cluster);
      chanSizes.back()++;
    }
    return result;
  }

  bool Clusterizer::AddNextChan(std::vector<inputType>& input, int32_t lastChan, Hit& cluster,
                                std::vector<Hit>& clustersOut, std::vector<int32_t>& digiIndRef,
                                std::vector<inputType::iterator>* lastChanPos)
  {
    //D.Smith 25.8.23: Why are "C" digis (position "2") not considered here?

    //const int32_t iDetId   = CbmTofAddress::GetUniqueAddress(iSm, iRpc, 0, 0, iSmType); //Kept for reference
    size_t numChan = fParams.fChanPar.size();
    int32_t chan   = lastChan + 1;

    while (fParams.fDeadStrips & (1 << chan)) {
      chan++;
      if ((size_t) chan >= numChan) {
        return false;
      }
    }
    if ((size_t) chan == numChan) {
      return false;
    }

    inputType& storDigi = input[chan];
    if (0 == storDigi.size()) {
      return false;
    }

    const double clusterTime = cluster.hitTime / cluster.weightsSum;
    const TofCell& cell      = fParams.fChanPar[chan].cell;

    //Get starting position in buffer
    auto i1 = (lastChanPos == nullptr) ? storDigi.begin() : (*lastChanPos)[chan];

    //Find first digi in buffer that can possibly be part of the cluster.
    //Relies on time-order to function properly.
    i1 = std::lower_bound(i1, storDigi.end(), clusterTime - fParams.fdMaxTimeDist,
                          [this, cell](const auto& obj, double val) {
                            return obj.first->GetTime() + cell.sizeY * fParams.fPosYMaxScal / fParams.fSigVel < val;
                          });

    //Store last position in input channels to avoid unnecessary checks.
    //Relies on time-order to function properly. Can only be used in first-level AddNextChan()
    //calls, as time-order is not guaranteed in nested calls.
    if (lastChanPos) {
      (*lastChanPos)[chan] = i1;
    }

    for (; i1 < storDigi.end() - 1; i1++) {

      const CbmTofDigi* xDigiA = i1->first;
      if (xDigiA->GetTime() > clusterTime + fParams.fdMaxTimeDist) {
        break;
      }
      auto i2 = i1;

      while (++i2 < storDigi.end()) {

        const CbmTofDigi* xDigiB = i2->first;
        if (xDigiA->GetSide() == xDigiB->GetSide()) {
          continue;
        }

        const double time = 0.5 * (xDigiA->GetTime() + xDigiB->GetTime());

        //Continue if digis are in the past of cluster time
        if (time <= clusterTime - fParams.fdMaxTimeDist) {
          continue;
        }

        //Break if digis are in the future of cluster time
        if (time >= clusterTime + fParams.fdMaxTimeDist) {
          break;
        }

        const double timeDif = xDigiA->GetTime() - xDigiB->GetTime();
        double posY          = fParams.fSigVel * timeDif * 0.5;

        //Break if position is outside of the detector
        if (std::abs(posY) > cell.sizeY * fParams.fPosYMaxScal) {
          break;
        }

        if (1 != xDigiA->GetSide()) {
          posY *= -1.;
        }

        if (std::abs(posY - cluster.hitPos.Y() / cluster.weightsSum) >= fParams.fdMaxSpaceDist) {
          continue;
        }

        // append digi pair to current cluster
        const double posX   = ((-(double) numChan / 2. + chan) + 0.5) * cell.sizeX;
        const double totSum = xDigiA->GetTot() + xDigiB->GetTot();

        ROOT::Math::XYZVector pos(posX, posY, 0.);
        cluster.add(pos, time, totSum, totSum);
        digiIndRef.push_back(i1->second);
        digiIndRef.push_back(i2->second);

        // remove digis at positions i1 and i2 from pool in efficient way (replaces two vector::erase calls).
        std::move(i1 + 1, i2, i1);
        std::move(i2 + 1, storDigi.end(), i2 - 1);
        storDigi.resize(storDigi.size() - 2);

        if (AddNextChan(input, chan, cluster, clustersOut, digiIndRef)) {
          return true;  // signal hit was already added
        }
        i1 = storDigi.end();  // jump to end of outer loop

        break;
      }
    }
    TofCell detcell = fParams.fChanPar[0].cell;  //D.Smith 17.8.23: This is equivalent to using iDetId, see below
                                                 //D.Smith 10.8.23: Why pass iDetId here and not iChId?
    cluster.normalize(fParams.fTimeRes);
    cluster.finalize(detcell, fParams);
    clustersOut.push_back(cluster);
    cluster.reset();
    return true;
  }

}  // namespace cbm::algo::tof
