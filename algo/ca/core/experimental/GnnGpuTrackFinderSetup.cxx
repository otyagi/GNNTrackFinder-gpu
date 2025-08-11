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

void GnnGpuTrackFinderSetup::SetupGrid()
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

void GnnGpuTrackFinderSetup::SetupMaterialMap()
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

void GnnGpuTrackFinderSetup::SetInputData()
{
  fTripletConstructor.fvHits.reset(frWData.Hits().size(), xpu::buf_io);
  xpu::h_view vfvHits{fTripletConstructor.fvHits};

  std::copy_n(frWData.Hits().begin(), frWData.Hits().size(), &vfvHits[0]);

  fQueue.copy(fTripletConstructor.fvHits, xpu::h2d);
}

void GnnGpuTrackFinderSetup::SetupIterationData(int iter)
{
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

void GnnGpuTrackFinderSetup::RunGpuTrackingSetup()
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

}
