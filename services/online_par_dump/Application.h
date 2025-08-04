/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include "ProgramOptions.h"

class Application {

 public:
  Application(ProgramOptions opts) : fOpts(opts) {}

  void Run();

 private:
  ProgramOptions fOpts;
};
