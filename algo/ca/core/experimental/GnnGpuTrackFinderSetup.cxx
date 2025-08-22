/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file CaGpuTrackFinderSetup.cxx
/// \brief The class is responsible for setting up the environment and the order in which kernels are launched to perform tracking using XPU

#include "GnnGpuTrackFinderSetup.h"

using namespace cbm::algo::ca;

GnnGpuTrackFinderSetup::GnnGpuTrackFinderSetup(WindowData& wData, const ca::Parameters<fvec>& pars)
  : fParameters(pars)
  , frWData(wData)
  , fIteration(0)
{
}

void GnnGpuTrackFinderSetup::SetupParameters()
{
  int nStations   = fParameters.GetNstationsActive();
  int nIterations = fParameters.GetCAIterations().size();
  fGraphConstructor.fParams.reset(nIterations, xpu::buf_io);
  xpu::h_view vfParams{fGraphConstructor.fParams};

  for (int i = 0; i < nIterations; i++) {
    GpuParameters gpuParams            = fParameters;
    fGraphConstructor.fParams_const[i] = fParameters;
    const auto& caIteration            = fParameters.GetCAIterations()[i];

    gpuParams.fNStations                          = nStations;
    gpuParams.maxSlopePV                          = caIteration.GetMaxSlopePV();
    gpuParams.maxQp                               = caIteration.GetMaxQp();
    gpuParams.maxDZ                               = caIteration.GetMaxDZ();
    fGraphConstructor.fParams_const[i].fNStations = nStations;
    fGraphConstructor.fParams_const[i].maxSlopePV = caIteration.GetMaxSlopePV();
    fGraphConstructor.fParams_const[i].maxQp      = caIteration.GetMaxQp();
    fGraphConstructor.fParams_const[i].maxDZ      = caIteration.GetMaxDZ();

    if (caIteration.GetElectronFlag()) {
      gpuParams.particleMass                          = constants::phys::ElectronMass;
      fGraphConstructor.fParams_const[i].particleMass = constants::phys::ElectronMass;
    }
    else {
      gpuParams.particleMass                          = constants::phys::MuonMass;
      fGraphConstructor.fParams_const[i].particleMass = constants::phys::MuonMass;
    }

    gpuParams.primaryFlag                          = caIteration.GetPrimaryFlag();
    fGraphConstructor.fParams_const[i].primaryFlag = caIteration.GetPrimaryFlag();

    if (caIteration.GetPrimaryFlag()) {
      gpuParams.targB                          = fParameters.GetVertexFieldValue();
      fGraphConstructor.fParams_const[i].targB = fParameters.GetVertexFieldValue();
    }
    else {
      gpuParams.targB                          = fParameters.GetStation(0).fieldSlice.GetFieldValue(0, 0);
      fGraphConstructor.fParams_const[i].targB = fParameters.GetStation(0).fieldSlice.GetFieldValue(0, 0);
    }

    gpuParams.doubletChi2Cut                               = caIteration.GetDoubletChi2Cut();
    gpuParams.tripletChi2Cut                               = caIteration.GetTripletChi2Cut();
    gpuParams.tripletFinalChi2Cut                          = caIteration.GetTripletFinalChi2Cut();
    gpuParams.isTargetField                                = (fabs(gpuParams.targB.y) > 0.001);
    fGraphConstructor.fParams_const[i].doubletChi2Cut      = caIteration.GetDoubletChi2Cut();
    fGraphConstructor.fParams_const[i].tripletChi2Cut      = caIteration.GetTripletChi2Cut();
    fGraphConstructor.fParams_const[i].tripletFinalChi2Cut = caIteration.GetTripletFinalChi2Cut();

    ca::MeasurementXy<float> targMeas(fParameters.GetTargetPositionX()[0], fParameters.GetTargetPositionY()[0],
                                      caIteration.GetTargetPosSigmaX() * caIteration.GetTargetPosSigmaX(),
                                      caIteration.GetTargetPosSigmaY() * caIteration.GetTargetPosSigmaX(), 0, 1, 1);
    gpuParams.targetMeasurement                          = targMeas;
    fGraphConstructor.fParams_const[i].targetMeasurement = targMeas;

    vfParams[i] = gpuParams;
  }

  fQueue.copy(fGraphConstructor.fParams, xpu::h2d);

  for (int ista = 0; ista < nStations; ista++) {
    fGraphConstructor.fStations_const[ista] = fParameters.GetStation(ista);
  }

  xpu::set<strGnnGpuGraphConstructor>(fGraphConstructor);
}  // SetupParameters

