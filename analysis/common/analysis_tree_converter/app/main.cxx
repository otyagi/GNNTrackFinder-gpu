/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese, Jan de Cuveland */
#include <Logger.h>

#include "Application.h"
#include "ProgramOptions.h"

using namespace cbm::atconverter;

int main(int argc, char* argv[])
{
  LOG(info) << "*****   CBM AnalysisTree Converter   *****";
  try {
    ProgramOptions opt(argc, argv);
    Application app(opt);
    app.Exec();
  }
  catch (std::exception const& e) {
    LOG(error) << e.what() << "; terminating.";
    return EXIT_FAILURE;
  }

  LOG(info) << "CBM AnalysisTree Converter: Program completed successfully; exiting.";
  return EXIT_SUCCESS;
}
