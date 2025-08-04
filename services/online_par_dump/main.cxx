/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "Application.h"

int main(int argc, char** argv)
{
  ProgramOptions opts{argc, argv};
  Application app{opts};
  app.Run();
  return 0;
}
