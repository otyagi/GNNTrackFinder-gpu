/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "BuildInfo.h"
#include "CbmDigiEvent.h"
#include "Exceptions.h"
#include "Options.h"
#include "Reco.h"
#include "RecoResultsInputArchive.h"
#include "RecoResultsOutputArchive.h"
#include "System.h"
#include "compat/Algorithm.h"
#include "compat/OpenMP.h"
#include "gpu/DeviceImage.h"
#include "util/MemoryLogger.h"

#include <StorableTimeslice.hpp>
#include <TimesliceAutoSource.hpp>

#include <log.hpp>
#include <sstream>

#include <xpu/host.h>

using namespace cbm::algo;

namespace chron = std::chrono;

std::shared_ptr<StorableRecoResults> makeStorableRecoResults(const fles::Timeslice& ts, const RecoResults& results)
{
  auto storable = std::make_shared<StorableRecoResults>(ts.index(), ts.start_time());

  storable->DigiEvents().reserve(results.events.size());
  for (const auto& digiEvent : results.events) {
    storable->DigiEvents().emplace_back(digiEvent.ToStorable());
  }

  // TODO: some of these copies can be avoided / made into moves
  storable->BmonDigis()  = ToStdVector(results.bmonDigis);
  storable->StsDigis()   = ToStdVector(results.stsDigis);
  storable->MuchDigis()  = ToStdVector(results.muchDigis);
  storable->Trd2dDigis() = ToStdVector(results.trd2dDigis);
  storable->TrdDigis()   = ToStdVector(results.trdDigis);
  storable->TofDigis()   = ToStdVector(results.tofDigis);
  storable->RichDigis()  = ToStdVector(results.richDigis);

  storable->StsClusters() = results.stsClusters;
  storable->StsHits()     = results.stsHits;
  storable->TofHits()     = results.tofHits;
  storable->TrdHits()     = results.trdHits;

  storable->Tracks()             = results.tracks;
  storable->TrackStsHitIndices() = results.trackStsHitIndices;
  storable->TrackTofHitIndices() = results.trackTofHitIndices;

  return storable;
}

bool dumpArchive(const Options& opts)
{
  // Limit the number of events per timeslice to dump to avoid spamming the log
  constexpr size_t DumpEventsPerTS   = 10;
  constexpr size_t DumpHitsPerSensor = 2;
  constexpr size_t DumpTracksPerTS   = 10;

  if (!opts.DumpArchive()) return false;

  L_(info) << "Dumping archive: " << opts.InputLocator();

  RecoResultsInputArchive archive(opts.InputLocator());

  auto desc = archive.descriptor();
  L_(info) << "Archive descriptor: ";
  L_(info) << " - time_created: " << desc.time_created();
  L_(info) << " - hostname: " << desc.hostname();
  L_(info) << " - username: " << desc.username();

  for (auto recoResults = archive.get(); !archive.eos(); recoResults = archive.get()) {
    if (recoResults == nullptr) {
      L_(error) << "Failed to read RecoResults from archive";
      break;
    }

    size_t nEvents = recoResults->DigiEvents().size();
    L_(info) << "TS " << recoResults->TsIndex() << " start: " << recoResults->TsStartTime() << " events: " << nEvents
             << ", stsHits: " << recoResults->StsHits().NElements()
             << ", tofHits: " << recoResults->TofHits().NElements() << ", tracks: " << recoResults->Tracks().size();

    for (size_t i = 0; i < std::min(DumpEventsPerTS, nEvents); i++) {
      const auto& digiEvent = recoResults->DigiEvents().at(i);
      L_(info) << " - Event " << i << " number: " << digiEvent.fNumber << "; time: " << digiEvent.fTime
               << "; nStsDigis: " << digiEvent.fData.Size(ECbmModuleId::kSts)
               << "; nTofDigis: " << digiEvent.fData.Size(ECbmModuleId::kTof);
    }

    if (nEvents > DumpEventsPerTS) L_(info) << "...";

    auto& stsHits = recoResults->StsHits();
    for (size_t m = 0; m < stsHits.NPartitions(); m++) {
      auto [hits, address] = stsHits.Partition(m);
      for (size_t i = 0; i < std::min(DumpHitsPerSensor, hits.size()); i++) {
        const auto& hit = hits[i];
        L_(info) << " - STS Hit " << i << " sensor: " << address << "; time: " << hit.fTime << ", X: " << hit.fX
                 << ", Y: " << hit.fY << ", Z: " << hit.fZ;
      }
    }

    L_(info) << "...";

    auto tofHits = recoResults->TofHits();
    for (size_t m = 0; m < tofHits.NPartitions(); m++) {
      auto [hits, address] = tofHits.Partition(m);
      for (size_t i = 0; i < std::min(DumpHitsPerSensor, hits.size()); i++) {
        const auto& hit = hits[i];
        L_(info) << " - TOF Hit " << i << " sensor: " << address << "; time: " << hit.Time() << ", X: " << hit.X()
                 << ", Y: " << hit.Y() << ", Z: " << hit.Z();
      }
    }

    L_(info) << "...";

    auto& tracks = recoResults->Tracks();
    for (size_t t = 0; t < std::min(tracks.size(), DumpTracksPerTS); t++) {
      const auto& track = tracks[t];
      L_(info) << " - Track " << t << " nHits: " << track.fNofHits << ", chi2: " << track.fParPV.ChiSq()
               << ", X: " << track.fParPV.X() << ", Y: " << track.fParPV.Y() << ", Z: " << track.fParPV.Z();
    }
  }

  return true;
}

