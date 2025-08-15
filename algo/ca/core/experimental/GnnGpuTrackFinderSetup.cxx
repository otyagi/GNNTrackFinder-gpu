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

  bool isCpu  = xpu::device::active().backend() == xpu::cpu;
  int nBlocks = isCpu ? fNHits : std::ceil(float(fNHits) / float(kEmbedHitsBlockSize));
  //nBlocks = 1;

  int nBlocksCD =
    nBlocks
    * constants::gpu::
      MaxDoubletsFromHit;  //std::ceil(float(nBlocks * maxDoubletsFromHit) / float(kSingletConstructorBlockSize));

  //***
  fGraphConstructor.fvTrackParams.reset(frWData.Hits().size(), xpu::buf_device);
  //***
  fGraphConstructor.fIteration = fIteration;

  xpu::set<strGnnGpuGraphConstructor>(fGraphConstructor);

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    fEventTimeMonitor.nIterations = fIteration;
    xpu::push_timer("Full_time");
    xpu::push_timer("EmbedHits_time");
  }

  fQueue.launch<EmbedHits>(xpu::n_blocks(GnnGpuConstants::kEmbedHitsBlockSize));

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                       = xpu::pop_timer();
    fEventTimeMonitor.EmbedHits_time[fIteration] = step_time;
    LOG(info) << "EmbedHits_time: " << step_time.wall();
    xpu::push_timer("NearestNeighbours_time");
  }

  fQueue.launch<NearestNeighbours>(xpu::n_blocks(GnnGpuConstants::kEmbedHitsBlockSize));

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                               = xpu::pop_timer();
    fEventTimeMonitor.NearestNeighbours_time[fIteration] = step_time;
    LOG(info) << "NearestNeighbours_time: " << step_time.wall();
    xpu::push_timer("MakeTriplets_time");
  }

  fQueue.launch<MakeTriplets>(xpu::n_blocks(GnnGpuConstants::kEmbedHitsBlockSize));

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings step_time                               = xpu::pop_timer();
    fEventTimeMonitor.MakeTriplets_time[fIteration] = step_time;
    LOG(info) << "MakeTriplets_time: " << step_time.wall();
    // xpu::push_timer("FitTriplets_time");
  }

  // fQueue.launch<FitTriplets>(xpu::n_blocks(GnnGpuConstants::kEmbedHitsBlockSize));

  // if constexpr (constants::gpu::GpuTimeMonitoring) {
  //   xpu::timings step_time                               = xpu::pop_timer();
  //   fEventTimeMonitor.FitTriplets_time[fIteration] = step_time;
  //   LOG(info) << "FitTriplets_time: " << step_time.wall();
    // xpu::push_timer("FitTriplets_time");
  // }

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

  // save edges as tracks
  fQueue.copy(fGraphConstructor.fDoublets, xpu::d2h);
  fQueue.copy(fGraphConstructor.fNNeighbours, xpu::d2h);
}

void GnnGpuTrackFinderSetup::SaveDoubletsAsTracks()
{
  // use doublets and NNeighbours to copy.
  // Dont bother about marking strips as used.
  frWData.RecoHitIndices().reserve(200000);
  frWData.RecoTracks().reserve(100000);

  const auto nHits = frWData.Hits().size();
  int nDoublets   = 0;
  for (int iHitL = 0; iHitL < nHits; iHitL++) {
    const auto& hitL = fGraphConstructor.fvHits[iHitL];
    if (fGraphConstructor.fNNeighbours[iHitL] == 0 || hitL.Station() > 10) continue;
    // LOG(info) << "iHitL: " << iHitL << ", Neighbours: " <<fGraphConstructor.fNNeighbours[iHitL];
    for (unsigned int iHitM = 0; iHitM < fGraphConstructor.fNNeighbours[iHitL]; iHitM++) {
      const auto& hitM = fGraphConstructor.fvHits[fGraphConstructor.fDoublets[iHitL][iHitM]];
      // LOG(info) << "iHitL: " << iHitL << " ID: " << hitL.Id();
      frWData.RecoHitIndices().push_back(hitL.Id());
      // LOG(info) << "iHitM: " << iHitM << " ID: " << hitM.Id();
      frWData.RecoHitIndices().push_back(hitM.Id());
      // used strips are marked
      frWData.IsHitKeyUsed(hitL.FrontKey()) = 1;
      frWData.IsHitKeyUsed(hitM.BackKey())  = 1;
      Track t;
      t.fNofHits = 2;
      frWData.RecoTracks().push_back(t);
      nDoublets++;
    }
  }
  LOG(info) << "Num doublets as tracks: " << nDoublets;
}

void GnnGpuTrackFinderSetup::SetupMetricLearning(const int iteration)
{
  const int nStations = fParameters.GetNstationsActive();

  fGraphConstructor.fEmbedCoord.reset(frWData.Hits().size(), xpu::buf_io);

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
  fGraphConstructor.fDoublets.reset(frWData.Hits().size(), xpu::buf_io);
  fGraphConstructor.fNNeighbours.reset(frWData.Hits().size(), xpu::buf_io);

  // Set starting index of hits for each station
  fGraphConstructor.fIndexFirstHitStation.reset(nStations + 1, xpu::buf_io);
  xpu::h_view fvIndexFirstHitStation{fGraphConstructor.fIndexFirstHitStation};

  int iHit                        = 0;
  int lastSta                     = 0;
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
}