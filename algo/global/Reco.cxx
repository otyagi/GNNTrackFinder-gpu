/* Copyright (C) 2023-2025 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], P.-A. Loizeau, Sergei Zharko */
#include "Reco.h"

#include "AlgoFairloggerCompat.h"
#include "AuxDigiData.h"
#include "BuildInfo.h"
#include "CbmDigiEvent.h"
#include "EventbuildChain.h"
#include "Exceptions.h"
#include "HistogramSender.h"
#include "ParFiles.h"
#include "RecoGeneralQa.h"
#include "StsDigiQa.h"
#include "TrackingSetup.h"
#include "bmon/Calibrate.h"
#include "bmon/Hitfind.h"
#include "bmon/ReadoutConfig.h"
#include "bmon/Unpack.h"
#include "ca/TrackingChain.h"
#include "ca/core/data/CaTrack.h"
#include "compat/OpenMP.h"
#include "evbuild/Config.h"
#include "kfp/KfpV0FinderChain.h"
#include "much/Unpack.h"
#include "qa/QaManager.h"
#include "qa/hitfind/BmonHitfindQa.h"
#include "qa/hitfind/TofHitfindQa.h"
#include "rich/Unpack.h"
#include "sts/ChannelMaskSet.h"
#include "sts/HitfinderChain.h"
#include "sts/Unpack.h"
#include "tof/Calibrate.h"
#include "tof/Hitfind.h"
#include "tof/Unpack.h"
#include "trd/Hitfind.h"
#include "trd/Unpack.h"
#include "trd2d/Unpack.h"
#include "util/TimingsFormat.h"
#include "util/TsUtils.h"
#include "yaml/Yaml.h"

#include <Monitor.hpp>
#include <System.hpp>

#include <xpu/host.h>

using namespace cbm::algo;
using fles::Subsystem;

namespace chron = std::chrono;

Reco::Reco() {}
Reco::~Reco() {}

void Reco::Validate(const Options& opts)
{
  if (!fs::exists(opts.ParamsDir())) throw FatalError("ParamsDir does not exist: {}", opts.ParamsDir().string());

  bool hasOutputFile = !opts.OutputFile().empty();
  bool hasOutputType = !opts.OutputTypes().empty();

  if (!hasOutputFile && hasOutputType) {
    throw FatalError("Output types specified, but no output file given: -o <file> missing");
  }

  if (hasOutputFile && !hasOutputType) {
    throw FatalError("Output file specified, but no output types given: -O <types> missing");
  }

  if (!BuildInfo::WITH_ZSTD && opts.CompressArchive()) {
    throw FatalError("Archive compression enabled but compiled without Zstd: Remove --archive-compression flag");
  }

  if (opts.Has(Step::LocalReco) && !opts.Has(Step::Unpack)) {
    throw FatalError("Local reco can't run without unpacking: Add 'Unpack' to the reco steps");
  }

  if (opts.Has(Step::Tracking) && !opts.Has(Step::LocalReco)) {
    throw FatalError("Tracking can't run without local reco: Add 'LocalReco' to the reco steps");
  }
}

