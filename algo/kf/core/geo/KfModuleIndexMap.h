/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfModuleIndexMap.h
/// \brief  A helper class to map external indices with the ones of KF-setup
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  10.09.2024

#pragma once

#include "KfDefs.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/utility.hpp>

#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace cbm::algo::kf
{

  /// \class ModuleIndexMap
  /// \brief Maps local detector and station indices to the material maps and field slices
  ///
  /// The class maps external indices of components combined into subsets to a plain global index.
  /// The indices both of the components inside a subset (iLoc) and the indices of the
  /// subsets themselves (iDetExt) must be unique, but can be unsorted and can have holes in their
  /// ranges. The global index (iGlob) is assigned automatically depending on the element ordering.
  ///
  /// The mapping is organized with the sequential contiguous containers (std::vector). The containers
  /// used are:
  /// (i)   fvDetExtToInt [ iDetExt ]  =>  iDetInt
  ///         size:  max(iDetExt)
  /// (ii)  fvDetIntToExt [ iDetInt ]  =>  iDetExt (int)
  ///         size:  number of registered subsystems
  /// (iii) fvDetLocOffset [ iDetInt ]  =>  max number of elements for iDetInt - 1
  ///         size:  max possible number of elements (including inactive/unused)
  ///         fvDetLocOffset[0] = 0, fvDetLocOffset.back() = n of max possible elements
  /// (iv)  fvGlbToLoc [ iGlob ] => pair(iDetInt, iLoc)
  ///         size:  number of registered components
  /// (v)   fvLocToGlb [ fvDetLocOffset[iDetInt] + iLoc] => iGlob
  ///         size:  max possible number of components (including inactive/unused)
  ///
  class alignas(VcMemAlign) ModuleIndexMap {
    friend class ModuleIndexMapFactory;
    friend class boost::serialization::access;

   public:
    /// \brief Gets total number of components
    int GetNofLayers() const { return fvGlbToLoc.size(); }

    /// \brief Converts internal layer index to pair (detID, locID)
    /// \tparam  EDetID  concrete index type of the det ID (can be either an enum, or an integral type)
    /// \param globId  Internal layer index
    /// \return  pair(detector ID, local ID) of the layer
    template<class EDetID = int>
    std::pair<EDetID, int> GlobalToLocal(int globId) const;

    /// \brief Converts external pair (detID, locID) to internal layer index
    /// \tparam  EDetID  concrete index type of the det ID (can be either an enum, or an integral type)
    /// \param   locId   Local ID of the layer
    /// \param   detId   Detector ID of the component
    /// \note   if the (detId, locId) pair was not registered, returns -1
    /// \return global index of the layer
    template<class EDetID>
    int LocalToGlobal(EDetID detId, int locId) const
    {
      return fvLocToGlb[fvDetLocOffset[fvDetExtToInt[static_cast<int>(detId)]] + locId];
    }

    /// \brief Disables a component
    /// \tparam  EDetID  concrete index type of the det ID (can be either an enum, or an integral type)
    /// \param   locId   Local ID of the layer
    /// \param   detId   Detector ID of the component
    template<class EDetID>
    void Disable(EDetID detId, int locId);

    /// \brief Resets the instance
    void Reset();

    /// \brief String representation of the instance
    std::string ToString(int indentLevel = 0) const;

   private:
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fvLocToGlb;
      ar& fvGlbToLoc;
      ar& fvDetLocOffset;
      ar& fvDetIntToExt;
      ar& fvDetExtToInt;
    }

    std::vector<int> fvLocToGlb{};
    std::vector<std::pair<int, int>> fvGlbToLoc{};
    std::vector<int> fvDetLocOffset{};  ///< First index of component for det
    std::vector<int> fvDetIntToExt{};   ///< Maps external detID to internal
    std::vector<int> fvDetExtToInt{};   ///< Maps internal detID to external
  };

  /// \class  ModuleIndexMapFactory
  /// \brief  Creates a valid module mapper
  class ModuleIndexMapFactory {
    /// \struct Component
    /// \brief  Structure to keep information on layers
    struct Component {
      Component(int detId, int locId, double z) : fZ(z), fLocId(locId), fDetId(detId) {}
      double fZ;   ///< Reference z-coordinate of the component
      int fLocId;  ///< Local index of component
      int fDetId;  ///< External index of detector subsystem
      bool operator<(const Component& rhs) const { return fZ < rhs.fZ; }
    };

   public:
    /// \brief Adds component info
    /// \param detId  Detector subsytem index (external)
    /// \param locId  Detector component local index (external)
    /// \param z      Reference z-component of the component [cm]
    template<class EDetID>
    void AddComponent(EDetID detId, int locId, double z);

    /// \brief Creates a module index map
    ModuleIndexMap MakeIndexMap() const;

    /// \brief Resets the instance
    void Reset();

   private:
    std::set<Component> fvComponentLayers;
  };

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<class EDetID>
  void ModuleIndexMap::Disable(EDetID detIdDisable, int locIdDisable)
  {
    // Check, if the detector id is there
    auto iDetIntDsbl{fvDetExtToInt[static_cast<int>(detIdDisable)]};
    if (iDetIntDsbl >= static_cast<int>(fvDetIntToExt.size())) {
      return;  // Nothing to disable, detector is already not in the map
    }

    auto& iGlbDsbl = fvLocToGlb[fvDetLocOffset[iDetIntDsbl] + locIdDisable];
    if (iGlbDsbl < 0) {
      return;  // Nothing to disable, component is already inactive
    }

    fvGlbToLoc.erase(fvGlbToLoc.begin() + iGlbDsbl);  // Removing component from glob->(det, loc) map
    for (auto& val : fvLocToGlb) {
      if (val > iGlbDsbl) {
        --val;
      }
    }
    iGlbDsbl = -1;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<class EDetID>
  inline std::pair<EDetID, int> ModuleIndexMap::GlobalToLocal(int globId) const
  {
    const auto& [iDetExt, iLoc] = fvGlbToLoc[globId];
    return std::pair(static_cast<EDetID>(iDetExt), iLoc);
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<class EDetID>
  void ModuleIndexMapFactory::AddComponent(EDetID detId, int locId, double z)
  {
    if (!fvComponentLayers.emplace(static_cast<int>(detId), locId, z).second) {
      std::stringstream msg;
      msg << "ModuleIndexMapFactory: attempt of adding a duplicating component with z = " << z
          << ", detID = " << static_cast<int>(detId) << " and locId = " << locId
          << ".\n The next components were already added:";
      for (const auto& c : fvComponentLayers) {
        msg << "\n\t- {z, detId, locId} = {" << c.fZ << ", " << c.fDetId << ", " << c.fLocId << "}";
      }
      throw std::logic_error(msg.str());
    }
  }


}  // namespace cbm::algo::kf
