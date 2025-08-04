/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_NICA_CBM_HELPERS_CBMDETECTORID_H_
#define CBMROOT_NICA_CBM_HELPERS_CBMDETECTORID_H_

#include <Hal/DataFormat.h>
namespace HalCbm
{
  namespace DetectorID
  {
    const UInt_t kTOF  = Hal::DetectorID::kTOF;
    const UInt_t kSTS  = Hal::DetectorID::kSTS;
    const UInt_t kMVD  = 29641;
    const UInt_t kRICH = 1283489;
    const UInt_t kTRD  = 38569;
    const UInt_t kECAL = 669117;
    const UInt_t kPSD  = 33421;
    const UInt_t kMUCH = 1065761;
  };  // namespace DetectorID
  enum class GeoSetup
  {
    kSis100Hadron   = 0,
    kSis100Electron = 1,
    kSis100Muon     = 2,
    kSis100Mini     = 3
  };
  enum class DataFormat
  {
    kDST          = 0,
    kAnalysisTree = 1,
    kUnknown      = 2
  };
}  // namespace HalCbm


#endif /* CBMROOT_NICA_CBM_HELPERS_CBMDETECTORID_H_ */