void Reco::Init(const Options& opts)
{
  if (fInitialized) throw std::runtime_error("Chain already initialized");

  Validate(opts);

  fContext.opts = opts;
  SetContext(&fContext);

  if (Opts().HistogramUri() != "") {
    fSender =
      std::make_shared<HistogramSender>(Opts().HistogramUri(), Opts().HistogramHwm(), Opts().CompressHistograms());
    // fContext.sender = fSender;

    fRunStartTimeNs = Opts().RunStart();
    if (0 == fRunStartTimeNs) {
      fRunStartTimeNs = chron::duration_cast<chron::nanoseconds>(chron::system_clock::now().time_since_epoch()).count();
    }
  }

  xpu::device_prop props{xpu::device::active()};
  L_(info) << "Running CBM Reco on Device '" << props.name() << "' (Using " << openmp::GetMaxThreads()
           << " OpenMP threads)";

  if (!opts.MonitorUri().empty()) {
    fContext.monitor = std::make_unique<cbm::Monitor>(opts.MonitorUri());
    L_(info) << "Monitoring enabled, sending to " << opts.MonitorUri();
  }

  // Reco Params
  fContext.recoParams = yaml::ReadFromFile<RecoParams>(opts.ParamsDir() / "RecoParams.yaml");

  ParFiles parFiles(opts.RunId());
  L_(info) << "Using parameter files for setup " << parFiles.setup;

  // QA instantiation
  if (fSender != nullptr) {
    // QA manager
    fQaManager = std::make_unique<qa::Manager>(fSender);
    fQaManager->SetContext(&fContext);

    // General QA
    fGeneralQa = std::make_unique<qa::RecoGeneralQa>(fRunStartTimeNs, fSender);
  }

  // Unpackers
  if (Opts().Has(Subsystem::BMON) && Opts().Has(Step::Unpack)) {
    bmon::ReadoutSetup readoutSetup =
      yaml::ReadFromFile<bmon::ReadoutSetup>(Opts().ParamsDir() / parFiles.bmon.readout);
    bmon::ReadoutConfig cfg{readoutSetup};
    fBmonUnpack = std::make_unique<bmon::Unpack>(cfg);
  }

  if (Opts().Has(Subsystem::MUCH) && Opts().Has(Step::Unpack)) {
    much::ReadoutConfig cfg{};
    fMuchUnpack = std::make_unique<much::Unpack>(cfg);
  }

  if (Opts().Has(Subsystem::RICH) && Opts().Has(Step::Unpack)) {
    rich::ReadoutConfig cfg{};
    fRichUnpack = std::make_unique<rich::Unpack>(cfg);
  }

  if (Opts().Has(Subsystem::STS) && Opts().Has(Step::Unpack)) {
    sts::ReadoutSetup readoutSetup = yaml::ReadFromFile<sts::ReadoutSetup>(Opts().ParamsDir() / parFiles.sts.readout);
    auto chanMask = yaml::ReadFromFile<sts::ChannelMaskSet>(Opts().ParamsDir() / parFiles.sts.chanMask);
    auto walkMap  = yaml::ReadFromFile<sts::WalkMap>(Opts().ParamsDir() / parFiles.sts.walkMap);
    bool bCollectAux               = (fSender != nullptr && Opts().CollectAuxData());
    sts::ReadoutConfig readout{readoutSetup, chanMask};
    sts::Unpack::Config cfg{.readout = readout, .walkMap = walkMap, .bCollectAuxData = bCollectAux};
    fStsUnpack = std::make_unique<sts::Unpack>(cfg);
    if (fSender != nullptr && Opts().Has(QaStep::UnpackSts)) {
      fStsDigiQa = std::make_unique<sts::DigiQa>(fSender);
      fStsDigiQa->SetUseAuxData(bCollectAux);
      fStsDigiQa->RegisterReadoutSetup(readoutSetup);
      fStsDigiQa->Init();
    }
  }

  if (Opts().Has(Subsystem::TOF) && Opts().Has(Step::Unpack)) {
    tof::ReadoutSetup readoutSetup = yaml::ReadFromFile<tof::ReadoutSetup>(Opts().ParamsDir() / parFiles.tof.readout);
    tof::ReadoutConfig cfg{readoutSetup};
    fTofUnpack = std::make_unique<tof::Unpack>(cfg);
  }

  if (Opts().Has(Subsystem::TRD) && Opts().Has(Step::Unpack)) {
    auto cfg   = yaml::ReadFromFile<trd::ReadoutConfig>(Opts().ParamsDir() / parFiles.trd.readout);
    fTrdUnpack = std::make_unique<trd::Unpack>(cfg);
  }

  if (Opts().Has(Subsystem::TRD2D) && Opts().Has(Step::Unpack)) {
    auto setup = yaml::ReadFromFile<trd2d::ReadoutSetup>(Opts().ParamsDir() / parFiles.trd.readout2d);
    auto calib = yaml::ReadFromFile<trd2d::ReadoutCalib>(Opts().ParamsDir() / parFiles.trd.fee2d);
    trd2d::Unpack::Config cfg{.roSetup = setup, .roCalib = calib};
    fTrd2dUnpack = std::make_unique<trd2d::Unpack>(cfg);
  }

  // --- Tracking setup
  auto pTrackingSetup = std::make_shared<TrackingSetup>();
  pTrackingSetup->SetContext(&fContext);
  pTrackingSetup->Use(Subsystem::STS, Opts().Has(Subsystem::STS));
  pTrackingSetup->Use(Subsystem::TRD, Opts().Has(Subsystem::TRD));
  pTrackingSetup->Use(Subsystem::TOF, Opts().Has(Subsystem::TOF));
  pTrackingSetup->Init();

  // --- Event building
  if (Opts().Has(Step::DigiTrigger)) {
    fs::path configFile = opts.ParamsDir() / "EventbuildConfig.yaml";
    evbuild::Config config(YAML::LoadFile(configFile.string()));
    fEventBuild =
      std::make_unique<evbuild::EventbuildChain>(config, (Opts().Has(QaStep::EventBuilding) ? fSender : nullptr));
    if (fQaManager != nullptr && Opts().Has(QaStep::V0Trigger)) {
      // FIXME: Replace with a common function SetTriggerQa(fQaManager)
      auto pTriggerQa = std::make_shared<evbuild::V0TriggerQa>(fQaManager);
      pTriggerQa->Init();
      fEventBuild->SetV0TriggerQa(pTriggerQa);
    }
    fEventBuild->RegisterTrackingSetup(pTrackingSetup);
  }

  // STS Hitfinder
  if (Opts().Has(fles::Subsystem::STS) && Opts().Has(Step::LocalReco)) {
    sts::HitfinderPars hitFinderSetup =
      yaml::ReadFromFile<sts::HitfinderPars>(opts.ParamsDir() / parFiles.sts.hitfinder);
    hitFinderSetup.landauTable = sts::LandauTable::FromFile(opts.ParamsDir() / "LandauWidthTable.txt");
    sts::HitfinderChainPars hitFinderPars;
    hitFinderPars.setup  = std::move(hitFinderSetup);
    hitFinderPars.memory = Params().sts.memory;
    fStsHitFinder        = std::make_unique<sts::HitfinderChain>();
    fStsHitFinder->SetContext(&fContext);
    fStsHitFinder->SetParameters(hitFinderPars);
  }

  // TOF Hitfinder
  if (Opts().Has(fles::Subsystem::TOF) && Opts().Has(Step::LocalReco)) {
    auto calibSetup = yaml::ReadFromFile<tof::CalibrateSetup>(opts.ParamsDir() / parFiles.tof.calibrate);
    fTofCalibrator  = std::make_unique<tof::Calibrate>(calibSetup);

    auto hitfindSetup = yaml::ReadFromFile<tof::HitfindSetup>(opts.ParamsDir() / parFiles.tof.hitfinder);
    fTofHitFinder     = std::make_unique<tof::Hitfind>(hitfindSetup);

    if (fQaManager != nullptr && Opts().Has(QaStep::RecoTof)) {
      fTofHitFinderQa = std::make_unique<tof::HitfindQa>(fQaManager, "TofHitfind");
      fTofHitFinderQa->InitParameters(hitfindSetup);
      fTofHitFinderQa->Init();
    }
  }

  if (Opts().Has(fles::Subsystem::TRD) && Opts().Has(Step::LocalReco)) {
    auto setup   = yaml::ReadFromFile<trd::HitfindSetup>(opts.ParamsDir() / parFiles.trd.hitfinder);
    auto setup2d = yaml::ReadFromFile<trd::Hitfind2DSetup>(opts.ParamsDir() / parFiles.trd.hitfinder2d);
    fTrdHitfind  = std::make_unique<trd::Hitfind>(setup, setup2d);
  }

  // Digi event reconstruction:
  {
    fbReconstructDigiEvents = Opts().ReconstructDigiEvents();
    // It makes no sence to reconstruct an event, if there is no STS, TRD or TOF
    fbReconstructDigiEvents &= Opts().Has(fles::Subsystem::STS);
    fbReconstructDigiEvents &= Opts().Has(fles::Subsystem::TRD);
    fbReconstructDigiEvents &= Opts().Has(fles::Subsystem::TOF);
    fbReconstructDigiEvents &= Opts().Has(fles::Subsystem::BMON);
  }

  // Tracking
  if (Opts().Has(Step::Tracking)) {
    if (fQaManager != nullptr && Opts().Has(QaStep::Tracking)) {
      fTracking = std::make_unique<TrackingChain>(ECbmRecoMode::Timeslice, fQaManager, "CaTimeslice");
    }
    else {
      fTracking = std::make_unique<TrackingChain>(ECbmRecoMode::Timeslice);
    }
    fTracking->RegisterSetup(pTrackingSetup);
    fTracking->SetContext(&fContext);
    fTracking->Init();
  }

  if (fbReconstructDigiEvents) {
    fEvSelectingMonitor.Reset();

    // BMON hit finding in event reconstruction
    auto bmonCalSetup = yaml::ReadFromFile<bmon::CalibrateSetup>(opts.ParamsDir() / parFiles.bmon.calibrate);
    auto bmonHitSetup = yaml::ReadFromFile<bmon::HitfindSetup>(opts.ParamsDir() / parFiles.bmon.hitfinder);
    fBmonCalibrator   = std::make_unique<bmon::Calibrate>(bmonCalSetup);
    fBmonHitFinder    = std::make_unique<bmon::Hitfind>(bmonHitSetup);
    if (fQaManager != nullptr && Opts().Has(QaStep::RecoBmon)) {
      fBmonHitFinderQa = std::make_unique<bmon::HitfindQa>(fQaManager, "BmonHitfindEvent");
      fBmonHitFinderQa->InitParameters(bmonCalSetup, bmonHitSetup);
      fBmonHitFinderQa->Init();
    }

    // Tracking in event reconstruction
    if (fQaManager != nullptr && Opts().Has(QaStep::Tracking)) {
      fTrackingEvent = std::make_unique<TrackingChain>(ECbmRecoMode::EventByEvent, fQaManager, "CaEvent");
    }
    else {
      fTrackingEvent = std::make_unique<TrackingChain>(ECbmRecoMode::EventByEvent);
    }
    fTrackingEvent->RegisterSetup(pTrackingSetup);
    fTrackingEvent->SetContext(&fContext);
    fTrackingEvent->Init();

    if (fQaManager != nullptr && Opts().Has(QaStep::V0Finder)) {
      fV0Finder = std::make_unique<V0FinderChain>(fQaManager);
    }
    else {
      fV0Finder = std::make_unique<V0FinderChain>();
    }
    fV0Finder->SetContext(&fContext);
    fV0Finder->SetBmonDefinedAddresses(fBmonHitFinder->GetDiamondAddresses());
    fV0Finder->Init();
  }

  // Initialize the QA manager
  if (fQaManager != nullptr) {
    fQaManager->Init();
  }

  fInitialized = true;

  L_(debug) << "CBM Reco finished initialization";
}


