/* Copyright (C) 2024-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov [committer] */

/// \file CaGpuTrackFinderSetup.cxx
/// \brief The class is responsible for setting up the environment and the order in which kernels are launched to perform tracking using XPU

#include "CaGpuTrackFinderSetup.h"

using namespace cbm::algo::ca;

GpuTrackFinderSetup::GpuTrackFinderSetup(WindowData& wData, const ca::Parameters<fvec>& pars)
  : fParameters(pars)
  , frWData(wData)
  , fIteration(0)
{
}

void GpuTrackFinderSetup::SetupParameters()
{
  int nStations   = fParameters.GetNstationsActive();
  int nIterations = fParameters.GetCAIterations().size();
  fTripletConstructor.fParams.reset(nIterations, xpu::buf_io);
  xpu::h_view vfParams{fTripletConstructor.fParams};

  for (int i = 0; i < nIterations; i++) {
    GpuParameters gpuParams              = fParameters;
    fTripletConstructor.fParams_const[i] = fParameters;
    const auto& caIteration              = fParameters.GetCAIterations()[i];

    gpuParams.fNStations                            = nStations;
    gpuParams.maxSlopePV                            = caIteration.GetMaxSlopePV();
    gpuParams.maxQp                                 = caIteration.GetMaxQp();
    gpuParams.maxDZ                                 = caIteration.GetMaxDZ();
    fTripletConstructor.fParams_const[i].fNStations = nStations;
    fTripletConstructor.fParams_const[i].maxSlopePV = caIteration.GetMaxSlopePV();
    fTripletConstructor.fParams_const[i].maxQp      = caIteration.GetMaxQp();
    fTripletConstructor.fParams_const[i].maxDZ      = caIteration.GetMaxDZ();

    if (caIteration.GetElectronFlag()) {
      gpuParams.particleMass                            = constants::phys::ElectronMass;
      fTripletConstructor.fParams_const[i].particleMass = constants::phys::ElectronMass;
    }
    else {
      gpuParams.particleMass                            = constants::phys::MuonMass;
      fTripletConstructor.fParams_const[i].particleMass = constants::phys::MuonMass;
    }

    gpuParams.primaryFlag                            = caIteration.GetPrimaryFlag();
    fTripletConstructor.fParams_const[i].primaryFlag = caIteration.GetPrimaryFlag();

    if (caIteration.GetPrimaryFlag()) {
      gpuParams.targB                            = fParameters.GetVertexFieldValue();
      fTripletConstructor.fParams_const[i].targB = fParameters.GetVertexFieldValue();
    }
    else {
      gpuParams.targB                            = fParameters.GetStation(0).fieldSlice.GetFieldValue(0, 0);
      fTripletConstructor.fParams_const[i].targB = fParameters.GetStation(0).fieldSlice.GetFieldValue(0, 0);
    }

    gpuParams.doubletChi2Cut                                 = caIteration.GetDoubletChi2Cut();
    gpuParams.tripletChi2Cut                                 = caIteration.GetTripletChi2Cut();
    gpuParams.tripletFinalChi2Cut                            = caIteration.GetTripletFinalChi2Cut();
    gpuParams.isTargetField                                  = (fabs(gpuParams.targB.y) > 0.001);
    fTripletConstructor.fParams_const[i].doubletChi2Cut      = caIteration.GetDoubletChi2Cut();
    fTripletConstructor.fParams_const[i].tripletChi2Cut      = caIteration.GetTripletChi2Cut();
    fTripletConstructor.fParams_const[i].tripletFinalChi2Cut = caIteration.GetTripletFinalChi2Cut();

    ca::MeasurementXy<float> targMeas(fParameters.GetTargetPositionX()[0], fParameters.GetTargetPositionY()[0],
                                      caIteration.GetTargetPosSigmaX() * caIteration.GetTargetPosSigmaX(),
                                      caIteration.GetTargetPosSigmaY() * caIteration.GetTargetPosSigmaX(), 0, 1, 1);
    gpuParams.targetMeasurement                            = targMeas;
    fTripletConstructor.fParams_const[i].targetMeasurement = targMeas;

    vfParams[i] = gpuParams;
  }

  fQueue.copy(fTripletConstructor.fParams, xpu::h2d);

  for (int ista = 0; ista < nStations; ista++) {
    fTripletConstructor.fStations_const[ista] = fParameters.GetStation(ista);
  }

  xpu::set<strGpuTripletConstructor>(fTripletConstructor);
}

