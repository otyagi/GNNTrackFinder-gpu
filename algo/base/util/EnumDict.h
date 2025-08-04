/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#ifndef CBM_ALGO_BASE_UTIL_SERIALIZABLEENUM_H
#define CBM_ALGO_BASE_UTIL_SERIALIZABLEENUM_H

#include <boost/algorithm/string/predicate.hpp>

#include <algorithm>
#include <iosfwd>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <xpu/defines.h>

namespace cbm::algo
{
  namespace detail
  {
    template<typename T>
    using EnumDict_t = std::vector<std::pair<std::string_view, T>>;

    template<typename T>
    inline const EnumDict_t<T> EnumDict;

    template<typename T>
    struct EnumHasDict : std::false_type {
    };

    template<typename T>
    inline constexpr bool EnumHasDict_v = EnumHasDict<T>::value;

    template<typename T, typename = std::enable_if_t<detail::EnumHasDict_v<T>>>
    std::vector<std::string_view> ValidEntries()
    {
      std::vector<std::string_view> entries;
      for (const auto& pair : EnumDict<T>) {
        entries.push_back(pair.first);
      }
      return entries;
    }

    void RaiseUnknownEntry(std::string_view str, const std::vector<std::string_view>& validEntries);
  }  // namespace detail

  template<typename T, typename = std::enable_if_t<detail::EnumHasDict_v<T>>>
  std::optional<T> FromString(std::string_view str, bool caseSensitive = false)
  {
    const auto& dict = detail::EnumDict<T>;
    auto it          = std::find_if(dict.begin(), dict.end(), [&](const auto& pair) {
      if (caseSensitive)
        return pair.first == str;
      else
        return boost::iequals(pair.first, str);
    });
    if (it == dict.end()) return std::nullopt;
    return it->second;
  }

  template<typename T, typename = std::enable_if_t<detail::EnumHasDict_v<T>>>
  std::string_view ToString(T t)
  {
    const auto& dict = detail::EnumDict<T>;
    auto it          = std::find_if(dict.begin(), dict.end(), [t](const auto& pair) { return pair.second == t; });
    if (it == dict.end()) throw std::runtime_error(fmt::format("Entry {} for enum missing!", static_cast<int>(t)));
    return it->first;
  }
}  // namespace cbm::algo

#if XPU_IS_CPU
/**
   * @brief Convert enums to strings and back.
   *
   * @param type The enum type.
   *
   * Example:
   * @code{.cpp}
   * enum class Detector {
   *  STS,
   *  TOF,
   * };
   *
   * CBM_ENUM_DICT(Detector,
   *  {"sts", Detector::STS},
   *  {"tof", Detector::TOF}
   * );
   *
   * // Use it like this:
   * L_(info) << ToString(Detector::STS); // Prints "sts"
   *
   * std::optional<Detector> d = FromString<Detector>("tof"); // *d == Detector::TOF
   * std::optional<Detector> d2 = FromString<Detector>("invalid"); // d2 == std::nullopt
   * @endcode
   */
#define CBM_ENUM_DICT(type, ...)                                                                                       \
  template<>                                                                                                           \
  inline const cbm::algo::detail::EnumDict_t<type> cbm::algo::detail::EnumDict<type> = {__VA_ARGS__};                  \
  template<>                                                                                                           \
  struct cbm::algo::detail::EnumHasDict<type> : std::true_type {                                                       \
  }
#else  // XPU_IS_CPU
// Disable macro in GPU code, causes some issues with nvcc
#define CBM_ENUM_DICT(type, ...)
#endif  // XPU_IS_CPU

// Stream operators for enums
// Placed in global namespace to be found by ADL e.g. for std::ostream_iterator
namespace std
{
  template<typename T, typename = std::enable_if_t<cbm::algo::detail::EnumHasDict_v<T>>>
  std::ostream& operator<<(std::ostream& os, T t)
  {
    os << cbm::algo::ToString(t);
    return os;
  }

  template<typename T, typename = std::enable_if_t<cbm::algo::detail::EnumHasDict_v<T>>>
  std::istream& operator>>(std::istream& is, T& t)
  {
    std::string str;
    is >> str;
    auto maybet = cbm::algo::FromString<T>(str);

    if (!maybet) {
      cbm::algo::detail::RaiseUnknownEntry(str, cbm::algo::detail::ValidEntries<T>());
    }
    t = *maybet;

    return is;
  }
}  // namespace std

#endif  //CBM_ALGO_BASE_UTIL_SERIALIZABLEENUM_H
