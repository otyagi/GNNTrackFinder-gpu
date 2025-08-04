/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmAnalysisManager.h"

#include "CbmFieldMapSym3.h"
#include "CbmHelix.h"
#include "HalCbmField.h"

HalCbmAnalysisManager::HalCbmAnalysisManager() {}

Bool_t HalCbmAnalysisManager::Init()
{
  if (fMagField == nullptr) {
    fMagField = new CbmFieldMapSym3("field_v18a");
    fMagField->Init();
    SetField(new HalCbmField(fMagField));
  }
  Bool_t stat = Hal::AnalysisManager::Init();
  CbmHelix::SetField(GetField());
  return stat;
}

HalCbmAnalysisManager::~HalCbmAnalysisManager()
{
  if (fMagField) delete fMagField;
}
