/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingSetup.h
/// \date   19.04.2024
/// \brief  A detector setup interface used for tracking input data initialization (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "SubChain.h"
#include "sts/TrackingInterface.h"
#include "tof/TrackingInterface.h"
#include "trd/TrackingInterface.h"

#include <type_traits>

// TODO: SZh 19.04.2024: Provide interfaces for other subsystems and redefine access

namespace cbm::algo
{
  /// \class TrackingSetup
  /// \brief A detector setup interface class for tracking input data initialization
  class TrackingSetup : public SubChain {
   public:
    /// \brief Default constructor
    TrackingSetup() = default;

    /// \brief Copy constructor
    TrackingSetup(const TrackingSetup&) = delete;

    /// \brief Move constructor
    TrackingSetup(TrackingSetup&&) = delete;

    /// \brief Destructor
    ~TrackingSetup() = default;

    /// \brief Initializer function
    void Init();

    /// \brief   Returns tracking station index by the detector element address
    /// \param   address  Unique address of an element
    /// \return  Local index of tracking station
    template<fles::Subsystem DetID>
    int GetTrackingStation(uint32_t address) const
    {
      if constexpr (DetID == fles::Subsystem::STS) {
        return fSts.GetTrackingStation(address);
      }
      else if constexpr (DetID == fles::Subsystem::TRD) {
        return fTrd.GetTrackingStation(address);
      }
      else if constexpr (DetID == fles::Subsystem::TOF) {
        return fTof.GetTrackingStation(address);
      }
      return -1;  // Default: no station is assigned, hit will be skept !
    }

    /// \brief Set detector subsystem usage
    /// \param det Detector ID (fles::Subsystem)
    /// \param flag  (not) use the subsystem
    void Use(fles::Subsystem det, bool flag = true)
    {
      using fles::Subsystem;
      switch (det) {
        case Subsystem::STS: fbUseSts = flag; break;
        case Subsystem::MVD: break;
        case Subsystem::MUCH: break;
        case Subsystem::TRD: fbUseTrd = flag; break;
        case Subsystem::TOF: fbUseTof = flag; break;
        default: break;
      }
    }

   private:
    sts::TrackingInterface fSts;  ///< STS tracking interface
    trd::TrackingInterface fTrd;  ///< TRD tracking interface
    tof::TrackingInterface fTof;  ///< TOF tracking interface
    bool fbUseSts = false;
    bool fbUseTrd = false;
    bool fbUseTof = false;
  };
}  // namespace cbm::algo
