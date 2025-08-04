/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaIdealHitProducerDetBase.h
/// @brief  A FairTask to run ideal hit producers for CA tracking purposes (implementation)
/// @author S.Zharko<s.zharko@gsi.de>
/// @since  01.06.2023

#include "CbmCaIdealHitProducer.h"

#include "CbmSetup.h"


using cbm::ca::IdealHitProducer;

ClassImp(cbm::ca::IdealHitProducer);

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus IdealHitProducer::Init()
{
  fbUseDet[ca::EDetectorID::kMvd]  = false;  //CbmSetup::Instance()->IsActive(ECbmModuleId::kMvd);
  fbUseDet[ca::EDetectorID::kSts]  = CbmSetup::Instance()->IsActive(ECbmModuleId::kSts);
  fbUseDet[ca::EDetectorID::kMuch] = CbmSetup::Instance()->IsActive(ECbmModuleId::kMuch);
  fbUseDet[ca::EDetectorID::kTrd]  = CbmSetup::Instance()->IsActive(ECbmModuleId::kTrd);
  fbUseDet[ca::EDetectorID::kTof]  = CbmSetup::Instance()->IsActive(ECbmModuleId::kTof);


  InitStatus ret = kSUCCESS;
  if (fbUseDet[ca::EDetectorID::kMvd]) {
    ret = std::max(fHitProducerMvd.Init(), ret);
  }
  if (fbUseDet[ca::EDetectorID::kSts]) {
    ret = std::max(fHitProducerSts.Init(), ret);
  }
  if (fbUseDet[ca::EDetectorID::kMuch]) {
    ret = std::max(fHitProducerMuch.Init(), ret);
  }
  if (fbUseDet[ca::EDetectorID::kTrd]) {
    ret = std::max(fHitProducerTrd.Init(), ret);
  }
  if (fbUseDet[ca::EDetectorID::kTof]) {
    ret = std::max(fHitProducerTof.Init(), ret);
  }
  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void IdealHitProducer::Exec(Option_t* option)
{
  if (fbUseDet[ca::EDetectorID::kMvd]) {
    fHitProducerMvd.Exec(option);
  }
  if (fbUseDet[ca::EDetectorID::kSts]) {
    fHitProducerSts.Exec(option);
  }
  if (fbUseDet[ca::EDetectorID::kMuch]) {
    fHitProducerMuch.Exec(option);
  }
  if (fbUseDet[ca::EDetectorID::kTrd]) {
    fHitProducerTrd.Exec(option);
  }
  if (fbUseDet[ca::EDetectorID::kTof]) {
    fHitProducerTof.Exec(option);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void IdealHitProducer::SetConfigName(const char* name)
{
  fHitProducerMvd.SetConfigName(name);
  fHitProducerSts.SetConfigName(name);
  fHitProducerMuch.SetConfigName(name);
  fHitProducerTrd.SetConfigName(name);
  fHitProducerTof.SetConfigName(name);
}
