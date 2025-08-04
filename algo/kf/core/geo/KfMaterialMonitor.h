/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer], Sergei Zharko */

/// \file   KfMaterialMonitor.h
/// \brief  A class to collect statistics for kf::MaterialMap
/// \author Sergey Gorbunov

#pragma once

#include "KfDefs.h"
#include "KfMaterialMap.h"

#include <algorithm>
#include <string>
#include <vector>

namespace cbm::algo::kf
{
  /// \class MaterialMonitor
  /// \brief A class to collect statistics for a material budget map of the KF-framework
  ///
  class MaterialMonitor {
   public:
    /// \brief Default constructor
    MaterialMonitor() : MaterialMonitor(nullptr) {}

    /// \brief Constructor
    /// \param  materialMap  Material map to be monitored
    /// \param  name         Name of the material map
    MaterialMonitor(const MaterialMap* materialMap, const std::string& name = "") : fName(name)
    {
      SetMaterial(materialMap);
    }

    /// \brief  Set a material budget map
    /// \param  materialMap  Material budget map to be monitored
    void SetMaterial(const MaterialMap* materialMap);

    /// \brief  Set a name of the material budget map
    /// \param  name Name of the material map
    void SetName(const std::string& name) { fName = name; }

    /// \brief Reset the map of active bins
    void ResetActiveBins() { std::fill(fActiveBinMap.begin(), fActiveBinMap.end(), 0); }

    /// \brief Mark a bin as active
    /// \param x  X-coordinate of the bin
    /// \param y  Y-coordinate of the bin
    void MarkActiveBin(float x, float y);

    /// \brief Update values of statistical variables with respect to the active map
    void EvaluateStatistics();

    /// \brief Print statistics to a string
    std::string ToString();

    /// \brief Get number of active bins in the map
    int GetActiveNbins() const { return fActiveNbins; }

    /// \brief Get minimal radiation thickness among all active bins
    double GetActiveRadThickMin() const { return fPassiveRadThickMin; }

    /// \brief Get maximal radiation thickness among all active bins
    double GetActiveRadThickMax() const { return fPassiveRadThickMax; }

    /// \brief Get average radiation thickness among all active bins
    double GetActiveRadThickMean() const { return fPassiveRadThickMean; }

    /// \brief Get number of passive bins in the map
    int GetPassiveNbins() const { return fPassiveNbins; }

    /// \brief Get minimal radiation thickness among all passive bins
    double GetPassiveRadThickMin() const { return fPassiveRadThickMin; }

    /// \brief Get maximal radiation thickness among all passive bins
    double GetPassiveRadThickMax() const { return fPassiveRadThickMax; }

    /// \brief Get average radiation thickness among all passive bins
    double GetPassiveRadThickMean() const { return fPassiveRadThickMean; }

    /// \brief Get the ration of hits that show up outside the material map
    double GetRatioOfOutsideHits() const { return fNhitsOutside / (fNhitsTotal + 1.e-8); }

    /// \brief Get the number of processed hits
    double GetNhits() const { return fNhitsTotal; }

   private:
    std::string fName{};                ///< Name of the material map
    std::vector<char> fActiveBinMap{};  ///< Map of active bins in the material map (bins where hits appear)

    const MaterialMap* fMaterial{nullptr};  ///< Pointer to the material map

    double fActiveRadThickMin{defs::Undef<double>};   ///< Active material: minimal thickness
    double fActiveRadThickMax{defs::Undef<double>};   ///< Active material: maximal thickness
    double fActiveRadThickMean{defs::Undef<double>};  ///< Active material: average thickness

    double fPassiveRadThickMin{defs::Undef<double>};   ///< Passive material: minimal thickness
    double fPassiveRadThickMax{defs::Undef<double>};   ///< Passive material: maximal thickness
    double fPassiveRadThickMean{defs::Undef<double>};  ///< Passive material: average thickness

    unsigned long fNhitsTotal{0};    ///< number of hits in statistics
    unsigned long fNhitsOutside{0};  ///< number of hits outside the material map

    int fActiveNbins{defs::Undef<int>};   ///< Active material: number of bins
    int fPassiveNbins{defs::Undef<int>};  ///< Passive material: number of bins
  };

}  // namespace cbm::algo::kf
