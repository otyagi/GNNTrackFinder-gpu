/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#ifndef HALCBMTASKMANAGER_H_
#define HALCBMTASKMANAGER_H_
#include "CbmHelix.h"
#include "FairField.h"
#include "FairTask.h"

#include <Hal/TaskManager.h>

namespace Hal
{
  class IOManager;
  class Task;
  class MagField;
}  // namespace Hal
/**
 * wrapper based on FairTask to use Hal::Task from FairRoot
 */
class HalCbmTaskManager : public Hal::Fair::TaskManager {
  Hal::MagField* fMagField;

 protected:
  InitStatus Init();

 public:
  HalCbmTaskManager();
  virtual ~HalCbmTaskManager();
  ClassDef(HalCbmTaskManager, 1)
};
#endif /* HALCBMTASKMANAGER_H_ */
