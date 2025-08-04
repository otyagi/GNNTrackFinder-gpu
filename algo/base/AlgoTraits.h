/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include <tuple>
#include <type_traits>

/**
 * @file AlgoTraits.h
 * @brief Type traits for online algorithms
 */

namespace cbm::algo::algo_traits
{

  namespace detail
  {
    template<typename...>
    struct ResultOf {
    };

    template<typename R, typename Algo, typename... Args>
    struct ResultOf<R (Algo::*)(Args...) const> {
      using type = R;
    };

    template<typename R, typename Algo, typename... Args>
    struct ResultOf<R (Algo::*)(Args...)> {
      using type = R;
    };

    template<typename Algo>
    struct ResultOf<Algo> : ResultOf<decltype(&Algo::operator())> {
    };

  }  // namespace detail

  /**
   * @brief Type alias for the return type produced by an algorithm when invoked via callable-operator
   */
  template<typename Algo>
  using ResultOf_t = typename detail::ResultOf<Algo>::type;

  // Currently assume algorithms return std::tuple<R, M, A>
  // where R is the output, M is the monitoring data and A is auxiliary data

  /**
   * @brief Type alias for the output type produced by an algorithm
   */
  template<typename Algo>
  using Output_t = typename std::tuple_element<0, ResultOf_t<Algo>>::type;

  /**
   * @brief Type alias for the monitoring type produced by an algorithm
   */
  template<typename Algo>
  using Monitor_t = typename std::tuple_element<1, ResultOf_t<Algo>>::type;

  /**
   * @brief Type alias for the auxiliary data type produced by an algorithm
   */
  template<typename Algo>
  using Aux_t = typename std::tuple_element<2, ResultOf_t<Algo>>::type;

}  // namespace cbm::algo::algo_traits