RecoResults Reco::Run(const fles::Timeslice& ts)
{
  if (!fInitialized) {
    throw std::runtime_error("Chain not initialized");
  }

  ProcessingMonitor procMon;

  RecoResults recoData;  /// transient
  RecoResults results;   /// persistent (return object)
  {
    xpu::scoped_timer t_(fmt::format("TS {}", ts.index()), &procMon.time);
    xpu::t_add_bytes(ts_utils::SizeBytes(ts));

    L_(info) << ">>> Processing TS " << ts.index();
    xpu::set<cbm::algo::Params>(Params());

    DigiData digis;
    AuxDigiData auxDigis;

    if (Opts().Has(Step::Unpack)) {
      xpu::scoped_timer timerU("Unpack", &procMon.timeUnpack);
      xpu::t_add_bytes(ts_utils::SizeBytes(ts));

      std::tie(digis.fBmon, auxDigis.fBmon)   = RunUnpacker(fBmonUnpack, ts);
      std::tie(digis.fMuch, auxDigis.fMuch)   = RunUnpacker(fMuchUnpack, ts);
      std::tie(digis.fRich, auxDigis.fRich)   = RunUnpacker(fRichUnpack, ts);
      std::tie(digis.fSts, auxDigis.fSts)     = RunUnpacker(fStsUnpack, ts);
      std::tie(digis.fTof, auxDigis.fTof)     = RunUnpacker(fTofUnpack, ts);
      std::tie(digis.fTrd, auxDigis.fTrd)     = RunUnpacker(fTrdUnpack, ts);
      std::tie(digis.fTrd2d, auxDigis.fTrd2d) = RunUnpacker(fTrd2dUnpack, ts);

      // No unpackers for these yet
      // digis.fPsd   = RunUnpacker(fPsdUnpack, ts);
      // digis.fFsd   = RunUnpacker(fFsdUnpack, ts);

      L_(info) << "TS contains Digis: STS=" << digis.fSts.size() << " MUCH=" << digis.fMuch.size()
               << " TOF=" << digis.fTof.size() << " BMON=" << digis.fBmon.size() << " TRD=" << digis.fTrd.size()
               << " TRD2D=" << digis.fTrd2d.size() << " RICH=" << digis.fRich.size() << " PSD=" << digis.fPsd.size()
               << " FSD=" << digis.fFsd.size();
      // --- Raw digi QAs
      if (fSender != nullptr && Opts().Has(Subsystem::STS)) {
        fStsDigiQa->RegisterDigiData(&digis.fSts);
        fStsDigiQa->RegisterAuxDigiData(&auxDigis.fSts);
        fStsDigiQa->SetTimesliceIndex(ts.index());
        fStsDigiQa->Exec();
      }
    }


    sts::HitfinderMon stsHitfinderMonitor;
    if (fStsHitFinder) {
      xpu::scoped_timer timerSTS("STS Reco", &procMon.timeSTS);
      xpu::t_add_bytes(digis.fSts.size() * sizeof(CbmStsDigi));
      bool storeClusters   = Opts().HasOutput(RecoData::Cluster);
      auto stsResults      = (*fStsHitFinder)(digis.fSts, storeClusters);
      stsHitfinderMonitor  = std::move(stsResults.monitor);
      recoData.stsHits     = stsResults.hits;
      recoData.stsClusters = std::move(stsResults.clusters);
      QueueStsRecoMetrics(stsHitfinderMonitor);
    }

    PartitionedVector<tof::Hit> tofHits;
    if (Opts().Has(Step::LocalReco) && Opts().Has(fles::Subsystem::TOF)) {
      xpu::scoped_timer timerTOF("TOF Reco", &procMon.timeTOF);
      xpu::t_add_bytes(digis.fTof.size() * sizeof(CbmTofDigi));
      auto [caldigis, calmonitor] = (*fTofCalibrator)(digis.fTof);
      auto nUnknownRPC            = calmonitor.fDigiCalibUnknownRPC;
      if (nUnknownRPC > 0) {
        L_(error) << "TOF Digis with unknown RPCs: " << nUnknownRPC;
      }
      auto [hits, hitmonitor, digiindices] = (*fTofHitFinder)(caldigis);
      if (fTofHitFinderQa != nullptr) {
        fTofHitFinderQa->RegisterHits(&hits);
        fTofHitFinderQa->Exec();
      }
      recoData.tofHits                     = std::move(hits);
      QueueTofCalibMetrics(calmonitor);
      QueueTofRecoMetrics(hitmonitor);
    }

    PartitionedVector<trd::Hit> trdHits;
    if (fTrdHitfind) {
      xpu::scoped_timer timerTRD("TRD Reco", &procMon.timeTRD);
      xpu::t_add_bytes(digis.fTrd.size() * sizeof(CbmTrdDigi));
      // FIXME: additional copy of digis, figure out how to pass 1d + 2d digis at once to hitfinder
      const auto& digis1d = digis.fTrd;
      const auto& digis2d = digis.fTrd2d;
      PODVector<CbmTrdDigi> allDigis{};
      allDigis.reserve(digis1d.size() + digis2d.size());
      std::copy(digis1d.begin(), digis1d.end(), std::back_inserter(allDigis));
      std::copy(digis2d.begin(), digis2d.end(), std::back_inserter(allDigis));
      auto trdResults  = (*fTrdHitfind)(allDigis);
      recoData.trdHits = std::move(std::get<0>(trdResults));
      QueueTrdRecoMetrics(std::get<1>(trdResults));
    }

    L_(info) << "TS contains Hits: STS=" << recoData.stsHits.NElements() << " TOF=" << recoData.tofHits.NElements()
             << " TRD=" << recoData.trdHits.NElements();


    // --- Tracking
    TrackingChain::Output_t trackingOutput{};
    if (Opts().Has(Step::Tracking)) {
      xpu::scoped_timer timerCA("CA", &procMon.timeCA);
      xpu::t_add_bytes(recoData.stsHits.NElements() * sizeof(sts::Hit));
      xpu::t_add_bytes(recoData.tofHits.NElements() * sizeof(tof::Hit));
      xpu::t_add_bytes(recoData.trdHits.NElements() * sizeof(trd::Hit));
      TrackingChain::Input_t input{
        .stsHits = recoData.stsHits,
        .tofHits = recoData.tofHits,
        .trdHits = recoData.trdHits,
      };
      trackingOutput  = fTracking->Run(input);
      recoData.tracks = std::move(trackingOutput.tracks);
      std::sort(recoData.tracks.begin(), recoData.tracks.end(),
                [](const cbm::algo::ca::Track& track1, const cbm::algo::ca::Track& track2) {
                  return track1.fParPV.Time() < track2.fParPV.Time();
                });
      QueueTrackingMetrics(trackingOutput.monitorData);
    }

    // --- Event building
    std::vector<DigiEvent> events;
    evbuild::EventbuildChainMonitorData evbuildMonitor;
    if (Opts().Has(Step::DigiTrigger)) {
      auto [ev, mon] = fEventBuild->Run(digis, recoData);
      events         = std::move(ev);
      evbuildMonitor = mon;
      QueueEvbuildMetrics(evbuildMonitor);
    }

    // --- Reconstruct and select digi events
    if (fbReconstructDigiEvents) {
      fEvSelectingMonitor.IncrementCounter(evselect::ECounter::Timeslices);
      fEvSelectingMonitor.IncrementCounter(evselect::ECounter::EventsTotal, events.size());
      for (auto& event : events) {
        fEvSelectingMonitor.StartTimer(evselect::ETimer::EventReconstruction);
        event.fSelectionTriggers = ReconstructEvent(event);
        fEvSelectingMonitor.StopTimer(evselect::ETimer::EventReconstruction);
      }
      auto v0FinderMonitor = fV0Finder->GetMonitor();
      fEvSelectingMonitor.IncrementCounter(evselect::ECounter::LambdaCandidates,
                                           v0FinderMonitor.GetCounterValue(kfp::ECounter::KfpLambdaCandidates));
    }

    // --- Filter data for output
    if (Opts().HasOutput(RecoData::DigiTimeslice)) {
      results.bmonDigis  = std::move(digis.fBmon);
      results.stsDigis   = std::move(digis.fSts);
      results.muchDigis  = std::move(digis.fMuch);
      results.trd2dDigis = std::move(digis.fTrd2d);
      results.trdDigis   = std::move(digis.fTrd);
      results.tofDigis   = std::move(digis.fTof);
      results.richDigis  = std::move(digis.fRich);
    }
    if (Opts().HasOutput(RecoData::Track)) {
      results.tracks             = std::move(recoData.tracks);
      results.trackStsHitIndices = std::move(trackingOutput.stsHitIndices);
      results.trackTofHitIndices = std::move(trackingOutput.tofHitIndices);
    }
    if (Opts().HasOutput(RecoData::DigiEvent)) results.events = std::move(events);
    if (Opts().HasOutput(RecoData::Cluster)) results.stsClusters = std::move(recoData.stsClusters);
    if (Opts().HasOutput(RecoData::Hit)) {
      results.stsHits = std::move(recoData.stsHits);
      results.tofHits = std::move(recoData.tofHits);
      results.trdHits = std::move(recoData.trdHits);
    }

    // QA
    if (fSender != nullptr) {
      (*fGeneralQa)(ts);

      // Send all the histograms, collected through the timeslice
      fQaManager->SetTimesliceId(ts.index());
      fQaManager->SendHistograms();
    }
  }
  PrintTimings(procMon.time);
  if (prevTsId) {
    procMon.tsDelta = ts.index() - *prevTsId;
  }
  prevTsId = ts.index();
  QueueProcessingMetrics(procMon);

  return results;
}

