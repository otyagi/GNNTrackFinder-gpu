/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Etienne Bechtel, Florian Uhlig */

#pragma once

#include "Cluster.h"
#include "DigiRec.h"
#include "Hit.h"
#include "HitFinderPars.h"
#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

#include <tuple>
#include <vector>

class CbmTrdDigi;

namespace cbm::algo::trd
{
  /**
  * \brief Rectangular pad module; Cluster finding and hit reconstruction algorithms
  **/
  class HitFinder {
   public:
    typedef std::pair<Hit, std::vector<DigiRec>> resultType;

    HitFinder(){};
    /**
  * \brief Constructor with placement
  **/
    HitFinder(HitFinderModPar params);
    virtual ~HitFinder(){};

    /* \brief Steering routine for building hits */
    std::vector<resultType> operator()(std::vector<Cluster>* clusters);

    double GetSpaceResolution(double val = 3.0);
    bool IsClusterComplete(const Cluster* cluster);


   protected:
   private:
    typedef std::tuple<int, const CbmTrdDigi*, double>
      inputType;  //digi index, pointer to digi (null if processed), digi time

    HitFinder(const HitFinder& ref);
    const HitFinder& operator=(const HitFinder& ref);

    Hit MakeHit(int cId, const Cluster* c, const std::vector<const CbmTrdDigi*>* digis, size_t);

    /** \brief Addressing ASIC on module based on id
   * \param[in] id module wise ASIC identifier
   * \return ASIC address within experiment
   */
    inline int GetPadRowCol(int address, int& c);

    HitFinderModPar fParams;  ///< Parameter container

    void TransformHitError(ROOT::Math::XYZVector& hitErr) const;

    // different error classes for the position resolution based on the simulation results
    // the error classes are defined for the different module types
    // TODO: move to parameter file
    static constexpr double kxVar_Value[2][5] = {{0.0258725, 0.0267693, 0.0344325, 0.0260322, 0.040115},
                                                 {0.0426313, 0.0426206, 0.0636962, 0.038981, 0.0723851}};
    static constexpr double kyVar_Value[2][5] = {{0.024549, 0.025957, 0.0250713, 0.0302682, 0.0291146},
                                                 {0.0401438, 0.0407502, 0.0397242, 0.0519485, 0.0504586}};
  };

}  // namespace cbm::algo::trd
