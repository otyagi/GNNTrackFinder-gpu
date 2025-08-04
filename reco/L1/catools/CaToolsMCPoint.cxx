/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CaToolsMCPoint.cxx
/// \brief  Internal class describing a MC point for CA tracking QA and performance (implementation)
/// \date   18.11.2022
/// \author S.Zharko <s.zharko@gsi.de>


#include "CaToolsMCPoint.h"

#include <iomanip>
#include <sstream>

using cbm::ca::tools::MCPoint;

// ---------------------------------------------------------------------------------------------------------------------
//
std::string MCPoint::ToString(int verbose, bool printHeader) const
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
        msg << std::setw(10) << "point ID (ext)" << ' ';
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
      msg << std::setw(10) << fTrackId << ' ';
      msg << std::setw(10) << fMotherId << '|';
      msg << std::setw(10) << fStationId << '|';
      msg << std::setw(10) << fPdgCode << ' ';
      if (verbose > 3) {
        msg << std::setw(10) << fMass << ' ';
        msg << std::setw(10) << fCharge << '|';
      }
      msg << std::setw(14) << fTime << ' ';
      msg << std::setw(14) << fPos[0] << ' ';
      msg << std::setw(14) << fPos[1] << ' ';
      msg << std::setw(14) << fPos[2] << '|';
      if (verbose > 1) {
        msg << std::setw(14) << fPosIn[2] << ' ';
        msg << std::setw(14) << fPosOut[2] << '|';
        msg << std::setw(14) << this->GetP() << '|';
        msg << std::setw(10) << fId << ' ';
        msg << std::setw(10) << fLinkKey.fIndex << ' ';
        if (verbose > 3) {
          msg << std::setw(10) << fLinkKey.fEvent << ' ';
          msg << std::setw(10) << fLinkKey.fFile << ' ';
        }
        std::stringstream msgHits;
        for (int iH : fvHitIndexes) {
          msgHits << iH << ' ';
        }
        msg << std::setw(16) << msgHits.str() << ' ';
      }
    }
  }
  return msg.str();
}
