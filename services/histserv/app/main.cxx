/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */
#include "Application.h"
#include "ProgramOptions.h"

#include <Logger.h>

#include <csignal>

namespace
{
  volatile sig_atomic_t signal_status = 0;
}

static void signal_handler(int sig) { signal_status = sig; }

using namespace cbm::services::histserv;

int main(int argc, char* argv[])
{
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  LOG(info) << "*****   Histogram server without FairMQ   *****";
  try {
    ProgramOptions opt(argc, argv);
    Application app(opt, &signal_status);
    app.Exec();
  }
  catch (std::exception const& e) {
    LOG(error) << e.what() << "; terminating.";
    return EXIT_FAILURE;
  }

  LOG(info) << "Histogram server without FairMQ: Program completed successfully; exiting.";
  return EXIT_SUCCESS;
}