void GnnGpuTrackFinderSetup::SetupGrid()
{
  unsigned int nStations     = fParameters.GetNstationsActive();
  unsigned int bin_start     = 0;
  unsigned int entries_start = 0;

  fGraphConstructor.fvGpuGrid.reset(nStations, xpu::buf_io);
  xpu::h_view vfvGpuGrid{fGraphConstructor.fvGpuGrid};

  for (unsigned int ista = 0; ista < nStations; ista++) {
    vfvGpuGrid[ista] = GpuGrid(frWData.Grid(ista), bin_start, entries_start);
    bin_start += frWData.Grid(ista).GetFirstBinEntryIndex().size();
    entries_start += frWData.Grid(ista).GetEntries().size();
  }

  fQueue.copy(fGraphConstructor.fvGpuGrid, xpu::h2d);

  fGraphConstructor.fgridFirstBinEntryIndex.reset(bin_start, xpu::buf_io);
  fGraphConstructor.fgridEntries.reset(entries_start, xpu::buf_io);

  xpu::h_view vfgridFirstBinEntryIndex{fGraphConstructor.fgridFirstBinEntryIndex};
  xpu::h_view vfgridEntries{fGraphConstructor.fgridEntries};

  bin_start = entries_start = 0;

  for (unsigned int ista = 0; ista < nStations; ista++) {
    std::copy_n(frWData.Grid(ista).GetFirstBinEntryIndex().begin(), frWData.Grid(ista).GetFirstBinEntryIndex().size(),
                &vfgridFirstBinEntryIndex[bin_start]);
    for (unsigned int i = 0; i < frWData.Grid(ista).GetEntries().size(); i++) {
      vfgridEntries[entries_start + i] = frWData.Grid(ista).GetEntries()[i].GetObjectId();
    }
    bin_start += frWData.Grid(ista).GetFirstBinEntryIndex().size();
    entries_start += frWData.Grid(ista).GetEntries().size();
  }
  fNHits = entries_start;

  fQueue.copy(fGraphConstructor.fgridFirstBinEntryIndex, xpu::h2d);
  fQueue.copy(fGraphConstructor.fgridEntries, xpu::h2d);
}

void GnnGpuTrackFinderSetup::SetupMaterialMap()
{
  ///Set up material map
  int bin_start  = 0;
  int nStations  = fParameters.GetNstationsActive();
  int fstStation = 0;
  if (nStations == 8) fstStation = 4;
  fGraphConstructor.fMaterialMap.reset(nStations, xpu::buf_io);
  xpu::h_view vfMaterialMap{fGraphConstructor.fMaterialMap};

  for (int ista = 0; ista < nStations; ista++) {
    vfMaterialMap[ista] = GpuMaterialMap(fParameters.GetGeometrySetup().GetMaterial(fstStation + ista), bin_start);
    bin_start += fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins()
                 * fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins();
  }

  fGraphConstructor.fMaterialMapTables.reset(bin_start, xpu::buf_io);
  xpu::h_view vfMaterialMapTables{fGraphConstructor.fMaterialMapTables};

  bin_start = 0;

  for (int ista = 0; ista < nStations; ista++) {
    std::copy_n(fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetTable().begin(),
                fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetTable().size(),
                &vfMaterialMapTables[bin_start]);
    bin_start += fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins()
                 * fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins();
  }

  fQueue.copy(fGraphConstructor.fMaterialMap, xpu::h2d);
  fQueue.copy(fGraphConstructor.fMaterialMapTables, xpu::h2d);
}  // SetupMaterialMap

void GnnGpuTrackFinderSetup::SetInputData()
{
  fGraphConstructor.fvHits.reset(frWData.Hits().size(), xpu::buf_io);
  xpu::h_view vfvHits{fGraphConstructor.fvHits};

  std::copy_n(frWData.Hits().begin(), frWData.Hits().size(), &vfvHits[0]);

  fQueue.copy(fGraphConstructor.fvHits, xpu::h2d);
}  // SetInputData

void GnnGpuTrackFinderSetup::SetupIterationData(int iter)
{
  fIteration = iter;

  fGraphConstructor.fIterationData.reset(1, xpu::buf_io);
  xpu::h_view vfIterationData{fGraphConstructor.fIterationData};
  vfIterationData[0].fNHits             = fNHits;
  vfIterationData[0].fIteration         = iter;
  vfIterationData[0].fNDoublets         = 0;
  vfIterationData[0].fNDoublets_counter = 0;
  vfIterationData[0].fNTriplets         = 0;
  vfIterationData[0].fNTriplets_counter = 0;

  fQueue.copy(fGraphConstructor.fIterationData, xpu::h2d);
}