void Reco::Finalize()
{
  if (fStsHitFinder) {
    fStsHitFinder->Finalize();
  }
  if (fTracking) {
    L_(info) << "Track finding in a timeslice:";
    fTracking->Finalize();
  }
  if (fTrackingEvent) {
    L_(info) << "Track finding in digi events:";
    fTrackingEvent->Finalize();
  }
  if (fbReconstructDigiEvents) {
    fV0Finder->Finalize();
    L_(info) << fEvSelectingMonitor.ToString();
  }


  if (Opts().Profiling() >= ProfilingSummary) {
    L_(info) << MakeReportSubtimers("Run Summary", fTimesliceTimesAcc) << "\n"
             << MakeReportSummary("Total", fTimesliceTimesAcc);

    if (Opts().TimingsFile() != "") {
      std::ofstream file(Opts().TimingsFile().string());
      file << MakeReportYaml(fTimesliceTimesAcc);
    }
  }
}

void Reco::PrintTimings(xpu::timings& timings)
{
  if (Opts().CollectKernelTimes()) {
    fTimesliceTimesAcc.merge(timings);
  }

  if (Opts().Profiling() >= ProfilingPerTS) {
    L_(info) << MakeReportSubtimers("TS timings", timings) << "\n" << MakeReportSummary("Total", timings);
  }
  else {
    L_(info) << "TS Processing time (Wall): " << timings.wall() << " ms";
  }
}

