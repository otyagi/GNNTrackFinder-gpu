/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "Hitfind.h"

#include "AlgoFairloggerCompat.h"
#include "compat/OpenMP.h"
#include "util/TimingsFormat.h"

#include <chrono>

using namespace std;
using fles::Subsystem;

namespace cbm::algo::tof
{
  // -----   Constructor   ------------------------------------------------------
  Hitfind::Hitfind(tof::HitfindSetup setup) : fNbSm(setup.NbSm), fNbRpc(setup.NbRpc), fStorDigi(fNbSm.size())
  {

    // Create one algorithm per RPC for TOF and configure it with parametersa
    for (uint32_t SmType = 0; SmType < fNbSm.size(); SmType++) {

      int32_t NbSm  = fNbSm[SmType];
      int32_t NbRpc = fNbRpc[SmType];
      fStorDigi[SmType].resize(NbSm * NbRpc);

      for (int32_t Sm = 0; Sm < NbSm; Sm++) {
        for (int32_t Rpc = 0; Rpc < NbRpc; Rpc++) {
          auto par = std::make_unique<cbm::algo::tof::ClusterizerRpcPar>();

          HitfindSetup::Rpc rpcPar = setup.rpcs[SmType][Sm * NbRpc + Rpc];
          par->fDeadStrips         = rpcPar.deadStrips;
          par->fPosYMaxScal        = rpcPar.posYMaxScal;
          par->fdMaxTimeDist       = rpcPar.maxTimeDist;
          par->fdMaxSpaceDist      = rpcPar.maxSpaceDist;
          par->fSigVel             = rpcPar.sigVel;
          par->fCPTOffYBinWidth    = rpcPar.CPTOffYBinWidth;
          par->fCPTOffY            = rpcPar.CPTOffY;
          par->fCPTOffYRange       = rpcPar.CPTOffYRange;
          par->fTimeRes            = rpcPar.timeRes;

          //geometry information, can in principle be done channel-wise but not needed for now
          const double* tra_ptr = rpcPar.cell.translation.data();
          const double* rot_ptr = rpcPar.cell.rotation.data();

          //channel parameters
          int32_t NbChan = rpcPar.chanPar.size();
          par->fChanPar.resize(NbChan);

          for (int32_t Ch = 0; Ch < NbChan; Ch++) {
            HitfindSetup::Channel chanPar   = rpcPar.chanPar[Ch];
            par->fChanPar[Ch].address       = chanPar.address;
            par->fChanPar[Ch].cell.pos      = ROOT::Math::XYZVector(tra_ptr[0], tra_ptr[1], tra_ptr[2]);
            par->fChanPar[Ch].cell.rotation = ROOT::Math::Rotation3D(&rot_ptr[0], &rot_ptr[9]);
            par->fChanPar[Ch].cell.sizeX    = rpcPar.cell.sizeX;
            par->fChanPar[Ch].cell.sizeY    = rpcPar.cell.sizeY;
          }
          fAlgo.emplace_back(std::move(*par));

          // fill unique rpc pointer vectors for parallelization
          fStorDigiPtr.push_back(&fStorDigi[SmType][Sm * NbRpc + Rpc]);
        }
      }
    }
    L_(info) << "--- Configured hitfinder algorithms for TOF.";
  }
  // ----------------------------------------------------------------------------


