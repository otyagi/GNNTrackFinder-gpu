/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#pragma once

#include "AlgoTraits.h"
#include "SubChain.h"
#include "evselector/RecoEventSelectorMonitor.h"
#include "global/RecoResults.h"

#include <xpu/host.h>

// fwd declarations
class CbmEventTriggers;

namespace fles
{
  class Timeslice;
}

namespace cbm::algo
{
  class HistogramSender;
  class Options;
  class TrackingChain;
  class V0FinderChain;

  template<class M>
  struct UnpackMonitor;

  namespace bmon
  {
    class Unpack;
    class Calibrate;
    class Hitfind;
    class HitfindQa;
  }

  namespace much
  {
    class Unpack;
  }

  namespace rich
  {
    class Unpack;
  }

  namespace sts
  {
    class Unpack;
    class DigiQa;
    class HitfinderChain;
    class HitfinderMon;
  }

  namespace tof
  {
    class Unpack;
    class Calibrate;
    struct CalibrateMonitorData;
    class Hitfind;
    class HitfindQa;
    struct HitfindMonitorData;
  }

  namespace trd
  {
    class Unpack;
    class Hitfind;
    struct HitfindMonitorData;
  }

  namespace trd2d
  {
    class Unpack;
  }

  namespace evbuild
  {
    class EventbuildChain;
    struct EventbuildChainMonitorData;
  }  // namespace evbuild

  namespace ca
  {
    template<class C, class T>
    class MonitorData;
    enum class ECounter;
    enum class ETimer;
    using TrackingMonitorData = MonitorData<ECounter, ETimer>;
  }  // namespace ca

  template<class Unpacker>
  using UnpackResult_t = std::tuple<algo_traits::Output_t<Unpacker>, algo_traits::Aux_t<Unpacker>>;

  namespace qa
  {
    class RecoGeneralQa;
    class Manager;
  }
}  // namespace cbm::algo

namespace cbm::algo
{
  struct ProcessingMonitor {
    xpu::timings time;           //< total processing time
    xpu::timings timeUnpack;     //< time spent in unpacking
    xpu::timings timeSTS;        //< time spent in STS reco
    xpu::timings timeTOF;        //< time spent in TOF reco
    xpu::timings timeTRD;        //< time spent in TRD reco
    xpu::timings timeCA;         //< time spent in tracking
    std::optional<i64> tsDelta;  //< id difference between current and previous timeslice
  };

  /**
   * @brief Monitor for additional processing steps
   * @note Used in the main function, this should be eventually merged with ProcessingMonitor and we have a single class that handles the full processing loop
   */
  struct ProcessingExtraMonitor {
    xpu::timings timeWriteArchive;  //< time spent writing archive
    size_t bytesWritten;            //< bytes written to archive (estimated)
    double timeIdle = 0.;           //< time spent idle (waiting for next timeslice) [ms]
  };

  class Reco : SubChain {
   public:
    Reco();
    ~Reco();

    Reco(const Reco&) = delete;
    Reco& operator=(const Reco&) = delete;
    Reco(Reco&&)                 = delete;
    Reco& operator=(Reco&&) = delete;

    void Init(const Options&);
    RecoResults Run(const fles::Timeslice&);

    CbmEventTriggers ReconstructEvent(const DigiEvent& event);
    void Finalize();
    void PrintTimings(xpu::timings&);

    void QueueProcessingExtraMetrics(const ProcessingExtraMonitor&);

   private:
    bool fInitialized            = false;
    bool fbReconstructDigiEvents = false;
    ChainContext fContext;
    xpu::timings fTimesliceTimesAcc;
    std::shared_ptr<HistogramSender> fSender;
    uint64_t fRunStartTimeNs = 0;

    std::optional<u64> prevTsId;

    // General QA
    std::unique_ptr<qa::RecoGeneralQa> fGeneralQa;  ///< QA of online processing itself

    // BMON
    std::unique_ptr<bmon::Unpack> fBmonUnpack;
    std::unique_ptr<bmon::Calibrate> fBmonCalibrator;
    std::unique_ptr<bmon::Hitfind> fBmonHitFinder;
    std::unique_ptr<bmon::HitfindQa> fBmonHitFinderQa;

    // MUCH
    std::unique_ptr<much::Unpack> fMuchUnpack;

    // RICH
    std::unique_ptr<rich::Unpack> fRichUnpack;

    // STS
    std::unique_ptr<sts::Unpack> fStsUnpack;
    std::unique_ptr<sts::DigiQa> fStsDigiQa;  ///< Raw STS-digis QA
    std::unique_ptr<sts::HitfinderChain> fStsHitFinder;

    // TOF
    std::unique_ptr<tof::Unpack> fTofUnpack;
    std::unique_ptr<tof::Calibrate> fTofCalibrator;
    std::unique_ptr<tof::Hitfind> fTofHitFinder;
    std::unique_ptr<tof::HitfindQa> fTofHitFinderQa;

    // TRD
    std::unique_ptr<trd::Unpack> fTrdUnpack;
    std::unique_ptr<trd2d::Unpack> fTrd2dUnpack;
    std::unique_ptr<trd::Hitfind> fTrdHitfind;

    // Eventbuilding
    std::unique_ptr<evbuild::EventbuildChain> fEventBuild;

    // Tracking
    std::unique_ptr<TrackingChain> fTracking;       ///< Tracking in timeslice
    std::unique_ptr<TrackingChain> fTrackingEvent;  ///< Tracking in event

    // V0-finding
    std::unique_ptr<V0FinderChain> fV0Finder;  ///< V0-finding chain (in event or a bunch of events)

    // Event selection
    evselect::Monitor fEvSelectingMonitor;  ///< Monitor for event selecting

    // QA
    std::unique_ptr<qa::Manager> fQaManager;

    static double FilterNan(double x) { return std::isnan(x) || std::isinf(x) ? 0. : x; }

    void Validate(const Options& opts);

    template<class Unpacker>
    auto RunUnpacker(const std::unique_ptr<Unpacker>&, const fles::Timeslice&) -> UnpackResult_t<Unpacker>;

    template<class MSMonitor>
    void QueueUnpackerMetricsDet(const UnpackMonitor<MSMonitor>&);
    void QueueStsRecoMetrics(const sts::HitfinderMon&);
    void QueueTofRecoMetrics(const tof::HitfindMonitorData&);
    void QueueTrdRecoMetrics(const trd::HitfindMonitorData&);
    void QueueTofCalibMetrics(const tof::CalibrateMonitorData&);
    void QueueEvbuildMetrics(const evbuild::EventbuildChainMonitorData&);
    void QueueTrackingMetrics(const ca::TrackingMonitorData&);
    void QueueProcessingMetrics(const ProcessingMonitor&);
  };
}  // namespace cbm::algo
