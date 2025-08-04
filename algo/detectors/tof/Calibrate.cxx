/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#include "Calibrate.h"

#include "AlgoFairloggerCompat.h"
#include "util/TimingsFormat.h"

#include <chrono>

using namespace std;
using fles::Subsystem;

namespace cbm::algo::tof
{
  // -----   Constructor  ------------------------------------------------------
  Calibrate::Calibrate(tof::CalibrateSetup setup) : fSetup(setup), smTypeOffset(1, 0), smOffset(1, 0), rpcOffset(1, 0)
  {
    // Initialize offset arrays for channel deadtime check
    for (uint32_t SmType = 0; SmType < fSetup.NbSm.size(); SmType++) {
      int32_t NbSm  = fSetup.NbSm[SmType];
      int32_t NbRpc = fSetup.NbRpc[SmType];
      smTypeOffset.push_back(smTypeOffset.back() + NbSm);
      for (int32_t Sm = 0; Sm < NbSm; Sm++) {
        smOffset.push_back(smOffset.back() + NbRpc);
        for (int32_t Rpc = 0; Rpc < NbRpc; Rpc++) {
          int32_t NbChan = fSetup.rpcs[SmType][Sm * NbRpc + Rpc].chanPar.size();
          rpcOffset.push_back(rpcOffset.back() + 2 * NbChan);  //Factor 2 for channel sides
        }
      }
    }
    mChannelDeadTime = std::vector<double>(rpcOffset.back(), std::numeric_limits<double>::quiet_NaN());
  }


  // -----   Execution   -------------------------------------------------------
  Calibrate::resultType Calibrate::operator()(gsl::span<const CbmTofDigi> digiIn)
  {
    xpu::push_timer("TofCalibrate");
    xpu::t_add_bytes(digiIn.size_bytes());

    // --- Output data
    resultType result = {};

    auto& calDigiOut = result.first;
    auto& monitor    = result.second;
    calDigiOut.reserve(digiIn.size());

    std::fill(mChannelDeadTime.begin(), mChannelDeadTime.end(), std::numeric_limits<double>::quiet_NaN());

    for (const auto& digi : digiIn) {
      const double SmType = digi.GetType();
      const double Sm     = digi.GetSm();
      const double Rpc    = digi.GetRpc();
      const double Chan   = digi.GetChannel();
      const double Side   = digi.GetSide();
      const int NbRpc     = fSetup.NbRpc[SmType];

      auto& rpcs = fSetup.rpcs;
      if (SmType >= rpcs.size() || Sm * NbRpc + Rpc >= rpcs.at(SmType).size()) {
        monitor.fDigiCalibUnknownRPC++;
        continue;
      }

      CalibrateSetup::Rpc& rpcPar      = fSetup.rpcs.at(SmType).at(Sm * NbRpc + Rpc);
      CalibrateSetup::Channel& chanPar = rpcPar.chanPar[Chan];

      // Check dead time
      const size_t chanIdx = rpcOffset[smOffset[smTypeOffset[SmType] + Sm] + Rpc] + Chan + Side * rpcPar.chanPar.size();
      const double deadTime = mChannelDeadTime[chanIdx];

      if (!std::isnan(deadTime) && digi.GetTime() <= deadTime) {
        mChannelDeadTime[chanIdx] = digi.GetTime() + rpcPar.channelDeadtime;
        monitor.fDigiDeadTimeCount++;
        continue;
      }
      mChannelDeadTime[chanIdx] = digi.GetTime() + rpcPar.channelDeadtime;

      // Create calibrated digi
      CbmTofDigi& pCalDigi = calDigiOut.emplace_back(digi);

      // Check if channel sides need to be swapped
      if (rpcPar.swapChannelSides && 5 != SmType && 8 != SmType) {
        pCalDigi.SetAddress(Sm, Rpc, Chan, (0 == Side) ? 1 : 0, SmType);
      }

      // calibrate Digi Time
      pCalDigi.SetTime(pCalDigi.GetTime() - chanPar.vCPTOff[Side]);

      // subtract Offset
      const double dTot = std::max(pCalDigi.GetTot() - chanPar.vCPTotOff[Side], 0.001);

      // calibrate Digi ToT
      pCalDigi.SetTot(dTot * chanPar.vCPTotGain[Side]);

      // walk correction
      std::vector<double>& walk = chanPar.vCPWalk[Side];
      const double dTotBinSize  = (rpcPar.TOTMax - rpcPar.TOTMin) / rpcPar.numClWalkBinX;
      int32_t iWx               = std::max((int32_t)((pCalDigi.GetTot() - rpcPar.TOTMin) / dTotBinSize), 0);
      iWx                       = std::min(iWx, rpcPar.numClWalkBinX - 1);

      const double dDTot = (pCalDigi.GetTot() - rpcPar.TOTMin) / dTotBinSize - (double) iWx - 0.5;
      double dWT         = walk[iWx];

      // linear interpolation to next bin
      if (dDTot > 0) {
        if (iWx < rpcPar.numClWalkBinX - 1) {
          dWT += dDTot * (walk[iWx + 1] - walk[iWx]);
        }
      }
      else {
        if (0 < iWx) {
          dWT -= dDTot * (walk[iWx - 1] - walk[iWx]);
        }
      }
      pCalDigi.SetTime(pCalDigi.GetTime() - dWT);  // calibrate Digi Time
    }

    /// Sort the buffers of hits due to the time offsets applied
    // (insert-sort faster than std::sort due to pre-sorting)
    for (std::size_t i = 1; i < calDigiOut.size(); ++i) {
      CbmTofDigi digi = calDigiOut[i];
      std::size_t j   = i;
      while (j > 0 && calDigiOut[j - 1].GetTime() > digi.GetTime()) {
        calDigiOut[j] = calDigiOut[j - 1];
        --j;
      }
      calDigiOut[j] = digi;
    }

    //  Kept for possible unsorted input
    //  std::sort(calDigiOut.begin(), calDigiOut.end(),
    //  [](const CbmTofDigi& a, const CbmTofDigi& b) -> bool { return a.GetTime() < b.GetTime(); });

    monitor.fTime     = xpu::pop_timer();
    monitor.fNumDigis = digiIn.size();
    return result;
  }

}  // namespace cbm::algo::tof
