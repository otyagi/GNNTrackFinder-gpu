/* Copyright (C) 2022 Johann Wolfgang Goethe-Universitaet Frankfurt, Frankfurt am Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Jan de Cuveland [committer] */

#include "Application.h"

#include "MainConfig.h"

#include <chrono>
#include <fstream>
#include <thread>

#include <yaml-cpp/yaml.h>

Application::Application(ProgramOptions const& opt) : fOpt(opt)
{
  // start up monitoring
  if (!fOpt.MonitorUri().empty()) {
    fMonitor = std::make_unique<cbm::Monitor>(fOpt.MonitorUri());
  }

  cbm::algo::evbuild::Config config(YAML::LoadFile(fOpt.ConfigYamlFile()));
  if (!fOpt.SaveConfigYamlFile().empty()) {
    std::ofstream fout(fOpt.SaveConfigYamlFile());
    fout << config.ToYaml();
  }

  CbmReco::Config recoConfig;
  recoConfig.dumpSetup = fOpt.DumpSetup();

  fCbmReco = std::make_unique<CbmReco>(recoConfig, fOpt.InputUri(), fOpt.OutputRootFile(), fOpt.ConfigYamlFile(),
                                       fOpt.MaxNumTs(), fOpt.HttpServerPort(), fMonitor.get());
}

void Application::Run() { fCbmReco->Run(); }

Application::~Application()
{
  // delay to allow monitor to process pending messages
  constexpr auto destruct_delay = std::chrono::milliseconds(200);
  std::this_thread::sleep_for(destruct_delay);
}
