/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Sergei Zharko */

/// \file   MaterialMonitor.cxx
/// \brief  Implementation of the MaterialMonitor class
/// \author Sergey Gorbunov

#include "KfMaterialMonitor.h"

#include "AlgoFairloggerCompat.h"

#include <iomanip>
#include <sstream>

namespace cbm::algo::kf
{
  namespace
  {
    using namespace cbm::algo;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void MaterialMonitor::SetMaterial(const MaterialMap* materialMap)
  {
    fMaterial = materialMap;

    fActiveBinMap.resize(0);

    if (fMaterial) {
      fActiveBinMap.assign(fMaterial->GetNbins() * fMaterial->GetNbins(), 0);
    }

    // TODO: What do the hits mean here?
    fNhitsTotal   = 0;
    fNhitsOutside = 0;

    EvaluateStatistics();
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void MaterialMonitor::MarkActiveBin(float x, float y)
  {
    /// mark a bin as active
    if (!fMaterial) {
      LOG(fatal) << "MaterialMonitor: material map is not set";
      return;
    }
    int i = fMaterial->GetBin(x, y);
    fNhitsTotal++;
    if (i < 0) {
      fNhitsOutside++;
    }
    else {
      fActiveBinMap[i] = 1;
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void MaterialMonitor::EvaluateStatistics()
  {
    /// update values of statistical variables with respect to the active map

    fActiveNbins  = 0;
    fPassiveNbins = 0;

    fActiveRadThickMin  = defs::Undef<double>;
    fActiveRadThickMax  = defs::Undef<double>;
    fActiveRadThickMean = defs::Undef<double>;

    fPassiveRadThickMin  = defs::Undef<double>;
    fPassiveRadThickMax  = defs::Undef<double>;
    fPassiveRadThickMean = defs::Undef<double>;

    if (!fMaterial) {
      return;
    }

    int nBins = fMaterial->GetNbins() * fMaterial->GetNbins();

    if (nBins != (int) fActiveBinMap.size()) {
      LOG(fatal) << "MaterialMonitor: map of active bins is not consistent with the material map: nbins "
                 << fActiveBinMap.size() << " != " << nBins;
      return;
    }

    for (int i = 0; i < nBins; i++) {
      double r = fMaterial->GetBinThicknessX0<double>(i);
      if (fActiveBinMap[i]) {  // active material
        if (fActiveNbins == 0) {
          fActiveRadThickMin  = r;
          fActiveRadThickMax  = r;
          fActiveRadThickMean = r;
        }
        else {
          fActiveRadThickMin = std::min(fActiveRadThickMin, r);
          fActiveRadThickMax = std::max(fActiveRadThickMax, r);
          fActiveRadThickMean += r;
        }
        fActiveNbins++;
      }
      else {
        // passive material
        if (fPassiveNbins == 0) {
          fPassiveRadThickMin  = r;
          fPassiveRadThickMax  = r;
          fPassiveRadThickMean = r;
        }
        else {
          fPassiveRadThickMin = std::min(fPassiveRadThickMin, r);
          fPassiveRadThickMax = std::max(fPassiveRadThickMax, r);
          fPassiveRadThickMean += r;
        }
        fPassiveNbins++;
      }
    }
    if (fActiveNbins + fPassiveNbins != nBins) {
      LOG(fatal) << "MaterialMonitor: wrong calculation of N passive / active bins ";
    }
    if (fActiveNbins > 0) {
      fActiveRadThickMean /= fActiveNbins;
    }
    if (fPassiveNbins > 0) {
      fPassiveRadThickMean /= fPassiveNbins;
    }
  }


  // -------------------------------------------------------------------------------------------------------------------
  //
  std::string MaterialMonitor::ToString()
  {
    /// print statistics to a string

    EvaluateStatistics();
    // FIXME: a ToString method should not change data of its class, but only represent their current state. I would
    //        call EvaluateStatistics explicitly and provide a const ToString method inside it.

    std::stringstream ss;
    ss << std::setprecision(2) << std::fixed;
    ss << "material map " << fName << ". ";

    if (fActiveNbins > 0) {
      ss << "Active material RL: min " << fActiveRadThickMin * 100. << "%, max " << fActiveRadThickMax * 100.
         << "%, mean " << fActiveRadThickMean * 100. << "%. ";
    }
    else {
      if (fNhitsTotal > 0) {
        ss << "No active material. ";
      }
      else {
        ss << "No hits yet to identify active areas. ";
      }
    }

    if (fPassiveNbins > 0) {
      ss << "Passive material RL: min " << fPassiveRadThickMin * 100. << "%, max " << fPassiveRadThickMax * 100.
         << "%, mean " << fPassiveRadThickMean * 100. << "%. ";
    }
    else {
      ss << "No passive material. ";
    }

    if (fNhitsTotal > 0) {
      if (fNhitsOutside == 0) {
        ss << "No hits outside of the map. ";
      }
      else {
        ss << "There are " << (100. * fNhitsOutside) / fNhitsTotal << "% hits outside the map!!! ";
      }
    }
    else {
      ss << "No hit statistics yet. ";
    }

    return ss.str();
  }

}  // namespace cbm::algo::kf
