/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include "Definitions.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

namespace cbm::algo::yaml
{

  // clang-format off
  using FundamentalTypes = std::tuple< bool
                                     , u8, i8
                                     , u16, i16
                                     , u32, i32
                                     , u64, i64
                                     , f32, f64
                                     , std::string
// Clang on macOS somehow treats unsigned long as a different type than uint64_t
#ifdef __APPLE__
                                      , unsigned long, long
#endif
                                      >;
  // clang-format on

  template<typename T, typename Tuple>
  struct has_type;

  template<typename T, typename... Us>
  struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {
  };

  template<typename T>
  constexpr bool IsFundamental = has_type<T, FundamentalTypes>::value;

  template<typename T>
  constexpr bool IsEnum = std::is_enum_v<T>;

  template<typename T>
  constexpr bool IsScalar = IsFundamental<T> || IsEnum<T>;

  template<typename T, typename = std::enable_if_t<IsFundamental<T>>>
  constexpr std::string_view Typename()
  {
    if constexpr (std::is_same_v<bool, T>) {
      return "bool";
    }
    else if constexpr (std::is_same_v<u8, T>) {
      return "u8";
    }
    else if constexpr (std::is_same_v<i8, T>) {
      return "i8";
    }
    else if constexpr (std::is_same_v<u16, T>) {
      return "u16";
    }
    else if constexpr (std::is_same_v<i16, T>) {
      return "i16";
    }
    else if constexpr (std::is_same_v<u32, T>) {
      return "u32";
    }
    else if constexpr (std::is_same_v<i32, T>) {
      return "i32";
    }
    else if constexpr (std::is_same_v<f32, T>) {
      return "f32";
    }
    else if constexpr (std::is_same_v<f64, T>) {
      return "f64";
    }
    else if constexpr (std::is_same_v<std::string, T>) {
      return "string";
    }
    else {
      return "unknown";
    }
  }

  template<typename>
  struct is_std_vector : std::false_type {
  };

  template<typename T, typename A>
  struct is_std_vector<std::vector<T, A>> : std::true_type {
  };

  template<typename T>
  constexpr bool IsVector = is_std_vector<T>::value;

  template<typename>
  struct is_std_array : std::false_type {
  };

  template<typename T, std::size_t N>
  struct is_std_array<std::array<T, N>> : std::true_type {
  };

  template<typename T>
  constexpr bool IsArray = is_std_array<T>::value;

  template<typename>
  struct is_std_map : std::false_type {
  };

  template<typename K, typename V, typename C, typename A>
  struct is_std_map<std::map<K, V, C, A>> : std::true_type {
  };

  template<typename T>
  constexpr bool IsMap = is_std_map<T>::value;

  template<typename>
  struct is_std_set : std::false_type {
  };

  template<typename K, typename C, typename A>
  struct is_std_set<std::set<K, C, A>> : std::true_type {
  };

  template<typename>
  struct is_std_unordered_set : std::false_type {
  };

  template<typename K, typename H, typename E, typename A>
  struct is_std_unordered_set<std::unordered_set<K, H, E, A>> : std::true_type {
  };

  template<typename T>
  constexpr bool IsSet = is_std_set<T>::value || is_std_unordered_set<T>::value;


}  // namespace cbm::algo::yaml
