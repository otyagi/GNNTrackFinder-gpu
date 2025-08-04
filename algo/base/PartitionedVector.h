/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_PARTITIONED_VECTOR_H
#define CBM_ALGO_BASE_PARTITIONED_VECTOR_H

#include "Definitions.h"
#include "util/PODAllocator.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>

#include <gsl/span>
#include <vector>

namespace cbm::algo
{
  template<typename T>
  class PartitionedSpan;

  /**
   * @brief A vector that is partitioned into multiple subvectors.
   *
   * @tparam T Type of the elements
   * @tparam Allocator Allocator for the underlying container
   *
   * @note The underlying container is contiguous in memory.
   */
  template<typename T, class Allocator = std::allocator<T>>
  class PartitionedVector {

   public:
    using Container_t = std::vector<T, Allocator>;  //< Underlying container type

    /**
     * @brief Default constructor. Creates an empty vector.
     */
    PartitionedVector() : fData(), fOffsets({0}), fAdresses() { EnsureDimensions(); }

    /**
     * @brief Constructor. Creates a vector with n partitions.
     *
     * @param data Underlying data. Assusmes that the data is already partitioned and takes ownership of it.
     * @param sizes Sizes of each partitions
     * @param addresses Hardware addresses of each partition
     *
     * @note Requires sizes.size() == addresses.size()
     */
    PartitionedVector(Container_t&& data, gsl::span<const size_t> sizes, gsl::span<const u32> addresses)
      : fData(std::move(data))
      , fOffsets()
      , fAdresses(addresses.begin(), addresses.end())
    {
      ComputeOffsets(sizes);
      EnsureDimensions();
    }

    /**
     * @brief Copy constructor. Copy the data from other vector.
     */
    template<typename OtherAllocator>
    PartitionedVector(const PartitionedVector<T, OtherAllocator>& other)
      : fData(other.Data().begin(), other.Data().end())
      , fOffsets(other.Offsets())
      , fAdresses(other.Addresses())
    {
      // TODO: this check is overkill? We already know that the dimensions are correct,
      // since they were already checked in the other vector
      EnsureDimensions();
    }

    template<typename U>
    PartitionedVector(PartitionedSpan<U> other)
      : fData(other.Data().begin(), other.Data().end())
      , fOffsets(other.Offsets().begin(), other.Offsets().end())
      , fAdresses(other.Addresses().begin(), other.Addresses().end())
    {
      EnsureDimensions();
    }

    /**
     * @brief Access data at partition i.
     */
    gsl::span<T> operator[](size_t i)
    {
      EnsureBounds(i);
      return UnsafePartitionSpan(i);
    }

    /**
     * @brief Access data at partition i.
     */
    gsl::span<const T> operator[](size_t i) const
    {
      EnsureBounds(i);
      return UnsafePartitionSpan(i);
    }

    /**
     * @brief Get the hardware address of partition i.
     */
    u32 Address(size_t i) const
    {
      EnsureBounds(i);
      return fAdresses[i];
    }

    /**
     * @brief Get a pair of the data and the hardware address of partition i.
     */
    std::pair<gsl::span<T>, u32> Partition(size_t i)
    {
      EnsureBounds(i);
      return std::pair<gsl::span<T>, u32>(UnsafePartitionSpan(i), fAdresses[i]);
    }

    /**
     * @brief Get a pair of the data and the hardware address of partition i.
     */
    std::pair<gsl::span<const T>, u32> Partition(size_t i) const
    {
      EnsureBounds(i);
      return std::pair<gsl::span<const T>, u32>(UnsafePartitionSpan(i), fAdresses[i]);
    }

    /**
     * @brief Get the number of partitions.
     */
    size_t NPartitions() const { return fAdresses.size(); }

    /**
     * @brief Get the size of partition i.
     */
    size_t Size(size_t i) const
    {
      EnsureBounds(i);
      return UnsafeSize(i);
    }

    /**
     * @brief Get the total number of elements in the container across all partitions.
     */
    size_t NElements() const { return fData.size(); }

    /**
     * @brief Return total size in bytes of the underlying data.
     */
    size_t SizeBytes() const { return fData.size() * sizeof(T); }

    /**
     * @brief Get the underlying data.
     */
    gsl::span<T> Data() { return fData; }

    /**
     * @brief Get the underlying data.
     */
    gsl::span<const T> Data() const { return fData; }

    /**
     * @brief Get the addresses.
     */
    const std::vector<u32>& Addresses() const { return fAdresses; }

    /**
     * @brief Get the underlying offsets.
     */
    const std::vector<size_t>& Offsets() const { return fOffsets; }

   private:
    Container_t fData;             //< Data
    std::vector<size_t> fOffsets;  // < Offsets of the partitions in fData
    std::vector<u32> fAdresses;    //< Hardware addresses of the partitions

    void EnsureDimensions() const
    {
      if (fOffsets.size() - 1 != fAdresses.size()) {
        throw std::runtime_error("PartitionedVector: fOffsets.size() != fAdresses.size()");
      }
      if (fOffsets.front() != 0) {
        throw std::runtime_error("PartitionedVector: fOffsets.front() != 0");
      }
      if (fOffsets.back() != fData.size()) {
        throw std::runtime_error("PartitionedVector: fOffsets.back() != fData.size()");
      }
    }

    void EnsureBounds(size_t i) const
    {
      if (i >= fAdresses.size()) throw std::out_of_range("PartitionedVector: index out of bounds");
    }

    void ComputeOffsets(gsl::span<const size_t> sizes)
    {
      fOffsets.reserve(sizes.size() + 1);
      fOffsets.push_back(0);
      for (auto n : sizes) {
        fOffsets.push_back(fOffsets.back() + n);
      }
    }

    size_t UnsafeSize(size_t i) const { return fOffsets[i + 1] - fOffsets[i]; }

    gsl::span<T> UnsafePartitionSpan(size_t i) { return gsl::span<T>(fData.data() + fOffsets[i], UnsafeSize(i)); }

    gsl::span<const T> UnsafePartitionSpan(size_t i) const
    {
      return gsl::span<const T>(fData.data() + fOffsets[i], UnsafeSize(i));
    }

   private:  // serialization
    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar& fData;
      ar& fOffsets;
      ar& fAdresses;
    }
  };

  template<typename T>
  using PartitionedPODVector = PartitionedVector<T, PODAllocator<T>>;

}  // namespace cbm::algo

#endif
