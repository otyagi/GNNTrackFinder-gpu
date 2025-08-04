/* Copyright (C) 2022 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Kilian Hunold */

#include "HitfinderChain.h"

#include "AlgoFairloggerCompat.h"
#include "Exceptions.h"
#include "PODVector.h"
#include "compat/OpenMP.h"

#include <numeric>

using namespace cbm::algo;

void sts::HitfinderChain::SetParameters(const HitfinderChainPars& parameters)
{
  fPars.emplace(parameters);
  AllocateStatic();
  auto& memoryPars = fPars->memory;

  if (memoryPars.IsAuto()) {
    if (xpu::device().active().backend() == xpu::cpu) {
      memoryPars.allocationMode = RecoParams::AllocationMode::Dynamic;
    }
    else {
      memoryPars.allocationMode = RecoParams::AllocationMode::Static;
    }
  }

  if (!memoryPars.IsDynamic() && !memoryPars.IsStatic()) {
    throw std::runtime_error(
      fmt::format("STS Hitfinder Chain: Unknown allocation mode: {}", ToString(memoryPars.allocationMode)));
  }

  if (memoryPars.IsStatic()) {
    AllocateDynamic(memoryPars.maxNDigisPerModule, memoryPars.maxNDigisPerTS);
  }
}

void sts::HitfinderChain::Finalize()
{
  // Explicitly free buffers in constant memory.
  // This avoids an issue in xpu with teardown order of static variables when using CPU.
  fHitfinder = {};
  xpu::set<TheHitfinder>(fHitfinder);
}

