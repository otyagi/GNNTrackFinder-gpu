/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "Server.h"

#include <TH1.h>
#include <THttpServer.h>
#include <TSystem.h>

#include <log.hpp>

#include <fmt/format.h>

using namespace cbm::explore;

ClassImp(Server);

Server::Server(Settings settings) : TNamed("server", "server"), fS(settings)
{
  L_(info) << "Starting Server with port " << fS.port;

  if (fS.sensor) L_(info) << "Filtering on sensor " << *fS.sensor;
  else {
    L_(info) << "Not filtering on sensor";
  }

  fServer = new THttpServer(fmt::format("http:{}", fS.port).c_str());
  fServer->SetTimer(0, true);

  // FIXME: Why doesn't root use the local jsroot by default???
  fServer->SetJSROOT("https://jsroot.gsi.de/7.1.0/");

  // register this object in http server
  fServer->Register("/", this);
  fServer->Hide("/server");

  // enable monitoring and
  // specify items to draw when page is opened
  // fServer->SetItemField("/", "_monitoring", "5000");

  // Specify layout. TODO: does not work?
  fServer->SetItemField("/", "_layout", "grid3x3");

  fServer->RegisterCommand("/NextTS", "/server/->RequestNextTS()");
  fServer->SetItemField("/NextTS", "_title", "Next Timeslice");

  if (fS.archive == nullptr) throw std::runtime_error("Archive must not be null");
  if (fS.histograms == nullptr) throw std::runtime_error("Histograms must not be null");

  if (fS.archive2 != nullptr || fS.histograms2 != nullptr) {
    if (fS.archive2 == nullptr) throw std::runtime_error("Archive2 must not be null");
    if (fS.histograms2 == nullptr) throw std::runtime_error("Histograms2 must not be null");
  }

  // Setup folders
  for (auto& folder : fS.histograms->GetFolders()) {
    fServer->CreateItem(folder.path.c_str(), folder.name.c_str());
    fServer->SetItemField(folder.path.c_str(), "_kind", "Folder");
  }

  // Setup histograms
  for (auto& [path, histo] : fS.histograms->GetHistos()) {
    fServer->Register(path.c_str(), histo);
  }
}

Server::~Server() {}

int Server::Run()
{
  NextTS();

  L_(info) << "Starting event loop";
  while (true) {

    int nRequests = 0;
    do {
      nRequests = fServer->ProcessRequests();
      if (nRequests > 0) L_(info) << "Processed " << nRequests << " requests";

      // sleep minimal time
      // TODO: This is terrible. And the sleep should not be in this loop.
      // But ROOTJS sometimes sends commands multiple times,
      // And this is the only way, i was able to catch duplicate commands
      gSystem->Sleep(SleepPerTick_ms);
    } while (nRequests > 0);

    if (fRequestNextTS) {
      NextTS();
      fRequestNextTS = false;
    }
  }

  return 0;
}

void Server::NextTS()
{
  L_(info) << "Fetching next Timeslice...";

  auto recoResults = fS.archive->get();
  if (fS.archive->eos()) {
    L_(info) << "End of archive reached";
    return;
  }

  HistoData fill {.data = recoResults.get(), .sensor = fS.sensor};

  fS.histograms->Reset();
  fS.histograms->FillHistos(fill);

  bool divide = fS.archive2 != nullptr;
  if (divide) {
    recoResults = fS.archive2->get();
    if (fS.archive2->eos()) throw std::runtime_error("End of archive2 reached too early");
    fS.histograms2->Reset();
    fill = {.data = recoResults.get(), .sensor = fS.sensor};
    fS.histograms2->FillHistos(fill);
    L_(info) << "Dividing histograms";
    fS.histograms->Div(*fS.histograms2);
  }

  L_(info) << "Finished Timeslice";
}