CbmEventTriggers Reco::ReconstructEvent(const DigiEvent& digiEvent)
{
  CbmEventTriggers triggers(0);
  RecoResults recoEvent;
  //* BMON hit reconstruction
  {
    fEvSelectingMonitor.StartTimer(evselect::ETimer::BmonHitFinder);
    auto [calDigis, calMonitor]          = (*fBmonCalibrator)(digiEvent.fBmon);
    auto [hits, hitMonitor, digiIndices] = (*fBmonHitFinder)(calDigis);
    fEvSelectingMonitor.StopTimer(evselect::ETimer::BmonHitFinder);
    if (fBmonHitFinderQa != nullptr) {
      fBmonHitFinderQa->RegisterDigis(&calDigis);
      fBmonHitFinderQa->RegisterHits(&hits);
      fBmonHitFinderQa->RegisterDigiIndices(&digiIndices);
      fBmonHitFinderQa->Exec();
    }
    recoEvent.bmonHits = std::move(hits);
  }

  //* STS hit reconstruction
  {
    fEvSelectingMonitor.StartTimer(evselect::ETimer::StsHitFinder);
    auto stsResults = (*fStsHitFinder)(digiEvent.fSts);
    fEvSelectingMonitor.StopTimer(evselect::ETimer::StsHitFinder);
    if (stsResults.hits.NElements() < 4) {  // TODO: Provide a config for cuts (testing mode for now)
      fEvSelectingMonitor.IncrementCounter(evselect::ECounter::EventsNeStsHits);
      return triggers;
    }
    recoEvent.stsHits = stsResults.hits;
  }

  //* TOF hit reconstruction
  {
    fEvSelectingMonitor.StartTimer(evselect::ETimer::TofHitFinder);
    auto [caldigis, calmonitor]          = (*fTofCalibrator)(digiEvent.fTof);
    auto [hits, hitmonitor, digiindices] = (*fTofHitFinder)(caldigis);
    fEvSelectingMonitor.StopTimer(evselect::ETimer::TofHitFinder);
    if (hits.NElements() < 2) {  // TODO: Provide a config for cuts (testing mode for now)
      fEvSelectingMonitor.IncrementCounter(evselect::ECounter::EventsNeTofHits);
      return triggers;
    }
    recoEvent.tofHits = std::move(hits);
  }

  //* TRD hit reconstruction
  {
    // FIXME: additional copy of digis, figure out how to pass 1d + 2d digis at once to hitfinder
    fEvSelectingMonitor.StartTimer(evselect::ETimer::TrdHitFinder);
    const auto& digis1d = digiEvent.fTrd;
    const auto& digis2d = digiEvent.fTrd2d;
    PODVector<CbmTrdDigi> allDigis{};
    allDigis.reserve(digis1d.size() + digis2d.size());
    std::copy(digis1d.begin(), digis1d.end(), std::back_inserter(allDigis));
    std::copy(digis2d.begin(), digis2d.end(), std::back_inserter(allDigis));
    auto trdResults = (*fTrdHitfind)(allDigis);
    fEvSelectingMonitor.StopTimer(evselect::ETimer::TrdHitFinder);
    recoEvent.trdHits = std::move(std::get<0>(trdResults));
  }

  //* Tracking
  {
    fEvSelectingMonitor.StartTimer(evselect::ETimer::TrackFinder);
    TrackingChain::Input_t input{.stsHits = recoEvent.stsHits,
                                 .tofHits = recoEvent.tofHits,
                                 .trdHits = recoEvent.trdHits};
    TrackingChain::Output_t output = fTrackingEvent->Run(input);
    recoEvent.tracks               = std::move(output.tracks);
    recoEvent.trackStsHitIndices   = std::move(output.stsHitIndices);
    recoEvent.trackTofHitIndices   = std::move(output.tofHitIndices);
    recoEvent.trackTrdHitIndices   = std::move(output.trdHitIndices);
    fEvSelectingMonitor.StopTimer(evselect::ETimer::TrackFinder);
    if (recoEvent.tracks.size() < 2) {  // Reject all events with less then two tracks
      fEvSelectingMonitor.IncrementCounter(evselect::ECounter::EventsNeTracks);
      return triggers;
    }
  }

  //* V0-selector
  fEvSelectingMonitor.StartTimer(evselect::ETimer::V0Finder);
  triggers = fV0Finder->ProcessEvent(recoEvent);
  if (triggers.Test(CbmEventTriggers::ETrigger::Lambda)) {
    L_(info) << "!!! Found event with potential lambda candidates";
    fEvSelectingMonitor.IncrementCounter(evselect::ECounter::EventsSelected);
  }
  fEvSelectingMonitor.StopTimer(evselect::ETimer::V0Finder);
  return triggers;
}


