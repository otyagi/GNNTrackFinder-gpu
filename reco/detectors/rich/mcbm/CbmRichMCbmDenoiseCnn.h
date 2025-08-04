/* Copyright (C) 2024 UGiessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Martin Beyer [committer] */

/**
* \file CbmRichMCbmDenoiseCnn.h
* \author M.Beyer
* \date 2024
**/

#if HAVE_ONNXRUNTIME

#pragma once

#include <TObject.h>
#include <TSystem.h>

#include <array>
#include <string>
#include <vector>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

class CbmEvent;
class CbmRichDigiMapManager;
class TClonesArray;

/**
* \class CbmRichMCbmDenoiseCnn
*
* \brief MCbm mRICH noise removal using a CNN
*
* \author M.Beyer
* \date 2024
**/
class CbmRichMCbmDenoiseCnn : public TObject {
 public:
  /** Default constructor */
  CbmRichMCbmDenoiseCnn() = default;

  /** Destructor */
  ~CbmRichMCbmDenoiseCnn() = default;

  /** Copy constructor (disabled) */
  CbmRichMCbmDenoiseCnn(const CbmRichMCbmDenoiseCnn&) = delete;

  /** Assignment operator (disabled) */
  CbmRichMCbmDenoiseCnn operator=(const CbmRichMCbmDenoiseCnn&) = delete;

  /** Initialize ONNX Runtime structures and .onnx model */
  void Init();

  /**
    * \brief Process time sorted RICH hits and created non overlapping sliding time windows
    * \param event if CbmEvent* is nullptr -> process on full Ts
    * \param richHits TClonesArray of RICH hits
    */
  void Process(CbmEvent* event, const TClonesArray* richHits);

  /**
    * \brief Preparing hits in a non overlapping sliding time window and doing inference. 
    * Fills the classification result as a flag into CbmRichHit.fIsNoiseNN
    * \param timeWindowHitIndices vector of hit indices from richHits TClonesArray
    * \param seedHitTime time of seed hit
    * \param richHits TClonesArray of RICH hits
    */
  void ProcessTimeWindow(const std::vector<int>& timeWindowHitIndices, const double& seedHitTime,
                         const TClonesArray* richHits);

  /**
    * \brief Mapping from pixel address to row major grid index
    * \param int Digi/Hit address 
    * \return Grid index [0, 2303]
    */
  int AddressToGridIndex(const int address);

  /**
    * \brief ONNX runtime inference on input array fInput filled in ProcessTimeWindow(...)
    * \param gridIndices vector of grid indices [0, 2303]
    * \return vector of classification outputs for the given grid indices
    */
  std::vector<float> Inference(const std::vector<int>& gridIndices);

  /**
    * \brief Set the classification threshold
    * \param threshold value between (0.0, 1.0). Larger values will remove hits more aggressively
    */
  void SetClassificationThreshold(float threshold) { fClassificationThreshold = threshold; }

 private:
  const double fTimeWindowLength{25.};
  const int fMinHitsInTimeWindow{7};

  float fClassificationThreshold{0.5};

  const std::string fOnnxFilePath =
    std::string(gSystem->Getenv("VMCWORKDIR")) + "/parameters/rich/mRich/rich_v21c_mcbm_UNet.onnx";

  // TODO: read names/shapes from onnx file
  const std::vector<const char*> fInputNames{"input"};
  const std::vector<const char*> fOutputNames{"output"};

  const std::array<int64_t, 4> fInputShape{1, 1, 72, 32};
  const std::array<int64_t, 4> fOutputShape{1, 1, 72, 32};

  std::array<float, 2304> fInput{};

  std::unique_ptr<Ort::Env> fOrtEnv{nullptr};
  std::unique_ptr<Ort::SessionOptions> fOrtSessionOptions{nullptr};
  std::unique_ptr<Ort::Session> fOrtSession{nullptr};
  std::unique_ptr<Ort::MemoryInfo> fOrtAllocatorInfo{nullptr};
  std::unique_ptr<Ort::RunOptions> fOrtRunOptions{nullptr};

  std::unique_ptr<Ort::Value> fOrtInput{nullptr};

  CbmRichDigiMapManager* fCbmRichDigiMapManager{nullptr};

  ClassDef(CbmRichMCbmDenoiseCnn, 1);
};

#endif
