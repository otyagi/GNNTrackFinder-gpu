/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include <log.hpp>

#include "Application.h"
#include "Options.h"

using namespace cbm::explore;

int main(int argc, char** argv)
{

  Options options(argc, argv);

  Application app(options);

  return app.Run();
}
