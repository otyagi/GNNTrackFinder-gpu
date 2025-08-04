/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Etienne Bechtel, Florian Uhlig */

#include "HitFinder.h"

#include "AlgoFairloggerCompat.h"
#include "CbmTrdDigi.h"

namespace cbm::algo::trd
{
  constexpr double HitFinder::kxVar_Value[2][5];
  constexpr double HitFinder::kyVar_Value[2][5];

  //_______________________________________________________________________________
  HitFinder::HitFinder(HitFinderModPar params) : fParams(params) {}

  //_______________________________________________________________________________
  std::vector<trd::HitFinder::resultType> HitFinder::operator()(std::vector<Cluster>* clusters)
  {
    const int nclusters = clusters->size();
    std::vector<resultType> hitData;

    for (int icluster = 0; icluster < nclusters; icluster++) {
      Cluster* cluster = &clusters->at(icluster);
      auto hit         = MakeHit(icluster, cluster, &cluster->GetDigis(), nclusters);
      if (hit.Address() == -1) continue;
      hitData.emplace_back(hit, std::vector<DigiRec>());  //DigiRec currently not used in 1D but in 2D
    }
    return hitData;
  }

  //_______________________________________________________________________________
  Hit HitFinder::MakeHit(int clusterId, const Cluster* cluster, const std::vector<const CbmTrdDigi*>* digis, size_t)
  {
    ROOT::Math::XYZVector hit_posV(0.0, 0.0, 0.0);

    double xVar        = 0;
    double yVar        = 0;
    double totalCharge = 0;
    double time        = 0.;
    int errorclass     = 0.;
    bool EB            = false;
    bool EBP           = false;

    size_t skipped = 0;
    for (auto& digi : *digis) {
      if (!digi) {
        skipped++;
        continue;
        L_(debug) << " no digi " << std::endl;
      }

      const double digiCharge = digi->GetCharge();

      // D.Smith 15.4.24: These get overwritten for each digi. Should be OR?
      errorclass = digi->GetErrorClass();
      EB         = digi->IsFlagged(0);
      EBP        = digi->IsFlagged(1);

      if (digiCharge <= 0.05) {
        skipped++;
        continue;
      }

      time += digi->GetTime();
      totalCharge += digiCharge;

      int digiCol;
      int digiRow = GetPadRowCol(digi->GetAddressChannel(), digiCol);

      const ROOT::Math::XYZVector& local_pad_posV  = fParams.rowPar[digiRow].padPar[digiCol].pos;
      const ROOT::Math::XYZVector& local_pad_dposV = fParams.rowPar[digiRow].padPar[digiCol].pos;

      double xMin = local_pad_posV.X() - local_pad_dposV.X();
      double xMax = local_pad_posV.X() + local_pad_dposV.X();
      xVar += (xMax * xMax + xMax * xMin + xMin * xMin) * digiCharge;

      double yMin = local_pad_posV.Y() - local_pad_dposV.Y();
      double yMax = local_pad_posV.Y() + local_pad_dposV.Y();
      yVar += (yMax * yMax + yMax * yMin + yMin * yMin) * digiCharge;

      hit_posV.SetX(hit_posV.X() + local_pad_posV.X() * digiCharge);
      hit_posV.SetY(hit_posV.Y() + local_pad_posV.Y() * digiCharge);
      hit_posV.SetZ(hit_posV.Z() + local_pad_posV.Z() * digiCharge);
    }
    time /= digis->size() - skipped;

    if (totalCharge <= 0) return Hit();

    hit_posV.SetX(hit_posV.X() / totalCharge);
    hit_posV.SetY(hit_posV.Y() / totalCharge);
    hit_posV.SetZ(hit_posV.Z() / totalCharge);

    if (EB) {
      xVar = kxVar_Value[0][errorclass];
      yVar = kyVar_Value[0][errorclass];
    }
    else {
      if (EBP) time -= 46;  //due to the event time of 0 in the EB mode and the ULong in the the digi time
      //TODO: move to parameter file
      xVar = kxVar_Value[1][errorclass];
      yVar = kyVar_Value[1][errorclass];
    }

    ROOT::Math::XYZVector global = fParams.translation + fParams.rotation(hit_posV);

    // TO DO: REMOVE MAGIC NUMBERS!
    if (!EB) {  // preliminary correction for angle dependence in the position
                // reconsutrction
      global.SetX(global.X() + (0.00214788 + global.X() * 0.000195394));
      global.SetY(global.Y() + (0.00370566 + global.Y() * 0.000213235));
    }

    ROOT::Math::XYZVector cluster_pad_dposV(xVar, yVar, 0);
    TransformHitError(cluster_pad_dposV);  //Gets overwritten below?

    // TODO: get momentum for more exact spacial error
    if ((fParams.orientation == 1) || (fParams.orientation == 3)) {
      cluster_pad_dposV.SetX(sqrt(fParams.padSizeErrY));  // Original version uses padSizeErrY here for some reason
      //cluster_pad_dposV.SetX( sqrt(fParams.padSizeErrX ) );  // x-component correct?
    }
    else {
      cluster_pad_dposV.SetY(sqrt(fParams.padSizeErrY));
    }

    // Set charge of incomplete clusters (missing NTs) to -1 (not deleting them because they are still relevant for tracking)
    if (!IsClusterComplete(cluster)) totalCharge = -1.0;

    return Hit(fParams.address, global, cluster_pad_dposV, 0, clusterId, totalCharge / 1e6, time,
               double(8.5));  // TODO: move to parameter file
  }

