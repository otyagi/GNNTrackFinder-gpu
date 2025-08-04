/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#ifndef CBM_ALGO_DATA_STS_HIT_H
#define CBM_ALGO_DATA_STS_HIT_H

#include "Definitions.h"

#include <boost/serialization/access.hpp>

namespace cbm::algo::sts
{

  struct Hit {
    real fX, fY;      ///< X, Y positions of hit [cm]
    real fZ;          ///< Z position of hit [cm]
    u32 fTime;        ///< Hit time [ns]
    real fDx, fDy;    ///< X, Y errors [cm]
    real fDxy;        ///< XY correlation
    real fTimeError;  ///< Error of hit time [ns]
    real fDu;         ///< Error of coordinate across front-side strips [cm]
    real fDv;         ///< Error of coordinate across back-side strips [cm]

    u32 fFrontClusterId;  ///< Index of front-side cluster, used by tracking to reduce combinatorics
    u32 fBackClusterId;   ///< Index of back-side cluster, used by tracking to reduce combinatorics

    // Interface for tracking
    double X() const { return fX; }
    double Y() const { return fY; }
    double Z() const { return fZ; }
    double Time() const { return fTime; }
    double Dx() const { return fDx; }
    double Dy() const { return fDy; }
    double TimeError() const { return fTimeError; }

   private:  // serialization
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar& fX;
      ar& fY;
      ar& fZ;
      ar& fTime;
      ar& fDx;
      ar& fDy;
      ar& fDxy;
      ar& fTimeError;
      ar& fDu;
      ar& fDv;

      ar& fFrontClusterId;
      ar& fBackClusterId;
    }
  };

}  // namespace cbm::algo::sts

#endif  // CBM_ALGO_DATA_STS_HIT_H
