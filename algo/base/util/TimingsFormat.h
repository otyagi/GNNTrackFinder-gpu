/* Copyright (C) 2023-2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer]*/
#ifndef CBM_ALGO_BASE_UTIL_TIMINGSFORMAT_H
#define CBM_ALGO_BASE_UTIL_TIMINGSFORMAT_H

#include "Definitions.h"

#include <string>
#include <string_view>

namespace xpu
{
  class timings;
}

namespace cbm::algo
{

  /**
   * @brief Print timings from top-level times and subtimers.
   */
  std::string MakeReport(std::string_view title, const xpu::timings& t, size_t align = 40);

  /**
   * @brief Print timings from subtimers.
   */
  std::string MakeReportSubtimers(std::string_view title, const xpu::timings& t, size_t align = 40);

  /**
   * @brief Only print the top-level times (Elapsed time, total kernel time, memcpy and memset times). Disregard subtimers and kernel times.
   */
  std::string MakeReportSummary(std::string_view, const xpu::timings& t, size_t align = 40);

  /**
   * @brief Print timings in YAML format.
   */
  std::string MakeReportYaml(const xpu::timings& t);

}  // namespace cbm::algo

#endif