void GnnGpuTrackFinderSetup::RunGpuTracking()
{
  fNTriplets = 0;

  xpu::device_prop prop{xpu::device::active()};

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    LOG(info) << "Running GPU tracking chain on device " << prop.name() << ", fIteration: " << fIteration;
  }

  bool isCpu              = xpu::device::active().backend() == xpu::cpu;
  float embedHitsBlocks   = std::ceil((float) frWData.Hits().size() / GnnGpuConstants::kEmbedHitsBlockSize);
  float fitTripletsBlocks = std::ceil((frWData.Hits().size() * 400.f) / GnnGpuConstants::kEmbedHitsBlockSize);

  fGraphConstructor.fIteration = fIteration;

  xpu::set<strGnnGpuGraphConstructor>(fGraphConstructor);

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    fEventTimeMonitor.nIterations = fIteration;
    xpu::push_timer("Full_time");
    xpu::push_timer("EmbedHits_time");
  }

  fQueue.launch<EmbedHits>(xpu::n_blocks(embedHitsBlocks));

  LOG(info) << "Num hits in event: " << frWData.Hits().size();
  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                       = xpu::pop_timer();
    fEventTimeMonitor.EmbedHits_time[fIteration] = step_time;
    LOG(info) << "EmbedHits_time: " << step_time.wall();
    xpu::push_timer("NearestNeighbours_time");
  }

  fQueue.launch<NearestNeighbours>(xpu::n_blocks(embedHitsBlocks));

  fQueue.copy(fGraphConstructor.fNNeighbours, xpu::d2h);
  xpu::h_view vfNNeighbours(fGraphConstructor.fNNeighbours);
  const int nDoublets = std::accumulate(vfNNeighbours.begin(), vfNNeighbours.end(), 0);
  LOG(info) << "GPU Tracking: Num doublets found: " << nDoublets;
  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                               = xpu::pop_timer();
    fEventTimeMonitor.NearestNeighbours_time[fIteration] = step_time;
    LOG(info) << "NearestNeighbours_time: " << step_time.wall();
    xpu::push_timer("MakeTripletsOT_time");
  }

  fQueue.launch<MakeTripletsOT>(xpu::n_blocks(embedHitsBlocks));

  fQueue.copy(fGraphConstructor.fNTriplets, xpu::d2h);
  xpu::h_view vfNTriplets(fGraphConstructor.fNTriplets);
  const int nTriplets = std::accumulate(vfNTriplets.begin(), vfNTriplets.end(), 0);
  LOG(info) << "GPU Tracking: Num Triplets constructed: " << nTriplets;
  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                            = xpu::pop_timer();
    fEventTimeMonitor.MakeTripletsOT_time[fIteration] = step_time;
    LOG(info) << "MakeTripletsOT_time: " << step_time.wall();
    xpu::push_timer("CompressTripletsOT_time");
  }

  // fGraphConstructor.fTripletsFlat.reset(nTriplets, xpu::buf_io);  // exact size
  // const int NBlocks = std::ceil((float)nTriplets / GnnGpuConstants::kScanBlockSize);
  // fQueue.launch<ExclusiveScan>(xpu::n_blocks(NBlocks));
  // LOG(info) << "Exclusive Scan.";
  // const int NBlockGroups = std::ceil((float)frWData.Hits().size() / GnnGpuConstants::kScanBlockSize);
  // fQueue.launch<AddBlockSums>(xpu::n_blocks(fitTripletsBlocks), NBlockGroups);
  // LOG(info) << "AddBlockSums";
  // fQueue.launch<AddOffsets>(xpu::n_blocks(fitTripletsBlocks));
  // LOG(info) << "AddOffsets";
  // fQueue.launch<CompressAllTripletsOrdered>(xpu::n_blocks(fitTripletsBlocks));

  // fQueue.copy(fGraphConstructor.fTripletsFlat, xpu::d2h);
  // xpu::h_view vfTripletsFlat(fGraphConstructor.fTripletsFlat);
  // const auto nTripletsFlat = vfTripletsFlat.size();
  // LOG(info) << "GPU Tracking: Triplets Compressed: " << nTripletsFlat;
  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                              = xpu::pop_timer();
    fEventTimeMonitor.CompressTriplets_time[fIteration] = step_time;
    LOG(info) << "CompressTripletsOT_time: " << step_time.wall();
    xpu::push_timer("FitTripletsOT_time");
  }

  fQueue.launch<FitTripletsOT>(xpu::n_blocks(fitTripletsBlocks));  // fitTripletBlocks

  fQueue.copy(fGraphConstructor.fTripletsSelected, xpu::d2h);
  xpu::h_view vfNTripletsSelected(fGraphConstructor.fTripletsSelected);
  size_t nTripletsSelected = 0;
  for (auto iHit = 0; iHit < vfNTripletsSelected.size(); iHit++) {
    const auto& tripsHit = vfNTripletsSelected[iHit];
    const int nTripHit   = vfNTriplets[iHit];  // uninitialized after nTripHit
    for (auto iTrip = 0; iTrip < nTripHit; iTrip++) {
      nTripletsSelected += tripsHit[iTrip];  // implicit cast to int
    }
  }
  LOG(info) << "GPU Tracking: Num Triplets after fitting: " << nTripletsSelected;
  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                           = xpu::pop_timer();
    fEventTimeMonitor.FitTripletsOT_time[fIteration] = step_time;
    LOG(info) << "FitTripletsOT_time: " << step_time.wall();
    xpu::push_timer("ConstructCandidates_time");
  }

  // fQueue.launch<ConstructCandidates>(xpu::n_blocks(embedHitsBlocks));

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                                 = xpu::pop_timer();
    fEventTimeMonitor.ConstructCandidates_time[fIteration] = step_time;
    LOG(info) << "ConstructCandidates_time: " << step_time.wall();
    // xpu::push_timer("ConstructCandidates_time");
  }

  fGraphConstructor.fIterationData.reset(0, xpu::buf_device);
  fGraphConstructor.fvGpuGrid.reset(0, xpu::buf_io);
  fGraphConstructor.fgridFirstBinEntryIndex.reset(0, xpu::buf_io);
  fGraphConstructor.fgridEntries.reset(0, xpu::buf_io);

  xpu::set<strGnnGpuGraphConstructor>(fGraphConstructor);  //TODO: check if we need to reset all the buffers here

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings t                           = xpu::pop_timer();
    fEventTimeMonitor.Total_time[fIteration] = t;
    fEventTimeMonitor.PrintTimings(fIteration);
  }

  // save doublets as tracks
  // fQueue.copy(fGraphConstructor.fDoublets, xpu::d2h);
  // fQueue.copy(fGraphConstructor.fNNeighbours, xpu::d2h);

  // save triplets as tracks
  fQueue.copy(fGraphConstructor.fTriplets, xpu::d2h);
  fQueue.copy(fGraphConstructor.fNTriplets, xpu::d2h);

  // save fitted triplets
  fQueue.copy(fGraphConstructor.fTripletsSelected, xpu::d2h);
  fQueue.copy(fGraphConstructor.fvTripletParams, xpu::d2h);
}

