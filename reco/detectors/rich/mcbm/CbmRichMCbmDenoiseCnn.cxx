/* Copyright (C) 2024 UGiessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Martin Beyer [committer] */

#if HAVE_ONNXRUNTIME

#include "CbmRichMCbmDenoiseCnn.h"

#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmRichDetectorData.h"
#include "CbmRichDigiMapManager.h"
#include "CbmRichHit.h"

#include <Logger.h>

#include <TClonesArray.h>
#include <TStopwatch.h>

#include <iostream>
#include <sstream>
#include <vector>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

void CbmRichMCbmDenoiseCnn::Init()
{
  fOrtEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, GetName());

  fOrtSessionOptions = std::make_unique<Ort::SessionOptions>();
  // Thread numbers need to be explicitly set. With the current Ort version it can't detect
  // the numbers on the cluster, should be fixed in a later version.
  fOrtSessionOptions->SetIntraOpNumThreads(1);
  fOrtSessionOptions->SetInterOpNumThreads(1);
  fOrtSessionOptions->SetExecutionMode(ORT_SEQUENTIAL);
  fOrtSessionOptions->SetGraphOptimizationLevel(ORT_ENABLE_ALL);

  fOrtSession = std::make_unique<Ort::Session>(*fOrtEnv, fOnnxFilePath.c_str(), *fOrtSessionOptions);

  fOrtRunOptions    = std::make_unique<Ort::RunOptions>(nullptr);
  fOrtAllocatorInfo = std::make_unique<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU));
  fOrtInput         = std::make_unique<Ort::Value>(Ort::Value::CreateTensor<float>(
    *fOrtAllocatorInfo, fInput.data(), fInput.size(), fInputShape.data(), fInputShape.size()));

  fCbmRichDigiMapManager = &CbmRichDigiMapManager::GetInstance();
}

void CbmRichMCbmDenoiseCnn::Process(CbmEvent* event, const TClonesArray* richHits)
{
  int nHits = event ? static_cast<int>(event->GetNofData(ECbmDataType::kRichHit)) : richHits->GetEntriesFast();
  std::vector<int> timeWindowHitIndices;
  for (int i = 0; i < nHits; i++) {  // Sliding time window loop
    timeWindowHitIndices.clear();
    int seedIdx = event ? static_cast<int>(event->GetIndex(ECbmDataType::kRichHit, static_cast<uint32_t>(i))) : i;
    CbmRichHit* seedHit = static_cast<CbmRichHit*>(richHits->At(seedIdx));
    if (!seedHit) continue;
    timeWindowHitIndices.push_back(seedIdx);
    int hitsInTimeWindow = 1;
    for (int j = i + 1; j < nHits; j++) {  // Search for hits in time window
      int hitIdx      = event ? static_cast<int>(event->GetIndex(ECbmDataType::kRichHit, j)) : j;
      CbmRichHit* hit = static_cast<CbmRichHit*>(richHits->At(hitIdx));
      if (!hit) continue;
      double dt = hit->GetTime() - seedHit->GetTime();
      if (dt < fTimeWindowLength) {
        hitsInTimeWindow++;
        timeWindowHitIndices.push_back(hitIdx);
      }
      else {
        break;
      }
    }

    if (hitsInTimeWindow >= fMinHitsInTimeWindow) {
      ProcessTimeWindow(timeWindowHitIndices, seedHit->GetTime(), richHits);
      i += hitsInTimeWindow - 1;  // Move to last hit inside time window
    }
    else {
      seedHit->SetIsNoiseNN(true);
    }
  }
}

void CbmRichMCbmDenoiseCnn::ProcessTimeWindow(const std::vector<int>& timeWindowHitIndices, const double& seedHitTime,
                                              const TClonesArray* richHits)
{
  fInput = {};  // Reset all input values to 0.0
  std::vector<int> gridIndices;
  gridIndices.reserve(50);
  for (const auto& hitIdx : timeWindowHitIndices) {
    CbmRichHit* hit    = static_cast<CbmRichHit*>(richHits->At(hitIdx));
    const auto gridIdx = AddressToGridIndex(hit->GetAddress());
    if (gridIdx < 0 || gridIdx >= static_cast<int>(fInput.size())) {
      LOG(error) << GetName() << "::ProcessTimeWindow: Invalid grid index: " << gridIdx;
      continue;
    }
    gridIndices.push_back(gridIdx);
    // Shift time by 1ns to distinguish empty pixels from seed hit time
    fInput[gridIdx] = static_cast<float>((hit->GetTime() - seedHitTime + 1.0) / (fTimeWindowLength + 1.0));
  }

  const auto output = Inference(gridIndices);

  for (std::size_t i = 0; i < timeWindowHitIndices.size(); i++) {
    CbmRichHit* hit = static_cast<CbmRichHit*>(richHits->At(timeWindowHitIndices[i]));
    bool isNoise    = output[i] < fClassificationThreshold;
    hit->SetIsNoiseNN(isNoise);
  }
}

std::vector<float> CbmRichMCbmDenoiseCnn::Inference(const std::vector<int>& gridIndices)
{
  auto output = fOrtSession->Run(*fOrtRunOptions.get(), fInputNames.data(), fOrtInput.get(), 1, fOutputNames.data(), 1);
  float* intarr = output.front().GetTensorMutableData<float>();

  std::vector<float> out(gridIndices.size());
  std::transform(gridIndices.begin(), gridIndices.end(), out.begin(),
                 [intarr](int gridIdx) { return intarr[gridIdx]; });
  return out;
}

int CbmRichMCbmDenoiseCnn::AddressToGridIndex(int address)
{
  CbmRichPixelData* pixel_data = fCbmRichDigiMapManager->GetPixelDataByAddress(address);

  // Calculate local X [0,7],Y [0,7] indices of one MAPMT
  int pmtUID  = pixel_data->fPmtId;
  int pmtIndX = (pmtUID >> 4) & 0xF;  // index ascending from -x to x
  int pmtIndY = pmtUID & 0xF;         // index ascending from y to -y

  // Calculate global X [0,31],Y [0,72] indices
  int globalIndX = 8 * pmtIndX + pixel_data->fPixelId % 8;
  int globalIndY = 8 * pmtIndY + pixel_data->fPixelId / 8;

  return 32 * globalIndY + globalIndX;
}

#endif  // HAVE_ONNXRUNTIME
