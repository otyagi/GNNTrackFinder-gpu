/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Accumulators.h
/// \date   03.03.2024
/// \brief  Custom accumulators for boost::histogram (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include <boost/histogram/fwd.hpp>  // for weighted_mean<>
#include <boost/histogram/make_histogram.hpp>
#include <boost/serialization/access.hpp>

#include <cmath>
#include <type_traits>

namespace boost::histogram
{
  /// \class  RootStyleProfileAccumulator
  /// \brief  A ROOT-style accumulator for the Boost-histogram profiles
  /// \tparam ValueType  Type of the values
  template<class ValueType>
  class RootStyleProfileAccumulator {
   public:
    using Value_t    = ValueType;
    using ConstRef_t = const Value_t&;
    using Weight_t   = weight_type<Value_t>;

    /// \brief Default constructor
    RootStyleProfileAccumulator() = default;

    /// \brief Copy constructor from the instances with other types
    template<class OtherValueType>
    RootStyleProfileAccumulator(const RootStyleProfileAccumulator<OtherValueType>& other)
      : fSumWV(other.fSumWV)
      , fSumWV2(other.fSumWV2)
      , fSumW(other.fSumW)
      , fSumW2(other.fSumW2)
    {
    }

    /// \brief Constructor from the external counters
    /// \param sumWV  Sum of weight * value
    /// \param sumWV  Sum of weight * value * value
    /// \param sumW   Sum of weights
    /// \param sumW2  Sum of weights
    RootStyleProfileAccumulator(ConstRef_t sumWV, ConstRef_t sumWV2, ConstRef_t sumW, ConstRef_t sumW2)
      : fSumWV(sumWV)
      , fSumWV2(sumWV2)
      , fSumW(sumW)
      , fSumW2(sumW2)
    {
    }

    /// \brief Inserts sample with 1. weight
    /// \param v Sample
    void operator()(ConstRef_t v) { operator()(weight(1.), v); }

    /// \brief Inserts sample with weight
    /// \param v Sample
    /// \param w Sample weight
    void operator()(const Weight_t& w, ConstRef_t v)
    {
      fSumWV += w.value * v;
      fSumWV2 += w.value * v * v;
      fSumW += w.value;
      fSumW2 += w.value * w.value;
    }

    /// \brief Adds another accumulator
    RootStyleProfileAccumulator& operator+=(const RootStyleProfileAccumulator& rhs)
    {
      if (rhs.fSumW == 0) {
        return *this;
      }
      fSumWV += rhs.fSumWV;
      fSumWV2 += rhs.fSumWV2;
      fSumW += rhs.fSumW;
      fSumW2 += rhs.fSumW2;
      return *this;
    }

    /// \brief Scales all entries by value
    RootStyleProfileAccumulator& operator*=(ConstRef_t s) noexcept
    {
      fSumWV *= s;
      fSumWV2 *= s * s;
      return *this;
    }

    /// \brief Equality operator
    bool operator==(const RootStyleProfileAccumulator& rhs) const noexcept
    {
      return fSumWV == rhs.fSumWV && fSumWV2 == rhs.fSumWV2 && fSumW == rhs.fSumW && fSumW2 == rhs.fSumW2;
    }

    /// \brief Non-equality operator
    bool operator!=(const RootStyleProfileAccumulator& rhs) const noexcept { return !operator==(rhs); }

    /// \brief Returns sum of weights
    ConstRef_t GetSumW() const noexcept { return fSumW; }

    /// \brief Returns sum of weight squares
    ConstRef_t GetSumW2() const noexcept { return fSumW2; }

    /// \brief Returns sum of products of weight multiplied by value
    ConstRef_t GetSumWV() const noexcept { return fSumWV; }

    /// \brief Returns sum of products of weight multiplied by squared value
    ConstRef_t GetSumWV2() const noexcept { return fSumWV2; }

    /// \brief Returns the mean value
    Value_t GetMean() const noexcept { return fSumW ? (fSumWV / fSumW) : 0; }

    /// \brief Returns effective sum of values
    Value_t GetEffCount() const noexcept { return fSumW2 ? (fSumW * fSumW / fSumW2) : 0; }

    /// \brief Returns variance
    Value_t GetVariance() const noexcept { return fSumW ? (fSumWV2 / fSumW - GetMean() * GetMean()) : 0; }

    /// \brief Returns standard deviation of the sample
    Value_t GetStdDev() const noexcept { return std::sqrt(GetVariance()); }

    /// \brief Returns standard error of the mean
    Value_t GetSEM() const noexcept { return fSumW ? GetStdDev() / std::sqrt(GetEffCount()) : 0; }

   private:
    friend class boost::serialization::access;
    /// \brief Serialization rule
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& make_nvp("fSumWV", fSumWV);
      ar& make_nvp("fSumWV2", fSumWV2);
      ar& make_nvp("fSumW", fSumW);
      ar& make_nvp("fSumW2", fSumW2);
    }

    Value_t fSumWV{};   ///< Sum of weight * value
    Value_t fSumWV2{};  ///< Sum of weight * value * value
    Value_t fSumW{};    ///< Sum of weights
    Value_t fSumW2{};   ///< Sum of squared weights
  };
}  // namespace boost::histogram

#ifndef BOOST_HISTOGRAM_DOXYGEN_INVOKED
namespace std
{
  template<class T, class U>
  /// Specialization for boost::histogram::accumulators::weighted_mean.
  struct common_type<boost::histogram::RootStyleProfileAccumulator<T>,
                     boost::histogram::RootStyleProfileAccumulator<U>> {
    using type = boost::histogram::RootStyleProfileAccumulator<common_type_t<T, U>>;
  };
}  // namespace std
#endif

namespace boost::histogram
{
  template<typename T>
  using RootStyleProfileStorage = dense_storage<RootStyleProfileAccumulator<T>>;

  /// \brief Maker for RootStyleProfileAccumulator
  template<class Axis, class... Axes, class = detail::requires_axis<Axis>>
  auto MakeRootStyleProfile(Axis&& axis, Axes&&... axes)
  {
    return make_histogram_with(RootStyleProfileStorage<double>(), std::forward<Axis>(axis),
                               std::forward<Axes>(axes)...);
  }
}  // namespace boost::histogram
