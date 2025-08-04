/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_BASE_TYPES_H
#define CBM_BASE_TYPES_H

#include "MicrosliceDescriptor.hpp"  // For fles::Subsystem
#include "util/EnumDict.h"

#include <cstdint>

namespace cbm::algo
{

  // typealias for Rust-like fixed size integer types
  using i8  = std::int8_t;
  using u8  = std::uint8_t;
  using i16 = std::int16_t;
  using u16 = std::uint16_t;
  using i32 = std::int32_t;
  using u32 = std::uint32_t;
  using i64 = std::int64_t;
  using u64 = std::uint64_t;
  using f32 = float;
  using f64 = double;

#ifdef CBM_ALGO_REAL64
  using real = f64;
#else
  using real = f32;
#endif

  enum class Step
  {
    Unpack,
    DigiTrigger,
    LocalReco,
    Tracking,
  };

  enum class RecoData
  {
    DigiTimeslice,  //< Raw output from unpackers
    DigiEvent,      //< Digis after event building
    Cluster,
    Hit,
    Track,
  };

  enum class Setup
  {
    mCBM2022,
    mCBM2024_03,
    mCBM2024_05,
    mCBM2025_02,
  };

  enum class QaStep
  {
    BeamBmon,
    UnpackBmon,
    UnpackSts,
    UnpackMvd,
    UnpackRich,
    UnpackTrd1d,
    UnpackTrd2d,
    UnpackMuch,
    UnpackTof,
    UnpackFsd,
    EventBuilding,
    RecoBmon,
    RecoSts,
    RecoMvd,
    RecoRich,
    RecoTrd1d,
    RecoTrd2d,
    RecoMuch,
    RecoTof,
    RecoFsd,
    Tracking,
    V0Finder,
    V0Trigger,
  };

}  // namespace cbm::algo

CBM_ENUM_DICT(fles::Subsystem,
  // CBM detectors
  {"STS", fles::Subsystem::STS},
  {"MVD", fles::Subsystem::MVD},
  {"RICH", fles::Subsystem::RICH},
  {"TRD", fles::Subsystem::TRD},
  {"TRD2D", fles::Subsystem::TRD2D},
  {"MUCH", fles::Subsystem::MUCH},
  {"TOF", fles::Subsystem::TOF},

  // Other detectors (experimental)
  {"ECAL", fles::Subsystem::ECAL},
  {"PSD", fles::Subsystem::PSD},
  {"BMON", fles::Subsystem::BMON},
  {"TRB3", fles::Subsystem::TRB3},
  {"Hodoscope", fles::Subsystem::Hodoscope},
  {"Cherenkov", fles::Subsystem::Cherenkov},
  {"LeadGlass", fles::Subsystem::LeadGlass},

  // FLES (pattern generators)
  {"FLES", fles::Subsystem::FLES},
);

CBM_ENUM_DICT(cbm::algo::Step,
  {"Unpack", Step::Unpack},
  {"DigiTrigger", Step::DigiTrigger},
  {"LocalReco", Step::LocalReco},
  {"Tracking", Step::Tracking}
);

CBM_ENUM_DICT(cbm::algo::RecoData,
  {"DigiTimeslice", RecoData::DigiTimeslice},
  {"DigiEvent", RecoData::DigiEvent},
  {"Cluster", RecoData::Cluster},
  {"Hit", RecoData::Hit},
  {"Track", RecoData::Track}
);

CBM_ENUM_DICT(cbm::algo::Setup,
  {"mCBM2022", cbm::algo::Setup::mCBM2022},
  {"mCBM2024_03", cbm::algo::Setup::mCBM2024_03},
  {"mCBM2024_05", cbm::algo::Setup::mCBM2024_05},
  {"mCBM2025_02", cbm::algo::Setup::mCBM2025_02}
);

CBM_ENUM_DICT(cbm::algo::QaStep,
  {"BeamBmon", cbm::algo::QaStep::BeamBmon},
  {"UnpackBmon", cbm::algo::QaStep::UnpackBmon},
  {"UnpackSts", cbm::algo::QaStep::UnpackSts},
  {"UnpackMvd", cbm::algo::QaStep::UnpackMvd},
  {"UnpackRich", cbm::algo::QaStep::UnpackRich},
  {"UnpackTrd1d", cbm::algo::QaStep::UnpackTrd1d},
  {"UnpackTrd2d", cbm::algo::QaStep::UnpackTrd2d},
  {"UnpackMuch", cbm::algo::QaStep::UnpackMuch},
  {"UnpackTof", cbm::algo::QaStep::UnpackTof},
  {"UnpackFsd", cbm::algo::QaStep::UnpackFsd},
  {"EventBuilding", cbm::algo::QaStep::EventBuilding},
  {"RecoBmon", cbm::algo::QaStep::RecoBmon},
  {"RecoSts", cbm::algo::QaStep::RecoSts},
  {"RecoMvd", cbm::algo::QaStep::RecoMvd},
  {"RecoRich", cbm::algo::QaStep::RecoRich},
  {"RecoTrd1d", cbm::algo::QaStep::RecoTrd1d},
  {"RecoTrd2d", cbm::algo::QaStep::RecoTrd2d},
  {"RecoMuch", cbm::algo::QaStep::RecoMuch},
  {"RecoTof", cbm::algo::QaStep::RecoTof},
  {"RecoFsd", cbm::algo::QaStep::RecoFsd},
  {"Tracking", cbm::algo::QaStep::Tracking},
  {"V0Finder", cbm::algo::QaStep::V0Finder},
  {"V0Trigger", cbm::algo::QaStep::V0Trigger}
);

#endif
