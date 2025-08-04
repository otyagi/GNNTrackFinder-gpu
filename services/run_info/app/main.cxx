/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   main.cxx
/// \brief  Main function of the run_info service
/// \since  24.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "Application.h"

#include <iostream>

using namespace cbm::services::run_info;

int main(int argc, char* argv[])
{
  Application app;
  try {
    auto info = app.ParseOptions(argc, argv);
    if (info.has_value()) {
      app.Print(info.value());
    }
  }
  catch (const std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
