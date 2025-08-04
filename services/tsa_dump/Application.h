/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#pragma once

#include "ProgramOptions.h"

class Application {

 public:
  Application(ProgramOptions opts) : fOpts(opts) {}

  void Run();

 private:
  ProgramOptions fOpts;
};
