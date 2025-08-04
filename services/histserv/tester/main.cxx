/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */
#include <Logger.h>

#include "Application.h"
#include "ProgramOptions.h"

using namespace cbm::services::histserv_tester;

int main(int argc, char* argv[])
{
  LOG(info) << "*****   Tester client for Histogram server without FairMQ   *****";
  try {
    ProgramOptions opt(argc, argv);
    Application app(opt);
    app.Exec();
  }
  catch (std::exception const& e) {
    LOG(error) << e.what() << "; terminating.";
    return EXIT_FAILURE;
  }

  LOG(info) << "Histogram server without FairMQ: Tester client Program completed successfully; exiting.";
  return EXIT_SUCCESS;
}