  // -----   Execution   -------------------------------------------------------
  Hitfind::resultType Hitfind::operator()(gsl::span<CbmTofDigi> digiIn)
  {
    // --- Output data
    resultType result = {};
    //auto& [clusterTs, monitor, digiInd] = result;    // TO DO: Re-activte this when compiler bug is fixed
    auto& clusterTs = std::get<0>(result);
    auto& monitor   = std::get<1>(result);
    auto& digiInd   = std::get<2>(result);

    // Loop over the digis array and store the Digis in separate vectors for
    // each RPC modules
    xpu::push_timer("TofHitfindChanSort");
    for (size_t idigi = 0; idigi < digiIn.size(); idigi++) {
      CbmTofDigi* pDigi = &(digiIn[idigi]);

      // These are doubles in the digi class
      const double SmType = pDigi->GetType();
      const double Sm     = pDigi->GetSm();
      const double Rpc    = pDigi->GetRpc();
      const int NbRpc     = fNbRpc[SmType];
      if (SmType >= fNbSm.size() || Sm * NbRpc + Rpc >= fNbSm[SmType] * NbRpc) {
        // Error already counted for monitoring during Digis calibration, so just discard here
        continue;
      }
      fStorDigi[SmType][Sm * NbRpc + Rpc].emplace_back(*pDigi, idigi);
    }
    monitor.fSortTime = xpu::pop_timer();

    PODVector<Hit> clustersFlat;   // cluster storage
    PODVector<size_t> chanSizes;   // nClusters per channel
    PODVector<u32> chanAddresses;  // channel addresses

    // Prefix arrays for parallelization
    std::vector<size_t> cluPrefix;
    std::vector<size_t> sizePrefix;
    std::vector<size_t> addrPrefix;
    std::vector<size_t> indPrefix;

    xpu::push_timer("TofHitfind");
    xpu::t_add_bytes(digiIn.size_bytes());

    CBM_PARALLEL()
    {
      const int ithread  = openmp::GetThreadNum();
      const int nthreads = openmp::GetNumThreads();

      CBM_OMP(single)
      {
        cluPrefix.resize(nthreads + 1);
        sizePrefix.resize(nthreads + 1);
        addrPrefix.resize(nthreads + 1);
        indPrefix.resize(nthreads + 1);
      }

      //auto [clusters, sizes, addresses, indices] = Clusterizer::resultType();    // TO DO: Re-activte this when compiler bug is fixed
      auto localresult = Clusterizer::resultType();
      auto& clusters   = std::get<0>(localresult);
      auto& sizes      = std::get<1>(localresult);
      auto& addresses  = std::get<2>(localresult);
      auto& indices    = std::get<3>(localresult);

      CBM_OMP(for schedule(dynamic) nowait)
      for (uint32_t iRpc = 0; iRpc < fAlgo.size(); iRpc++) {

        // Get digis
        std::vector<std::pair<CbmTofDigi, int32_t>>& digiExp = *fStorDigiPtr[iRpc];

        // Build clusters
        //auto [rpc_clu, rpc_size, rpc_addr, rpc_ind] = fAlgo[iRpc](digiExp);     // TO DO: Re-activte this when compiler bug is fixed
        auto rpc_result = fAlgo[iRpc](digiExp);
        auto& rpc_clu   = std::get<0>(rpc_result);
        auto& rpc_size  = std::get<1>(rpc_result);
        auto& rpc_addr  = std::get<2>(rpc_result);
        auto& rpc_ind   = std::get<3>(rpc_result);

        // Append clusters to output
        clusters.insert(clusters.end(), std::make_move_iterator(rpc_clu.begin()),
                        std::make_move_iterator(rpc_clu.end()));

        // store partition size
        sizes.insert(sizes.end(), std::make_move_iterator(rpc_size.begin()), std::make_move_iterator(rpc_size.end()));

        // Store hw address of partition
        addresses.insert(addresses.end(), std::make_move_iterator(rpc_addr.begin()),
                         std::make_move_iterator(rpc_addr.end()));

        // store digi indices
        indices.insert(indices.end(), std::make_move_iterator(rpc_ind.begin()), std::make_move_iterator(rpc_ind.end()));

        // Clear digi storage
        digiExp.clear();
      }
      cluPrefix[ithread + 1]  = clusters.size();
      sizePrefix[ithread + 1] = sizes.size();
      addrPrefix[ithread + 1] = addresses.size();
      indPrefix[ithread + 1]  = indices.size();
      CBM_OMP(barrier)

      CBM_OMP(single)
      {
        for (int i = 1; i < (nthreads + 1); i++) {
          cluPrefix[i] += cluPrefix[i - 1];
          sizePrefix[i] += sizePrefix[i - 1];
          addrPrefix[i] += addrPrefix[i - 1];
          indPrefix[i] += indPrefix[i - 1];
        }

        clustersFlat.resize(cluPrefix[nthreads]);
        chanSizes.resize(sizePrefix[nthreads]);
        chanAddresses.resize(addrPrefix[nthreads]);
        digiInd.resize(indPrefix[nthreads]);
      }
      std::move(clusters.begin(), clusters.end(), clustersFlat.begin() + cluPrefix[ithread]);
      std::move(sizes.begin(), sizes.end(), chanSizes.begin() + sizePrefix[ithread]);
      std::move(addresses.begin(), addresses.end(), chanAddresses.begin() + addrPrefix[ithread]);
      std::move(indices.begin(), indices.end(), digiInd.begin() + indPrefix[ithread]);
    }

    // Monitoring
    monitor.fTime     = xpu::pop_timer();
    monitor.fNumDigis = digiIn.size();
    monitor.fNumHits  = clustersFlat.size();

    // Create ouput vector
    clusterTs = PartitionedVector(std::move(clustersFlat), chanSizes, chanAddresses);

    return result;
  }
  // ----------------------------------------------------------------------------

}  // namespace cbm::algo::tof
