/* Copyright (C) 2010-2017 Frankfurt Institute for Advanced Studies, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel,  Sergey Gorbunov, Igor Kulakov [committer], Maksym Zyzak */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction 
 *  
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de 
 *
 *====================================================================
 *
 *  L1 Monte Carlo information
 *
 *====================================================================
 */

#ifndef CbmL1MCPoint_H
#define CbmL1MCPoint_H

#include "CaVector.h"

#include <iomanip>
#include <sstream>
#include <string>

namespace
{
  using cbm::algo::ca::Vector;
}

struct CbmL1MCPoint {

  CbmL1MCPoint() = default;

  static bool compareIDz(const CbmL1MCPoint& a, const CbmL1MCPoint& b)
  {
    return (a.ID < b.ID) || ((a.ID == b.ID) && (a.z < b.z));
  }

  static bool pcompareIDz(const CbmL1MCPoint* a, const CbmL1MCPoint* b)
  {
    return (a->ID < b->ID) || ((a->ID == b->ID) && (a->z < b->z));
  }

  double x      = 0.;
  double y      = 0.;
  double z      = 0.;
  double px     = 0.;
  double py     = 0.;
  double pz     = 0.;
  double xIn    = 0.;
  double yIn    = 0.;
  double zIn    = 0.;
  double pxIn   = 0.;
  double pyIn   = 0.;
  double pzIn   = 0.;
  double xOut   = 0.;
  double yOut   = 0.;
  double zOut   = 0.;
  double pxOut  = 0.;
  double pyOut  = 0.;
  double pzOut  = 0.;
  double p      = 0.;
  double q      = 0.;
  double mass   = 0.;
  double time   = 0.;
  int pdg       = 0;
  int ID        = 0;
  int mother_ID = 0;
  int iStation  = 0;
  int pointId   = -1;
  int file      = -1;
  int event     = -1;
  Vector<int> hitIds{"CbmL1MCPoint::hitIds"};  // indices of CbmL1Hits in L1->vStsHits array

  /// Temporary log function for debugging
  std::string ToString(int verbose = 3, bool printHeader = false) const
  {
    if (verbose < 1) {
      return std::string();
    }

    std::stringstream msg;
    msg.precision(4);
    if (printHeader) {
      if (verbose > 0) {
        msg << std::setw(10) << "track ID" << ' ';
        msg << std::setw(10) << "mother ID" << '|';
        msg << std::setw(10) << "station ID" << '|';
        msg << std::setw(10) << "PDG" << ' ';
        if (verbose > 3) {
          msg << std::setw(10) << "m [GeV/c2]" << ' ';
          msg << std::setw(10) << "q [e]" << '|';
        }
        msg << std::setw(14) << "t [ns]" << ' ';
        msg << std::setw(14) << "x [cm]" << ' ';
        msg << std::setw(14) << "y [cm]" << ' ';
        msg << std::setw(14) << "z [cm]" << '|';
        if (verbose > 1) {
          msg << std::setw(14) << "zIn [cm]" << ' ';
          msg << std::setw(14) << "zOut [cm]" << '|';
          msg << std::setw(14) << "p [GeV/c]" << '|';
          msg << std::setw(10) << "point ID" << ' ';
          if (verbose > 3) {
            msg << std::setw(10) << "event ID" << ' ';
            msg << std::setw(10) << "file ID" << ' ';
          }
          msg << std::setw(16) << "hit indices" << ' ';
        }
      }
    }
    else {
      if (verbose > 0) {
        msg << std::setw(10) << ID << ' ';
        msg << std::setw(10) << mother_ID << '|';
        msg << std::setw(10) << iStation << '|';
        msg << std::setw(10) << pdg << ' ';
        if (verbose > 3) {
          msg << std::setw(10) << mass << ' ';
          msg << std::setw(10) << q << '|';
        }
        msg << std::setw(14) << time << ' ';
        msg << std::setw(14) << x << ' ';
        msg << std::setw(14) << y << ' ';
        msg << std::setw(14) << z << '|';
        if (verbose > 1) {
          msg << std::setw(14) << zIn << ' ';
          msg << std::setw(14) << zOut << '|';
          msg << std::setw(14) << p << '|';
          msg << std::setw(10) << pointId << ' ';
          if (verbose > 3) {
            msg << std::setw(10) << event << ' ';
            msg << std::setw(10) << file << ' ';
          }
          std::stringstream msgHits;
          for (int iH : hitIds) {
            msgHits << iH << ' ';
          }
          msg << std::setw(16) << msgHits.str() << ' ';
        }
      }
    }
    return msg.str();
  }
};

#endif
