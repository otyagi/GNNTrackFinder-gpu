/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include "BaseTypes.h"
#include "Definitions.h"
#include "Property.h"
#include "compat/Filesystem.h"
#include "util/EnumDict.h"

#include <sstream>
#include <string_view>

#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

namespace cbm::algo::yaml
{

  template<typename T>
  T Read(const YAML::Node& node);

  template<typename T, T... Values, typename Func>
  constexpr void ForEach(std::integer_sequence<T, Values...>, Func&& func)
  {
    (func(std::integral_constant<T, Values>{}), ...);
  }

  template<typename T, typename = void>
  struct GetFmtTag {
    static constexpr std::optional<YAML::EMITTER_MANIP> value = {};
  };

  template<typename T>
  struct GetFmtTag<T, std::void_t<decltype(T::FormatAs)>> {
    static constexpr std::optional<YAML::EMITTER_MANIP> value = T::FormatAs;
  };

  template<typename T, typename = void>
  struct ShouldMergeProperty {
    static constexpr bool value = false;
  };

  template<typename T>
  struct ShouldMergeProperty<T, std::void_t<decltype(T::MergeProperty)>> {
    static constexpr bool value = T::MergeProperty;
  };

  template<typename T>
  T ReadFromFile(fs::path path)
  {
    YAML::Node node = YAML::LoadFile(path.string());
    return Read<T>(node);
  }


