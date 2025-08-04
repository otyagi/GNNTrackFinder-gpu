/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMFIELD_H_
#define CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMFIELD_H_

#include <Hal/MagField.h>

class FairField;

class HalCbmField : public Hal::MagField {
  FairField* fField = {nullptr};

 public:
  HalCbmField(FairField* f = nullptr);
  TVector3 GetField(Double_t x, Double_t y, Double_t z) const;
  virtual ~HalCbmField();
  HalCbmField(const HalCbmField& other);
  ClassDef(HalCbmField, 1)
};

#endif /* CBMROOT_2023_ANALYSIS_PWGC2F_FEMTOSCOPY_HAL_HELPERS_HALCBMFIELD_H_ */