  void HitFinder::TransformHitError(ROOT::Math::XYZVector& hitErr) const
  {
    double x, y;
    x = hitErr.X();
    y = hitErr.Y();

    if ((fParams.orientation == 1) || (fParams.orientation == 3)) {  // for orientations 1 or 3
      hitErr.SetX(y);                                                // swap errors
      hitErr.SetY(x);                                                // swap errors
    }
  }


  double HitFinder::GetSpaceResolution(double val)
  {

    std::pair<double, double> res[12] = {
      std::make_pair(0.5, 0.4),  std::make_pair(1, 0.35),   std::make_pair(2, 0.3),    std::make_pair(2.5, 0.3),
      std::make_pair(3.5, 0.28), std::make_pair(4.5, 0.26), std::make_pair(5.5, 0.26), std::make_pair(6.5, 0.26),
      std::make_pair(7.5, 0.26), std::make_pair(8.5, 0.26), std::make_pair(8.5, 0.26), std::make_pair(9.5, 0.26)};

    double selval = 0.;

    for (int n = 0; n < 12; n++) {
      if (val < res[0].first) selval = res[0].second;
      if (n == 11) {
        selval = res[11].second;
        break;
      }
      if (val >= res[n].first && val <= res[n + 1].first) {
        double dx    = res[n + 1].first - res[n].first;
        double dy    = res[n + 1].second - res[n].second;
        double slope = dy / dx;
        selval       = (val - res[n].first) * slope + res[n].second;
        break;
      }
    }

    return selval;
  }

  bool HitFinder::IsClusterComplete(const Cluster* cluster)
  {
    int colMin = fParams.rowPar[0].padPar.size();  //numCols
    int rowMin = fParams.rowPar.size();            //numRows
    int colMax = 0;
    int rowMax = 0;

    for (auto& digi : cluster->GetDigis()) {
      int digiCol;
      int digiRow = GetPadRowCol(digi->GetAddressChannel(), digiCol);

      if (digiCol < colMin) colMin = digiCol;
      if (digiRow < rowMin) rowMin = digiRow;
      if (digiCol > colMax) colMax = digiCol;
      if (digiRow > rowMax) rowMax = digiRow;
    }

    const uint16_t nCols = colMax - colMin + 1;
    const uint16_t nRows = rowMax - rowMin + 1;

    CbmTrdDigi* digiMap[nRows][nCols];                        //create array on stack for optimal performance
    memset(digiMap, 0, sizeof(CbmTrdDigi*) * nCols * nRows);  //init with nullpointers

    for (auto& digi : cluster->GetDigis()) {
      int digiCol;
      int digiRow = GetPadRowCol(digi->GetAddressChannel(), digiCol);

      if (digiMap[digiRow - rowMin][digiCol - colMin])
        return false;  // To be investigated why this sometimes happens (Redmin Issue 2914)

      digiMap[digiRow - rowMin][digiCol - colMin] = const_cast<CbmTrdDigi*>(digi);
    }

    // check if each row of the cluster starts and ends with a kNeighbor digi
    for (int iRow = 0; iRow < nRows; ++iRow) {
      int colStart = 0;
      while (digiMap[iRow][colStart] == nullptr)
        ++colStart;
      if (digiMap[iRow][colStart]->GetTriggerType() != static_cast<int>(CbmTrdDigi::eTriggerType::kNeighbor))
        return false;

      int colStop = nCols - 1;
      while (digiMap[iRow][colStop] == nullptr)
        --colStop;
      if (digiMap[iRow][colStop]->GetTriggerType() != static_cast<int>(CbmTrdDigi::eTriggerType::kNeighbor))
        return false;
    }

    return true;
  }

  int HitFinder::GetPadRowCol(int address, int& c)
  {
    c = address % fParams.rowPar[0].padPar.size();
    return address / fParams.rowPar[0].padPar.size();
  }

}  // namespace cbm::algo::trd