void GnnGpuTrackFinderSetup::SaveDoubletsAsTracks()
{
  // use doublets and NNeighbours to copy.
  frWData.RecoHitIndices().reserve(200000);
  frWData.RecoTracks().reserve(100000);

  const auto nHits = frWData.Hits().size();
  int nDoublets    = 0;
  for (std::size_t iHitL = 0; iHitL < nHits; iHitL++) {
    const auto& hitL = fGraphConstructor.fvHits[iHitL];
    if (fGraphConstructor.fNNeighbours[iHitL] == 0 || hitL.Station() > 10) continue;
    // LOG(info) << "iHitL: " << iHitL << ", Neighbours: " <<fGraphConstructor.fNNeighbours[iHitL];
    for (unsigned int iHitM = 0; iHitM < fGraphConstructor.fNNeighbours[iHitL]; iHitM++) {
      const auto& hitM = fGraphConstructor.fvHits[fGraphConstructor.fDoublets[iHitL][iHitM]];
      // LOG(info) << "iHitL: " << iHitL << " ID: " << hitL.Id();
      frWData.RecoHitIndices().push_back(hitL.Id());
      // LOG(info) << "iHitM: " << iHitM << " ID: " << hitM.Id();
      frWData.RecoHitIndices().push_back(hitM.Id());
      Track t;
      t.fNofHits = 2;
      frWData.RecoTracks().push_back(t);
      nDoublets++;
    }
  }
  LOG(info) << "Num doublets as tracks: " << nDoublets;
}

void GnnGpuTrackFinderSetup::SaveTripletsAsTracks()
{
  // use doublets and NNeighbours to copy.
  frWData.RecoHitIndices().reserve(200000);
  frWData.RecoTracks().reserve(100000);

  const auto nHits = frWData.Hits().size();
  int nTriplets    = 0;
  for (std::size_t iHitL = 0; iHitL < nHits; iHitL++) {
    const auto& hitL = fGraphConstructor.fvHits[iHitL];
    if (fGraphConstructor.fNNeighbours[iHitL] == 0 || hitL.Station() > 9) continue;
    const auto& tripletsHitL = fGraphConstructor.fTriplets[iHitL];
    // LOG(info) << "iHitL: " << iHitL << ", Neighbours: " <<fGraphConstructor.fNNeighbours[iHitL];
    for (unsigned int iTriplet = 0; iTriplet < fGraphConstructor.fNTriplets[iHitL]; iTriplet++) {
      const auto& tripletsIndexes = tripletsHitL[iTriplet];
      const auto& hitM            = fGraphConstructor.fvHits[tripletsIndexes[0]];
      const auto& hitR            = fGraphConstructor.fvHits[tripletsIndexes[1]];
      // LOG(info) << "iHitM: " << iHitM << " ID: " << hitM.Id();
      // LOG(info) << "iHitR: " << iHitR << " ID: " << hitR.Id();
      frWData.RecoHitIndices().push_back(hitL.Id());
      frWData.RecoHitIndices().push_back(hitM.Id());
      frWData.RecoHitIndices().push_back(hitR.Id());
      Track t;
      t.fNofHits = 3;
      frWData.RecoTracks().push_back(t);
      nTriplets++;
    }
  }
  LOG(info) << "Num triplets as tracks: " << nTriplets;
}

void GnnGpuTrackFinderSetup::SaveFittedTripletsAsTracks()
{
  // use doublets and NNeighbours to copy.
  frWData.RecoHitIndices().reserve(200000);
  frWData.RecoTracks().reserve(100000);

  const auto nHits = frWData.Hits().size();
  int nTriplets    = 0;
  for (std::size_t iHitL = 0; iHitL < nHits; iHitL++) {
    const auto& hitL = fGraphConstructor.fvHits[iHitL];
    if (fGraphConstructor.fNNeighbours[iHitL] == 0 || hitL.Station() > 9 || fGraphConstructor.fNTriplets[iHitL] == 0)
      continue;
    const auto& tripletsHitL = fGraphConstructor.fTriplets[iHitL];
    // LOG(info) << "iHitL: " << iHitL << ", Neighbours: " <<fGraphConstructor.fNNeighbours[iHitL];
    for (unsigned int iTriplet = 0; iTriplet < fGraphConstructor.fNTriplets[iHitL]; iTriplet++) {
      const bool isSelected = fGraphConstructor.fTripletsSelected[iHitL][iTriplet];
      if (!isSelected) continue;
      const auto& tripletsIndexes = tripletsHitL[iTriplet];
      const auto& hitM            = fGraphConstructor.fvHits[tripletsIndexes[0]];
      const auto& hitR            = fGraphConstructor.fvHits[tripletsIndexes[1]];
      // LOG(info) << "iHitM: " << iHitM << " ID: " << hitM.Id();
      // LOG(info) << "iHitR: " << iHitR << " ID: " << hitR.Id();
      frWData.RecoHitIndices().push_back(hitL.Id());
      frWData.RecoHitIndices().push_back(hitM.Id());
      frWData.RecoHitIndices().push_back(hitR.Id());
      Track t;
      t.fNofHits = 3;
      frWData.RecoTracks().push_back(t);
      nTriplets++;
    }
  }
  LOG(info) << "Num triplets as tracks (after fitting): " << nTriplets;
}

