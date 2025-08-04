/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include "Definitions.h"

#include <optional>
#include <string_view>
#include <tuple>

#include <yaml-cpp/emittermanip.h>

namespace cbm::algo::yaml
{

  template<typename Class, typename T>
  class Property {

   private:
    T Class::*fMember;
    std::string_view fKey;
    std::string_view fDescription;
    std::optional<YAML::EMITTER_MANIP> fFormat;
    std::optional<YAML::EMITTER_MANIP> fFormatEntries;

   public:
    using ClassType = Class;
    using ValueType = T;

    Property() = delete;

    constexpr Property(T Class::*member, std::string_view key, std::string_view description = "",
                       std::optional<YAML::EMITTER_MANIP> fmt = {}, std::optional<YAML::EMITTER_MANIP> fmtEntries = {})
      : fMember(member)
      , fKey(key)
      , fDescription(description)
      , fFormat(fmt)
      , fFormatEntries(fmtEntries)
    {
    }

    Property(const Property&) = delete;
    Property& operator=(const Property&) = delete;

    Property(Property&&) = default;
    Property& operator=(Property&&) = default;

    std::string_view Key() const { return fKey; }
    std::string_view Description() const { return fDescription; }
    std::optional<YAML::EMITTER_MANIP> Format() const { return fFormat; }
    std::optional<YAML::EMITTER_MANIP> FormatEntries() const { return fFormatEntries; }

    T& Get(Class& object) const { return object.*fMember; }
    const T& Get(const Class& object) const { return object.*fMember; }

    void Set(Class& object, const T& value) const { object.*fMember = value; }
  };

  template<typename Class, typename T>
  Property(T Class::*member, std::string_view key, std::string_view description) -> Property<Class, T>;

}  // namespace cbm::algo::yaml

#define CBM_YAML_PROPERTIES(...)                                                                                       \
 public:                                                                                                               \
  static constexpr auto Properties = std::make_tuple(__VA_ARGS__)

/**
 * @brief Optional tag to specify a formatting of the class (YAML::Flow vs YAML::Block)
*/
#define CBM_YAML_FORMAT(tag)                                                                                           \
 public:                                                                                                               \
  static constexpr std::optional<YAML::EMITTER_MANIP> FormatAs = tag

/**
 * @brief Optional flag to indicate that the class should be treated like a type
 * of it's property. Only has an effect on classes with a single property.
 *
 * @note This is useful to make some config files more compact.
 */
#define CBM_YAML_MERGE_PROPERTY()                                                                                      \
 public:                                                                                                               \
  static constexpr bool MergeProperty = true
