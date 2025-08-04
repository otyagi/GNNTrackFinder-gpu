/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "PODVector.h"
#include "PartitionedVector.h"
#include "gtest/gtest.h"

using namespace cbm::algo;

template<typename T>
void EXPECT_CONTAINER_EQ(gsl::span<const T> a, std::vector<T> b)
{
  EXPECT_EQ(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a[i], b[i]);
  }
}

class PartitionedVectorTest : public ::testing::Test {
 protected:
  std::vector<i32> fData;
  std::vector<size_t> fSizes;
  std::vector<size_t> fSizesInvalid;
  std::vector<u32> fAddresses;
  std::vector<u32> fAddressesInvalid;

  void SetUp() override
  {
    fData             = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    fSizes            = {2, 4, 3};
    fSizesInvalid     = {2, 4, 4};
    fAddresses        = {0x0, 0x100, 0x200};
    fAddressesInvalid = {0x0, 0x100};
  }

  void Ensure(const PartitionedVector<i32>& vec)
  {
    EXPECT_EQ(vec.NElements(), 9);
    EXPECT_EQ(vec.NPartitions(), 3);

    EXPECT_EQ(vec.Size(0), 2);
    EXPECT_EQ(vec[0].size(), 2);
    EXPECT_EQ(vec.Address(0), 0x0);
    EXPECT_CONTAINER_EQ(vec[0], {1, 2});

    auto part = vec.Partition(0);
    EXPECT_EQ(part.first.size(), vec.Size(0));
    EXPECT_EQ(part.second, vec.Address(0));

    EXPECT_EQ(vec.Size(1), 4);
    EXPECT_EQ(vec[1].size(), 4);
    EXPECT_EQ(vec.Address(1), 0x100);
    EXPECT_CONTAINER_EQ(vec[1], {3, 4, 5, 6});

    part = vec.Partition(1);
    EXPECT_EQ(part.first.size(), vec.Size(1));
    EXPECT_EQ(part.second, vec.Address(1));

    EXPECT_EQ(vec.Size(2), 3);
    EXPECT_EQ(vec[2].size(), 3);
    EXPECT_EQ(vec.Address(2), 0x200);
    EXPECT_CONTAINER_EQ(vec[2], {7, 8, 9});

    part = vec.Partition(2);
    EXPECT_EQ(part.first.size(), vec.Size(2));
    EXPECT_EQ(part.second, vec.Address(2));
  }
};

TEST_F(PartitionedVectorTest, IsDefaultConstructable)
{
  PartitionedVector<i32> vec;
  EXPECT_EQ(vec.NElements(), 0);
  EXPECT_EQ(vec.NPartitions(), 0);
}

TEST_F(PartitionedVectorTest, CanCreateWithPartitions)
{
  PartitionedVector vec(std::move(fData), fSizes, fAddresses);
  Ensure(vec);
}

TEST_F(PartitionedVectorTest, ThrowsOnNumAddressesMismatchesNumPartions)
{
  EXPECT_THROW(PartitionedVector vec(std::move(fData), fSizes, fAddressesInvalid), std::runtime_error);
}

TEST_F(PartitionedVectorTest, ThrowsOnSizesMismatchesDataSize)
{
  EXPECT_THROW(PartitionedVector vec(std::move(fData), fSizesInvalid, fAddresses), std::runtime_error);
}

TEST_F(PartitionedVectorTest, ThrowsOnOutOfBounds)
{
  PartitionedVector vec(std::move(fData), fSizes, fAddresses);

  EXPECT_THROW(vec[3], std::out_of_range);
  EXPECT_THROW(vec.Partition(3), std::out_of_range);
  EXPECT_THROW(vec.Address(3), std::out_of_range);
  EXPECT_THROW(vec.Size(3), std::out_of_range);
}

TEST_F(PartitionedVectorTest, IsCopyConstructable)
{
  PartitionedVector vec(std::move(fData), fSizes, fAddresses);

  PartitionedVector vecCopy(vec);
  Ensure(vecCopy);
}
