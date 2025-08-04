/* Copyright (C) 2022 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Sebastian Heinemann */
#ifndef CBM_ALGO_BASE_POD_ALLOCATOR_H
#define CBM_ALGO_BASE_POD_ALLOCATOR_H

#include <cstdlib>
#include <type_traits>
#include <utility>

namespace cbm::algo
{
  /**
  * @brief Allocator for plain old data types.
  *
  * Custom allocator for plain old data types (POD) that does not initialize the allocated memory.
  */
  template<class T>
  class PODAllocator {
   public:
    // Eventually we should enable this assert, but not all Digis have a trivial constructor yet.
    // static_assert(std::is_trivially_constructible_v<T>, "PODAllocator only works for POD types");

    // Ensure T has no vtable
    static_assert(std::is_standard_layout_v<T>, "PODAllocator only works with types with standard layout");

    using value_type = T;

    T* allocate(size_t size) { return static_cast<T*>(std::malloc(size * sizeof(T))); }
    void deallocate(T* p_t, size_t) { std::free(p_t); }

    template<class... Args>
    void construct([[maybe_unused]] T* obj, Args&&... args)
    {
      if constexpr (sizeof...(args) > 0) new (obj) T(std::forward<Args>(args)...);
    }

    // Required for std::move
    bool operator==(const PODAllocator&) const { return true; }
    bool operator!=(const PODAllocator&) const { return false; }
  };
}  // namespace cbm::algo

#endif  // CBM_ALGO_BASE_POD_ALLOCATOR_H
