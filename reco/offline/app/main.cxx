/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Jan de Cuveland */
#include "Application.h"
#include "ProgramOptions.h"

#include <Logger.h>

using namespace cbm::reco::offline;

int main(int argc, char* argv[])
{
  LOG(info) << "*****   CBM Offline Reconstruction   *****";
  try {
    ProgramOptions opt(argc, argv);
    Application app(opt);
    app.Exec();
  }
  catch (std::exception const& e) {
    LOG(error) << e.what() << "; terminating.";
    return EXIT_FAILURE;
  }

  LOG(info) << "CBM Offline Reconstruction: Program completed successfully; exiting.";
  return EXIT_SUCCESS;
}
