/* Copyright (C) 2021 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig [committer], Alexandru Bercuci*/

#include "CbmTrdUnpackFaspConfig.h"

CbmTrdUnpackFaspConfig::CbmTrdUnpackFaspConfig(std::string detGeoSetupTag, UInt_t runid)
  : CbmRecoUnpackConfig("CbmTrdUnpackFaspConfig", detGeoSetupTag, runid)
{
}

CbmTrdUnpackFaspConfig::~CbmTrdUnpackFaspConfig() {}

// ---- Init ----

// ---- chooseAlgo ----
std::shared_ptr<CbmTrdUnpackFaspAlgo> CbmTrdUnpackFaspConfig::chooseAlgo()
{
  if (fDoLog) LOG(info) << fName << "::Init - chooseAlgo";

  // Default unpacker selection
  // Unpacker algo from mcbm 2021 on and hopefully default for a long time.
  auto algo = std::make_shared<CbmTrdUnpackFaspAlgo>();
  LOG(info) << fName << "::chooseAlgo() - selected algo = " << algo->Class_Name();
  return algo;

  LOG(error) << fName
             << "::Init - chooseAlgo() - no algorithm created something went wrong. We can not work like this!";
  return nullptr;
}

// ---- reset ----
void CbmTrdUnpackFaspConfig::reset()
{
  uint32_t uNbLostDigis = fAlgo->ResetTimeslice();
  if (uNbLostDigis /*&& fDoLog*/) {
    LOG(info) << fName << "::reset - Lost digis after processing timeslice: " << uNbLostDigis;
  }
}

//_____________________________________________________________________
void CbmTrdUnpackFaspConfig::InitAlgo()
{
  if (fDoLog) LOG(info) << fName << "::InitAlgo - Setup Fasp mapping";

  // If we have a monitor in the config add it to the algo
  if (fMonitor) fAlgo->SetMonitor(fMonitor);

  // Now we have all information required to initialise the algorithm
  fAlgo->Init();

  // Finish initialization of the monitoring task after all information for the run are gathered
  if (fMonitor) {
    /*if (fDoLog)*/ LOG(info) << fName << "::InitAlgo - Setup monitoring task";
    fMonitor->Init();
    fMonitor->MapMaskedChannels(fAlgo->GetAsicPar());
  }
}

ClassImp(CbmTrdUnpackFaspConfig)