void GnnGpuTrackFinderSetup::FindTracksCpu(const bool doCompetition)
{
  std::vector<std::vector<unsigned int>> tracklets;
  tracklets.reserve(10000000);
  std::vector<float> trackletScores;
  trackletScores.reserve(10000000);
  std::vector<std::array<float, 7>> trackletFitParams;  // store fit params of last triplet added to tracklet
  trackletFitParams.reserve(10000000);

  const unsigned int nHits = frWData.Hits().size();
  int nTriplets            = 0;
  for (unsigned int iHitL = 0; iHitL < nHits; iHitL++) {
    const auto& hitL = fGraphConstructor.fvHits[iHitL];
    if (fGraphConstructor.fNNeighbours[iHitL] == 0 || hitL.Station() > 9 || fGraphConstructor.fNTriplets[iHitL] == 0)
      continue;
    const auto& tripletsHitL = fGraphConstructor.fTriplets[iHitL];
    for (unsigned int iTriplet = 0; iTriplet < fGraphConstructor.fNTriplets[iHitL]; iTriplet++) {
      const bool isSelected = fGraphConstructor.fTripletsSelected[iHitL][iTriplet];
      if (!isSelected) continue;
      const auto& tripletsIndexes = tripletsHitL[iTriplet];
      const auto& hitM            = fGraphConstructor.fvHits[tripletsIndexes[0]];
      const auto& hitR            = fGraphConstructor.fvHits[tripletsIndexes[1]];
      tracklets.push_back(std::vector<unsigned int>{iHitL, tripletsIndexes[0], tripletsIndexes[1]});
      trackletScores.push_back(0.);
      const auto tripletParam = fGraphConstructor.fvTripletParams[iHitL][iTriplet];
      trackletFitParams.push_back(tripletParam);
      nTriplets++;
    }
  }
  LOG(info) << "FindTracksCpu(): Num triplets used for making tracks: " << nTriplets;

  /// organize triplets by station
  const int NStations = 12;
  std::vector<std::vector<std::vector<unsigned int>>> tripletsByStation(NStations);
  std::vector<std::vector<float>> tripletsScore(NStations);
  std::vector<std::vector<std::array<float, 7>>> tripletsFitParams(NStations);
  for (int i = 0; i < nTriplets; i++) {
    const int sta = frWData.Hit(tracklets[i][0]).Station();
    tripletsByStation[sta].push_back(tracklets[i]);
    tripletsScore[sta].push_back(0.);
    tripletsFitParams[sta].push_back(trackletFitParams[i]);
  }

  constexpr float degree_to_rad = 3.14159 / 180.0;
  double angle1YZ, angle2YZ, angleDiffYZ1, angle1XZ, angle2XZ, angleDiffXZ1;
  double angle3YZ, angle4YZ, angleDiffYZ2, angle3XZ, angle4XZ, angleDiffXZ2;
  double angleDiffYZ, angleDiffXZ;
  // cuts from distribution figures for overlapping triplets
  constexpr float YZ_cut         = 3.0f * degree_to_rad;
  constexpr float XZ_cut_neg_min = -2.0f * degree_to_rad;
  constexpr float XZ_cut_neg_max = 2.0f * degree_to_rad;
  constexpr float XZ_cut_pos_min = -2.0f * degree_to_rad;
  constexpr float XZ_cut_pos_max = 2.0f * degree_to_rad;

  /// go over every tracklet and see if it can be extended with overlapping triplet
  for (int iTracklet = 0; iTracklet < (int) tracklets.size(); ++iTracklet) {
    const auto& tracklet = tracklets[iTracklet];
    int length           = tracklet.size();
    int middleSta        = frWData.Hit(tracklet[length - 2]).Station();
    const bool isJumpTripletLast =
      (frWData.Hit(tracklet[length - 1]).Station() - frWData.Hit(tracklet[length - 3]).Station()) == 3;

    for (int iTriplet = 0; iTriplet < (int) tripletsByStation[middleSta].size(); ++iTriplet) {
      // check overlapping triplet
      if (tracklet[length - 2] != tripletsByStation[middleSta][iTriplet][0]) continue;
      if (tracklet[length - 1] != tripletsByStation[middleSta][iTriplet][1]) continue;

      /// check difference of angle difference between triplets in XZ and YZ
      const auto& h1 = frWData.Hit(tracklet[length - 3]);
      const auto& h2 = frWData.Hit(tracklet[length - 2]);
      const auto& h3 = frWData.Hit(tracklet[length - 1]);
      const auto& h4 = frWData.Hit(tripletsByStation[middleSta][iTriplet][2]);

      // YZ angle 1
      angle1YZ     = std::atan2(h2.Y() - h1.Y(), h2.Z() - h1.Z());
      angle2YZ     = std::atan2(h3.Y() - h2.Y(), h3.Z() - h2.Z());
      angleDiffYZ1 = angle1YZ - angle2YZ;
      // XZ angle 1
      angle1XZ     = std::atan2(h2.X() - h1.X(), h2.Z() - h1.Z());
      angle2XZ     = std::atan2(h3.X() - h2.X(), h3.Z() - h2.Z());
      angleDiffXZ1 = angle1XZ - angle2XZ;
      // YZ angle 2
      angle3YZ     = std::atan2(h3.Y() - h2.Y(), h3.Z() - h2.Z());
      angle4YZ     = std::atan2(h4.Y() - h3.Y(), h4.Z() - h3.Z());
      angleDiffYZ2 = angle3YZ - angle4YZ;
      // XZ angle 2
      angle3XZ     = std::atan2(h3.X() - h2.X(), h3.Z() - h2.Z());
      angle4XZ     = std::atan2(h4.X() - h3.X(), h4.Z() - h3.Z());
      angleDiffXZ2 = angle3XZ - angle4XZ;

      angleDiffYZ = angleDiffYZ1 - angleDiffYZ2;
      angleDiffXZ = angleDiffXZ1 - angleDiffXZ2;

      // YZ cut
      if (angleDiffYZ < -YZ_cut || angleDiffYZ > YZ_cut) continue;
      // positive particles curve -ve in XZ and -ve particles curve +ve in XZ
      if (angleDiffXZ1 < 0) {  // positive particles
        if (angleDiffXZ < XZ_cut_pos_min || angleDiffXZ > XZ_cut_pos_max) continue;
      }
      else {
        if (angleDiffXZ < XZ_cut_neg_min || angleDiffXZ > XZ_cut_neg_max) continue;
      }

      // check momentum compatibility of overlapping triplets
      const auto& oldFitParams = trackletFitParams[iTracklet];  // [chi2, qp, Cqp, Tx, C22, Ty, C33]
      const auto& newFitParams = tripletsFitParams[middleSta][iTriplet];
      // check qp compatibility
      float dqp = oldFitParams[1] - newFitParams[1];
      float Cqp = oldFitParams[2] + newFitParams[2];

      if (!std::isfinite(dqp)) continue;
      if (!std::isfinite(Cqp)) continue;

      float qpchi2Cut = 10.0f;  // def - 10.0f
      if (dqp * dqp > qpchi2Cut * Cqp) continue;

      /// new score should have component of how well the triplets match in momentum
      float newScore = trackletScores[iTracklet] + tripletsScore[middleSta][iTriplet];
      newScore += dqp * dqp / Cqp;  // add momentum chi2 to score

      // create new tracklet with last hit of triplet added
      std::vector<unsigned int> newTracklet = tracklet;
      newTracklet.push_back(tripletsByStation[middleSta][iTriplet][2]);
      tracklets.push_back(newTracklet);
      trackletScores.push_back(newScore);
      trackletFitParams.push_back(newFitParams);
    }
  }

  LOG(info) << "Num tracks constructed: " << tracklets.size();

  std::vector<std::pair<std::vector<unsigned int>, float>> trackAndScores;
  const int min_length = 4;
  // for iter 1. No fitting
  // min length condition
  for (int itracklet = 0; itracklet < (int) tracklets.size(); itracklet++) {
    if (tracklets[itracklet].size() >= min_length) {
      trackAndScores.push_back(std::make_pair(tracklets[itracklet], trackletScores[itracklet]));
    }
  }
  LOG(info) << "[iter 0] Num candidate tracks with length > 4 : " << trackAndScores.size();

  /// remove tracks with chi2 > max_chi2. where max_chi2 is 10*(2*hits - 5) //@TODO: check this
  float trackChi2Cut = 10.0f;  // When not fitting candidates and using q/p proxy to chi2.
  for (int iTrack = 0; iTrack < (int) trackAndScores.size(); iTrack++) {
    if (trackAndScores[iTrack].second > trackChi2Cut * (trackAndScores[iTrack].first.size() - 2)) {
      trackAndScores.erase(trackAndScores.begin() + iTrack);
      iTrack--;
    }
  }
  LOG(info) << "[iter 0] Num tracks after tracks chi2 cut: " << trackAndScores.size();

  if (doCompetition) {  // do track competition

    /// sort tracks by length and lower scores (chi2) first
    std::sort(trackAndScores.begin(), trackAndScores.end(),
              [](const std::pair<std::vector<unsigned int>, float>& a, const std::pair<std::vector<unsigned int>, float>& b) {
                if (a.first.size() == b.first.size()) {
                  return a.second < b.second;
                }
                return a.first.size() > b.first.size();
              });
    LOG(info) << "Tracks sorted.";

#define ALTRUISTIC_COMP
#ifdef ALTRUISTIC_COMP
    /// --- TEST ---
    for (std::size_t iTrack = 0; iTrack < trackAndScores.size(); iTrack++) {
      /// check that all hits are not used
      auto& track    = trackAndScores[iTrack].first;
      bool remove    = false;
      uint nUsedHits = 0;
      std::vector<int> usedHitIDs;
      std::vector<int> usedHitIndexesInTrack;
      for (std::size_t iHit = 0; iHit < track.size(); iHit++) {
        const ca::Hit& hit = frWData.Hit(track[iHit]);
        if (frWData.IsHitKeyUsed(hit.FrontKey()) || frWData.IsHitKeyUsed(hit.BackKey())) {
          nUsedHits++;
          usedHitIDs.push_back(track[iHit]);
          usedHitIndexesInTrack.push_back(iHit);
        }
      }
      if (nUsedHits == 0) {  // clean tracks
        /// mark all hits as used
        for (const auto& hit : track) {
          frWData.IsHitKeyUsed(frWData.Hit(hit).FrontKey()) = 1;
          frWData.IsHitKeyUsed(frWData.Hit(hit).BackKey())  = 1;
        }
        continue;  // go next track
      }
      else if (nUsedHits > 0) {  // some hits used but still >=4 hits left
        if (track.size() - nUsedHits >= 4) {
          // remove used hits.
          // read usedHitIndexes in reverse order
          std::sort(usedHitIndexesInTrack.begin(), usedHitIndexesInTrack.end(), std::greater<int>());
          for (const auto usedHitIndex : usedHitIndexesInTrack) {
            track.erase(track.begin() + usedHitIndex);
          }
          // mark remaining hits as used
          for (const auto& hit : track) {
            frWData.IsHitKeyUsed(frWData.Hit(hit).FrontKey()) = 1;
            frWData.IsHitKeyUsed(frWData.Hit(hit).BackKey())  = 1;
          }
          continue;  // go next track
        }
        else {
          remove = true;  // remove track if begging not successful
        }
      }

      if (remove
          && (track.size() - nUsedHits == 3)) {  // 'beg' for hit from longer accepted track only if one hit required
        for (std::size_t iBeg = 0; iBeg < iTrack; iBeg++) {  // track to beg from
          if (trackAndScores[iBeg].first.size() <= trackAndScores[iTrack].first.size())
            continue;                                        // only beg from longer tracks
          if (trackAndScores[iBeg].first.size() < 5) break;  // atleast 4 hits must be left after donation
          if (trackAndScores[iBeg].second < trackAndScores[iTrack].second)
            continue;  // dont donate to higher chi2 beggar

          auto& begTrack = trackAndScores[iBeg].first;
          for (std::size_t iBegHit = 0; iBegHit < begTrack.size(); iBegHit++) {
            const auto begHit = begTrack[iBegHit];
            if (begHit == usedHitIDs[0]) continue;  // dont let exact hit be borrowed.
            if (frWData.Hit(begHit).FrontKey() == frWData.Hit(usedHitIDs[0]).FrontKey()
                || frWData.Hit(begHit).BackKey()
                     == frWData.Hit(usedHitIDs[0]).BackKey()) {  // only one track will match
              // remove iBegHit from begTrack
              begTrack.erase(begTrack.begin() + iBegHit);
              // reset hit flags. Will be reset by beggar
              frWData.IsHitKeyUsed(frWData.Hit(begHit).FrontKey()) = 0;
              frWData.IsHitKeyUsed(frWData.Hit(begHit).BackKey())  = 0;

              remove = false;
              break;
            }
          }
        }
      }

      if (remove) {
        trackAndScores.erase(trackAndScores.begin() + iTrack);
        iTrack--;
        continue;
      }
      /// mark all hits as used
      for (const auto& hit : track) {
        frWData.IsHitKeyUsed(frWData.Hit(hit).FrontKey()) = 1;
        frWData.IsHitKeyUsed(frWData.Hit(hit).BackKey())  = 1;
      }
    }
#endif  // ALTRUISTIC_COMP

  }  // track competition

  // save tracks
  frWData.RecoHitIndices().reserve(200000);
  frWData.RecoTracks().reserve(100000);

  if (trackAndScores.size() == 0) {  // no tracks found
    return;
  }
  for (const auto& [track, _] : trackAndScores) {
    for (const auto& iHit : track) {
      const ca::Hit& hit = frWData.Hit(iHit);
      // used strips are marked
      frWData.IsHitKeyUsed(hit.FrontKey()) = 1;
      frWData.IsHitKeyUsed(hit.BackKey())  = 1;
      frWData.RecoHitIndices().push_back(hit.Id());
    }
    Track t;
    t.fNofHits = track.size();
    frWData.RecoTracks().push_back(t);
  }

  LOG(info) << "FindTracksCPU(): Num tracks competition: " << trackAndScores.size();
}