sts::HitfinderChain::Result sts::HitfinderChain::operator()(gsl::span<const CbmStsDigi> digis, bool storeClusters)
{
  EnsureParameters();

  Result result;

  size_t nModules     = fPars->setup.modules.size();
  size_t nModuleSides = nModules * 2;
  size_t nDigisTotal  = digis.size();

  // Getting the digis on the GPU requires 3 steps
  // 1. Sort digis into buckets by module
  DigiMap digiMap = CountDigisPerModules(digis);
  // 2. Once we know number of digis per module, we can allocate
  //    the dynamic buffers on the gpu, as the buffer sizes depend on that value
  if (fPars->memory.IsDynamic())
    AllocateDynamic(digiMap.maxNDigisPerModule, nDigisTotal);
  else {
    if (digiMap.maxNDigisPerModule > fPars->memory.maxNDigisPerModule) {
      throw ProcessingError("STS Hitfinder Chain: Too many digis per module for static allocation: {} > {}",
                            digiMap.maxNDigisPerModule, fPars->memory.maxNDigisPerModule);
    }
    if (nDigisTotal > fPars->memory.maxNDigisPerTS) {
      throw ProcessingError("STS Hitfinder Chain: Too many digis per timeslice for static allocation: {} > {}",
                            nDigisTotal, fPars->memory.maxNDigisPerTS);
    }
  }
  // 3. Copy digis into flat array with offsets per module
  FlattenDigis(digis, digiMap);

#ifdef CBM_ONLINE_USE_FAIRLOGGER
  if (fair::Logger::GetConsoleSeverity() == fair::Severity::trace) {
#else
  if (Opts().LogLevel() == trace) {
#endif  // CBM_ONLINE_USE_FAIRLOGGER
    EnsureDigiOffsets(digiMap);
  }

  xpu::queue queue;

  // Set constants
  auto& hfc            = fHitfinder;
  hfc.maxHitsPerModule = fPars->memory.NHitsUpperBound(digiMap.maxNDigisPerModule);
  if (hfc.maxHitsPerModule > hfc.hitsAllocatedPerModule) {
    L_(error) << fmt::format("STS Hitfinder Chain: Too many hits per module for static allocation: {} > {}",
                             hfc.maxHitsPerModule, hfc.hitsAllocatedPerModule);
    hfc.maxHitsPerModule = hfc.hitsAllocatedPerModule;
  }

  // Clear buffers
  // Not all buffers have to initialized, but useful for debugging
  L_(debug) << "STS Hitfinder Chain: Clearing buffers...";
  // xpu::memset(hfc.digisPerModule, 0);
  // xpu::memset(hfc.digisPerModuleTmp, 0);
  // xpu::memset(hfc.digisSortedPerModule, 0);
  // xpu::memset(hfc.digiOffsetPerModule, 0);
  queue.memset(hfc.monitor, 0);
  queue.memset(hfc.digiConnectorsPerModule, 0);
  queue.memset(hfc.channelOffsetPerModule, 0);
  // queue.memset(hfc.clusterIdxPerModule, 0);
  // xpu::memset(hfc.clusterIdxPerModuleTmp, 0);
  // xpu::memset(hfc.clusterIdxSortedPerModule, 0);
  // xpu::memset(hfc.clusterDataPerModule, 0);
  queue.memset(hfc.nClustersPerModule, 0);
  // xpu::memset(hfc.hitsPerModule, 0);
  queue.memset(hfc.nHitsPerModule, 0);
  queue.memset(hfc.maxClusterTimeErrorByModuleSide, 0);

  L_(debug) << "STS Hitfinder Chain: Copy digis buffers...";
  xpu::set<TheHitfinder>(fHitfinder);

  // Only copy the actual number of digis, not the whole allocated buffer
  // TODO add support in xpu, for buffer copies with offset + size
  const CbmStsDigi* digisH = xpu::h_view(hfc.digisPerModule).data();
  CbmStsDigi* digisD       = hfc.digisPerModule.get();
  if (digisH != digisD) queue.copy(digisH, digisD, nDigisTotal);
  queue.copy(hfc.digiOffsetPerModule, xpu::h2d);

  L_(debug) << "STS Hitfinder Chain: Sort Digis...";
  // TODO: skip temporary buffers and sort directly into digisSortedPerModule

  queue.launch<SortDigis>(xpu::n_blocks(nModuleSides));
  xpu::k_add_bytes<SortDigis>(nDigisTotal * sizeof(CbmStsDigi));
#ifdef CBM_ONLINE_USE_FAIRLOGGER
  if (fair::Logger::GetConsoleSeverity() == fair::Severity::trace) {
#else
  if (Opts().LogLevel() == trace) {
#endif  // CBM_ONLINE_USE_FAIRLOGGER
    L_(trace) << "Ensuring STS digis are sorted...";
    queue.copy(hfc.digisPerModule, xpu::d2h);
    queue.copy(hfc.digiOffsetPerModule, xpu::d2h);
    queue.wait();
    EnsureDigisSorted();
  }

  L_(debug) << "STS Hitfinder Chain: Find Clusters...";
  if (!Params().sts.findClustersMultiKernels) {
    queue.launch<FindClusters>(xpu::n_blocks(nModuleSides));
  }
  else {
    queue.launch<ChannelOffsets>(xpu::n_blocks(nModuleSides));
    xpu::k_add_bytes<ChannelOffsets>(nDigisTotal * sizeof(CbmStsDigi));
    queue.launch<CreateDigiConnections>(xpu::n_threads(nDigisTotal));
    xpu::k_add_bytes<CreateDigiConnections>(nDigisTotal * sizeof(CbmStsDigi));
    queue.launch<CreateClusters>(xpu::n_threads(nDigisTotal));
    xpu::k_add_bytes<CreateClusters>(nDigisTotal * sizeof(CbmStsDigi));
  }
#ifdef CBM_ONLINE_USE_FAIRLOGGER
  if (fair::Logger::GetConsoleSeverity() == fair::Severity::trace) {
#else
  if (Opts().LogLevel() == trace) {
#endif  // CBM_ONLINE_USE_FAIRLOGGER
    L_(trace) << "Ensuring STS channel offsets correct...";
    xpu::buffer_prop propsOffset{hfc.channelOffsetPerModule};
    std::vector<u32> channelOffsetPerModule;
    channelOffsetPerModule.resize(propsOffset.size());
    queue.copy(hfc.channelOffsetPerModule.get(), channelOffsetPerModule.data(), propsOffset.size());
    queue.wait();
    EnsureChannelOffsets(channelOffsetPerModule);

    L_(trace) << "Ensuring STS clusters are ok...";
    xpu::buffer_prop props{hfc.clusterIdxPerModule};

    std::vector<ClusterIdx> clusterIdxPerModule;
    clusterIdxPerModule.resize(props.size());
    std::vector<PaddedToCacheLine<int>> nClustersPerModule;
    nClustersPerModule.resize(fPars->setup.modules.size() * 2, 0);

    queue.copy(hfc.clusterIdxPerModule.get(), clusterIdxPerModule.data(), props.size());
    queue.copy(hfc.nClustersPerModule.get(), nClustersPerModule.data(), nClustersPerModule.size());
    queue.wait();
    EnsureClustersSane(clusterIdxPerModule, nClustersPerModule);
  }

  L_(debug) << "STS Hitfinder Chain: Sort Clusters...";
  queue.launch<SortClusters>(xpu::n_blocks(nModuleSides));

  L_(debug) << "STS Hitfinder Chain: Find Hits...";
  queue.copy(hfc.nClustersPerModule, xpu::d2h);
  queue.wait();
  xpu::h_view nClusters(hfc.nClustersPerModule);
  size_t nClustersTotal = std::accumulate(nClusters.begin(), nClusters.end(), 0);
  xpu::k_add_bytes<SortClusters>(nClustersTotal * sizeof(ClusterIdx));

  size_t nClustersFront = std::accumulate(nClusters.begin(), nClusters.begin() + nModules, 0);

  bool isCpu = xpu::device::active().backend() == xpu::cpu;
  // bool isCpu          = false;
  int threadsCPU      = kFindHitsChunksPerModule * nModules;
  xpu::grid findHitsG = xpu::n_threads(isCpu ? threadsCPU : nClustersFront);
  queue.launch<FindHits>(findHitsG);
  xpu::k_add_bytes<FindHits>(nClustersTotal * sizeof(sts::Cluster));

  queue.copy(hfc.nHitsPerModule, xpu::d2h);
  queue.wait();

  auto hits = FlattenHits(queue);
  queue.copy(hfc.monitor, xpu::d2h);

  queue.wait();

  // TODO: make this step optional / hide behind flag
  {
    xpu::scoped_timer hitStreams_{"Sort Hits"};

    // TODO: Make number of streams configurable
    hits = SplitHitsIntoStreams(hits, 100);
    SortHitsWithinPartition(hits);
#ifdef CBM_ONLINE_USE_FAIRLOGGER
    if (fair::Logger::GetConsoleSeverity() == fair::Severity::trace) {
#else
    if (Opts().LogLevel() == trace) {
#endif  // CBM_ONLINE_USE_FAIRLOGGER
      EnsureHitsSorted(hits);
    }
  }

  xpu::h_view monitor(hfc.monitor);

  // Note: Checking for cluster bucket overflow is probably paranoid
  // as we allocate enough memory for one cluster per digi.
  if (monitor[0].nClusterBucketOverflow > 0) {
    L_(error) << "STS Hitfinder Chain: Cluster bucket overflow! " << monitor[0].nClusterBucketOverflow
              << " clusters were discarded!";

    for (size_t m = 0; m < nModules * 2; m++) {
      if (static_cast<size_t>(nClusters[m]) > hfc.maxClustersPerModule) {
        L_(error) << "STS Hitfinder Chain: Cluster bucket overflow in module " << m << " with " << *nClusters[m]
                  << " (of " << hfc.maxClustersPerModule << " max)"
                  << " clusters!";
        nClusters[m] = hfc.maxClustersPerModule;
      }
    }
  }

  xpu::h_view nHits(hfc.nHitsPerModule);
  if (monitor[0].nHitBucketOverflow > 0) {
    L_(error) << "STS Hitfinder Chain: Hit bucket overflow! " << monitor[0].nHitBucketOverflow
              << " hits were discarded!";

    for (size_t m = 0; m < nModules; m++) {
      const size_t nHitsModule     = nHits[m];
      const size_t nClustersModule = nClusters[m];
      const float hitsPerCluster   = nClustersModule > 0 ? nHitsModule / float(nClustersModule) : 0;
      if (nHitsModule > hfc.maxHitsPerModule) {
        L_(error) << "STS Hitfinder Chain: Hit bucket overflow in module " << m << " with " << nHitsModule << " (of "
                  << hfc.maxHitsPerModule << " max) hits! "
                  << "Module has " << nClustersModule << " clusters. " << hitsPerCluster << " hits per cluster.";
        nHits[m] = hfc.maxHitsPerModule;
      }
    }
  }

  PartitionedVector<sts::Cluster> clusters;
  if (storeClusters) clusters = FlattenClusters(queue);

  size_t nHitsTotal = std::accumulate(nHits.begin(), nHits.end(), 0);
  L_(debug) << "Timeslice contains " << nHitsTotal << " STS hits and " << nClustersTotal << " STS clusters";

  result.hits                  = std::move(hits);
  result.clusters              = std::move(clusters);
  result.monitor.nClusterTotal = nClustersTotal;
  result.monitor.nHitsTotal    = result.hits.NElements();
  result.monitor.SetDeviceMon(monitor[0]);
  return result;
}

void sts::HitfinderChain::EnsureParameters()
{
  if (fPars == std::nullopt) throw std::runtime_error("sts::HitfinderChain: Parameters not set. Can't continue!");
}

void sts::HitfinderChain::AllocateStatic()
{
  EnsureParameters();

  // Shorthands for common constants
  const int nChannels    = fPars->setup.nChannels / 2;  // Only count channels on one side of the module
  const int nModules     = fPars->setup.modules.size();
  const int nModuleSides = 2 * nModules;  // Number of module front + backsides

  xpu::queue q;

  // Set GPU constants
  fHitfinder.nModules  = nModules;
  fHitfinder.nChannels = nChannels;
  fHitfinder.asic      = fPars->setup.asic;

  // Copy landau table
  size_t nLandauTableEntries = fPars->setup.landauTable.values.size();
  fHitfinder.landauTableSize = nLandauTableEntries;
  fHitfinder.landauStepSize  = fPars->setup.landauTable.stepSize;
  fHitfinder.landauTable.reset(nLandauTableEntries, xpu::buf_io);
  q.copy(fPars->setup.landauTable.values.data(), fHitfinder.landauTable.get(), nLandauTableEntries);

  // Copy transformation matrix
  fHitfinder.localToGlobalRotationByModule.reset(9 * nModules, xpu::buf_io);
  fHitfinder.localToGlobalTranslationByModule.reset(3 * nModules, xpu::buf_io);

  // - Copy matrix into flat array
  xpu::h_view hRot(fHitfinder.localToGlobalRotationByModule);
  xpu::h_view hTrans(fHitfinder.localToGlobalTranslationByModule);
  for (int m = 0; m < nModules; m++) {
    const auto& module = fPars->setup.modules.at(m);
    std::copy_n(module.localToGlobal.rotation.data(), 9, &hRot[m * 9]);
    std::copy_n(module.localToGlobal.translation.data(), 3, &hTrans[m * 3]);
  }

  // - Then copy to GPU
  q.copy(fHitfinder.localToGlobalRotationByModule, xpu::h2d);
  q.copy(fHitfinder.localToGlobalTranslationByModule, xpu::h2d);

  // Copy Sensor Parameteres
  fHitfinder.sensorPars.reset(nModules, xpu::buf_io);
  xpu::h_view hSensorPars(fHitfinder.sensorPars);
  for (int m = 0; m < nModules; m++) {
    const auto& module = fPars->setup.modules.at(m);
    auto& gpuPars      = hSensorPars[m];
    gpuPars.dY         = module.dY;
    gpuPars.pitch      = module.pitch;
    gpuPars.stereoF    = module.stereoF;
    gpuPars.stereoB    = module.stereoB;
    gpuPars.lorentzF   = module.lorentzF;
    gpuPars.lorentzB   = module.lorentzB;
  }
  q.copy(fHitfinder.sensorPars, xpu::h2d);

  // Time errors
  fHitfinder.maxClusterTimeErrorByModuleSide.reset(nModuleSides, xpu::buf_device);

  fHitfinder.monitor.reset(1, xpu::buf_io);

  q.wait();
}

void sts::HitfinderChain::AllocateDynamic(size_t maxNDigisPerModule, size_t nDigisTotal)
{
  L_(debug) << "STS Hitfinder Chain: Allocating dynamic memory for " << maxNDigisPerModule << " digis per module and "
            << nDigisTotal << " digis in total";
  EnsureParameters();

  xpu::scoped_timer t_("Allocate");

  // TODO: some of these buffers have a constant size and can be allocated statically.
  // Just the data they contain is static.

  fPars->memory.maxNDigisPerModule = maxNDigisPerModule;
  fPars->memory.maxNDigisPerTS     = nDigisTotal;

  // Shorthands for common constants
  const int nChannels    = fPars->setup.nChannels / 2;  // Only count channels on one side of the module
  const int nModules     = fPars->setup.modules.size();
  const int nModuleSides = 2 * nModules;  // Number of module front + backsides

  const size_t maxNClustersPerModule = fPars->memory.MaxNClustersPerModule();
  const size_t maxNHitsPerModule     = fPars->memory.MaxNHitsPerModule();

  // TODO: this number should be much lower, estimate from max digis per TS
  const size_t maxHitsTotal = maxNHitsPerModule * nModules;

  // Allocate Digi Buffers
  fHitfinder.digiOffsetPerModule.reset(nModuleSides + 1, xpu::buf_io);
  fHitfinder.digisPerModule.reset(nDigisTotal, xpu::buf_io);

  fHitfinder.digisPerModuleTmp.reset(nDigisTotal, xpu::buf_device);
  fHitfinder.digiConnectorsPerModule.reset(nDigisTotal, xpu::buf_device);

  fHitfinder.channelOffsetPerModule.reset(nModuleSides * nChannels, xpu::buf_device);

  // Allocate Cluster Buffers
  fHitfinder.maxClustersPerModule = maxNClustersPerModule;

  fHitfinder.clusterIdxPerModule.reset(maxNClustersPerModule * nModuleSides, xpu::buf_device);
  fHitfinder.clusterIdxPerModuleTmp.reset(maxNClustersPerModule * nModuleSides, xpu::buf_device);
  fHitfinder.clusterIdxSortedPerModule.reset(nModuleSides, xpu::buf_device);

  fHitfinder.clusterDataPerModule.reset(maxNClustersPerModule * nModuleSides, xpu::buf_io);
  fHitfinder.nClustersPerModule.reset(nModuleSides, xpu::buf_io);

  // Allocate Hit Buffers
  fHitfinder.hitsAllocatedPerModule = maxNHitsPerModule;
  fHitfinder.hitsPerModule.reset(maxNHitsPerModule * nModules, xpu::buf_device);
  fHitfinder.nHitsPerModule.reset(nModules, xpu::buf_io);

  fHitfinder.hitsFlatCapacity = maxHitsTotal;
  fHitfinder.hitsFlat.reset(maxHitsTotal, xpu::buf_pinned);
}

sts::HitfinderChain::DigiMap sts::HitfinderChain::CountDigisPerModules(gsl::span<const CbmStsDigi> digis)
{
  L_(debug) << "STS Hitfinder Chain: Sorting " << digis.size() << " digis into modules";
  xpu::scoped_timer t_("Count Digis By Module");
  xpu::t_add_bytes(digis.size_bytes());

  size_t nModules     = fPars->setup.modules.size();
  size_t nModuleSides = nModules * 2;
  DigiMap digiMap;
  digiMap.nDigisPerModule.resize(nModuleSides);

  // Create map from module address to index
  // This could be done just once in the beginning,
  // but overhead is negligible compared to the digi copying
  digiMap.addrToIndex.resize(1 << 17, InvalidModule);
  for (size_t m = 0; m < nModules; m++) {
    const auto& module         = fPars->setup.modules[m];
    i32 paddr                  = CbmStsAddress::PackDigiAddress(module.address);
    digiMap.addrToIndex[paddr] = u16(m);
  }

  int nChannelsPerSide = fPars->setup.nChannels / 2;

  // TODO: try to parallise
  // Count digis per module in parallel
  // Then calculate offset per module
  // Write digis directly to flat array in parallel
  // Use atomic add to increment offset per module
  digiMap.nDigisPerThread.resize(openmp::GetMaxThreads());
  for (auto& v : digiMap.nDigisPerThread)
    v.resize(nModuleSides, 0);

  CBM_PARALLEL()
  {
    int threadId = openmp::GetThreadNum();  // Move out of the loop, seems to create small overhead
    auto& nDigis = digiMap.nDigisPerThread.at(threadId);

    CBM_OMP(for schedule(static))
    for (size_t i = 0; i < digis.size(); i++) {
      const auto& digi = digis[i];
      u16 moduleIndex  = digiMap.ModuleIndex(digi);

      if (moduleIndex == InvalidModule) continue;
      // if (moduleIndex == InvalidModule || digi.GetAddress() == 0x10000002) continue;


      bool isFront = digi.GetChannel() < nChannelsPerSide;
      nDigis[moduleIndex + (isFront ? 0 : nModules)]++;
    }
  }


  // Sum up digis per module
  for (auto& v : digiMap.nDigisPerThread) {
    for (size_t m = 0; m < nModuleSides; m++) {
      digiMap.nDigisPerModule[m] += v[m];
    }
  }

  // if (nPulsers > 0) L_(warning) << "STS Hitfinder: Discarded " << nPulsers << " pulser digis";

  // Print digi counts per module
  for (const auto& mod : fPars->setup.modules) {
    // FIXME should use module index here
    i32 moduleAddr  = CbmStsAddress::PackDigiAddress(mod.address);
    u16 moduleIndex = digiMap.addrToIndex[moduleAddr];
    L_(debug) << "Module " << moduleAddr << " has " << digiMap.nDigisPerModule[moduleIndex] << " front digis and "
              << digiMap.nDigisPerModule[moduleIndex + nModules] << " back digis";
  }

  digiMap.maxNDigisPerModule = 0;
  for (const auto& nDigis : digiMap.nDigisPerModule) {
    digiMap.maxNDigisPerModule = std::max(digiMap.maxNDigisPerModule, nDigis);
  }

  return digiMap;
}

void sts::HitfinderChain::FlattenDigis(gsl::span<const CbmStsDigi> digis, DigiMap& digiMap)
{
  L_(debug) << "STS Hitfinder Chain: Flattening digis";
  xpu::scoped_timer t_("Flatten Digis");
  xpu::t_add_bytes(digis.size_bytes());

  const auto& modules = fPars->setup.modules;

  size_t nModules      = modules.size();
  size_t nModuleSides  = nModules * 2;
  int nChannelsPerSide = fPars->setup.nChannels / 2;
  xpu::h_view pDigisFlat(fHitfinder.digisPerModule);  // Final input copied the GPU

  xpu::h_view pMdigiOffset(fHitfinder.digiOffsetPerModule);
  pMdigiOffset[0] = 0;
  for (size_t m = 1; m < nModuleSides + 1; m++) {
    pMdigiOffset[m] = pMdigiOffset[m - 1] + digiMap.nDigisPerModule.at(m - 1);
  }

  std::vector<std::vector<size_t>> digiOffsetsPerThread(openmp::GetMaxThreads());
  for (auto& v : digiOffsetsPerThread)
    v.resize(nModuleSides);
  for (size_t i = 1; i < digiOffsetsPerThread.size(); i++) {
    for (size_t m = 0; m < nModuleSides; m++) {
      for (size_t j = 0; j < i; j++) {
        digiOffsetsPerThread[i][m] += digiMap.nDigisPerThread[j][m];
      }
    }
  }

  CBM_PARALLEL()
  {
    int threadId      = openmp::GetThreadNum();  // Move out of the loop, seems to create small overhead
    auto& digiOffsets = digiOffsetsPerThread.at(threadId);

    CBM_OMP(for schedule(static))
    for (size_t i = 0; i < digis.size(); i++) {
      const auto& digi = digis[i];

      u16 moduleIndex = digiMap.ModuleIndex(digi);

      if (moduleIndex == InvalidModule) continue;

      bool isFront = digi.GetChannel() < nChannelsPerSide;
      moduleIndex += isFront ? 0 : nModules;

      size_t moduleOffset = pMdigiOffset[moduleIndex];
      size_t digiOffset   = digiOffsets[moduleIndex]++;

      pDigisFlat[moduleOffset + digiOffset] = digi;
    }
  }

  // Channels in Digis are counted 0 to 2047 by default.
  // Channel 0 -> 1023 is the front side of the module
  // Channel 1024 -> 2047 is the back side of the module
  // Cluster finder only operates on one side, so we have to shift the channel numbers
  CBM_PARALLEL_FOR(schedule(static))
  for (size_t i = pMdigiOffset[nModules]; i < pMdigiOffset[nModuleSides]; i++) {
    auto& digi = pDigisFlat[i];
    digi.SetChannel(digi.GetChannel() - nChannelsPerSide);
  }
}

PartitionedSpan<sts::Hit> sts::HitfinderChain::FlattenHits(xpu::queue queue)
{
  auto& hfc = fHitfinder;
  xpu::h_view nHits(hfc.nHitsPerModule);

  size_t nHitsTotal = 0;
  for (int m = 0; m < hfc.nModules; m++)
    nHitsTotal += GetNHits(nHits, m);
  L_(debug) << "STS Hitfinder Chain: Flattening " << nHitsTotal << " hits";

  if (nHitsTotal > hfc.hitsFlatCapacity)
    throw ProcessingError("STS Hitfinder Chain: Not enough memory for hits: {} > {}", nHitsTotal, hfc.hitsFlatCapacity);

  // hitBuffer is potentially bigger than nHitsTotal
  // So construct a span to the memory region we actually use
  xpu::h_view hitBuffer(hfc.hitsFlat);
  gsl::span hits(hitBuffer.data(), nHitsTotal);

  // Transfer Hits / Cluster back to host
  // Hits and Clusters are stored in buckets. Buckets aren't necessarily full.
  // So we have to copy each bucket individually.
  // Alternative: flatten both arrays on the GPU and transfer flat array back.
  // Latter would require a lot of memory but give us parallel copying when running on the host.
  //
  // Also at this point, values in nHits could be wrong if the bucket overflowed.
  // So take extra care to not access nHits directly.
  if (xpu::device::active().backend() == xpu::cpu) {
    // Hits already in host memory
    // Flatten in parallel
    // Note: this is still pretty slow for mCBM because there are only 9 modules...
    xpu::scoped_timer t_("Flatten Hits");
    xpu::t_add_bytes(nHitsTotal * sizeof(sts::Hit));
    CBM_PARALLEL_FOR(schedule(dynamic))
    for (int m = 0; m < hfc.nModules; m++) {
      size_t offset = 0;
      for (int i = 0; i < m; i++) {
        offset += GetNHits(nHits, i);
      }
      std::copy_n(hfc.hitsPerModule.get() + hfc.hitsAllocatedPerModule * m, GetNHits(nHits, m), hits.begin() + offset);
    }
  }
  else {  // GPU
    // Initialize host memory:
    // memset(hits.data(), 0, nHitsTotal * sizeof(sts::Hit));

    // Hits not in host memory
    // FIXME this is very slow (1.7 GBs vs 16 GBs we should get from PCIe 3.0 x16)
    // I assume because of page faults as the allocated memory is not touched before
    // But even with a memset on host memory before, throughput only goes up to 3.7 GBs
    size_t nHitsCopied = 0;
    for (int m = 0; m < hfc.nModules; m++) {
      size_t numHitsInModule = GetNHits(nHits, m);
      queue.copy(hfc.hitsPerModule.get() + hfc.hitsAllocatedPerModule * m, hits.data() + nHitsCopied, numHitsInModule);
      nHitsCopied += numHitsInModule;
    }
  }


  // Could do this more efficiently, cache addresses once in the beginning, ...
  // doesn't really matter overhead wise
  fHitOffsets.clear();
  // nModules + 1 entries, first entry is always 0, last entry is total number of hits (required by PartitionedSpan)
  fHitOffsets.push_back(0);
  for (int m = 0; m < hfc.nModules; m++)
    fHitOffsets.push_back(fHitOffsets.back() + GetNHits(nHits, m));

  fAddresses = {};
  for (auto& m : fPars->setup.modules)
    fAddresses.push_back(m.address);

  PartitionedSpan partitionedHits(hits, fHitOffsets, fAddresses);
  return partitionedHits;
}

PartitionedVector<sts::Cluster> sts::HitfinderChain::FlattenClusters(xpu::queue queue)
{
  auto& hfc = fHitfinder;

  xpu::h_view nClusters(hfc.nClustersPerModule);
  int maxClustersPerModule = hfc.maxClustersPerModule;
  sts::Cluster* clusters   = hfc.clusterDataPerModule.get();

  int nModuleSides = nClusters.size();

  L_(debug) << "Copy clusters from " << nModuleSides << " modules sides";

  std::vector<u32> addresses;
  for (size_t i = 0; i < 2; i++) {  // Repeat twice for front and back
    for (auto& m : fPars->setup.modules)
      addresses.push_back(m.address);
  }

  size_t nClustersTotal = std::accumulate(nClusters.begin(), nClusters.end(), 0);

  // TODO: copy could be made much more efficient by using pinned memory
  // But clusters are stored for debugging only, so no effort is made to optimize this
  std::vector<sts::Cluster> clustersFlat;
  clustersFlat.resize(nClustersTotal);
  size_t offset = 0;
  for (int m = 0; m < nModuleSides; m++) {
    size_t nClustersInModule = nClusters[m];
    queue.copy(clusters + maxClustersPerModule * m, clustersFlat.data() + offset, nClustersInModule);
    offset += nClustersInModule;
  }
  queue.wait();

  std::vector<size_t> sizes(nClusters.begin(), nClusters.end());
  PartitionedVector<sts::Cluster> out(std::move(clustersFlat), gsl::make_span(sizes), gsl::make_span(addresses));
  return out;
}

size_t sts::HitfinderChain::GetNHits(xpu::h_view<PaddedToCacheLine<int>> nHitsPerModule, int module)
{
  return std::min<size_t>(nHitsPerModule[module], fHitfinder.maxHitsPerModule);
}

PartitionedSpan<sts::Hit> sts::HitfinderChain::SplitHitsIntoStreams(PartitionedSpan<sts::Hit> hits, int nstreamsMax)
{
  constexpr size_t MinHitsPerStream = 10;

  int nSensors         = hits.NPartitions();
  int streamsPerSensor = std::max(1, nstreamsMax / nSensors);

  // TODO: move into helper function in base/ ?
  auto nextMultipleOf = [](int x, int mult) {
    if (mult == 0) {
      return x;
    }

    int rem = x % mult;
    if (rem == 0) {
      return x;
    }

    return x + mult - rem;
  };

  fHitStreamOffsets.clear();
  fStreamAddresses.clear();

  fHitStreamOffsets.push_back(0);
  for (size_t sensor = 0; sensor < hits.NPartitions(); sensor++) {
    auto [hitsSensor, addr] = hits.Partition(sensor);
    const size_t nHits      = hitsSensor.size();

    // Sensor has too few hits -> create only one stream for entire sensor
    if (nHits < streamsPerSensor * MinHitsPerStream) {
      fHitStreamOffsets.push_back(fHitStreamOffsets.back() + nHits);
      fStreamAddresses.push_back(addr);
      continue;
    }

    size_t rem = nHits % streamsPerSensor;
    if (rem == 0) {
      size_t streamSize = nHits / streamsPerSensor;
      for (int s = 0; s < streamsPerSensor; s++) {
        fHitStreamOffsets.push_back(fHitStreamOffsets.back() + streamSize);
        fStreamAddresses.push_back(addr);
      }
    }
    else {
      size_t streamSize = nextMultipleOf(nHits, streamsPerSensor) / streamsPerSensor;
      size_t nHitsLeft  = nHits;
      for (int s = 0; s < streamsPerSensor; s++) {
        fHitStreamOffsets.push_back(fHitStreamOffsets.back() + std::min(nHitsLeft, streamSize));
        fStreamAddresses.push_back(addr);
        nHitsLeft -= streamSize;
      }
    }
  }

  PartitionedSpan<sts::Hit> hitStreams(hits.Data(), fHitStreamOffsets, fStreamAddresses);
  return hitStreams;
}

void sts::HitfinderChain::SortHitsWithinPartition(PartitionedSpan<sts::Hit> hits)
{
  CBM_PARALLEL_FOR(schedule(dynamic))
  for (size_t i = 0; i < hits.NPartitions(); i++) {
    auto stream = hits[i];
    std::sort(stream.begin(), stream.end(), [](const sts::Hit& a, const sts::Hit& b) { return a.fTime < b.fTime; });
  }
}

void sts::HitfinderChain::EnsureDigiOffsets(DigiMap& digi)
{
  xpu::h_view digiOffset(fHitfinder.digiOffsetPerModule);

  size_t nModules = fPars->setup.modules.size();
  size_t offset   = 0;
  // Front
  for (size_t m = 0; m < nModules * 2; m++) {
    if (digiOffset[m] != offset) {
      throw FatalError("Module {}: Digi offset mismatch: {} != {}", m, digiOffset[m], offset);
    }
    size_t nDigis = digi.nDigisPerModule[m];
    offset += nDigis;
  }

  if (offset != digiOffset[2 * nModules]) {
    throw FatalError("Digi offset mismatch: {} != {}", digiOffset[2 * nModules], offset);
  }
}

void sts::HitfinderChain::EnsureDigisSorted()
{
  xpu::h_view digiOffset(fHitfinder.digiOffsetPerModule);
  xpu::h_view digis(fHitfinder.digisPerModule);

  bool isSorted = true;

  for (size_t m = 0; m < fPars->setup.modules.size(); m++) {
    int nDigis = digiOffset[m + 1] - digiOffset[m];
    if (nDigis == 0) continue;

    auto* digisModule = &digis[digiOffset[m]];

    for (int i = 0; i < nDigis - 1; i++) {
      const auto &digi1 = digisModule[i], &digi2 = digisModule[i + 1];

      if ((digi1.GetChannel() < digi2.GetChannel())
          || (digi1.GetChannel() == digi2.GetChannel()
              && digi1.GetTime() <= digi2.GetTime())  // Unpacker sometimes produces multiple
                                                      // digis with the same time and channel, FIXME!
      ) {
        continue;
      }

      isSorted = false;
      L_(error) << "Module " << m << " not sorted: " << digi1.ToString() << " " << digi2.ToString();
      break;
    }
  }

  if (!isSorted) {
    throw FatalError("Digis are not sorted");
  }
}

void sts::HitfinderChain::EnsureChannelOffsets(gsl::span<u32> channelOffsetsByModule)
{
  xpu::h_view digisPerModule(fHitfinder.digisPerModule);
  xpu::h_view digiOffsetPerModule(fHitfinder.digiOffsetPerModule);
  for (size_t m = 0; m < fPars->setup.modules.size() * 2; m++) {
    int nChannels = fPars->setup.nChannels / 2;  // Consider module sides

    std::vector<u32> expectedChannelOffsets(nChannels, 0);

    int offset = digiOffsetPerModule[m];
    int nDigis = digiOffsetPerModule[m + 1] - offset;
    gsl::span<CbmStsDigi> digis(&digisPerModule[offset], nDigis);
    gsl::span<u32> channelOffsets = channelOffsetsByModule.subspan(m * nChannels, nChannels);

    if (nDigis == 0) continue;

    if (channelOffsets[0] != 0) {
      throw FatalError("Module {}: First channel offset is not 0", m);
    }

    int chan = digis[0].GetChannel();
    for (int i = 0; i < nDigis; i++) {
      if (digis[i].GetChannel() != chan) {
        while (chan < digis[i].GetChannel()) {
          chan++;
          expectedChannelOffsets[chan] = i;
        }
      }
    }
    while (chan < nChannels) {
      chan++;
      expectedChannelOffsets[chan] = nDigis;
    }

    for (int i = 0; i < nChannels; i++) {
      if (channelOffsets[i] != expectedChannelOffsets[i]) {
        throw FatalError("Module {}: Channel offset for channel {} is {} but should be {}", m, i, channelOffsets[i],
                         expectedChannelOffsets[i]);
      }
    }
  }
}

void sts::HitfinderChain::EnsureClustersSane(gsl::span<ClusterIdx> hClusterIdx,
                                             gsl::span<PaddedToCacheLine<int>> hNClusters)
{
  for (size_t m = 0; m < 2 * fPars->setup.modules.size(); m++) {
    int nClusters = hNClusters[m];

    L_(trace) << "Module " << m << " has " << nClusters << " clusters of " << fHitfinder.maxClustersPerModule;

    if (nClusters == 0) continue;

    if (nClusters < 0) {
      throw FatalError("Module {} has negative number of clusters {}", m, nClusters);
    }
    if (size_t(nClusters) > fHitfinder.maxClustersPerModule) {
      throw FatalError("Module {} has {} clusters, but only {} are allowed", m, nClusters,
                       fHitfinder.maxClustersPerModule);
    }

    auto* clusterIdx = &hClusterIdx[m * fHitfinder.maxClustersPerModule];

    for (int i = 0; i < nClusters; i++) {
      auto& cidx = clusterIdx[i];

      if (int(cidx.fIdx) < 0 || size_t(cidx.fIdx) >= fHitfinder.maxClustersPerModule) {
        throw FatalError("Cluster {} of module {} has invalid index {}", i, m, cidx.fIdx);
      }

      if (cidx.fTime == 0xFFFFFFFF) {
        throw FatalError("Cluster {} of module has invalid time {}", i, m, cidx.fTime);
      }
    }
  }

  L_(trace) << "Clusters ok";
}

void sts::HitfinderChain::EnsureHitsSorted(PartitionedSpan<sts::Hit> hits)
{
  L_(trace) << "Ensure STS Hits sorted";

  for (size_t i = 0; i < hits.NPartitions(); i++) {
    auto [part, addr] = hits.Partition(i);
    bool ok = std::is_sorted(part.begin(), part.end(), [](const Hit& a, const Hit& b) { return a.fTime < b.fTime; });
    L_(trace) << "Stream " << i << ": addr=" << addr << ", nhits=" << part.size();
    if (!ok) {
      throw FatalError{"Hits not sorted in stream %zd (addr %u)", i, addr};
    }
  }

  L_(trace) << "STS Hits sorted!";
}
