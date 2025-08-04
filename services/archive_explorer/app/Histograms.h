/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include "HistogramCollection.h"

class TH1F;
class TH1I;

namespace cbm::explore
{

  class Histograms : public HistogramCollection {

  public:
    Histograms();

    virtual ~Histograms() {}

    void FillHistos(HistoData fill) override;

  private:
    // Sts Digis
    TH1I* fHStsDigisTime = nullptr;

    // Sts Clusters
    TH1I* fHStsClustersTime = nullptr;

    // Sts Hits
    TH1F* fHStsHitsX         = nullptr;
    TH1F* fHStsHitsY         = nullptr;
    TH1F* fHStsHitsZ         = nullptr;
    TH1I* fHStsHitsTime      = nullptr;
    TH1F* fHStsHitsDx        = nullptr;
    TH1F* fHStsHitsDy        = nullptr;
    TH1F* fHStsHitsDxy       = nullptr;
    TH1F* fHStsHitsTimeError = nullptr;
    TH1F* fHStsHitsDu        = nullptr;
    TH1F* fHStsHitsDv        = nullptr;

    TH1I* fHNHitsFromCluster = nullptr;
  };

}  // namespace cbm::explore
