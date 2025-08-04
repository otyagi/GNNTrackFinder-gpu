/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include <Rtypes.h>
#include <TNamed.h>

#include <memory>

#include "HistogramCollection.h"
#include "RecoResultsInputArchive.h"

class THttpServer;
class TH1;
class TH1F;
class TH1I;

namespace cbm::explore
{

  class Server : public TNamed {

  public:
    struct Settings {
      int port;
      std::optional<uint32_t> sensor;  // optional sensor to filter on
      std::shared_ptr<algo::RecoResultsInputArchive> archive;
      std::shared_ptr<HistogramCollection> histograms;

      // Optional second archive and histograms for comparison
      // histograms will be divided by this one, if provided
      std::shared_ptr<algo::RecoResultsInputArchive> archive2;
      std::shared_ptr<HistogramCollection> histograms2;
    };

    Server(Settings settings);

    virtual ~Server();

    int Run();

    // Server commands
    void RequestNextTS() { fRequestNextTS = true; }

  private:
    static constexpr int SleepPerTick_ms = 50;

    // Internal state
    THttpServer* fServer = nullptr;                           //!
    Settings fS;                                              //!

    // Server commands
    bool fRequestNextTS = false;

    void NextTS();

    ClassDef(Server, 1);
  };

}  // namespace cbm::explore
