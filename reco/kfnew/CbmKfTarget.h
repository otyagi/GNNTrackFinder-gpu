/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmKfTarget.h
/// \brief  Target property initialization and access in CBM (header)
/// \since  02.09.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

// TODO: Move this class somewhere in the cbmroot/core

#pragma once

#include "KfDefs.h"

#include <mutex>

class TString;
class TGeoNode;

namespace cbm::kf
{
  /// \class Target
  /// \brief CBM target accessor and property handler
  class Target {
   public:
    /// \brief Gets target center x-coordinate [cm]
    double GetX() const { return fX; }

    /// \brief Gets target center y-coordinate [cm]
    double GetY() const { return fY; }

    /// \brief Gets target center z-coordinate [cm]
    double GetZ() const { return fZ; }

    /// \brief Gets target half thickness [cm]
    double GetDz() const { return fDz; }

    /// \brief Gets target transverse size [cm]
    double GetRmax() const { return fRmax; }

    /// \brief Instance access
    static Target* Instance();

    // Copy and move elimination
    Target(const Target&) = delete;
    Target(Target&&)      = delete;
    Target& operator=(const Target&) = delete;
    Target& operator=(Target&&) = delete;

   protected:
    /// \brief Default constructor
    Target() = default;


    /// \brief Destructor
    ~Target() = default;

    /// \brief Target initializer
    void Init();

    /// \brief Finds a target
    static void FindTargetNode(TString& targetPath, TGeoNode*& targetNode);

    double fX{cbm::algo::kf::defs::Undef<double>};     ///< reference x-coordinate of the target position [cm]
    double fY{cbm::algo::kf::defs::Undef<double>};     ///< reference y-coordinate of the target position [cm]
    double fZ{cbm::algo::kf::defs::Undef<double>};     ///< reference z-coordinate of the target position [cm]
    double fDz{cbm::algo::kf::defs::Undef<double>};    ///< target half-thickness [cm]
    double fRmax{cbm::algo::kf::defs::Undef<double>};  ///< target transverse size [cm]

   private:
    static Target* fpInstance;
    static std::mutex fMutex;
  };
}  // namespace cbm::kf
