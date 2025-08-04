/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include <memory>

#include "Options.h"

namespace cbm::explore
{

  class Server;

  class Application {

  public:
    Application(const Options& options);

    ~Application();

    int Run();

  private:
    Options fOpts;

    std::unique_ptr<Server> fServer;
  };

}  // namespace cbm::explore