template<class Unpacker>
auto Reco::RunUnpacker(const std::unique_ptr<Unpacker>& unpacker, const fles::Timeslice& ts) -> UnpackResult_t<Unpacker>
{
  if (!unpacker) {
    return {};
  }
  auto [digis, monitor, aux] = (*unpacker)(ts);
  QueueUnpackerMetricsDet(monitor);
  return std::make_tuple(digis, aux);
}

template<class MSMonitor>
void Reco::QueueUnpackerMetricsDet(const UnpackMonitor<MSMonitor>& monitor)
{
  if (!HasMonitor()) {
    return;
  }

  std::string_view det = ToString(monitor.system);

  auto MkKey = [&](std::string_view key) { return fmt::format("{}{}", key, Capitalize(det)); };

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           {
                             {MkKey("unpackBytesIn"), monitor.sizeBytesIn},
                             {MkKey("unpackBytesOut"), monitor.sizeBytesOut},
                             {MkKey("unpackExpansionFactor"), monitor.ExpansionFactor()},
                             {MkKey("unpackNumMs"), monitor.numMs},
                             {MkKey("unpackNumErrInvalidSysVer"), monitor.errInvalidSysVer},
                             {MkKey("unpackNumErrInvalidEqId"), monitor.errInvalidEqId},
                           });
}

