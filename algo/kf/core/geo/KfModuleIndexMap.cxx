/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfModuleIndexMap.cxx
/// \brief  A helper class to map external indices with the ones of KF-setup (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  16.09.2024

#include "KfModuleIndexMap.h"

namespace cbm::algo::kf
{
  // -------------------------------------------------------------------------------------------------------------------
  //
  void ModuleIndexMap::Reset()
  {
    fvLocToGlb.clear();
    fvGlbToLoc.clear();
    fvDetLocOffset.clear();
    fvDetIntToExt.clear();
    fvDetExtToInt.clear();
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  std::string ModuleIndexMap::ToString(int indentLevel) const
  {
    using fmt::format;
    constexpr char IndentCh = '\t';
    std::string indent(indentLevel, IndentCh);
    std::stringstream msg;
    msg << indent << format("{:>8} {:>8} {:>8}\n", "det.ID", "loc.ID", "glob.ID");
    for (int iGlb = 0; iGlb < GetNofLayers(); ++iGlb) {
      const auto& [iDetExt, iLoc] = fvGlbToLoc[iGlb];
      msg << indent << format("{:>8} {:>8} {:>8}\n", iDetExt, iLoc, iGlb);
    }

    return msg.str();
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  ModuleIndexMap ModuleIndexMapFactory::MakeIndexMap() const
  {
    ModuleIndexMap indexMap;

    auto& vDetIntToExt  = indexMap.fvDetIntToExt;
    auto& vDetExtToInt  = indexMap.fvDetExtToInt;
    auto& vDetLocOffset = indexMap.fvDetLocOffset;
    auto& vLocToGlb     = indexMap.fvLocToGlb;
    auto& vGlbToLoc     = indexMap.fvGlbToLoc;

    // ------- Component layout
    // Initialization of internal and external detector subsystem mapping
    for (const auto& component : fvComponentLayers) {
      auto iDetExt = component.fDetId;
      if (iDetExt < 0) {
        std::stringstream msg;
        msg << "ModuleIndexMapFactory::MakeIndexMap: illegal detector subsystem enumeration: " << iDetExt
            << " is out of range [0, " << defs::MaxNofDetSubsystems << "). Enties of the EDetID enumeration are "
            << "required to be non-negative";
        throw std::out_of_range(msg.str());
      }
      if (iDetExt >= static_cast<int>(vDetExtToInt.size())) {
        vDetExtToInt.resize(iDetExt + 1, -1);
      }

      if (vDetExtToInt[iDetExt] == -1) {  // Detector subsystem is not registered
        // Assign an internal index to the detID:
        vDetExtToInt[iDetExt] = static_cast<int>(vDetIntToExt.size());
        vDetIntToExt.push_back(iDetExt);
      }
    }
    {  // Estimate offsets
      int nDet = vDetIntToExt.size();
      vDetLocOffset.resize(nDet + 1, 0);
      for (const auto& component : fvComponentLayers) {
        int pick            = vDetExtToInt[component.fDetId] + 1;
        vDetLocOffset[pick] = std::max(vDetLocOffset[pick], component.fLocId + 1);
      }
      for (int iDet{0}; iDet < nDet; ++iDet) {
        vDetLocOffset[iDet + 1] += vDetLocOffset[iDet];
      }
    }

    vLocToGlb.resize(vDetLocOffset.back(), -1);
    vGlbToLoc.resize(fvComponentLayers.size());
    for (auto itComp = fvComponentLayers.cbegin(); itComp != fvComponentLayers.cend(); ++itComp) {
      const auto& component{*itComp};
      int iDetInt                              = vDetExtToInt[component.fDetId];
      int iLoc                                 = component.fLocId;
      int iGlb                                 = std::distance(fvComponentLayers.begin(), itComp);
      vLocToGlb[vDetLocOffset[iDetInt] + iLoc] = iGlb;
      vGlbToLoc[iGlb]                          = std::pair(component.fDetId, iLoc);
    }

    return indexMap;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void ModuleIndexMapFactory::Reset() { fvComponentLayers.clear(); }
}  // namespace cbm::algo::kf
