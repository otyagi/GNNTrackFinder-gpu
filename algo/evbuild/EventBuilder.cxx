/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "EventBuilder.h"

#include <cassert>
#include <iomanip>

#include <xpu/host.h>

using std::is_sorted;
using std::vector;

namespace cbm::algo::evbuild
{

  // -----   Algorithm execution   --------------------------------------------
  EventBuilder::resultType EventBuilder::operator()(const DigiData& ts, const vector<double> triggers,
                                                    std::optional<DigiEventSelector> selector) const
  {
    xpu::push_timer("EventBuilder");
    xpu::t_add_bytes(ts.TotalSizeBytes());

    // --- Output data
    resultType result = {};
    auto& events      = result.first;
    events.resize(triggers.size());

    std::transform(triggers.begin(), triggers.end(), events.begin(),
                   [&ts, &result, this](const double& trigger) { return BuildEvent(ts, result.second, trigger); });

    // --- Apply event selector
    if (selector.has_value()) {
      auto notSelected = [&](DigiEvent& ev) { return !((*selector)(ev)); };
      auto removeIt    = std::remove_if(events.begin(), events.end(), notSelected);
      events.erase(removeIt, events.end());
    }

    EventBuilderMonitorData& monitor = result.second;

    monitor.sts.nDigis += ts.fSts.size();
    monitor.rich.nDigis += ts.fRich.size();
    monitor.much.nDigis += ts.fMuch.size();
    monitor.trd.nDigis += ts.fTrd.size();
    monitor.trd2d.nDigis += ts.fTrd2d.size();
    monitor.tof.nDigis += ts.fTof.size();
    monitor.psd.nDigis += ts.fPsd.size();
    monitor.fsd.nDigis += ts.fFsd.size();
    monitor.bmon.nDigis += ts.fBmon.size();
    monitor.numTriggers += triggers.size();
    monitor.numEvents += result.first.size();

    monitor.time = xpu::pop_timer();
    return result;
  }

  // --- Build a single event
  DigiEvent EventBuilder::BuildEvent(const DigiData& ts, EventBuilderMonitorData& monitor, double trigger) const
  {
    DigiEvent event;
    event.fTime = trigger;

    // --- Loop over systems
    for (auto entry : fConfig.fWindows) {

      auto system       = entry.first;
      const double tMin = trigger + entry.second.first;
      const double tMax = trigger + entry.second.second;

      // --- Build the event using trigger window
      switch (system) {
        case ECbmModuleId::kSts: {
          event.fSts = CopyRange(ts.fSts, tMin, tMax);
          break;
        }
        case ECbmModuleId::kRich: {
          event.fRich = CopyRange(ts.fRich, tMin, tMax);
          break;
        }
        case ECbmModuleId::kMuch: {
          event.fMuch = CopyRange(ts.fMuch, tMin, tMax);
          break;
        }
        case ECbmModuleId::kTrd: {
          event.fTrd = CopyRange(ts.fTrd, tMin, tMax);
          break;
        }
        case ECbmModuleId::kTrd2d: {
          event.fTrd2d = CopyRange(ts.fTrd2d, tMin, tMax);
          break;
        }
        case ECbmModuleId::kTof: {
          event.fTof = CopyRange(ts.fTof, tMin, tMax);
          break;
        }
        case ECbmModuleId::kPsd: {
          event.fPsd = CopyRange(ts.fPsd, tMin, tMax);
          break;
        }
        case ECbmModuleId::kFsd: {
          event.fFsd = CopyRange(ts.fFsd, tMin, tMax);
          break;
        }
        case ECbmModuleId::kBmon: {
          event.fBmon = CopyRange(ts.fBmon, tMin, tMax);
          break;
        }
        default: break;
      }
    }
    monitor.sts.nDigisInEvents += event.fSts.size();
    monitor.rich.nDigisInEvents += event.fRich.size();
    monitor.much.nDigisInEvents += event.fMuch.size();
    monitor.trd.nDigisInEvents += event.fTrd.size();
    monitor.trd2d.nDigisInEvents += event.fTrd2d.size();
    monitor.tof.nDigisInEvents += event.fTof.size();
    monitor.psd.nDigisInEvents += event.fPsd.size();
    monitor.fsd.nDigisInEvents += event.fFsd.size();
    monitor.bmon.nDigisInEvents += event.fBmon.size();
    return event;
  }
  // --------------------------------------------------------------------------


  // -----   Info to string   -------------------------------------------------
  std::string EventBuilder::ToString() const
  {
    std::stringstream out;
    out << "--- Using EventBuilder with event windows:";
    for (const auto& entry : fConfig.fWindows) {
      out << "\n  " << std::left << std::setw(5) << ::ToString(entry.first) << ": ";
      out << "  [" << std::right << std::setw(5) << entry.second.first;
      out << ", " << std::right << std::setw(5) << entry.second.second << "] ns";
    }
    return out.str();
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::algo::evbuild
