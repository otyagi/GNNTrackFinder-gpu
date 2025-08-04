/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

#ifndef CbmKfUtil_H
#define CbmKfUtil_H 1

#include "CbmDefs.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdTrackingInterface.h"
#include "KfTrackParam.h"
#include "Rtypes.h"

class FairTrackParam;

///
/// Collection of useful utilites for CbmKf
///
namespace cbm::kf
{

  /// copy fair track param to Ca track param
  cbm::algo::kf::TrackParamD ConvertTrackParam(const FairTrackParam& par);

  /// copy Ca track param to fair track param
  FairTrackParam ConvertTrackParam(const cbm::algo::kf::TrackParamD& t);

  inline const CbmTrackingDetectorInterfaceBase* GetTrackingInterface(const cbm::algo::ca::EDetectorID caDetId)
  {
    switch (caDetId) {
      case cbm::algo::ca::EDetectorID::kMvd: return CbmMvdTrackingInterface::Instance();
      case cbm::algo::ca::EDetectorID::kSts: return CbmStsTrackingInterface::Instance();
      case cbm::algo::ca::EDetectorID::kMuch: return CbmMuchTrackingInterface::Instance();
      case cbm::algo::ca::EDetectorID::kTrd: return CbmTrdTrackingInterface::Instance();
      case cbm::algo::ca::EDetectorID::kTof: return CbmTofTrackingInterface::Instance();
      default: return nullptr;
    }
  }

  inline const CbmTrackingDetectorInterfaceBase* GetTrackingInterface(const ECbmModuleId cbmDetId)
  {
    cbm::algo::ca::EDetectorID caDetId = cbm::algo::ca::ToCaDetectorID(cbmDetId);
    return GetTrackingInterface(caDetId);
  }

}  // namespace cbm::kf

#endif
