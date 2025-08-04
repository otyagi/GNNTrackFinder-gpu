/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include <cstddef>

#include <xpu/defines.h>

/**
 * @file PaddedValue.h
 * @brief This file contains the definition of the PaddedValue class.
 */

namespace cbm::algo
{

  /**
   * @brief A class that represents a value with padding to a certain size.
   * @tparam T The type of the value.
   * @tparam N Number of bytes the value should be padded to.
   *
   * @note This class is useful for aligning values to a certain size, e.g. to ensure that atomic counters are spread across different cache lines. (Prevent false sharing)
   */
  template<typename T, size_t N>
  class PaddedValue {
    static_assert(N % alignof(T) == 0, "N must be a multiple of alignof(T)");

   public:
    XPU_D PaddedValue() = default;
    XPU_D PaddedValue(const T& value) : fValue(value) {}

    XPU_D PaddedValue(const PaddedValue& other) : fValue(other.fValue) {}
    XPU_D PaddedValue& operator=(const PaddedValue& other)
    {
      fValue = other.fValue;
      return *this;
    }

    XPU_D PaddedValue(PaddedValue&& other) : fValue(std::move(other.fValue)) {}
    XPU_D PaddedValue& operator=(PaddedValue&& other)
    {
      fValue = std::move(other.fValue);
      return *this;
    }

    XPU_D T& operator=(const T& value)
    {
      fValue = value;
      return fValue;
    }

    XPU_D T& Get() { return fValue; }
    XPU_D const T& Get() const { return fValue; }

    XPU_D T* operator&() { return &fValue; }
    XPU_D const T* operator&() const { return &fValue; }

    XPU_D T& operator*() { return fValue; }
    XPU_D const T& operator*() const { return fValue; }

    XPU_D operator T&() { return fValue; }
    XPU_D operator const T&() const { return fValue; }

    XPU_D operator T*() { return &fValue; }
    XPU_D operator const T*() const { return &fValue; }

    XPU_D T* operator->() { return &fValue; }
    XPU_D const T* operator->() const { return &fValue; }

   private:
    T fValue;
    unsigned char fPadding[N - sizeof(T)];
  };

  inline constexpr size_t SizeOfCacheLine = 64;

  template<typename T>
  using PaddedToCacheLine = PaddedValue<T, SizeOfCacheLine>;

}  // namespace cbm::algo
