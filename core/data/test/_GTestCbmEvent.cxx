/* Copyright (C) 2016-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "CbmDefs.h"
#include "CbmEvent.h"
#include "CbmMatch.h"
#include "CbmVertex.h"
#include "compareVertex.h"

#include <TMatrixTSym.h>

#include <vector>

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>


struct defaultCbmEvent {

  std::vector<uint32_t> mctrack{11, 23};
  std::vector<uint32_t> stspoint{1};
  std::vector<uint32_t> stsdigi{2};
  std::vector<uint32_t> stscluster{4};
  std::vector<uint32_t> stshit{5};
  std::vector<uint32_t> ststrack{13, 12, 11};

  std::vector<float> covMatrix = {0., 1., 2., 1., 3., 4., 2., 4., 5.};

  CbmVertex GetDefaultVertex()
  {
    TMatrixFSym Cov(3);
    Cov(0, 0) = covMatrix[0];  // 0 1 2
    Cov(0, 1) = covMatrix[1];  // 1 3 4
    Cov(0, 2) = covMatrix[2];  // 2 4 5
    Cov(1, 0) = covMatrix[3];
    Cov(1, 1) = covMatrix[4];
    Cov(1, 2) = covMatrix[5];
    Cov(2, 0) = covMatrix[6];
    Cov(2, 1) = covMatrix[7];
    Cov(2, 2) = covMatrix[8];
    CbmVertex testVertex{"Vertex", "Vertex", 1., 2., 3., 4., 5, 6, Cov};
    return testVertex;
  }
};

void compareEventDataMembers(CbmEvent& test, int32_t evnumber, double starttime, double endtime, double tzero,
                             int32_t numobjects, CbmMatch* match, CbmVertex* vertex)
{
  int32_t retValInt{-111};
  double retValDouble{-111.};

  retValInt = test.GetNumber();
  EXPECT_EQ(evnumber, retValInt);

  retValDouble = test.GetStartTime();
  EXPECT_EQ(starttime, retValDouble);

  retValDouble = test.GetEndTime();
  EXPECT_EQ(endtime, retValDouble);

  retValDouble = test.GetTzero();
  EXPECT_EQ(tzero, retValDouble);

  retValInt = test.GetNofData();
  EXPECT_EQ(numobjects, retValInt);

  EXPECT_EQ(match, test.GetMatch());

  if (!vertex) {
    double val[6] = {0., 0., 0., 0., 0., 0.};
    compareVertexDataMembers(*(test.GetVertex()), 0., 0., 0., 0., 0, 0, val);
  }
  else {
    double val[6] = {vertex->GetCovariance(0, 0), vertex->GetCovariance(0, 1), vertex->GetCovariance(0, 2),
                     vertex->GetCovariance(1, 1), vertex->GetCovariance(1, 2), vertex->GetCovariance(2, 2)};
    compareVertexDataMembers(*(test.GetVertex()), vertex->GetX(), vertex->GetY(), vertex->GetZ(), vertex->GetChi2(),
                             vertex->GetNDF(), vertex->GetNTracks(), val);
  }
}

void compareEventMap(CbmEvent& test, int32_t numobjects, int32_t numobjectstype, ECbmDataType type,
                     std::vector<uint32_t> indices)
{
  int32_t retValInt{-111};
  uint32_t retValUInt{111};

  retValInt = test.GetNofData();
  EXPECT_EQ(numobjects, retValInt);

  retValInt = test.GetNofData(type);
  EXPECT_EQ(numobjectstype, retValInt);

  if (numobjectstype > 0) {
    for (int32_t i = 0; i < numobjectstype; ++i) {
      retValUInt = test.GetIndex(type, i);
      EXPECT_EQ(indices[i], retValUInt);
    }
  }
}

CbmEvent ConstructTestObject(defaultCbmEvent& defEvent)
{

  CbmEvent test{-111, 1., 2.};

  for (auto const& data : defEvent.mctrack)
    test.AddData(ECbmDataType::kMCTrack, data);
  for (auto const& data : defEvent.stspoint)
    test.AddData(ECbmDataType::kStsPoint, data);
  for (auto const& data : defEvent.stsdigi)
    test.AddData(ECbmDataType::kStsDigi, data);
  for (auto const& data : defEvent.stscluster)
    test.AddData(ECbmDataType::kStsCluster, data);
  for (auto const& data : defEvent.stshit)
    test.AddData(ECbmDataType::kStsHit, data);
  for (auto const& data : defEvent.ststrack)
    test.AddData(ECbmDataType::kStsTrack, data);

  return test;
}

TEST(_GTestCbmEvent, CheckDefaultConstructor)
{
  CbmEvent test;
  compareEventDataMembers(test, -1, 0., 0., -999999., 0, nullptr, nullptr);
}

TEST(_GTestCbmEvent, CheckStandardConstructor)
{
  CbmEvent test{-111};
  {
    SCOPED_TRACE("CheckStandardConstructor");
    compareEventDataMembers(test, -111, 0., 0., -999999., 0, nullptr, nullptr);
  }
}

TEST(_GTestCbmEvent, CheckAssignmentConstructor)
{

  defaultCbmEvent defEvent;

  CbmEvent test = ConstructTestObject(defEvent);

  CbmEvent test_assign = test;

  // The original object should not change
  compareEventDataMembers(test, -111, 1., 2., -999999., 9, nullptr, nullptr);
  compareEventMap(test, 9, 2, ECbmDataType::kMCTrack, defEvent.mctrack);
  compareEventMap(test, 9, 1, ECbmDataType::kStsPoint, defEvent.stspoint);
  compareEventMap(test, 9, 1, ECbmDataType::kStsDigi, defEvent.stsdigi);
  compareEventMap(test, 9, 1, ECbmDataType::kStsCluster, defEvent.stscluster);
  compareEventMap(test, 9, 1, ECbmDataType::kStsHit, defEvent.stshit);
  compareEventMap(test, 9, 3, ECbmDataType::kStsTrack, defEvent.ststrack);

  // the assigned constructed object should be identical
  compareEventDataMembers(test_assign, -111, 1., 2., -999999., 9, nullptr, nullptr);
  compareEventMap(test_assign, 9, 2, ECbmDataType::kMCTrack, defEvent.mctrack);
  compareEventMap(test_assign, 9, 1, ECbmDataType::kStsPoint, defEvent.stspoint);
  compareEventMap(test_assign, 9, 1, ECbmDataType::kStsDigi, defEvent.stsdigi);
  compareEventMap(test_assign, 9, 1, ECbmDataType::kStsCluster, defEvent.stscluster);
  compareEventMap(test_assign, 9, 1, ECbmDataType::kStsHit, defEvent.stshit);
  compareEventMap(test_assign, 9, 3, ECbmDataType::kStsTrack, defEvent.ststrack);
}

TEST(_GTestCbmEvent, CheckCopyConstructor)
{

  defaultCbmEvent defEvent;

  CbmEvent test = ConstructTestObject(defEvent);

  CbmEvent test_copy(test);

  // The original object should not change
  compareEventDataMembers(test, -111, 1., 2., -999999., 9, nullptr, nullptr);
  compareEventMap(test, 9, 2, ECbmDataType::kMCTrack, defEvent.mctrack);
  compareEventMap(test, 9, 1, ECbmDataType::kStsPoint, defEvent.stspoint);
  compareEventMap(test, 9, 1, ECbmDataType::kStsDigi, defEvent.stsdigi);
  compareEventMap(test, 9, 1, ECbmDataType::kStsCluster, defEvent.stscluster);
  compareEventMap(test, 9, 1, ECbmDataType::kStsHit, defEvent.stshit);
  compareEventMap(test, 9, 3, ECbmDataType::kStsTrack, defEvent.ststrack);

  // the assigned constructed object should be identical
  compareEventDataMembers(test_copy, -111, 1., 2., -999999., 9, nullptr, nullptr);
  compareEventMap(test_copy, 9, 2, ECbmDataType::kMCTrack, defEvent.mctrack);
  compareEventMap(test_copy, 9, 1, ECbmDataType::kStsPoint, defEvent.stspoint);
  compareEventMap(test_copy, 9, 1, ECbmDataType::kStsDigi, defEvent.stsdigi);
  compareEventMap(test_copy, 9, 1, ECbmDataType::kStsCluster, defEvent.stscluster);
  compareEventMap(test_copy, 9, 1, ECbmDataType::kStsHit, defEvent.stshit);
  compareEventMap(test_copy, 9, 3, ECbmDataType::kStsTrack, defEvent.ststrack);
}

TEST(_GTestCbmEvent, CheckMoveConstructor)
{
  defaultCbmEvent defEvent;

  CbmEvent test = ConstructTestObject(defEvent);

  CbmEvent test_move = std::move(test);

  // The original object should be the default constructed
  // POd data is not moved but copied such that some data members are still
  // present
  compareEventDataMembers(test, -111, 1., 2., -999999., 0, nullptr, nullptr);

  // the moved object should be identical to the initial one
  compareEventDataMembers(test_move, -111, 1., 2., -999999., 9, nullptr, nullptr);
  compareEventMap(test_move, 9, 2, ECbmDataType::kMCTrack, defEvent.mctrack);
  compareEventMap(test_move, 9, 1, ECbmDataType::kStsPoint, defEvent.stspoint);
  compareEventMap(test_move, 9, 1, ECbmDataType::kStsDigi, defEvent.stsdigi);
  compareEventMap(test_move, 9, 1, ECbmDataType::kStsCluster, defEvent.stscluster);
  compareEventMap(test_move, 9, 1, ECbmDataType::kStsHit, defEvent.stshit);
  compareEventMap(test_move, 9, 3, ECbmDataType::kStsTrack, defEvent.ststrack);
}


TEST(_GTestCbmEvent, TestSwap)
{


  CbmEvent test1{-111};
  {
    SCOPED_TRACE("TestSwap: Check first object before swap");
    compareEventDataMembers(test1, -111, 0., 0., -999999., 0, nullptr, nullptr);
  }

  std::vector<uint32_t> mctrack;
  mctrack.push_back(11);

  CbmEvent test2{-333, 1., 2.};
  test2.AddData(ECbmDataType::kMCTrack, 11);
  {
    SCOPED_TRACE("TestSwap: Check second object before swap");
    compareEventDataMembers(test2, -333, 1., 2., -999999., 1, nullptr, nullptr);
    compareEventMap(test2, 1, 1, ECbmDataType::kMCTrack, mctrack);
  }

  test1.Swap(test2);

  {
    SCOPED_TRACE("TestSwap: Check first object after swap");
    compareEventDataMembers(test1, -333, 1., 2., -999999., 1, nullptr, nullptr);
    compareEventMap(test1, 1, 1, ECbmDataType::kMCTrack, mctrack);
  }

  {
    SCOPED_TRACE("TestSwap: Check second object after swap");
    compareEventDataMembers(test2, -111, 0., 0., -999999., 0, nullptr, nullptr);
  }
}

TEST(_GTestCbmEvent, CheckSettersAndGetters)
{
  CbmEvent test{-111, 1., 2.};
  {
    SCOPED_TRACE("CheckSettersAndGetters: Initial Test");
    compareEventDataMembers(test, -111, 1., 2., -999999., 0, nullptr, nullptr);
  }

  test.SetStartTime(-23.);
  {
    SCOPED_TRACE("CheckSettersAndGetters: SetStartTime");
    compareEventDataMembers(test, -111, -23., 2., -999999., 0, nullptr, nullptr);
  }

  test.SetEndTime(-45.);
  {
    SCOPED_TRACE("CheckSettersAndGetters: SetEndTime");
    compareEventDataMembers(test, -111, -23., -45., -999999., 0, nullptr, nullptr);
  }

  test.SetTzero(-345.);
  {
    SCOPED_TRACE("CheckSettersAndGetters: SetTzero");
    compareEventDataMembers(test, -111, -23., -45., -345., 0, nullptr, nullptr);
  }

  CbmMatch* testMatch = new CbmMatch();
  test.SetMatch(testMatch);
  {
    SCOPED_TRACE("CheckSettersAndGetters: SetMatch");
    compareEventDataMembers(test, -111, -23., -45., -345., 0, testMatch, nullptr);
  }
}

TEST(_GTestCbmEvent, CheckAddData)
{
  CbmEvent test{-111, 1., 2.};
  {
    SCOPED_TRACE("CheckAddData: Initial Test");
    compareEventDataMembers(test, -111, 1., 2., -999999., 0, nullptr, nullptr);
  }

  std::vector<uint32_t> mctrack;
  std::vector<uint32_t> stspoint;
  std::vector<uint32_t> stsdigi;
  std::vector<uint32_t> stscluster;
  std::vector<uint32_t> stshit;
  std::vector<uint32_t> ststrack;


  mctrack.push_back(11);
  test.AddData(ECbmDataType::kMCTrack, 11);
  {
    SCOPED_TRACE("CheckAddData: Add first MCTrack");
    compareEventDataMembers(test, -111, 1., 2., -999999., 1, nullptr, nullptr);
    compareEventMap(test, 1, 1, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 1, 0, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 1, 0, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 1, 0, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 1, 0, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 1, 0, ECbmDataType::kStsTrack, ststrack);
  }

  mctrack.push_back(23);
  test.AddData(ECbmDataType::kMCTrack, 23);
  {
    SCOPED_TRACE("CheckAddData: Add second MCTrack");
    compareEventDataMembers(test, -111, 1., 2., -999999., 2, nullptr, nullptr);
    compareEventMap(test, 2, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 2, 0, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 2, 0, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 2, 0, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 2, 0, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 2, 0, ECbmDataType::kStsTrack, ststrack);
  }

  // Test special case where index for existing data type isn't present
  uint32_t retValUInt = test.GetIndex(ECbmDataType::kMCTrack, 25);
  EXPECT_EQ(-2, retValUInt);

  // Test special case where data type isn't existing
  retValUInt = test.GetIndex(ECbmDataType::kTrdHit, 25);
  EXPECT_EQ(-1, retValUInt);

  stspoint.push_back(1);
  test.AddData(ECbmDataType::kStsPoint, 1);
  {
    SCOPED_TRACE("CheckAddData: Add StsPoint");
    compareEventDataMembers(test, -111, 1., 2., -999999., 3, nullptr, nullptr);
    compareEventMap(test, 3, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 3, 1, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 3, 0, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 3, 0, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 3, 0, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 3, 0, ECbmDataType::kStsTrack, ststrack);
  }

  stsdigi.push_back(2);
  test.AddData(ECbmDataType::kStsDigi, 2);
  {
    SCOPED_TRACE("CheckAddData: Add StsDigi");
    compareEventDataMembers(test, -111, 1., 2., -999999., 4, nullptr, nullptr);
    compareEventMap(test, 4, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 4, 1, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 4, 1, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 4, 0, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 4, 0, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 4, 0, ECbmDataType::kStsTrack, ststrack);
  }

  stscluster.push_back(3);
  test.AddData(ECbmDataType::kStsCluster, 3);
  {
    SCOPED_TRACE("CheckAddData: Add StsCluster");
    compareEventDataMembers(test, -111, 1., 2., -999999., 5, nullptr, nullptr);
    compareEventMap(test, 5, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 5, 1, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 5, 1, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 5, 1, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 5, 0, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 5, 0, ECbmDataType::kStsTrack, ststrack);
  }

  stshit.push_back(4);
  test.AddData(ECbmDataType::kStsHit, 4);
  {
    SCOPED_TRACE("CheckAddData: Add StsHit");
    compareEventDataMembers(test, -111, 1., 2., -999999., 6, nullptr, nullptr);
    compareEventMap(test, 6, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 6, 1, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 6, 1, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 6, 1, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 6, 1, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 6, 0, ECbmDataType::kStsTrack, ststrack);
  }

  ststrack.push_back(5);
  test.AddData(ECbmDataType::kStsTrack, 5);
  {
    SCOPED_TRACE("CheckAddData: Add StsTrack");
    compareEventDataMembers(test, -111, 1., 2., -999999., 7, nullptr, nullptr);
    compareEventMap(test, 7, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 7, 1, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 7, 1, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 7, 1, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 7, 1, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 7, 1, ECbmDataType::kStsTrack, ststrack);
  }

  ststrack.push_back(6);
  test.AddStsTrack(6);
  {
    SCOPED_TRACE("CheckAddData: Add StsTrack via AddStsTrack");
    compareEventDataMembers(test, -111, 1., 2., -999999., 8, nullptr, nullptr);
    compareEventMap(test, 8, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 8, 1, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 8, 1, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 8, 1, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 8, 1, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 8, 2, ECbmDataType::kStsTrack, ststrack);
  }

  retValUInt = test.GetStsTrackIndex(0);
  EXPECT_EQ(5, retValUInt);

  retValUInt = test.GetStsTrackIndex(1);
  EXPECT_EQ(6, retValUInt);

  retValUInt = test.GetStsTrackIndex(2);
  EXPECT_EQ(-2, retValUInt);

  retValUInt = test.GetNofStsTracks();
  EXPECT_EQ(2, retValUInt);

  ststrack.clear();
  ststrack.push_back(13);
  ststrack.push_back(12);
  ststrack.push_back(11);

  test.SetStsTracks(ststrack);
  retValUInt = test.GetNofStsTracks();
  EXPECT_EQ(3, retValUInt);
  {
    SCOPED_TRACE("CheckAddData: Add StsTracks via StsStsTrack");
    compareEventDataMembers(test, -111, 1., 2., -999999., 9, nullptr, nullptr);
    compareEventMap(test, 9, 2, ECbmDataType::kMCTrack, mctrack);
    compareEventMap(test, 9, 1, ECbmDataType::kStsPoint, stspoint);
    compareEventMap(test, 9, 1, ECbmDataType::kStsDigi, stsdigi);
    compareEventMap(test, 9, 1, ECbmDataType::kStsCluster, stscluster);
    compareEventMap(test, 9, 1, ECbmDataType::kStsHit, stshit);
    compareEventMap(test, 9, 3, ECbmDataType::kStsTrack, ststrack);
  }
}

TEST(_GTestCbmEvent, SortIndices)
{

  defaultCbmEvent defEvent;

  CbmEvent test = ConstructTestObject(defEvent);

  {
    SCOPED_TRACE("CheckAddData: Check indices after sorting");
    compareEventDataMembers(test, -111, 1., 2., -999999., 9, nullptr, nullptr);
    compareEventMap(test, 9, 2, ECbmDataType::kMCTrack, defEvent.mctrack);
    compareEventMap(test, 9, 1, ECbmDataType::kStsPoint, defEvent.stspoint);
    compareEventMap(test, 9, 1, ECbmDataType::kStsDigi, defEvent.stsdigi);
    compareEventMap(test, 9, 1, ECbmDataType::kStsCluster, defEvent.stscluster);
    compareEventMap(test, 9, 1, ECbmDataType::kStsHit, defEvent.stshit);
    compareEventMap(test, 9, 3, ECbmDataType::kStsTrack, defEvent.ststrack);
  }

  std::vector<uint32_t> ststrack_sorted = {11, 12, 13};
  test.SortIndices();

  {
    SCOPED_TRACE("CheckAddData: Check indices after sorting");
    compareEventDataMembers(test, -111, 1., 2., -999999., 9, nullptr, nullptr);
    compareEventMap(test, 9, 2, ECbmDataType::kMCTrack, defEvent.mctrack);
    compareEventMap(test, 9, 1, ECbmDataType::kStsPoint, defEvent.stspoint);
    compareEventMap(test, 9, 1, ECbmDataType::kStsDigi, defEvent.stsdigi);
    compareEventMap(test, 9, 1, ECbmDataType::kStsCluster, defEvent.stscluster);
    compareEventMap(test, 9, 1, ECbmDataType::kStsHit, defEvent.stshit);
    compareEventMap(test, 9, 3, ECbmDataType::kStsTrack, ststrack_sorted);
  }
}

TEST(_GTestCbmEvent, CheckClearData)
{

  defaultCbmEvent defEvent;

  CbmEvent test = ConstructTestObject(defEvent);

  test.ClearData(ECbmDataType::kStsTrack);
  compareEventDataMembers(test, -111, 1., 2., -999999., 6, nullptr, nullptr);
}

TEST(_GTestCbmEvent, CheckClear)
{

  defaultCbmEvent defEvent;

  CbmEvent test = ConstructTestObject(defEvent);

  test.Clear("");
  compareEventDataMembers(test, -111, 1., 2., -999999., 0, nullptr, nullptr);
}


TEST(_GTestCbmEvent, CheckPrintFunction)
{

  defaultCbmEvent defEvent;

  CbmEvent test = ConstructTestObject(defEvent);

  EXPECT_STREQ("Event -111 at t = 1 ns. Registered data types: 6, data "
               "objects: 9, without matches\n          -- Data type 0, number "
               "of data 2\n          -- Data type 200, number of data 1\n      "
               "    -- Data type 201, number of data 1\n          -- Data type "
               "202, number of data 1\n          -- Data type 203, number of "
               "data 1\n          -- Data type 204, number of data 3\n",
               test.ToString().c_str());
}


TEST(_GTestCbmEvent, CheckSetVertex)
{

  TMatrixFSym Cov(3);
  Cov(0, 0) = 0.;  // 0 1 2
  Cov(0, 1) = 1.;  // 1 3 4
  Cov(0, 2) = 2.;  // 2 4 5
  Cov(1, 0) = 1.;
  Cov(1, 1) = 3.;
  Cov(1, 2) = 4.;
  Cov(2, 0) = 2.;
  Cov(2, 1) = 4.;
  Cov(2, 2) = 5.;
  CbmVertex testVertex{"Vertex", "Vertex", 1., 2., 3., 4., 5, 6, Cov};

  CbmEvent test;
  test.SetVertex(1., 2., 3., 4., 5, 6, Cov);
  {
    SCOPED_TRACE("CheckSetVertex");
    compareEventDataMembers(test, -1, 0., 0., -999999., 0, nullptr, &testVertex);
  }
}
