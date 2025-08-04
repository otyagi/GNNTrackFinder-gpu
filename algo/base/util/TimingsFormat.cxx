/* Copyright (C) 2023-2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "TimingsFormat.h"

#include <iomanip>
#include <sstream>

#include <fmt/format.h>
#include <xpu/host.h>
#include <yaml-cpp/emitter.h>

// Helper functions
namespace
{
  // Remove the cbm::algo:: prefix from kernel names
  std::string_view KernelNameStripped(const xpu::kernel_timings& kt)
  {
    constexpr std::string_view prefix = "cbm::algo::";
    if (kt.name().compare(0, prefix.size(), prefix) == 0) {
      return kt.name().substr(prefix.size());
    }
    else {
      return kt.name();
    }
  }

  void MakeReportYamlEntry(std::string_view name, double time, double throughput, YAML::Emitter& ss)
  {
    if (!std::isnormal(throughput)) {
      throughput = 0;
    }
    ss << YAML::Key << std::string{name};
    ss << YAML::Flow << YAML::Value;
    ss << YAML::BeginMap;
    ss << YAML::Key << "time" << YAML::Value << time;
    ss << YAML::Key << "throughput" << YAML::Value << throughput;
    ss << YAML::EndMap;
  }

  void MakeReportYamlTimer(const xpu::timings& t, YAML::Emitter& ss)
  {
    ss << YAML::BeginMap;
    MakeReportYamlEntry("wall", t.wall(), t.throughput(), ss);
    MakeReportYamlEntry("memcpy_h2d", t.copy(xpu::h2d), t.throughput_copy(xpu::h2d), ss);
    MakeReportYamlEntry("memcpy_d2h", t.copy(xpu::d2h), t.throughput_copy(xpu::d2h), ss);
    MakeReportYamlEntry("memset", t.memset(), t.throughput_memset(), ss);
    for (const auto& st : t.children()) {
      if (st.kernels().empty() && st.children().empty()) {
        MakeReportYamlEntry(st.name(), st.wall(), st.throughput(), ss);
      }
      else {
        ss << YAML::Key << std::string{st.name()} << YAML::Value;
        MakeReportYamlTimer(st, ss);
      }
    }
    for (const auto& kt : t.kernels()) {
      MakeReportYamlEntry(KernelNameStripped(kt), kt.total(), kt.throughput(), ss);
    }

    ss << YAML::EndMap;
  }

}  // namespace

namespace cbm::algo
{

  class TimingsFormat {

   public:
    void Begin(size_t align)
    {
      fAlign = align;
      fSS    = std::stringstream();
    }

    void Title(std::string_view title)
    {
      Indent();
      fSS << fmt::format("{:<{}}\n", title, fAlign);
    }

    std::string Finalize() { return fSS.str(); }

    void Fmt(const xpu::timings& t)
    {
      fIndent += 2;
      Measurement("Memcpy(h2d)", t.copy(xpu::h2d), t.throughput_copy(xpu::h2d));
      NewLine();
      Measurement("Memcpy(d2h)", t.copy(xpu::d2h), t.throughput_copy(xpu::d2h));
      NewLine();
      Measurement("Memset", t.memset(), t.throughput_memset());
      NewLine();

      // Merge subtimers with identical names
      // Useful eg in unpacking, where unpacker might be called multiple times per TS
      std::unordered_map<std::string, xpu::timings> subtimers;
      for (xpu::timings& st : t.children()) {
        subtimers[std::string(st.name())].merge(st);
      }

      for (auto& [name, st] : subtimers) {
        if (st.kernels().empty() && st.children().empty()) {
          Measurement(name, st.wall(), st.throughput());
          NewLine();
        }
        else {
          Title(name);
          Fmt(st);
          NewLine();
        }
      }

      for (xpu::kernel_timings& kt : t.kernels()) {
        Measurement(KernelNameStripped(kt), kt.total(), kt.throughput());
        NewLine();
      }

      if (!t.kernels().empty()) {
        Measurement("Kernel time", t.kernel_time(), t.throughput_kernels());
        NewLine();
      }
      Measurement("Wall time", t.wall(), t.throughput());
      fIndent -= 2;
    }

    void FmtSubtimers(const xpu::timings& t)
    {
      const auto subtimes = t.children();
      for (auto it = subtimes.begin(); it != subtimes.end(); ++it) {
        Title(it->name());
        Fmt(*it);
        if (std::next(it) != subtimes.end()) {
          NewLine();
        }
      }
    }

    void FmtSummary(const xpu::timings& t)
    {
      fIndent += 2;
      Measurement("Memcpy(h2d)", t.copy(xpu::h2d), t.throughput_copy(xpu::h2d));
      NewLine();
      Measurement("Memcpy(d2h)", t.copy(xpu::d2h), t.throughput_copy(xpu::d2h));
      NewLine();
      Measurement("Memset", t.memset(), t.throughput_memset());
      NewLine();
      Measurement("Kernel time", t.kernel_time(), t.throughput_kernels());
      NewLine();
      Measurement("Wall time", t.wall(), t.throughput());
      fIndent -= 2;
    }

    void NewLine() { fSS << "\n"; }

   private:
    void Measurement(std::string_view name, f64 time, f64 throughput)
    {
      Indent();
      fSS << std::setw(fAlign) << std::setfill(' ') << std::left << fmt::format("{}:", name);
      Real(time, 10, 3, "ms");
      if (std::isnormal(throughput)) {
        fSS << " (";
        Real(throughput, 7, 3, "GB/s");
        fSS << ")";
      }
    }

    void Real(double x, int width, int precision, std::string_view unit)
    {
      fSS << std::setw(width) << std::setfill(' ') << std::right << std::fixed << std::setprecision(precision) << x
          << " " << unit;
    }

    void Indent() { fSS << std::setw(fIndent) << std::setfill(' ') << std::left << ""; }

    size_t fAlign  = 0;
    size_t fIndent = 0;
    std::stringstream fSS;

  };  // class TimingsFormat

  std::string MakeReport(std::string_view title, const xpu::timings& t, size_t align)
  {
    TimingsFormat tf;
    tf.Begin(align);
    tf.Title(title);
    tf.Fmt(t);
    return tf.Finalize();
  }

  std::string MakeReportSubtimers(std::string_view title, const xpu::timings& t, size_t align)
  {
    TimingsFormat tf;
    tf.Begin(align);
    tf.Title(title);
    tf.FmtSubtimers(t);
    return tf.Finalize();
  }

  std::string MakeReportSummary(std::string_view title, const xpu::timings& t, size_t align)
  {
    TimingsFormat tf;
    tf.Begin(align);
    tf.Title(title);
    tf.FmtSummary(t);
    return tf.Finalize();
  }

  std::string MakeReportYaml(const xpu::timings& t)
  {
    YAML::Emitter ss;
    ss << YAML::BeginDoc;
    ss << YAML::Precision(6);
    ss << YAML::BeginMap;
    for (const auto& subtimer : t.children()) {
      ss << YAML::Key << std::string{subtimer.name()};
      ss << YAML::Value;
      MakeReportYamlTimer(subtimer, ss);
    }
    ss << YAML::EndMap;
    ss << YAML::EndDoc;
    return ss.c_str();
  }

}  // namespace cbm::algo
