/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmQaCheckerTypedefs.h
/// @brief  Common definitions for QA-Checker framework
/// @author S. Zharko <s.zharko@gsi.de>
/// @since  08.02.2023

#ifndef CbmQaCheckerTypedefs_h
#define CbmQaCheckerTypedefs_h 1

#include <boost/range/iterator_range.hpp>

#include <bitset>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace cbm::qa::checker
{
  /// @enum  ECmpMethod
  /// @brief Comparison method
  enum class ECmpMethod : uint8_t
  {
    Exact,  ///< exact equality (point-by-point, error-by-error)
    Ratio,  ///< ratio equality (max and min ratios do not exceed a required range)
    Chi2,   ///< equality within a chi2 hypothesis test
    END     ///< end of enumeration
  };

  /// @enum  ECmpInference
  /// @brief The object comparison inference
  /// @note  The sequence of the elements is important, please, do not shuffle!
  enum class ECmpInference : uint8_t
  {
    StronglyEqual = 0,  ///< All the comparison methods gave equality
    WeaklyEqual,        ///< At least one of the comparison methods showed equality
    Different           ///< Neither of the comparison methods showed equality
  };

  /// @brief  String representation of the ECmpInference enum
  inline std::string ToString(ECmpInference inference)
  {
    switch (inference) {
      case ECmpInference::StronglyEqual: return "\e[1;32msame\e[0m";
      case ECmpInference::WeaklyEqual: return "\e[1;33mconsistent\e[0m";
      case ECmpInference::Different: return "\e[1;31mdifferent\e[0m";
      default: return "";
    }
  }

  // Aliases
  using MapStrToStr_t     = std::unordered_map<std::string, std::string>;
  using MapStrToStrVect_t = std::unordered_map<std::string, std::vector<std::string>>;

  template<class T>
  using VectRange_t = boost::iterator_range<typename std::vector<T>::iterator>;

  // Constants
  constexpr double kLegendSize[2] = {.3, .05};  ///< width and height in % of the pad size
  constexpr float kRatioMin       = 0.95;       ///< Minimal acceptable ratio
  constexpr float kRatioMax       = 1.05;       ///< Maximal acceptable ratio
  constexpr float kPvalThrsh      = 0.05;       ///< P-value threshold


}  // namespace cbm::qa::checker

#endif  // CbmQaCheckerTypedefs_h
