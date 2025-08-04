/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#pragma once

#include "CbmTrdDigi.h"
#include "Cluster2D.h"
#include "DigiRec.h"
#include "Hit.h"
#include "HitFactory2D.h"
#include "HitFinder2DPars.h"
#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

#include <cstdint>
#include <tuple>
#include <unordered_map>
#include <vector>

#define NANODE 9

using std::vector;

namespace cbm::algo::trd
{

  /** @class HitFinder2D
 ** @brief Cluster finding and hit reconstruction algorithms for the TRD(2D) module.
 ** @author Alexandru Bercucic <abercuci@niham.nipne.ro>
 ** @since 01.02.2019
 ** @date 01.10.2021
 **
 ** Extend the TRD module reconstructor for the particular case of inner TRD-2D modules.
 ** The class is a collection of algorithms to :
 ** - identify time and spatially correlated digis and build clusters
 ** - identify the type of clusters and apply further merging/deconvolution
 ** - apply FEE (channel gain, baseline) and detector (energy gain, maps, etc) calibration
 ** - steer the calculation of hit 4D parameters (x, y, t, E)
 **/

  class HitFinder2D {
   public:
    typedef std::pair<Hit, std::vector<DigiRec>> resultType;

    /** \brief Default constructor.*/
    HitFinder2D(){};

    /** \brief Constructor with placement */
    HitFinder2D(HitFinder2DModPar params);

    virtual ~HitFinder2D(){};

    /** \brief Steering routine for building hits **/
    std::vector<resultType> operator()(std::vector<Cluster2D>* clusters);

    std::pair<int, HitFactory2D> ProjectDigis(const Cluster2D* cluster);
    /** \brief Implement topologic cuts for hit merging
   * \return index of anode wire wrt to boundary or 0 if check fails
   */
    bool BuildHit(Hit* h, HitFactory2D& hitF);

    /** \brief Time offset to synchronize TRD2D hits to the rest of detectors
   * \param dt offset in [ns]
   */
    void SetHitTimeOffset(int dt) { fHitTimeOff = dt; }

   private:
    HitFinder2D(const HitFinder2D& ref);
    const HitFinder2D& operator=(const HitFinder2D& ref);

    Hit MakeHit(int cId, const Cluster2D* cluster);

    const float fClk = CbmTrdDigi::Clk(CbmTrdDigi::eCbmTrdAsicType::kFASP);

    HitFinder2DModPar fParams;  ///< Parameter container

    /** \brief Addressing ASIC on module based on id
   * \param[in] id module wise ASIC identifier
   * \return ASIC address within experiment
   */
    inline int GetPadRowCol(int address, int& c);

    void CalibrateHit(Hit* h, const double dx, const double dy, const double edx, const double edy, const double edt,
                      const double time, const double tdrift, const double eloss, const HitFactory2D& hitF);

    int fHitTimeOff = 0;  //! hit time offset for synchronization

    static Double_t fgDT[3];  //! FASP delay wrt signal
  };


}  // namespace cbm::algo::trd
