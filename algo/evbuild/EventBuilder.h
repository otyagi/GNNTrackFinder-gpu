/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBM_ALGO_EVENTBUILDER_H
#define CBM_ALGO_EVENTBUILDER_H 1

#include "CbmDefs.h"
#include "DigiData.h"
#include "DigiEventSelector.h"
#include "EventBuilderConfig.h"

#include <algorithm>
#include <gsl/span>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <xpu/host.h>

namespace cbm::algo::evbuild
{

  /** @struct EventBuilderDetectorMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 23 Jun 2023
   ** @brief Monitoring data for event building for one detector
   **/
  struct EventBuilderDetectorMonitorData {
    size_t nDigisInEvents = 0;  ///< Number of digis collected into events
    size_t nDigis         = 0;  ///< Full number of digis in input source
  };


  /** @struct EventBuilderMonitorData
   ** @author Dominik Smith <d.smith@gsi.de>
   ** @since 23 Jun 2023
   ** @brief Monitoring data for event building
   **/
  struct EventBuilderMonitorData {
    EventBuilderDetectorMonitorData sts;    ///< Monitoring data for STS
    EventBuilderDetectorMonitorData much;   ///< Monitoring data for MUCH
    EventBuilderDetectorMonitorData tof;    ///< Monitoring data for TOF
    EventBuilderDetectorMonitorData bmon;   ///< Monitoring data for Bmon
    EventBuilderDetectorMonitorData trd;    ///< Monitoring data for TRD
    EventBuilderDetectorMonitorData trd2d;  ///< Monitoring data for TRD2D
    EventBuilderDetectorMonitorData rich;   ///< Monitoring data for RICH
    EventBuilderDetectorMonitorData psd;    ///< Monitoring data for PSD
    EventBuilderDetectorMonitorData fsd;    ///< Monitoring data for FSD
    size_t numTriggers = 0;                 ///< Number of input triggers
    size_t numEvents   = 0;                 ///< Number of built and selected events
    xpu::timings time;                      ///< Time for event building
  };


  /** @class EventBuilder
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 2021
   ** @brief Constructs CbmDigiEvents out of CbmDigiTimeslices
   **
   ** Events are constructed by copying digi data from the source (CbmDigiTimeslice).
   ** Digis are selected in trigger windows, the sizes of which relative to a trigger time are configurable.
   ** For each trigger time, an event is generated. The time intervals may overlap, resulting in digis
   ** being attributed to multiple events.
   **
   ** The source digi vectors (in CbmDigiTimeslice) must be sorted w.r.t. time, otherwise the behaviour is
   ** undefined.
   **
   ** The trigger vector must be sorted.
   **/
  class EventBuilder {

   public:
    typedef std::pair<std::vector<DigiEvent>, EventBuilderMonitorData> resultType;

    /** @brief Constructor **/
    EventBuilder(const EventBuilderConfig& config) : fConfig(config) {}


    /** @brief Destructor **/
    virtual ~EventBuilder(){};


    /** @brief Execution
     ** @param  ts       Digi source (timeslice)
     ** @param  triggers List of trigger times
     ** @param  selector Optional event selector
     ** @return Vector of constructed events and monitoring data
     **/
    resultType operator()(const DigiData& ts, const std::vector<double> triggers,
                          std::optional<DigiEventSelector> selector) const;


    /** @brief Info to string **/
    std::string ToString() const;


   private:  // methods
    /** @brief Build a single event from a trigger time
     ** @param  ts      Digi source (timeslice)
     ** @param  monitor Monitoring data
     ** @param  trigger Trigger time
     ** @return Digi event
     **/
    DigiEvent BuildEvent(const DigiData& ts, EventBuilderMonitorData& monitor, double trigger) const;


    /** @brief Copy data objects in a given time interval from the source to the target vector
     ** @param source Source data vector
     ** @param tMin   Minimal time
     ** @param tMax   Maximal time
     ** @return Target data vector
     **
     ** The Data class specialisation must implement the method double GetTime(), which is used to
     ** check whether the Data object falls into the specified time interval.
     **
     ** The source vector must be ordered w.r.t. GetTime(), otherwise the behaviour is undefined.
     **
     ** TODO: The current implementation searches, for each trigger, the entire source vector. This
     ** can surely be optimised when the contract that the trigger vector be sorted is properly exploited,
     ** e.g., by starting the search for the first digi in the trigger window from the start of the
     ** previous trigger window. This, however, requires bookkeeping hardly to be realised without
     ** changing the state of the class. I leave this for the future and for bright specialists.
     **/
    template<typename Vector>
    static Vector CopyRange(const Vector& source, double tMin, double tMax)
    {
      using Data = typename Vector::value_type;
      auto comp1 = [](const Data& obj, double value) { return obj.GetTime() < value; };
      auto comp2 = [](double value, const Data& obj) { return value < obj.GetTime(); };
      auto lower = std::lower_bound(source.begin(), source.end(), tMin, comp1);
      auto upper = std::upper_bound(lower, source.end(), tMax, comp2);
      return Vector(lower, upper);
    }


   private:                      // data members
    EventBuilderConfig fConfig;  ///< Configuration / parameters
  };

}  // namespace cbm::algo::evbuild

#endif /* CBM_ALGO_EVENTBUILDER_H */
