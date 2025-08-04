/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "Application.h"

#include <log.hpp>

#include "Histograms.h"
#include "RecoResultsInputArchive.h"
#include "Server.h"

using namespace cbm::explore;
using namespace cbm::algo;

Application::Application(const Options& options) : fOpts(options)
{
  L_(info) << "Starting application with port " << fOpts.Port();

  L_(info) << "Opening archive " << fOpts.Input();

  auto archive = std::make_shared<RecoResultsInputArchive>(fOpts.Input().string());
  auto histograms = std::make_shared<Histograms>();

  std::shared_ptr<RecoResultsInputArchive> archive2;
  std::shared_ptr<Histograms> histograms2;
  if (not fOpts.Input2().empty()) {
    L_(info) << "Opening second archive " << fOpts.Input2();
    archive2    = std::make_shared<RecoResultsInputArchive>(fOpts.Input2().string());
    histograms2 = std::make_shared<Histograms>();
  }

  Server::Settings settings {
    .port        = fOpts.Port(),
    .sensor      = fOpts.Sensor(),
    .archive     = archive,
    .histograms  = histograms,
    .archive2    = archive2,
    .histograms2 = histograms2,
  };

  fServer = std::make_unique<Server>(settings);
}

Application::~Application() {}

int Application::Run()
{
  fServer->Run();
  return 0;
}