  template<typename T>
  T Read(const YAML::Node& node)
  {
// Disable uninitialized warning for the whole function.
// GCC 11.4 sometimes incorrectly warns about uninitialized variables in constexpr if branches.
// I'm fairly certain this is a compiler bug, because it only happens when
// explicitly instantiating the function template. And the error message
// references a variable 't' that doesn't exist in the function!
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    using Type = std::remove_cv_t<std::remove_reference_t<T>>;

    static_assert(!IsEnum<T> || detail::EnumHasDict_v<T>, "Enum must have a dictionary to be deserializable");

    // TODO: error handling
    if constexpr (IsFundamental<Type>) {
      return node.as<Type>();
    }
    else if constexpr (IsEnum<Type>) {
      std::optional<T> maybet = FromString<T>(node.as<std::string>());
      if (!maybet) {
        throw std::runtime_error(fmt::format("Invalid enum value: {}", node.as<std::string>()));
      }
      return *maybet;
    }
    else if constexpr (IsVector<Type>) {
      Type vector;
      for (const auto& element : node) {
        vector.push_back(Read<typename Type::value_type>(element));
      }
      return vector;
    }
    else if constexpr (IsArray<Type>) {
      Type array  = {};
      auto vector = Read<std::vector<typename Type::value_type>>(node);
      if (vector.size() != array.size()) {
        throw std::runtime_error(fmt::format("Array size mismatch: expected {}, got {}", array.size(), vector.size()));
      }
      std::copy(vector.begin(), vector.end(), array.begin());
      return array;
    }
    else if constexpr (IsSet<Type>) {
      Type set;
      for (const auto& element : node) {
        set.insert(Read<typename Type::value_type>(element));
      }
      return set;
    }
    else if constexpr (IsMap<Type>) {
      using Key_t = typename Type::key_type;
      using Val_t = typename Type::mapped_type;

      static_assert(IsScalar<Key_t>, "Map key must be a fundamental or enum type");

      Type map{};
      for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
        const auto& key       = it->first;
        const auto& value     = it->second;
        map[Read<Key_t>(key)] = Read<Val_t>(value);
      }
      return map;
    }
    else {
      Type object{};

      constexpr auto nProperties = std::tuple_size<decltype(Type::Properties)>::value;

      if constexpr (nProperties == 1 && ShouldMergeProperty<T>::value) {
        auto& property   = std::get<0>(Type::Properties);
        using ValueType  = std::remove_cv_t<std::remove_reference_t<decltype(property.Get(std::declval<Type>()))>>;
        ValueType& value = property.Get(object);
        value            = Read<ValueType>(node);
      }
      else {
        ForEach(std::make_integer_sequence<std::size_t, nProperties>{}, [&](auto index) {
          auto& property   = std::get<index>(Type::Properties);
          using ValueType  = std::remove_cv_t<std::remove_reference_t<decltype(property.Get(object))>>;
          ValueType& value = property.Get(object);
          value            = Read<ValueType>(node[std::string{property.Key()}]);
        });
      }

      return object;
    }

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
  }

  template<typename T>
  std::string MakeDocString(int indent = 0)
  {
    using Type = std::remove_cv_t<std::remove_reference_t<T>>;

    std::stringstream docString;

    if constexpr (IsFundamental<Type>) {
      docString << Typename<Type>();
    }
    else if constexpr (IsVector<Type> || IsArray<Type>) {
      using ChildType = typename Type::value_type;
      if constexpr (IsFundamental<ChildType>) {
        docString << std::string(indent, ' ') << "list of " << Typename<ChildType>() << std::endl;
      }
      else {
        docString << std::string(indent, ' ') << "list of" << std::endl;
        docString << MakeDocString<ChildType>(indent + 2);
      }
    }
    else {
      constexpr auto nProperties = std::tuple_size<decltype(Type::Properties)>::value;

      ForEach(std::make_integer_sequence<std::size_t, nProperties>{}, [&](auto index) {
        using ChildType = std::remove_cv_t<
          std::remove_reference_t<decltype(std::get<index>(Type::Properties).Get(std::declval<Type>()))>>;
        auto& property = std::get<index>(Type::Properties);
        if constexpr (IsFundamental<ChildType>) {
          docString << std::string(indent, ' ') << property.Key() << ": " << property.Description() << " ["
                    << Typename<ChildType>() << "]" << std::endl;
        }
        else {
          docString << std::string(indent, ' ') << property.Key() << ": " << property.Description() << std::endl;
          docString << MakeDocString<ChildType>(indent + 2);
        }
      });
    }
    return docString.str();
  }

  class Dump {

   public:
    template<typename T>
    std::string operator()(const T& object, int floatPrecision = 6)
    {
      YAML::Emitter ss;
      ss << YAML::BeginDoc;
      ss << YAML::Precision(floatPrecision);
      DoDump(object, ss);
      ss << YAML::EndDoc;
      return ss.c_str();
    }

   private:
    template<typename T>
    void DoDump(const T& object, YAML::Emitter& ss, std::optional<YAML::EMITTER_MANIP> formatEntries = {})
    {
      static_assert(!IsEnum<T> || detail::EnumHasDict_v<T>, "Enum must have a dictionary");

      if constexpr (IsFundamental<T>) {
        // Take care that i8 and u8 are printed as integers not as characters
        if constexpr (std::is_same_v<T, i8> || std::is_same_v<T, u8>)
          ss << i32(object);
        else
          ss << object;
      }
      else if constexpr (IsEnum<T>) {
        ss << std::string{ToString<T>(object)};
      }
      else if constexpr (IsVector<T> || IsArray<T> || IsSet<T>) {
        ss << YAML::BeginSeq;
        // Special case for vector<bool> because it is not a real vector
        // Clang does not the compile the generic version of the loop
        // in this case.
        if constexpr (std::is_same_v<T, std::vector<bool>>) {
          for (bool element : object) {
            if (formatEntries.has_value()) {
              ss << formatEntries.value();
            }
            ss << element;
          }
        }
        else {
          for (const auto& element : object) {
            if (formatEntries.has_value()) {
              ss << formatEntries.value();
            }
            DoDump(element, ss);
          }
        }
        ss << YAML::EndSeq;
      }
      else if constexpr (IsMap<T>) {
        ss << YAML::BeginMap;
        for (const auto& [key, value] : object) {
          if (formatEntries.has_value()) {
            ss << formatEntries.value();
          }
          ss << YAML::Key << key;
          ss << YAML::Value;
          DoDump(value, ss);
        }
        ss << YAML::EndMap;
      }
      else {
        constexpr auto nProperties = std::tuple_size<decltype(T::Properties)>::value;
        if (auto fmtTag = GetFmtTag<T>::value; fmtTag.has_value()) {
          ss << fmtTag.value();
        }

        if constexpr (nProperties == 1 && ShouldMergeProperty<T>::value) {
          auto& property = std::get<0>(T::Properties);
          auto& value    = property.Get(object);
          auto format    = property.Format();
          if (format.has_value()) {
            ss << format.value();
          }
          DoDump(value, ss, property.FormatEntries());
        }
        else {
          ss << YAML::BeginMap;
          ForEach(std::make_integer_sequence<std::size_t, nProperties>{}, [&](auto index) {
            auto& property = std::get<index>(T::Properties);
            auto& value    = property.Get(object);
            auto format    = property.Format();
            ss << YAML::Key << std::string{property.Key()};
            if (format.has_value()) {
              ss << format.value();
            }
            ss << YAML::Value;
            DoDump(value, ss, property.FormatEntries());
          });
          ss << YAML::EndMap;
        }
      }
    }
  };

}  // namespace cbm::algo::yaml

/**
 * @brief Declare the external instantiation of the Read and Dump functions for a type.
 * @param type The type to declare the external instantiation for.
 * @note This macro should be used in a header file to declare the external instantiation of the Read and Dump.
 * Must be paired with CBM_YAML_INSTANTIATE in a source file.
 **/
#define CBM_YAML_EXTERN_DECL(type)                                                                                     \
  extern template type cbm::algo::yaml::Read<type>(const YAML::Node& node);                                            \
  extern template std::string cbm::algo::yaml::Dump::operator()<type>(const type& value, int floatPrecision)

/**
 * @brief Explicitly instantiate the Read and Dump functions for a type.
 * @see CBM_YAML_EXTERN_DECL
 */
#define CBM_YAML_INSTANTIATE(type)                                                                                     \
  template type cbm::algo::yaml::Read<type>(const YAML::Node& node);                                                   \
  template std::string cbm::algo::yaml::Dump::operator()<type>(const type& value, int floatPrecision);
