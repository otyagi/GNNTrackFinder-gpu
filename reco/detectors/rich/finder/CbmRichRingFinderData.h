/* Copyright (C) 2013-2021 UGiessen/JINR-LIT, Giessen/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer] */

#ifndef CBM_RICH_RING_FINDER_DATA_H_
#define CBM_RICH_RING_FINDER_DATA_H_

#include "CbmRichRingLight.h"

#include <functional>

/**
* \class CbmRichHoughHit
*
* \brief Implementation of RICH hit for ring finder algorithm.
*
* \author Semen Lebedev
* \date 2008
**/
class CbmRichHoughHit {
 public:
  /**
    * \brief Standard constructor.
    */
  CbmRichHoughHit() : fHit(), fX2plusY2(0.f), fTime(0.), fIsUsed(false) {}

  virtual ~CbmRichHoughHit() {}

  CbmRichHitLight fHit;
  float fX2plusY2;
  double fTime;
  bool fIsUsed;
};

#endif /* CBM_RICH_RING_FINDER_DATA_H_ */
