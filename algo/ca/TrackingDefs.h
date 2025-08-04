/* Copyright (C) 2023-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TrackingDefs.h
/// \date   22.10.2023
/// \brief  Definitions for tracking in the online reconstruction
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include "CaEnumArray.h"
#include "CbmDefs.h"
#include "MicrosliceDescriptor.hpp"  // For fles::Subsystem

#include <tuple>

namespace cbm::algo
{
  namespace mvd
  {
    class Hit;
  }
  namespace sts
  {
    struct Hit;
  }
  namespace much
  {
    class Hit;
  }
  namespace trd
  {
    class Hit;
  }
  namespace tof
  {
    struct Hit;
  }

  namespace ca
  {
    template<fles::Subsystem subsys>
    constexpr EDetectorID FromFlesSubsystem()
    {
      if constexpr (subsys == fles::Subsystem::STS) {
        return EDetectorID::kSts;
      }
      else if constexpr (subsys == fles::Subsystem::MVD) {
        return EDetectorID::kMvd;
      }
      else if constexpr (subsys == fles::Subsystem::MUCH) {
        return EDetectorID::kMuch;
      }
      else if constexpr (subsys == fles::Subsystem::TRD) {
        return EDetectorID::kTrd;
      }
      if constexpr (subsys == fles::Subsystem::TOF) {
        return EDetectorID::kTof;
      }
      else {
        return EDetectorID::END;
      }
    }

    template<EDetectorID detID>
    constexpr fles::Subsystem ToFlesSubsystem()
    {
      if constexpr (detID == EDetectorID::kMvd) {
        return fles::Subsystem::MVD;
      }
      else if constexpr (detID == EDetectorID::kSts) {
        return fles::Subsystem::STS;
      }
      else if constexpr (detID == EDetectorID::kMuch) {
        return fles::Subsystem::MUCH;
      }
      else if constexpr (detID == EDetectorID::kTrd) {
        return fles::Subsystem::TRD;
      }
      else if constexpr (detID == EDetectorID::kTof) {
        return fles::Subsystem::TOF;
      }
      else if constexpr (detID == EDetectorID::END) {
        return fles::Subsystem::FLES;  // Default ()
      }
    }

    /// \brief  Alias to array, indexed by the EDetectorID enum
    /// \note   To be used only in CBM-specific code
    template<typename T>
    using DetIdArray_t = EnumArray<EDetectorID, T>;

    /// \struct DetIdTypeArr_t
    /// \brief  Array of types, indexed by EDetectorID
    template<class... Types>
    struct DetIdTypeArr_t {
      template<EDetectorID DetID>
      using at                          = std::tuple_element_t<static_cast<std::size_t>(DetID), std::tuple<Types...>>;
      static constexpr std::size_t size = sizeof...(Types);
    };

    /// \brief Hit vector types
    using MvdHit     = ::cbm::algo::mvd::Hit;
    using StsHit     = ::cbm::algo::sts::Hit;
    using MuchHit    = ::cbm::algo::much::Hit;
    using TrdHit     = ::cbm::algo::trd::Hit;
    using TofHit     = ::cbm::algo::tof::Hit;
    using HitTypes_t = DetIdTypeArr_t<MvdHit, StsHit, MuchHit, TrdHit, TofHit>;

    /// \brief Detector subsystem names
    constexpr DetIdArray_t<const char*> kDetName = {{"MVD", "STS", "MUCH", "TRD", "TOF"}};

  }  // namespace ca
}  // namespace cbm::algo
