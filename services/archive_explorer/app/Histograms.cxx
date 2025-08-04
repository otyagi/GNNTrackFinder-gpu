/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "Histograms.h"

#include <TH1.h>

#include <log.hpp>
#include <unordered_map>

#include "StorableRecoResults.h"

using namespace cbm::explore;

Histograms::Histograms()
{
  CreateFolder("/sts", "STS");
  CreateFolder("/sts/digis", "Digis");
  CreateHisto(fHStsDigisTime, "/sts/digis", "hStsDigisTime", "Sts Digis Time", 1000, 0, 128000000);

  CreateFolder("/sts/clusters", "Clusters");
  CreateHisto(fHStsClustersTime, "/sts/clusters", "hStsClustersTime", "Sts Clusters Time", 1000, 0, 128000000);

  CreateFolder("/sts/hits", "Hits");
  CreateHisto(fHStsHitsX, "/sts/hits", "hStsHitsX", "Sts Hits X", 100, -10, 10);
  CreateHisto(fHStsHitsY, "/sts/hits", "hStsHitsY", "Sts Hits Y", 100, -10, 10);
  CreateHisto(fHStsHitsZ, "/sts/hits", "hStsHitsZ", "Sts Hits Z", 100, 0, 50);
  CreateHisto(fHStsHitsTime, "/sts/hits", "hStsHitsTime", "Sts Hits Time", 1000, 0, 128000000);
  CreateHisto(fHStsHitsDx, "/sts/hits", "hStsHitsDx", "Sts Hits Dx", 100, 0, 0.1);
  CreateHisto(fHStsHitsDy, "/sts/hits", "hStsHitsDy", "Sts Hits Dy", 100, 0, 0.1);
  CreateHisto(fHStsHitsDxy, "/sts/hits", "hStsHitsDxy", "Sts Hits Dxy", 100, 0, 0.1);
  CreateHisto(fHStsHitsTimeError, "/sts/hits", "hStsHitsTimeError", "Sts Hits Time Error", 100, 0, 0.1);
  CreateHisto(fHStsHitsDu, "/sts/hits", "hStsHitsDu", "Sts Hits Du", 100, 0, 0.1);
  CreateHisto(fHStsHitsDv, "/sts/hits", "hStsHitsDv", "Sts Hits Dv", 100, 0, 0.1);

  CreateHisto(fHNHitsFromCluster, "/sts/hits", "hNHitsFromCluster", "Number of hits from cluster", 100, 0, 100);
}

void Histograms::FillHistos(HistoData fill)
{
  auto& stsDigis = fill.data->StsDigis();
  L_(info) << "Filling histos with " << stsDigis.size() << " STS digis";
  for (auto& digi : stsDigis) {
    if (fill.Skip(digi.GetAddress())) continue;
    fHStsDigisTime->Fill(digi.GetTimeU32());
  }

  auto& stsClusters = fill.data->StsClusters();
  for (size_t p = 0; p < stsClusters.NPartitions(); p++) {
    auto [sensor, address] = stsClusters.Partition(p);
    if (fill.Skip(address)) continue;
    L_(info) << "Filling STS sensor " << address << " with " << sensor.size() << " clusters";
    for (auto& cluster : sensor) {
      fHStsClustersTime->Fill(cluster.fTime);
    }
  }

  auto& stsHits = fill.data->StsHits();
  for (size_t p = 0; p < stsHits.NPartitions(); p++) {
    auto [sensor, address] = stsHits.Partition(p);
    if (fill.Skip(address)) continue;
    L_(info) << "Filling STS sensor " << address << " with " << sensor.size() << " hits";

    std::unordered_map<int, int> nHitsFromClusterF, nHitsFromClusterB;

    for (auto& hit : sensor) {
      fHStsHitsX->Fill(hit.X());
      fHStsHitsY->Fill(hit.Y());
      fHStsHitsZ->Fill(hit.Z());
      fHStsHitsTime->Fill(hit.Time());
      fHStsHitsDx->Fill(hit.Dx());
      fHStsHitsDy->Fill(hit.Dy());
      fHStsHitsDxy->Fill(hit.fDxy);
      fHStsHitsTimeError->Fill(hit.TimeError());
      fHStsHitsDu->Fill(hit.fDu);
      fHStsHitsDv->Fill(hit.fDv);

      nHitsFromClusterF[hit.fFrontClusterId]++;
      nHitsFromClusterB[hit.fBackClusterId]++;
    }

    for (auto& [clusterId, nHits] : nHitsFromClusterF)
      fHNHitsFromCluster->Fill(nHits);

    for (auto& [clusterId, nHits] : nHitsFromClusterB)
      fHNHitsFromCluster->Fill(nHits);
  }
}
