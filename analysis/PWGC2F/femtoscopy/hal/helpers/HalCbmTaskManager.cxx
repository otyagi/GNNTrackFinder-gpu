/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "HalCbmTaskManager.h"

#include "CbmFieldMap.h"
#include "CbmFieldMapSym3.h"
#include "HalCbmField.h"

#include <FairRunAna.h>

#include <iostream>
#include <vector>

#include <Hal/IOManager.h>
#include <Hal/RootManager.h>


HalCbmTaskManager::HalCbmTaskManager() : fMagField(nullptr) {}

InitStatus HalCbmTaskManager::Init()
{
  if (!FairRunAna::Instance()->GetField()) {
    FairField* field = new CbmFieldMapSym3("field_v18a");
    field->Init();
    fMagField = new HalCbmField(field);
  }
  else {
    fMagField = new HalCbmField(FairRunAna::Instance()->GetField());
  }
  CbmHelix::SetField(fMagField);
  fManager = new Hal::Fair::RootManager();
  return Hal::Fair::TaskManager::Init();
}

HalCbmTaskManager::~HalCbmTaskManager()
{
  delete fMagField;
  delete fManager;
}