void Reco::QueueStsRecoMetrics(const sts::HitfinderMon& monitor)
{
  if (!HasMonitor()) return;

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           {
                             {"stsRecoNumClusters", (unsigned long) monitor.nClusterTotal},
                             {"stsRecoNumHits", (unsigned long) monitor.nHitsTotal},
                             {"stsRecoNumClusterBucketOverflow", monitor.nClusterBucketOverflow},
                             {"stsRecoNumHitBucketOverflow", monitor.nHitBucketOverflow},
                           });
}

void Reco::QueueTofRecoMetrics(const tof::HitfindMonitorData& mon)
{
  if (!HasMonitor()) return;

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           {
                             {"tofRecoNumDigisIn", mon.fNumDigis},
                             {"tofRecoNumHits", mon.fNumHits},
                           });
}

void Reco::QueueTrdRecoMetrics(const trd::HitfindMonitorData& mon)
{
  if (!HasMonitor()) {
    return;
  }

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           {
                             {"trdRecoNumDigisIn", mon.numDigis},
                             {"trdRecoNumHits", mon.numHits},
                           });
}

void Reco::QueueTofCalibMetrics(const tof::CalibrateMonitorData& mon)
{
  if (!HasMonitor()) return;

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           {
                             {"tofCalibTimeTotal", mon.fTime.wall()},
                             {"tofCalibThroughput", FilterNan(mon.fTime.throughput())},
                             {"tofCalibNumDigisIn", mon.fNumDigis},
                             {"tofCalibUnknownRPC", mon.fDigiCalibUnknownRPC},
                           });
}


