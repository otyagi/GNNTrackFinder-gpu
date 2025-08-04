/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_PARTITIONED_SPAN_H
#define CBM_ALGO_BASE_PARTITIONED_SPAN_H

#include "Definitions.h"

#include <array>
#include <gsl/span>
#include <stdexcept>
#include <vector>

namespace cbm::algo
{

  template<typename T, typename Allocator>
  class PartitionedVector;

  namespace detail
  {
    template<typename U, typename T>
    using EnableOnConst = std::enable_if_t<std::is_const_v<T> && std::is_same_v<U, std::remove_cv_t<T>>>;

    template<typename U, typename T>
    using EnableOnNonConst = std::enable_if_t<!std::is_const_v<T> && std::is_same_v<U, std::remove_cv_t<T>>>;
  }  // namespace detail

  template<typename T>
  class PartitionedSpan {

   public:
    PartitionedSpan() : fData(), fOffsets(NullOffset), fAdresses() { EnsureDimensions(); }

    // Intellisense and clang workaround, fails on template deduction with stl containers for some reason
    // #if  defined(__INTELLISENSE__) || defined(__clang__)
    template<typename Allocator>
    PartitionedSpan(std::vector<T, Allocator>& container, gsl::span<const size_t> offsets,
                    gsl::span<const u32> addresses)
      : fData(container)
      , fOffsets(offsets)
      , fAdresses(addresses)
    {
      EnsureDimensions();
    }

    // FIXME disable if T is non-const via SFINAE, otherwise get misleading compiler errors
    template<typename Allocator>
    PartitionedSpan(const std::vector<T, Allocator>& container, gsl::span<const size_t> offsets,
                    gsl::span<const u32> addresses)
      : fData(container)
      , fOffsets(offsets)
      , fAdresses(addresses)
    {
      EnsureDimensions();
    }

    template<size_t N>
    PartitionedSpan(std::array<T, N>& container, gsl::span<const size_t> offsets, gsl::span<const u32> addresses)
      : fData(container)
      , fOffsets(offsets)
      , fAdresses(addresses)
    {
      EnsureDimensions();
    }

    // FIXME disable if T is non-const via SFINAE
    template<size_t N>
    PartitionedSpan(const std::array<T, N>& container, gsl::span<const size_t> offsets, gsl::span<const u32> addresses)
      : fData(container)
      , fOffsets(offsets)
      , fAdresses(addresses)
    {
      EnsureDimensions();
    }
    // #endif

    PartitionedSpan(gsl::span<T> data, gsl::span<const size_t> offsets, gsl::span<const u32> addresses)
      : fData(data)
      , fOffsets(offsets)
      , fAdresses(addresses)
    {
      EnsureDimensions();
    }

    template<typename U, typename Allocator, typename = detail::EnableOnConst<U, T>>
    PartitionedSpan(const PartitionedVector<U, Allocator>& container)
      : fData(container.Data())
      , fOffsets(container.Offsets())
      , fAdresses(container.Addresses())
    {
      EnsureDimensions();
    }

    template<typename U, typename Allocator, typename = detail::EnableOnNonConst<U, T>>
    PartitionedSpan(PartitionedVector<U, Allocator>& container)
      : fData(container.Data())
      , fOffsets(container.Offsets())
      , fAdresses(container.Addresses())
    {
      EnsureDimensions();
    }

    template<typename U, typename = detail::EnableOnConst<U, T>>
    PartitionedSpan(PartitionedSpan<U> other)
      : fData(other.Data())
      , fOffsets(other.Offsets())
      , fAdresses(other.Addresses())
    {
    }

    gsl::span<T> operator[](size_t i) const
    {
      EnsureBounds(i);
      return UnsafePartitionSpan(i);
    }

    u32 Address(size_t i) const
    {
      EnsureBounds(i);
      return fAdresses[i];
    }

    std::pair<gsl::span<T>, u32> Partition(size_t i) const
    {
      EnsureBounds(i);
      return std::pair<gsl::span<T>, u32>(UnsafePartitionSpan(i), fAdresses[i]);
    }

    size_t NPartitions() const { return fAdresses.size(); }

    size_t Size(size_t i) const
    {
      EnsureBounds(i);
      return UnsafeSize(i);
    }

    size_t NElements() const { return fData.size(); }

    gsl::span<T> Data() const { return fData; }

    gsl::span<const u32> Addresses() const { return fAdresses; }

    gsl::span<const size_t> Offsets() const { return fOffsets; }

   private:
    // Required for default constructor, don't use std::array to avoid additional dependency
    static constexpr size_t NullOffset[1] = {0};

    gsl::span<T> fData;
    gsl::span<const size_t> fOffsets;
    gsl::span<const u32> fAdresses;

    // FIXME code duplication with PartitionedVector

    void EnsureDimensions() const
    {
      if (fOffsets.size() - 1 != fAdresses.size()) {
        throw std::runtime_error("PartitionedSpan: fOffsets.size() != fAdresses.size()");
      }
      if (fOffsets.front() != 0) throw std::runtime_error("PartitionedSpan: fOffsets.front() != 0");
      if (fOffsets.back() != fData.size()) {
        throw std::runtime_error("PartitionedSpan: fOffsets.back() != fData.size()");
      }
    }

    void EnsureBounds(size_t i) const
    {
      if (i >= fAdresses.size()) throw std::out_of_range("PartitionedSpan: index out of bounds");
    }

    size_t UnsafeSize(size_t i) const { return fOffsets[i + 1] - fOffsets[i]; }

    gsl::span<T> UnsafePartitionSpan(size_t i) const { return fData.subspan(fOffsets[i], UnsafeSize(i)); }
  };

  // template auto deduction
  template<typename T, template<typename> class Container>
  PartitionedSpan(Container<T>&, gsl::span<const size_t>, gsl::span<const u32>) -> PartitionedSpan<T>;

  template<typename T, template<typename> class Container>
  PartitionedSpan(const Container<T>&, gsl::span<const size_t>, gsl::span<const u32>) -> PartitionedSpan<const T>;

  template<typename T, typename Allocator>
  PartitionedSpan(PartitionedVector<T, Allocator>&) -> PartitionedSpan<T>;

  template<typename T, typename Allocator>
  PartitionedSpan(const PartitionedVector<T, Allocator>&) -> PartitionedSpan<const T>;

}  // namespace cbm::algo

#endif