void GpuTrackFinderSetup::SetupGrid()
{
  unsigned int nStations     = fParameters.GetNstationsActive();
  unsigned int bin_start     = 0;
  unsigned int entries_start = 0;

  fTripletConstructor.fvGpuGrid.reset(nStations, xpu::buf_io);
  xpu::h_view vfvGpuGrid{fTripletConstructor.fvGpuGrid};

  for (unsigned int ista = 0; ista < nStations; ista++) {
    vfvGpuGrid[ista] = GpuGrid(frWData.Grid(ista), bin_start, entries_start);
    bin_start += frWData.Grid(ista).GetFirstBinEntryIndex().size();
    entries_start += frWData.Grid(ista).GetEntries().size();
  }

  fTripletConstructor.fgridFirstBinEntryIndex.reset(bin_start, xpu::buf_io);
  fTripletConstructor.fgridEntries.reset(entries_start, xpu::buf_io);

  xpu::h_view vfgridFirstBinEntryIndex{fTripletConstructor.fgridFirstBinEntryIndex};
  xpu::h_view vfgridEntries{fTripletConstructor.fgridEntries};

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

  fQueue.copy(fTripletConstructor.fvGpuGrid, xpu::h2d);
}

void GpuTrackFinderSetup::SetupMaterialMap()
{
  ///Set up material map
  int bin_start  = 0;
  int nStations  = fParameters.GetNstationsActive();
  int fstStation = 0;
  if (nStations == 8) fstStation = 4;
  fTripletConstructor.fMaterialMap.reset(nStations, xpu::buf_io);
  xpu::h_view vfMaterialMap{fTripletConstructor.fMaterialMap};

  for (int ista = 0; ista < nStations; ista++) {
    vfMaterialMap[ista] = GpuMaterialMap(fParameters.GetGeometrySetup().GetMaterial(fstStation + ista), bin_start);
    bin_start += fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins()
                 * fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins();
  }

  fTripletConstructor.fMaterialMapTables.reset(bin_start, xpu::buf_io);
  xpu::h_view vfMaterialMapTables{fTripletConstructor.fMaterialMapTables};

  bin_start = 0;

  for (int ista = 0; ista < nStations; ista++) {
    std::copy_n(fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetTable().begin(),
                fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetTable().size(),
                &vfMaterialMapTables[bin_start]);
    bin_start += fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins()
                 * fParameters.GetGeometrySetup().GetMaterial(fstStation + ista).GetNbins();
  }

  fQueue.copy(fTripletConstructor.fMaterialMap, xpu::h2d);
  fQueue.copy(fTripletConstructor.fMaterialMapTables, xpu::h2d);
}

void GpuTrackFinderSetup::SetInputData()
{
  fTripletConstructor.fvHits.reset(frWData.Hits().size(), xpu::buf_io);
  xpu::h_view vfvHits{fTripletConstructor.fvHits};

  std::copy_n(frWData.Hits().begin(), frWData.Hits().size(), &vfvHits[0]);

  fQueue.copy(fTripletConstructor.fvHits, xpu::h2d);
}

void GpuTrackFinderSetup::SetupIterationData(int iter)
{
  //  xpu::scoped_timer t_("SetupIterationData");
  fIteration = iter;

  fTripletConstructor.fIterationData.reset(1, xpu::buf_io);
  xpu::h_view vfIterationData{fTripletConstructor.fIterationData};
  vfIterationData[0].fNHits             = fNHits;
  vfIterationData[0].fIteration         = iter;
  vfIterationData[0].fNDoublets         = 0;
  vfIterationData[0].fNDoublets_counter = 0;
  vfIterationData[0].fNTriplets         = 0;
  vfIterationData[0].fNTriplets_counter = 0;

  fQueue.copy(fTripletConstructor.fIterationData, xpu::h2d);
}

