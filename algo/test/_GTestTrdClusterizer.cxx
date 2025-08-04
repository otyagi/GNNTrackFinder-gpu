/* Copyright (C) 2024 Facility for Anti-Proton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Axel Puntke [committer] */

#include "CbmTrdDigi.h"
#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"
#include "trd/Clusterizer.h"
#include "trd/HitFinderPars.h"

//Charge values to be used for self- and neighbor-triggers
double CHARGE_ST = 1000;
double CHARGE_NT = 500;

//Value to be used for module address
int MADDR = 21;

cbm::algo::trd::Clusterizer* CreateTestModule()
{
  //create type 5 module for testing
  cbm::algo::trd::HitFinderModPar modPar;
  modPar.padSizeX    = 0.666667;
  modPar.padSizeY    = 12.0;
  modPar.padSizeErrX = 0.0;
  modPar.padSizeErrY = 0.0;
  modPar.padSizeErrY = 0.0;
  modPar.address     = MADDR;
  modPar.orientation = 0;

  int nCols = 144;
  int nRows = 24;

  for (int iRow = 0; iRow < nRows; ++iRow) {
    cbm::algo::trd::HitFinderRowPar rowPar;
    for (int iCol = 0; iCol < nCols; ++iCol) {
      cbm::algo::trd::HitFinderPadPar padPar;
      rowPar.padPar.push_back(padPar);
    }

    modPar.rowPar.push_back(rowPar);
  }

  return new cbm::algo::trd::Clusterizer(modPar);
}

//returns all possible combinations of the digis, each combination separated in time by timeSeparation
std::vector<CbmTrdDigi*> PermutateDigis(std::vector<CbmTrdDigi*> digis, double timeSeparation = 3000)
{
  std::vector<CbmTrdDigi*> digisPermutated;

  std::vector<int> v;
  for (size_t i = 0; i < digis.size(); ++i)
    v.push_back(i);

  double timeAdd = 0;

  do {
    for (auto e : v) {
      digisPermutated.push_back(digis.at(e));
      digisPermutated.back()->SetTime(digis.at(e)->GetTime() + timeAdd);
    }
    timeAdd += timeSeparation;
  } while (std::next_permutation(v.begin(), v.end()));

  std::sort(digisPermutated.begin(), digisPermutated.end(),
            [](const CbmTrdDigi* lhs, const CbmTrdDigi* rhs) { return lhs->GetTime() < rhs->GetTime(); });

  return digisPermutated;
}

//returns digi vector in corrent format to pass to the clusterizer
const std::vector<std::pair<CbmTrdDigi, int32_t>> CreateClusterizerInputVector(std::vector<CbmTrdDigi*> digis)
{
  std::vector<std::pair<CbmTrdDigi, int32_t>> inVec;
  int32_t idx = 0;

  for (auto digi : digis)
    inVec.push_back(std::make_pair<CbmTrdDigi, int32_t>(std::move(*digi), idx++));

  return inVec;
}

std::vector<uint16_t> GetClusterSizes(std::vector<cbm::algo::trd::Cluster> clusters)
{
  std::vector<uint16_t> clusterSizes;

  for (auto cluster : clusters)
    clusterSizes.push_back(cluster.GetNofDigis());

  return clusterSizes;
}


TEST(_GTestTrdClusterizer, Check3PadCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  digis.push_back(new CbmTrdDigi(15, MADDR, CHARGE_NT, 100, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(16, MADDR, CHARGE_ST, 100, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(17, MADDR, CHARGE_NT, 100, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 3), nPermutations);
}

TEST(_GTestTrdClusterizer, Check1PadSTCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;
  digis.push_back(new CbmTrdDigi(16, MADDR, CHARGE_ST, 100, CbmTrdDigi::eTriggerType::kSelf, 0));

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digis));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusterSizes, (std::vector<uint16_t>){1});
  //EXPECT_FALSE(module->IsClusterComplete( (CbmTrdCluster*)(clusters->At(0)) ));
}

TEST(_GTestTrdClusterizer, Check1PadNTCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;
  digis.push_back(new CbmTrdDigi(16, MADDR, CHARGE_NT, 100, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digis));

  EXPECT_EQ(clusters.size(), 0);
  //EXPECT_FALSE(module->IsClusterComplete( (CbmTrdCluster*)(clusters->At(0)) ));
}

TEST(_GTestTrdClusterizer, Check2PadLeftNTCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;
  digis.push_back(new CbmTrdDigi(15, MADDR, CHARGE_NT, 100, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(16, MADDR, CHARGE_ST, 100, CbmTrdDigi::eTriggerType::kSelf, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), 2);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 2), 2);
  //EXPECT_FALSE(module->IsClusterComplete( (CbmTrdCluster*)(clusters->At(0)) ));
}

TEST(_GTestTrdClusterizer, Check2PadRightNTCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;
  digis.push_back(new CbmTrdDigi(16, MADDR, CHARGE_ST, 100, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(17, MADDR, CHARGE_NT, 100, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), 2);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 2), 2);
  //EXPECT_FALSE(module->IsClusterComplete( (CbmTrdCluster*)(clusters->At(0)) ));
}

