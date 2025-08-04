/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau */

/*
   This algo was based on CbmTofSimpClusterizer, which can be used only for simulation of the main setup and
   is as the name implies a simplified solution.
   A later step will be the replacement with a version based on CbmTofEventClusterizer, which is the version
   currently maintained, based on what we learned from real data at mCBM.
   This step will be required to apply the algo to real/online data and to prepare
   our simulations for first CBM beam
*/

#ifndef CBM_ALGO_TOF_HITFINDER_H
#define CBM_ALGO_TOF_HITFINDER_H

// TOF Classes and includes
class CbmTofDigi;

// ROOT Classes and includes
#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

// C++ Classes and includes
#include <cmath>
#include <memory>
#include <vector>

namespace cbm::algo::tof
{
  struct Cell {
    double sizeX, sizeY;
    ROOT::Math::XYZVector pos;
    ROOT::Math::Rotation3D rotation;
  };

  struct HitFinderChanPar {
    std::vector<double> fvCPTOff;               //[nbSide]
    std::vector<double> fvCPTotGain;            //[nbSide]
    std::vector<std::vector<double>> fvCPWalk;  //[nbSide][nbWalkBins]
    int32_t address;                            //unique address
    Cell cell;
  };

  struct HitFinderRpcPar {
    double FeeTimeRes;
    double SysTimeRes;
    double CPSigPropSpeed;
    double outTimeFactor;
    double TOTMax;
    double TOTMin;
    double maxTimeDist;
    double maxSpaceDist;
    double gapSize;
    int32_t numGaps;
    int32_t numClWalkBinX;
    std::vector<HitFinderChanPar> fChanPar = {};
  };

  struct Cluster {
    //temporary values
    std::vector<int32_t> vDigiIndRef;
    ROOT::Math::XYZVector weightedPos = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
    double weightedTime               = 0.0;
    double weightedTimeErr            = 0.0;
    double weightsSum                 = 0.0;

    //after finalization
    ROOT::Math::XYZVector globalPos = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
    ROOT::Math::XYZVector globalErr = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
    int32_t channel                 = 0;
    int32_t detId                   = 0;

    int32_t numChan() { return vDigiIndRef.size() / 2; }

    void reset()
    {
      vDigiIndRef.clear();
      weightedPos     = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
      globalPos       = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
      globalErr       = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
      weightedTime    = 0.0;
      weightedTimeErr = 0.0;
      weightsSum      = 0.0;
      channel         = 0;
      detId           = 0;
    }

    void add(ROOT::Math::XYZVector pos, double time, double timeErr, double weight, int32_t digiIndA, int32_t digiIndB)
    {
      vDigiIndRef.push_back(digiIndA);
      vDigiIndRef.push_back(digiIndB);
      weightedPos += pos * weight;
      weightedTime += time * weight;
      weightedTimeErr += timeErr * weight;
      weightsSum += weight;
    }

    void normalize(double timeErr)
    {
      // a/=b is translated to a := a*(1/b) in the ROOT::Math::XYZVector class, which has a different
      // rounding behavior than a := (a/b). In rare cases this leads to 1.000... becoming 0.999... inside
      // the floor() operation in the finalize() function of this class, and results in a different
      // channel being associated with the cluster. To reproduce the output of the old hit finder, we
      // divide element-wise instead. Perhaps floor() should be replaced by round().
      //////// weightedPos /= weightsSum;

      weightedPos.SetXYZ(weightedPos.X() / weightsSum, weightedPos.Y() / weightsSum, weightedPos.Z() / weightsSum);
      weightedTime /= weightsSum;
      weightedTimeErr = timeErr;
    }

    void finalize(const Cell& trafoCell, const int32_t iTrafoCell, const HitFinderRpcPar& par)
    {
      // prepare local->global trafo
      ROOT::Math::Rotation3D rotMatrix = trafoCell.rotation;

      // get offset from weighted cluster position by rotation to master frame
      ROOT::Math::XYZVector hitposOffset = rotMatrix(weightedPos);

      //get hit position by adding offset to cell coordinates
      globalPos = trafoCell.pos + hitposOffset;

      // Simple errors, not properly done at all for now
      // Right way of doing it should take into account the weight distribution
      // and real system time resolution
      ROOT::Math::XYZVector hitErrLocal;
      hitErrLocal.SetX(trafoCell.sizeX / std::sqrt(12.0));    // Single strips approximation
      hitErrLocal.SetY(par.FeeTimeRes * par.CPSigPropSpeed);  // Use the electronics resolution
      hitErrLocal.SetZ(par.numGaps * par.gapSize / 10.0       // Change gap size in cm
                       / std::sqrt(12.0));                    // Use full RPC thickness as "Channel" Z size

      //store results
      globalErr = rotMatrix(hitErrLocal);
      channel   = iTrafoCell + floor(weightedPos.X() / trafoCell.sizeX);
      detId     = par.fChanPar[channel].address;
      weightedTime *= par.outTimeFactor;
    }
  };

  class HitFinder {
   public:
    typedef std::vector<Cluster> resultType;
    typedef std::pair<std::vector<std::vector<CbmTofDigi*>>, std::vector<std::vector<int32_t>>> inputType;

    /**
       ** @brief Constructor.
       **/
    HitFinder(){};

    /**
       ** @brief Destructor.
       **/
    ~HitFinder(){};

    /**
       ** @brief Build clusters out of ToF Digis and store the resulting info in a TofHit.
       **/
    resultType operator()(std::vector<CbmTofDigi> digisIn, const std::vector<int32_t>& digiIndexIn);

    /** @brief Set the parameter container
     ** @param params Vectorer to parameter container
     **/
    void SetParams(std::unique_ptr<HitFinderRpcPar> params) { fParams = *(std::move(params)); }

   private:
    HitFinderRpcPar fParams = {};  ///< Parameter container

    inputType calibrateDigis(std::vector<CbmTofDigi>& digisIn, const std::vector<int32_t>& digiIndexIn);
    resultType buildClusters(inputType& input);

    int32_t numSameSide;  // Digis quality
  };

}  // namespace cbm::algo::tof

#endif  // CBM_ALGO_TOF_HITFINDER_H