void GpuTrackFinderSetup::RunGpuTrackingSetup()
{
  fNTriplets = 0;

  xpu::device_prop prop{xpu::device::active()};

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    LOG(info) << "Running GPU tracking chain on device " << prop.name() << ", fIteration: " << fIteration;
  }

  bool isCpu  = xpu::device::active().backend() == xpu::cpu;
  int nBlocks = isCpu ? fNHits : std::ceil(float(fNHits) / float(kSingletConstructorBlockSize));
  //nBlocks = 1;

  int nBlocksCD =
    nBlocks
    * constants::gpu::
      MaxDoubletsFromHit;  //std::ceil(float(nBlocks * maxDoubletsFromHit) / float(kSingletConstructorBlockSize));

  //***
  fTripletConstructor.fvTrackParams.reset(frWData.Hits().size(), xpu::buf_device);
  fTripletConstructor.fHitDoublets.reset(frWData.Hits().size() * constants::gpu::MaxDoubletsFromHit, xpu::buf_device);
  //***
  fTripletConstructor.fIteration = fIteration;

  xpu::set<strGpuTripletConstructor>(fTripletConstructor);

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    fEventTimeMonitor.nIterations = fIteration;
    xpu::push_timer("Full_time");

    xpu::push_timer("PrepareData_time");
  }

  //  if(fIteration == 0) fQueue.copy(fTripletConstructor.fvHits, xpu::h2d);

  //  fQueue.copy(fTripletConstructor.fIterationData, xpu::h2d);

  //  fQueue.copy(fTripletConstructor.fvGpuGrid, xpu::h2d);

  fQueue.copy(fTripletConstructor.fgridFirstBinEntryIndex, xpu::h2d);

  fQueue.copy(fTripletConstructor.fgridEntries, xpu::h2d);

  //  if(fIteration == 0) fQueue.copy(fTripletConstructor.fMaterialMap, xpu::h2d);

  //  if(fIteration == 0) fQueue.copy(fTripletConstructor.fMaterialMapTables, xpu::h2d);

  fQueue.memset(fTripletConstructor.fHitDoublets, 0);

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings pd_time                           = xpu::pop_timer();
    fEventTimeMonitor.PrepareData_time[fIteration] = pd_time;

    xpu::push_timer("MakeSinglets_time");
  }

  fQueue.launch<MakeSinglets>(xpu::n_blocks(nBlocks));

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings sind_time                          = xpu::pop_timer();
    fEventTimeMonitor.MakeSinglets_time[fIteration] = sind_time;

    xpu::push_timer("CompressDoublets_time");
  }

  fQueue.launch<MakeDoublets>(xpu::n_blocks(nBlocks));

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings dind_time                          = xpu::pop_timer();
    fEventTimeMonitor.MakeDoublets_time[fIteration] = dind_time;

    xpu::push_timer("ResetDoublets_time");
  }

  fQueue.copy(fTripletConstructor.fIterationData, xpu::d2h);
  xpu::h_view vfIterationData1{fTripletConstructor.fIterationData};

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    LOG(info) << "GPU tracking :: Doublets found: " << vfIterationData1[0].fNDoublets;
  }

  if (vfIterationData1[0].fNDoublets > 0) {
    fTripletConstructor.fHitDoubletsCompressed.reset(vfIterationData1[0].fNDoublets, xpu::buf_device);
    fTripletConstructor.fvTrackParamsDoublets.reset(vfIterationData1[0].fNDoublets, xpu::buf_device);
    fTripletConstructor.fHitTriplets.reset(vfIterationData1[0].fNDoublets * constants::gpu::MaxTripletsFromDoublet,
                                           xpu::buf_device);

    fQueue.memset(fTripletConstructor.fHitTriplets, 0);
    xpu::set<strGpuTripletConstructor>(fTripletConstructor);

    if constexpr (constants::gpu::GpuTimeMonitoring) {
      xpu::timings rd_time                             = xpu::pop_timer();
      fEventTimeMonitor.ResetDoublets_time[fIteration] = rd_time;

      xpu::push_timer("CompressDoublets_time");
    }

    fQueue.launch<CompressDoublets>(xpu::n_blocks(nBlocksCD));

    if constexpr (constants::gpu::GpuTimeMonitoring) {
      xpu::timings cd_time                                = xpu::pop_timer();
      fEventTimeMonitor.CompressDoublets_time[fIteration] = cd_time;

      xpu::push_timer("FitDoublets_time");
    }

    int nBlocksD = isCpu ? vfIterationData1[0].fNDoublets
                         : std::ceil(float(vfIterationData1[0].fNDoublets) / float(kSingletConstructorBlockSize));

    fQueue.launch<FitDoublets>(xpu::n_blocks(nBlocksD));

    if constexpr (constants::gpu::GpuTimeMonitoring) {
      xpu::timings fd_time                           = xpu::pop_timer();
      fEventTimeMonitor.FitDoublets_time[fIteration] = fd_time;

      xpu::push_timer("MakeTriplets_time");
    }

    fQueue.launch<MakeTriplets>(xpu::n_blocks(nBlocksD));

    if constexpr (constants::gpu::GpuTimeMonitoring) {
      xpu::timings mt_time                            = xpu::pop_timer();
      fEventTimeMonitor.MakeTriplets_time[fIteration] = mt_time;

      xpu::push_timer("ResetTriplets_time");
    }

    fTripletConstructor.fHitDoublets.reset(0, xpu::buf_device);  // TODO: check if we need it for the next iteration

    fQueue.copy(fTripletConstructor.fIterationData, xpu::d2h);
    xpu::h_view vfIterationData2{fTripletConstructor.fIterationData};

    if (vfIterationData2[0].fNTriplets > 0) {
      fTripletConstructor.fHitTripletsCompressed.reset(vfIterationData2[0].fNTriplets, xpu::buf_device);
      //      fTripletConstructor.fvTrackParamsTriplets.reset(vfIterationData2[0].fNTriplets, xpu::buf_device);
      fTripletConstructor.fvTriplets.reset(vfIterationData2[0].fNTriplets, xpu::buf_io);

      int nBlocksCT = isCpu ? vfIterationData1[0].fNDoublets * constants::gpu::MaxTripletsFromDoublet
                            : std::ceil(float(vfIterationData1[0].fNDoublets * constants::gpu::MaxTripletsFromDoublet)
                                        / float(kSingletConstructorBlockSize));

      if constexpr (constants::gpu::CpuSortTriplets) {
        fTripletConstructor.fTripletSortHelper.reset(vfIterationData2[0].fNTriplets, xpu::buf_io);
        //        fTripletConstructor.fTripletSortHelperTmp.reset(vfIterationData2[0].fNTriplets, xpu::buf_io);
        //        fTripletConstructor.dst.reset(nBlocksCT, xpu::buf_io);
      }

      xpu::set<strGpuTripletConstructor>(fTripletConstructor);

      if constexpr (constants::gpu::GpuTimeMonitoring) {
        xpu::timings rt_time                             = xpu::pop_timer();
        fEventTimeMonitor.ResetTriplets_time[fIteration] = rt_time;

        LOG(info) << "GPU tracking :: Triplets found: " << vfIterationData2[0].fNTriplets;

        xpu::push_timer("CompressTriplets_time");
      }

      fQueue.launch<CompressTriplets>(xpu::n_blocks(nBlocksCT));

      if constexpr (constants::gpu::GpuTimeMonitoring) {
        xpu::timings ct_time                                = xpu::pop_timer();
        fEventTimeMonitor.CompressTriplets_time[fIteration] = ct_time;

        xpu::push_timer("FitTriplets_time");
      }

      fQueue.launch<FitTriplets>(
        xpu::n_blocks(/*fTripletConstructor.fIterationData[0].fNTriplets*/ vfIterationData2[0].fNTriplets));

      if constexpr (constants::gpu::GpuTimeMonitoring) {
        xpu::timings ft_time                           = xpu::pop_timer();
        fEventTimeMonitor.FitTriplets_time[fIteration] = ft_time;

        xpu::push_timer("SortTriplets_time");
      }

      if constexpr (constants::gpu::CpuSortTriplets) {
        fQueue.launch<SortTriplets>(xpu::n_blocks(vfIterationData2[0].fNTriplets));
      }

      if constexpr (constants::gpu::GpuTimeMonitoring) {
        xpu::timings srt_time                           = xpu::pop_timer();
        fEventTimeMonitor.SortTriplets_time[fIteration] = srt_time;
      }

      fQueue.copy(fTripletConstructor.fvTriplets, xpu::d2h);
      fQueue.copy(fTripletConstructor.fIterationData, xpu::d2h);

      if constexpr (constants::gpu::CpuSortTriplets) {
        fQueue.copy(fTripletConstructor.fTripletSortHelper, xpu::d2h);
      }

      xpu::h_view vfIterationData3{fTripletConstructor.fIterationData};
      fNTriplets = vfIterationData3[0].fNTriplets;
    }
  }

  fTripletConstructor.fIterationData.reset(0, xpu::buf_device);
  fTripletConstructor.fvGpuGrid.reset(0, xpu::buf_io);
  fTripletConstructor.fgridFirstBinEntryIndex.reset(0, xpu::buf_io);
  fTripletConstructor.fgridEntries.reset(0, xpu::buf_io);

  xpu::set<strGpuTripletConstructor>(fTripletConstructor);  //TODO: check if we need to reset all the buffers here

  if constexpr (constants::gpu::GpuTimeMonitoring) {
    xpu::timings t                           = xpu::pop_timer();
    fEventTimeMonitor.Total_time[fIteration] = t;
    fEventTimeMonitor.PrintTimings(fIteration);
  }
}
