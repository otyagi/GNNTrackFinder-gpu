/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "Application.h"

int main(int argc, char** argv)
{
  ProgramOptions opts{argc, argv};
  Application app{opts};
  app.Run();
  return 0;
}
