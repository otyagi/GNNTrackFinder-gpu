/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau, Felix Weiglhofer */
#pragma once

#include "ClusterizerPars.h"
#include "Definitions.h"
#include "Math/Rotation3D.h"
#include "Math/Vector3Dfwd.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/split_member.hpp>

#include <cstdint>
#include <vector>

namespace boost::serialization
{
  template<class Archive>
  void save(Archive& ar, const ROOT::Math::XYZVector& val, const unsigned int /*version*/)
  {
    ar << val.X();
    ar << val.Y();
    ar << val.Z();
  }

  template<class Archive>
  void load(Archive& ar, ROOT::Math::XYZVector& val, const unsigned int /*version*/)
  {
    double x, y, z;
    ar >> x >> y >> z;
    val.SetXYZ(x, y, z);
  }

  template<class Archive>
  void serialize(Archive& ar, ROOT::Math::XYZVector& val, const unsigned int version)
  {
    split_free(ar, val, version);
  }
}  // namespace boost::serialization


namespace cbm::algo::tof
{
  struct Hit {
    ROOT::Math::XYZVector hitPos    = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
    ROOT::Math::XYZVector hitPosErr = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
    double hitTime                  = 0.0;
    double hitTimeErr               = 0.0;
    int32_t address                 = 0;
    size_t numchan                  = 0;
    double weightsSum               = 0.0;

    // Interface for tracker

    double X() const { return hitPos.X(); }
    double Y() const { return hitPos.Y(); }
    double Z() const { return hitPos.Z(); }
    double Time() const { return hitTime; }
    double Dx() const { return hitPosErr.X(); }
    double Dy() const { return hitPosErr.Y(); }
    double TimeError() const { return hitTimeErr; }

    // Interface end

    int32_t numChan() const { return numchan; }

    void reset()
    {
      hitPos     = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
      hitPosErr  = ROOT::Math::XYZVector(0.0, 0.0, 0.0);
      hitTime    = 0.0;
      hitTimeErr = 0.0;
      weightsSum = 0.0;
      address    = 0;
      numchan    = 0;
    }

    void add(ROOT::Math::XYZVector pos, double time, double timeErr, double weight)
    {
      hitPos += pos * weight;
      hitTime += time * weight;
      hitTimeErr += timeErr * weight;
      weightsSum += weight;
      numchan++;
    }

    void normalize(double timeErr)
    {
      // a/=b is translated to a := a*(1/b) in the ROOT::Math::XYZVector class, which has a different
      // rounding behavior than a := (a/b). In rare cases this leads to 1.000... becoming 0.999... inside
      // the floor() operation in the finalize() function of this class, and results in a different
      // channel being associated with the cluster. To reproduce the output of the old hit finder, we
      // divide element-wise instead. Perhaps floor() should be replaced by round().
      //////// hitPos /= weightsSum;

      hitPos.SetXYZ(hitPos.X() / weightsSum, hitPos.Y() / weightsSum, hitPos.Z() / weightsSum);
      hitTime /= weightsSum;
      hitTimeErr = timeErr;
    }

    void finalize(const TofCell& trafoCell, const ClusterizerRpcPar& par)
    {
      //get hit channel
      const int32_t channel = par.fChanPar.size() / 2 + floor(hitPos.X() / trafoCell.sizeX);

      // D.Smith 15.8.23: This channel correction doesn't seem to be needed
      //  if (channel < 0) channel = 0;
      //  if (channel > par.fChanPar.size() - 1) channel = par.fChanPar.size() - 1;
      address = par.fChanPar[channel].address;

      //Calibrate hit time if applicable
      if (par.fCPTOffYRange != 0) {
        const double dBin = (hitPos.Y() + par.fCPTOffYRange) / par.fCPTOffYBinWidth;
        const int i1      = floor(dBin);
        const int i2      = ceil(dBin);
        hitTime -= par.fCPTOffY[i1] + (dBin - i1) * (par.fCPTOffY[i2] - par.fCPTOffY[i1]);
      }

      //get offset by rotation to master frame. get hit position by adding offset to cell coordinates
      hitPos = trafoCell.pos + trafoCell.rotation(hitPos);

      // Simple errors, not properly done at all for now
      // Right way of doing it should take into account the weight distribution
      // and real system time resolution
      hitPosErr = ROOT::Math::XYZVector(0.5, 0.5, 0.5);
    }

    template<class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar& hitPos;
      ar& hitPosErr;
      ar& hitTime;
      ar& hitTimeErr;
      ar& address;
      ar& numchan;
      ar& weightsSum;
    }
  };

}  // namespace cbm::algo::tof