int main(int argc, char** argv)
{
  Options opts(argc, argv);

  logging::add_console(opts.LogLevel());

  if (!opts.LogFile().empty()) logging::add_file(opts.LogFile().string(), opts.LogLevel());

  // XPU
  xpu::settings settings;
  settings.profile = opts.CollectKernelTimes();
  settings.device  = opts.Device();
  if (opts.LogLevel() == trace) {
    settings.verbose      = true;
    settings.logging_sink = [](std::string_view msg) { L_(trace) << msg; };
  }
  xpu::initialize(settings);
  xpu::preload<GPUReco>();

  auto ompThreads = opts.NumOMPThreads();
  if (ompThreads) {
    L_(debug) << *ompThreads << " OpenMP threads requested";
    openmp::SetNumThreads(*ompThreads);
  }
  GetGlobalSTLThreadPool().set_num_threads(openmp::GetMaxThreads());

  L_(info) << "CBMRECO buildType=" << BuildInfo::BUILD_TYPE << " gpuDebug=" << BuildInfo::GPU_DEBUG
           << " parallelSTL=" << BuildInfo::WITH_PARALLEL_STL << " OMP=" << BuildInfo::WITH_OMP
           << " ZSTD=" << BuildInfo::WITH_ZSTD << " commit=" << BuildInfo::GIT_HASH;
  std::stringstream ss;
  for (int i = 0; i < argc; i++) {
    ss << argv[i] << " ";
  }
  L_(info) << ss.str();

  if (dumpArchive(opts)) return 0;

  Reco reco;
  MemoryLogger memoryLogger;

  auto startProcessing = chron::high_resolution_clock::now();
  reco.Init(opts);

  fles::TimesliceAutoSource source(opts.InputLocator());

  ProcessingExtraMonitor extraMonitor;

  std::optional<RecoResultsOutputArchive> archive;
  if (!opts.OutputFile().empty()) {
    L_(info) << "Writing results to file: " << opts.OutputFile();
    fles::ArchiveCompression compression = fles::ArchiveCompression::None;
    if (opts.CompressArchive()) {
      compression = fles::ArchiveCompression::Zstd;
    }
    archive.emplace(opts.OutputFile().string(), compression);
  }

  int tsIdx  = 0;
  int num_ts = opts.NumTimeslices();
  if (num_ts > 0) num_ts += opts.SkipTimeslices();
  L_(debug) << "Starting to fetch timeslices from source...";

  auto startFetchTS = chron::high_resolution_clock::now();
  while (auto timeslice = source.get()) {
    if (tsIdx < opts.SkipTimeslices()) {
      tsIdx++;
      continue;
    }

    std::unique_ptr<fles::Timeslice> ts;
    if (opts.ReleaseMode()) {
      ts = std::make_unique<fles::StorableTimeslice>(*timeslice);
      timeslice.reset();
    }
    else {
      ts = std::move(timeslice);
    }

    auto endFetchTS       = chron::high_resolution_clock::now();
    auto durationFetchTS  = endFetchTS - startFetchTS;
    extraMonitor.timeIdle = chron::duration_cast<chron::duration<double, std::milli>>(durationFetchTS).count();

    try {
      RecoResults result = reco.Run(*ts);
      if (archive) {
        xpu::scoped_timer t_{"Write Archive", &extraMonitor.timeWriteArchive};
        auto storable             = makeStorableRecoResults(*ts, result);
        extraMonitor.bytesWritten = storable->SizeBytes();
        xpu::t_add_bytes(extraMonitor.bytesWritten);
        archive->put(storable);
      }
    }
    catch (const ProcessingError& e) {
      // TODO: Add flag if we want to abort on exception or continue with next timeslice
      L_(error) << "Caught ProcessingError while processing timeslice " << tsIdx << ": " << e.what();
    }
    reco.QueueProcessingExtraMetrics(extraMonitor);

    // Release memory after each timeslice and log memory usage
    // This is useful to detect memory leaks as the memory usage should be constant between timeslices
    ts.reset();
    memoryLogger.Log();

    tsIdx++;

    if (num_ts > 0 && tsIdx >= num_ts) break;

    startFetchTS = chron::high_resolution_clock::now();
  }

  if (archive) archive->end_stream();

  reco.Finalize();
  auto endProcessing = chron::high_resolution_clock::now();
  auto duration      = chron::duration_cast<chron::milliseconds>(endProcessing - startProcessing);
  L_(info) << "Total Processing time (Wall): " << duration.count() << " ms";

  return 0;
}
