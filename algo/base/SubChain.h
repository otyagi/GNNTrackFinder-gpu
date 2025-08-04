/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_SUBCHAIN_H
#define CBM_ALGO_BASE_SUBCHAIN_H

#include "ChainContext.h"

#include <gsl/pointers>

namespace cbm::algo
{
  class SubChain {

   public:
    const ChainContext* GetContext() { return fContext; }

    void SetContext(const ChainContext* ctx) { fContext = ctx; }

    const Options& Opts() const { return gsl::make_not_null(fContext)->opts; }
    const RecoParams& Params() const { return gsl::make_not_null(fContext)->recoParams; }

    bool HasMonitor() const { return gsl::make_not_null(fContext)->monitor != nullptr; }

    Monitor& GetMonitor() const
    {
      // Need Get-prefix to avoid conflict with Monitor-class name
      if (!HasMonitor()) throw std::runtime_error("No monitor available");
      return *gsl::make_not_null(fContext)->monitor;
    }


   private:
    const ChainContext* fContext = nullptr;
  };
}  // namespace cbm::algo

#endif