void GnnGpuTrackFinderSetup::SetupGNN(const int iteration)
{
  const int nStations = fParameters.GetNstationsActive();
  const int NHits     = frWData.Hits().size();

  fGraphConstructor.fEmbedCoord.reset(NHits, xpu::buf_io);

  std::vector<int> EmbNetTopology_ = {3, 16, 16, 6};
  EmbedNet EmbNet_                 = EmbedNet(EmbNetTopology_);
  const std::string srcDir         = "/home/tyagi/cbmroot/NN/";
  if (iteration == 0) {
    std::string fNameModel   = "embed/embed";
    std::string fNameWeights = srcDir + fNameModel + "Weights_11.txt";
    std::string fNameBiases  = srcDir + fNameModel + "Biases_11.txt";
    EmbNet_.loadModel(fNameWeights, fNameBiases);
  }
  else if (iteration == 3) {
    // std::string fNameModel   = "embed/embed";
    // std::string fNameWeights = srcDir + fNameModel + "Weights_13.txt";
    // std::string fNameBiases  = srcDir + fNameModel + "Biases_13.txt";
    // EmbNet_.loadModel(fNameWeights, fNameBiases);
  }
  const auto weights = EmbNet_.getWeights();
  const auto biases  = EmbNet_.getBias();

  std::array<std::array<float, 3>, 16> embedWeights_0;   ///< Layer 0
  std::array<std::array<float, 16>, 16> embedWeights_1;  ///< Layer 1
  std::array<std::array<float, 16>, 6> embedWeights_2;   ///< Layer 2
  std::array<float, 16> embedBias_0;                     ///< Layer 0
  std::array<float, 16> embedBias_1;                     ///< Layer 1
  std::array<float, 6> embedBias_2;                      ///< Layer 2

  // Load weights and biases layer by layer
  // Move all of this to within EmbNet_ class
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 16; ++j) {
      embedWeights_0[i][j] = weights[0][i][j];
    }
  }
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 16; ++j) {
      embedWeights_1[i][j] = weights[1][i][j];
    }
  }
  for (int i = 0; i < 16; ++i) {
    for (int j = 0; j < 6; ++j) {
      embedWeights_2[i][j] = weights[2][i][j];
    }
  }
  for (int i = 0; i < 16; ++i) {
    embedBias_0[i] = biases[0][i];
  }
  for (int i = 0; i < 16; ++i) {
    embedBias_1[i] = biases[1][i];
  }
  for (int i = 0; i < 6; ++i) {
    embedBias_2[i] = biases[2][i];
  }
  // LOG(info) << "Weights loaded into arrays";

  fGraphConstructor.fEmbedParameters.reset(1, xpu::buf_io);
  xpu::h_view vEmbedParaP{fGraphConstructor.fEmbedParameters};

  // Set weights and biases to embedParameters
  vEmbedParaP[0].embedWeights_0 = embedWeights_0;
  vEmbedParaP[0].embedWeights_1 = embedWeights_1;
  vEmbedParaP[0].embedWeights_2 = embedWeights_2;
  vEmbedParaP[0].embedBias_0    = embedBias_0;
  vEmbedParaP[0].embedBias_1    = embedBias_1;
  vEmbedParaP[0].embedBias_2    = embedBias_2;

  // Copy to GPU
  fQueue.copy(fGraphConstructor.fEmbedParameters, xpu::h2d);
  // LOG(info) << "Copied embedding parameters to GPU.";

  // Setup for doublets
  fGraphConstructor.fDoublets.reset(NHits, xpu::buf_io);
  fGraphConstructor.fNNeighbours.reset(NHits, xpu::buf_io);

  // Set starting index of hits for each station
  fGraphConstructor.fIndexFirstHitStation.reset(nStations + 1, xpu::buf_io);
  xpu::h_view fvIndexFirstHitStation{fGraphConstructor.fIndexFirstHitStation};

  int iHit    = 0;
  int lastSta = 0;

  fvIndexFirstHitStation[lastSta] = 0;
  for (const auto& hit : frWData.Hits()) {
    // LOG(info) << "Hit: " << iHit << " , Station: " << hit.Station(); // hits ordered by Station
    const int curSta = hit.Station();
    if (curSta == lastSta + 1) {
      fvIndexFirstHitStation[curSta] = iHit;
      // LOG(info) << fvIndexFirstHitStation[curSta];
      lastSta = curSta;
    }
    iHit++;
  }
  fvIndexFirstHitStation[nStations] = iHit;
  // LOG(info) << iHit;
  fQueue.copy(fGraphConstructor.fIndexFirstHitStation, xpu::h2d);

  // Set Triplets
  fGraphConstructor.fTriplets.reset(NHits, xpu::buf_io);
  fGraphConstructor.fNTriplets.reset(NHits, xpu::buf_io);

  /// Flatten triplets
  // fGraphConstructor.fOffsets.reset(NHits, xpu::buf_device);
  // const int nBlocks = std::ceil(NHits / GnnGpuConstants::kScanBlockSize);
  // fGraphConstructor.fBlockOffsets.reset(nBlocks,
  //                                       xpu::buf_device);  // size: numBlocks used by scan
  // fGraphConstructor.fBlockOffsetsLast.reset(std::ceil(nBlocks / GnnGpuConstants::kScanBlockSize),
  //                                           xpu::buf_device);  // size: ceil(numBlocks / kScanBlockSize)

  // Initialize triplet fit params
  fGraphConstructor.fvTripletParams.reset(NHits, xpu::buf_io);
  fGraphConstructor.fTripletsSelected.reset(NHits, xpu::buf_io);
}