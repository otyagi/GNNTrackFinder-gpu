/* Copyright (C) 2021 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig [committer] */

#include "CbmTrdUnpackConfig.h"

#include "CbmTrdUnpackAlgoLegacy2020R.h"
#include "CbmTrdUnpackAlgoR.h"

#include <Logger.h>

#include <Rtypes.h>
#include <RtypesCore.h>

#include <memory>
#include <vector>

CbmTrdUnpackConfig::CbmTrdUnpackConfig(std::string detGeoSetupTag, UInt_t runid)
  : CbmRecoUnpackConfig("CbmTrdUnpackConfig", detGeoSetupTag, runid)
{
}

CbmTrdUnpackConfig::~CbmTrdUnpackConfig() {}

// ---- Init ----
void CbmTrdUnpackConfig::SetAlgo()
{
  LOG(info) << fName << "::Init -";

  // First choose the derived unpacking algorithm to be used and pass the raw to digi method
  auto algo = chooseAlgo();

  // If we got a handmade spadic we pass it to the algorithm and the RTD method
  if (fSpadic) {
    algo->SetSpadicObject(fSpadic);
    fRTDMethod->SetSpadicObject(fSpadic);
  }

  if (fDoLog) LOG(info) << fName << "::Init - SetRawToDigiMethod";
  algo->SetRawToDigiMethod(fRTDMethod);

  if (fDoLog) LOG(info) << fName << "::Init - SetParFilesBasePath";
  algo->SetParFilesBasePath(fParFilesBasePath);

  // Set the global system time offset
  if (fDoLog) LOG(info) << fName << "::SetAlgo - SetSystemTimeOffset";
  algo->SetSystemTimeOffset(fSystemTimeOffset);

  // Set the flag controlling the overlap ignore
  if (fDoLog) LOG(info) << fName << "::SetAlgo - SetDoIgnoreOverlappMs";
  algo->SetDoIgnoreOverlappMs(fDoIgnoreOverlappMs);

  // Set the global system time offset
  if (fDoLog) LOG(info) << fName << "::SetAlgo - SetElinkTimeOffsetMap";
  algo->SetElinkTimeOffsetMap(fElinkTimeOffsetMap);

  // If we have a monitor in the config add it to the algo
  if (fMonitor) algo->SetMonitor(fMonitor);

  // Pass the algo to its member in the base class
  fAlgo = algo;
}

// ---- chooseAlgo ----
std::shared_ptr<CbmTrdUnpackAlgoBaseR> CbmTrdUnpackConfig::chooseAlgo()
{
  if (fDoLog) LOG(info) << fName << "::Init - chooseAlgo";

  // Non default unpacker selection
  // Legacy unpacker for data taken before mcbm 2021
  if (fGeoSetupTag.find("Desy2019") != fGeoSetupTag.npos) {
    auto algo = std::make_shared<CbmTrdUnpackAlgoLegacy2020R>();
    LOG(info) << fName << "::chooseAlgo() - selected algo = " << algo->Class_Name();
    return algo;
  }
  if (fGeoSetupTag.find("trd_IkfSmallChamber") != fGeoSetupTag.npos) {
    auto algo = std::make_shared<CbmTrdUnpackAlgoLegacy2020R>();
    LOG(info) << fName << "::chooseAlgo() - selected algo = " << algo->Class_Name();
    return algo;
  }
  if (fGeoSetupTag.find("v20a_mcbm") != fGeoSetupTag.npos) {
    auto algo = std::make_shared<CbmTrdUnpackAlgoLegacy2020R>();
    LOG(info) << fName << "::chooseAlgo() - selected algo = " << algo->Class_Name();
    return algo;
  }

  // Default unpacker selection
  // Unpacker algo from mcbm 2021 on and hopefully default for a long time.
  auto algo = std::make_shared<CbmTrdUnpackAlgoR>();
  LOG(info) << fName << "::chooseAlgo() - selected algo = " << algo->Class_Name();
  return algo;

  LOG(error) << fName
             << "::Init - chooseAlgo() - no algorithm created something went wrong. We can not work like this!";
  return nullptr;
}

void CbmTrdUnpackConfig::SetElinkTimeOffset(std::uint32_t criid, std::uint8_t elinkid, std::int32_t offsetNs)
{
  // create vector for criid if not yet existing
  if (fElinkTimeOffsetMap.find(criid) == fElinkTimeOffsetMap.end()) {
    //vector has fixed size of 256 (max elinkid) and is initialized to 0
    fElinkTimeOffsetMap.insert(std::make_pair(criid, std::vector<int32_t>(256, 0)));
  }

  fElinkTimeOffsetMap[criid][elinkid] = offsetNs;
}


ClassImp(CbmTrdUnpackConfig)