void Reco::QueueEvbuildMetrics(const evbuild::EventbuildChainMonitorData& mon)
{
  if (!HasMonitor()) return;

  const MetricTagSet tags = {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}};

  size_t nDigisTotal         = 0;
  size_t nDigisInEventsTotal = 0;

  auto queueDetMetrics = [&](std::string_view det, auto& detMon) {
    size_t nDigis         = detMon.nDigis;
    size_t nDigisInEvents = detMon.nDigisInEvents;
    double selectionRatio = nDigis > 0 ? double(nDigisInEvents) / nDigis : 0;

    nDigisTotal += nDigis;
    nDigisInEventsTotal += nDigisInEvents;

    GetMonitor().QueueMetric("cbmreco", tags,
                             {{fmt::format("{}NumDigisTotal", det), nDigis},
                              {fmt::format("{}NumDigisInEvents", det), nDigisInEvents},
                              {fmt::format("{}EvSelectionRatio", det), selectionRatio}});
  };

  queueDetMetrics("sts", mon.evbuild.sts);
  queueDetMetrics("much", mon.evbuild.much);
  queueDetMetrics("tof", mon.evbuild.tof);
  queueDetMetrics("bmon", mon.evbuild.bmon);
  queueDetMetrics("trd", mon.evbuild.trd);
  queueDetMetrics("trd2d", mon.evbuild.trd2d);
  queueDetMetrics("rich", mon.evbuild.rich);

  double totalSelectionRatio = nDigisTotal > 0 ? double(nDigisInEventsTotal) / nDigisTotal : 0;
  GetMonitor().QueueMetric("cbmreco", tags,
                           {{"digiTriggerTimeTotal", mon.digiMultTrigger.time.wall()},
                            {"digiTriggerThroughput", FilterNan(mon.digiMultTrigger.time.throughput())},
                            {"hitTriggerTimeTotal", mon.hitMultTrigger.time.wall()},
                            {"hitTriggerThroughput", FilterNan(mon.hitMultTrigger.time.throughput())},
                            {"v0TriggerNumTrackPairs", mon.v0Trigger.numTrackPairs},
                            {"v0TriggerNumTrackPairsCoinc", mon.v0Trigger.numTrackPairsAfterTimeCut},
                            {"v0TriggerErrTracksUnsorted", mon.v0Trigger.errTracksUnsorted},
                            {"v0TriggerTimeTotal", mon.v0Trigger.time.wall()},
                            {"v0TriggerThroughput", FilterNan(mon.v0Trigger.time.throughput())},
                            {"eventbuildTimeTotal", mon.evbuild.time.wall()},
                            {"eventbuildThroughput", FilterNan(mon.evbuild.time.throughput())},
                            {"numTrigger", mon.evbuild.numTriggers},
                            {"numEvents", mon.evbuild.numEvents},
                            {"totalEvSelectionRatio", totalSelectionRatio}});
}

void Reco::QueueTrackingMetrics(const ca::TrackingMonitorData& monitor)
{
  if (!HasMonitor()) {
    return;
  }

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           {{"caTrackFinderTime", monitor.GetTimer(ca::ETimer::FindTracks).GetTotalMs()},
                            {"caTrackFitterTime", monitor.GetTimer(ca::ETimer::FitTracks).GetTotalMs()},
                            {"caNofRecoTracks", monitor.GetCounterValue(ca::ECounter::RecoTrack)},
                            {"caNofRecoHitsTotal", monitor.GetCounterValue(ca::ECounter::RecoHit)},
                            {"caNofRecoHitsUsed", monitor.GetCounterValue(ca::ECounter::RecoHitUsed)},
                            {"caNofWindows", monitor.GetCounterValue(ca::ECounter::SubTS)}});
}

void Reco::QueueProcessingMetrics(const ProcessingMonitor& mon)
{
  if (!HasMonitor()) {
    return;
  }

  MetricFieldSet fields = {
    {"processingTimeTotal", mon.time.wall()},   {"processingThroughput", FilterNan(mon.time.throughput())},
    {"caRecoTimeTotal", mon.timeCA.wall()},     {"caRecoThroughput", FilterNan(mon.timeCA.throughput())},
    {"trdRecoTimeTotal", mon.timeTRD.wall()},   {"trdRecoThroughput", FilterNan(mon.timeTRD.throughput())},
    {"tofRecoTimeTotal", mon.timeTOF.wall()},   {"tofRecoThroughput", FilterNan(mon.timeTOF.throughput())},
    {"stsRecoTimeTotal", mon.timeSTS.wall()},   {"stsRecoThroughput", FilterNan(mon.timeSTS.throughput())},
    {"unpackTimeTotal", mon.timeUnpack.wall()}, {"unpackThroughput", FilterNan(mon.timeUnpack.throughput())}};

  if (mon.tsDelta) {
    fields.emplace_back("tsDelta", *mon.tsDelta);
  }

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           std::move(fields));
}

void Reco::QueueProcessingExtraMetrics(const ProcessingExtraMonitor& mon)
{
  if (!HasMonitor()) {
    return;
  }

  MetricFieldSet fields = {{"processingTimeIdle", FilterNan(mon.timeIdle)},
                           {"processingTimeWriteArchive", mon.timeWriteArchive.wall()},
                           {"processingThroughputWriteArchive", FilterNan(mon.timeWriteArchive.throughput())},
                           {"processingBytesWritten", FilterNan(mon.bytesWritten)}};

  GetMonitor().QueueMetric("cbmreco", {{"hostname", fles::system::current_hostname()}, {"child", Opts().ChildId()}},
                           std::move(fields));
}
