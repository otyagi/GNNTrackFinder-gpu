/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMANALYSISMANAGER_H_
#define CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMANALYSISMANAGER_H_

#include <Hal/AnalysisManager.h>
/**
 * class that is used only to
 */

class FairField;
class HalCbmAnalysisManager : public Hal::AnalysisManager {
  FairField* fMagField = {nullptr};

 public:
  HalCbmAnalysisManager();
  HalCbmAnalysisManager(const HalCbmAnalysisManager& other) = delete;
  virtual Bool_t Init();
  virtual ~HalCbmAnalysisManager();
  ClassDef(HalCbmAnalysisManager, 1)
};

#endif /* CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMANALYSISMANAGER_H_ */
