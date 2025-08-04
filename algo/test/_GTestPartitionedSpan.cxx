/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "PartitionedSpan.h"
#include "PartitionedVector.h"

#include <gtest/gtest.h>

using namespace cbm::algo;

template<typename T>
void EXPECT_CONTAINER_EQ(gsl::span<T> a, std::vector<i32> b)
{
  EXPECT_EQ(a.size(), b.size());
  for (size_t i = 0; i < a.size(); ++i) {
    EXPECT_EQ(a[i], b[i]);
  }
}

class PartitionedSpanTest : public ::testing::Test {
 protected:
  std::vector<i32> fData;
  std::vector<size_t> fSizes;
  std::vector<size_t> fOffsets;
  std::vector<size_t> fOffsetsInvalid;
  std::vector<u32> fAddresses;
  std::vector<u32> fAddressesInvalid;

  void SetUp() override
  {
    fData             = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    fSizes            = {2, 4, 3};
    fOffsets          = {0, 2, 6, 9};
    fOffsetsInvalid   = {0, 2, 6};
    fAddresses        = {0x0, 0x100, 0x200};
    fAddressesInvalid = {0x0, 0x100};
  }

  template<typename T>  // T is either i32 or 'const i32'
  void Ensure(const PartitionedSpan<T>& vec)
  {
    EXPECT_EQ(vec.NElements(), 9);
    EXPECT_EQ(vec.NPartitions(), 3);

    EXPECT_EQ(vec.Size(0), 2);

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

TEST_F(PartitionedSpanTest, IsDefaultConstructable)
{
  PartitionedSpan<i32> vec;
  EXPECT_EQ(vec.NElements(), 0);
  EXPECT_EQ(vec.NPartitions(), 0);
}

TEST_F(PartitionedSpanTest, CanCreateWithPartitions)
{
  PartitionedSpan vec(fData, fOffsets, fAddresses);
  Ensure(vec);
}

TEST_F(PartitionedSpanTest, IsConstructableFromPartitionedVector)
{
  PartitionedVector vec(std::move(fData), fSizes, fAddresses);

  PartitionedSpan span(vec);
  Ensure(span);
}

TEST_F(PartitionedSpanTest, IsCopyConstructable)
{
  PartitionedSpan vec(fData, fOffsets, fAddresses);

  PartitionedSpan vecCopy(vec);
  Ensure(vecCopy);
}

TEST_F(PartitionedSpanTest, IsConstCopyConstructable)
{
  PartitionedSpan vec(fData, fOffsets, fAddresses);

  PartitionedSpan<const i32> vecCopy(vec);
  Ensure(vecCopy);
}

TEST_F(PartitionedSpanTest, ThrowsOnNumAddressesMismatchesNumPartions)
{
  EXPECT_THROW(PartitionedSpan vec(fData, fOffsets, fAddressesInvalid), std::runtime_error);
}

TEST_F(PartitionedSpanTest, ThrowsOnOffsetsMismatchesDataSize)
{
  EXPECT_THROW(PartitionedSpan vec(fData, fOffsetsInvalid, fAddresses), std::runtime_error);
}

TEST_F(PartitionedSpanTest, ThrowsOnOutOfBounds)
{
  PartitionedSpan vec(fData, fOffsets, fAddresses);

  EXPECT_THROW(vec[3], std::out_of_range);
  EXPECT_THROW(vec.Partition(3), std::out_of_range);
  EXPECT_THROW(vec.Address(3), std::out_of_range);
  EXPECT_THROW(vec.Size(3), std::out_of_range);
}