/* currently fails at permutation 3 (digi order 143, 144, 142) */
TEST(_GTestTrdClusterizer, Check2PadWithSTOnNextRowCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  digis.push_back(new CbmTrdDigi(142, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(143, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));

  digis.push_back(new CbmTrdDigi(144, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations * 2);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 2), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 1), nPermutations);
}

TEST(_GTestTrdClusterizer, Check2PadWithNTOnNextRowCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  digis.push_back(new CbmTrdDigi(142, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(143, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));

  digis.push_back(new CbmTrdDigi(144, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 2), nPermutations);
}

TEST(_GTestTrdClusterizer, Check2PadWithSTOnPrevRowCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  digis.push_back(new CbmTrdDigi(143, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));

  digis.push_back(new CbmTrdDigi(144, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(145, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations * 2);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 2), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 1), nPermutations);
}

TEST(_GTestTrdClusterizer, Check2PadWithNTOnPrevRowCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  digis.push_back(new CbmTrdDigi(143, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  digis.push_back(new CbmTrdDigi(144, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(145, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 2), nPermutations);
}

TEST(_GTestTrdClusterizer, CheckAdjacent3PadClusters)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;
  digis.push_back(new CbmTrdDigi(149, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(150, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(151, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  digis.push_back(new CbmTrdDigi(152, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(153, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(154, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations * 2);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 3), nPermutations * 2);
}

TEST(_GTestTrdClusterizer, Check3PadWithSingleNTCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;
  digis.push_back(new CbmTrdDigi(149, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(150, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(151, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  digis.push_back(new CbmTrdDigi(152, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 3), nPermutations);
}

TEST(_GTestTrdClusterizer, CheckLeftAndRightBordersFullyTriggered)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  for (int row = 0; row < 24; ++row) {
    digis.push_back(new CbmTrdDigi(row * 144, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
    digis.push_back(new CbmTrdDigi((row + 1) * 144 - 1, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  }

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digis));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  //no row merging expected because rows only get merged when they have at least 3 digis
  EXPECT_EQ(clusters.size(), 48);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 1), 48);
}

/*
// 2-row clusters currently not supported by the new clusterizer
TEST(_GTestTrdClusterizer, Check2Row6PadCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  // clusters need to have approximately same COG, but displacement of 0 is not supported yet (should be though)
  // the order has to be the one below for the current clusterizer (Mar 17 2024)
  // it is e.g. important, that the selftriggers come first and the left NTs have lower charge than the right ones (also not good)
  digis.push_back(new CbmTrdDigi(503, MADDR, 1.0, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(502, MADDR, 0.5, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(504, MADDR, 0.9, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(647, MADDR, 1.0, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(646, MADDR, 0.5, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(648, MADDR, 0.9, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 6), nPermutations);
}

// 2-row clusters currently not supported by the new clusterizer
TEST(_GTestTrdClusterizer, Check2Row4Plus3PadCluster)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  digis.push_back(new CbmTrdDigi(503, MADDR, 1.0, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(504, MADDR, 1.0, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(502, MADDR, 0.5, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(505, MADDR, 0.9, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(647, MADDR, 1.0, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(646, MADDR, 0.5, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(648, MADDR, 0.9, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 7), nPermutations);
}
*/

TEST(_GTestTrdClusterizer, CheckTimeDistanceSmallEnough)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  //digis with time distance <= 1 Spadic clock cycle (62.5 ns) should be in same cluster
  digis.push_back(new CbmTrdDigi(503, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(504, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(505, MADDR, CHARGE_ST, 562, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(506, MADDR, CHARGE_NT, 562, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 4), nPermutations);
}

TEST(_GTestTrdClusterizer, CheckTimeDistanceTooHigh)
{
  cbm::algo::trd::Clusterizer* module = CreateTestModule();

  std::vector<CbmTrdDigi*> digis;

  //digis with time distance > 1 Spadic clock cycle (62.5 ns) should not be in same cluster
  digis.push_back(new CbmTrdDigi(503, MADDR, CHARGE_NT, 500, CbmTrdDigi::eTriggerType::kNeighbor, 0));
  digis.push_back(new CbmTrdDigi(504, MADDR, CHARGE_ST, 500, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(505, MADDR, CHARGE_ST, 563, CbmTrdDigi::eTriggerType::kSelf, 0));
  digis.push_back(new CbmTrdDigi(506, MADDR, CHARGE_NT, 563, CbmTrdDigi::eTriggerType::kNeighbor, 0));

  std::vector<CbmTrdDigi*> digisPermutated = PermutateDigis(digis);
  int nPermutations                        = digisPermutated.size() / digis.size();

  std::vector<cbm::algo::trd::Cluster> clusters = (*module)(CreateClusterizerInputVector(digisPermutated));

  std::vector<uint16_t> clusterSizes = GetClusterSizes(clusters);

  EXPECT_EQ(clusters.size(), nPermutations * 2);
  EXPECT_EQ(std::count(clusterSizes.begin(), clusterSizes.end(), 2), nPermutations * 2);
}
