/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith */
#pragma once

#include "AlgoFairloggerCompat.h"
#include "Definitions.h"
#include "PODVector.h"
#include "Timeslice.hpp"
#include "UnpackMSBase.h"
#include "compat/Algorithm.h"
#include "compat/OpenMP.h"
#include "util/StlUtils.h"

#include <cstdint>
#include <gsl/span>
#include <map>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo
{

  namespace detail
  {

    struct UnpackMonitorBase {
      fles::Subsystem system;       // subsystem
      size_t numComponents    = 0;  // number of components used
      size_t numMs            = 0;  // number of microslices
      size_t sizeBytesIn      = 0;  // total size of microslice contents
      size_t sizeBytesOut     = 0;  // total size of unpacked digis
      size_t errInvalidSysVer = 0;
      size_t errInvalidEqId   = 0;

      double ExpansionFactor() const { return sizeBytesIn > 0 ? static_cast<double>(sizeBytesOut) / sizeBytesIn : 0.0; }
    };

    /**
     * @brief Collection of MS data to unpack
     *
     * @note Helper struct for CommonUnpacker. Moved out of class body to avoid code duplication from different template specializations.
     */
    struct MSData {
      std::vector<fles::MicrosliceDescriptor> msDesc;  // microslice descriptors
      std::vector<const u8*> msContent;                // pointer to microslice contents

      UnpackMonitorBase monitor;  // monitoring data

      MSData(const fles::Timeslice& ts, fles::Subsystem system, gsl::span<u16> legalEqIds);
      ~MSData() = default;
    };

  }  // namespace detail

  template<class MSMonitor>
  struct UnpackMonitor : detail::UnpackMonitorBase {
    std::vector<MSMonitor> msMonitor;  // monitoring data per microslice
  };

  template<class MSAux>
  struct UnpackAux {
    std::vector<MSAux> msAux;  // auxiliary data per microslice
  };

  struct UnpackKey {
    u16 eqId;
    u8 sysVer;

    UnpackKey(u16 eqId_, u8 sysVer_) : eqId(eqId_), sysVer(sysVer_) {}
    UnpackKey(const fles::MicrosliceDescriptor& msDesc) : eqId(msDesc.eq_id), sysVer(msDesc.sys_ver) {}

    bool operator<(const UnpackKey& other) const
    {
      return eqId < other.eqId || (eqId == other.eqId && sysVer < other.sysVer);
    }
  };

  template<class Digi, class MSMonitor, class MSAux>
  class CommonUnpacker {
   protected:
    using Monitor_t = UnpackMonitor<MSMonitor>;
    using Aux_t     = UnpackAux<MSAux>;
    using Result_t  = std::tuple<PODVector<Digi>, Monitor_t, Aux_t>;
    using Unpack_t  = UnpackMSBase<Digi, MSMonitor, MSAux>;

    std::map<UnpackKey, std::unique_ptr<Unpack_t>> fAlgos;  //< Equipment id to unpacker map. Filled by child class!

    Result_t DoUnpack(const fles::Subsystem subsystem, const fles::Timeslice& ts) const
    {
      xpu::scoped_timer t_(fles::to_string(subsystem));
      auto legalEqIds = GetEqIds();

      detail::MSData msData{ts, subsystem, gsl::make_span(legalEqIds)};
      const size_t numMs = msData.monitor.numMs;

      Result_t out;
      auto& digisOut   = std::get<0>(out);
      auto& monitorOut = std::get<1>(out);
      auto& auxOut     = std::get<2>(out);

      static_cast<detail::UnpackMonitorBase&>(monitorOut) = msData.monitor;

      std::vector<std::vector<Digi>> msDigis(numMs);  // unpacked digis per microslice
      monitorOut.msMonitor.resize(numMs);             // monitoring data per microslice
      auxOut.msAux.resize(numMs);                     // auxiliary data per microslice

      xpu::t_add_bytes(msData.monitor.sizeBytesIn);

      xpu::push_timer("Unpack");
      xpu::t_add_bytes(msData.monitor.sizeBytesIn);
      CBM_PARALLEL_FOR(schedule(dynamic))
      for (size_t i = 0; i < numMs; i++) {
        auto& msDesc = msData.msDesc[i];
        auto algo    = fAlgos.find(UnpackKey{msDesc});

        if (algo == fAlgos.end()) {
          if (!Contains(legalEqIds, msDesc.eq_id)) {
            L_(error) << "Invalid equip id " << std::hex << int{msDesc.eq_id} << std::dec << " for subsystem "
                      << ToString(subsystem);
            monitorOut.errInvalidEqId++;
          }
          else {
            L_(error) << "Invalid system version " << std::hex << int{msDesc.sys_ver} << std::dec << " for subsystem "
                      << ToString(subsystem);
            monitorOut.errInvalidSysVer++;
          }
          continue;
        }

        auto result             = (*algo->second)(msData.msContent[i], msData.msDesc[i], ts.start_time());
        msDigis[i]              = std::move(std::get<0>(result));
        monitorOut.msMonitor[i] = std::move(std::get<1>(result));
        auxOut.msAux[i]         = std::move(std::get<2>(result));
      }
      xpu::pop_timer();

      xpu::push_timer("Resize");
      size_t nDigisTotal = 0;
      for (const auto& digis : msDigis) {
        nDigisTotal += digis.size();
      }
      digisOut.resize(nDigisTotal);
      monitorOut.sizeBytesOut = nDigisTotal * sizeof(Digi);
      xpu::pop_timer();

      xpu::push_timer("Merge");
      xpu::t_add_bytes(monitorOut.sizeBytesOut);
      CBM_PARALLEL_FOR(schedule(dynamic))
      for (size_t i = 0; i < numMs; i++) {
        size_t offset = 0;
        for (size_t x = 0; x < i; x++)
          offset += msDigis[x].size();
        std::copy(msDigis[i].begin(), msDigis[i].end(), digisOut.begin() + offset);
      }
      xpu::pop_timer();

      xpu::push_timer("Sort");
      xpu::t_add_bytes(monitorOut.sizeBytesOut);
      DoSort(digisOut);
      xpu::pop_timer();

      return out;
    }

   private:
    void DoSort(gsl::span<Digi> digis) const
    {
      Sort(digis.begin(), digis.end(), [](const Digi& a, const Digi& b) { return a.GetTime() < b.GetTime(); });
    }

    std::vector<u16> GetEqIds() const
    {
      std::vector<u16> eqIds;
      eqIds.reserve(fAlgos.size());
      for (const auto& [key, algo] : fAlgos) {
        eqIds.push_back(key.eqId);
      }
      return eqIds;
    }

    std::vector<u8> GetSysVers(u16 eqId) const
    {
      std::vector<u8> sysVers;
      for (const auto& [key, algo] : fAlgos) {
        if (key.eqId == eqId) {
          sysVers.push_back(key.sysVer);
        }
      }
      return sysVers;
    }
  };
}  // namespace cbm::algo
