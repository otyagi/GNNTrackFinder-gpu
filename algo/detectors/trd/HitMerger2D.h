/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#pragma once

#include "CbmTrdDigi.h"
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

using std::vector;
class DigiRec;

namespace cbm::algo::trd
{

  /** @class HitMerger2D
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

  class HitMerger2D {
   public:
    typedef std::pair<Hit, std::vector<DigiRec>> inputType;
    typedef std::pair<std::vector<inputType>, std::vector<inputType>> outputType;

    /** \brief Default constructor.*/
    HitMerger2D(){};

    /** \brief Constructor with placement */
    HitMerger2D(HitFinder2DModPar params);

    virtual ~HitMerger2D(){};

    /** \brief Steering routine for building hits **/
    outputType operator()(std::vector<inputType>& hitsRow1, std::vector<inputType>& hitsRow2);

    std::pair<int, HitFactory2D> ProjectDigis(std::vector<DigiRec>* cid, std::vector<DigiRec>* cjd);
    /** \brief Implement topologic cuts for hit merging
   * \return index of anode wire wrt to boundary or 0 if check fails
   */
    int CheckMerge(std::vector<DigiRec>* cid, std::vector<DigiRec>* cjd);
    /** \brief Algorithm for hit merging
   * \param[in] h hit to be modified by addition of hp.
   * \param[in] a0 anode hypothesis around boundary (see CheckMerge function).
   * \return TRUE if succesful.
   */
    bool MergeHits(Hit* h, int a0, HitFactory2D& hitF);


    /** \brief Time offset to synchronize TRD2D hits to the rest of detectors
   * \param dt offset in [ns]
   */
    void SetHitTimeOffset(int dt) { fHitTimeOff = dt; }

   private:
    HitMerger2D(const HitMerger2D& ref);
    const HitMerger2D& operator=(const HitMerger2D& ref);

    Hit MakeHit(int cId);

    const float fClk = CbmTrdDigi::Clk(CbmTrdDigi::eCbmTrdAsicType::kFASP);

    HitFinder2DModPar fParams;  ///< Parameter container

    void CalibrateHit(Hit* h, const double dx, const double dy, const double edx, const double edy, const double edt,
                      const double time, const double tdrift, const double eloss, const HitFactory2D& hitF);

    /** \brief Addressing ASIC on module based on id
   * \param[in] id module wise ASIC identifier
   * \return ASIC address within experiment
   */
    inline int GetPadRowCol(int address, int& c);

    int fHitTimeOff = 0;  //! hit time offset for synchronization
  };


}  // namespace cbm::algo::trd
