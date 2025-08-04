/* Copyright (C) 2024-2025 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer], Dominik Smith, Alex Bercuci */

#include "Unpack.h"

#include "AlgoFairloggerCompat.h"

using namespace cbm::algo::trd2d;
using fles::Subsystem;

Unpack::Unpack(const Config& config) : fConfig(config)
{
  /** Register the algorithm versions available for TRD2D. For the moment (25.01.14), there is 
 * no distinction between ALGO and MESSAGE version. The following mapping is 
 * assumed:
 * eMessageVersion::kMessLegacy - refers to the version WITH digi buffering
 * eMessageVersion::kMess24     - refers to the version WITHOUT digi buffering
 */
  constexpr std::array<u8, 2> AlgoVersion = {(u8) eMessageVersion::kMessLegacy, (u8) eMessageVersion::kMess24};

  // Create one algorithm per component for TRD and configure it with parameters
  ReadoutSetup setup = fConfig.roSetup;
  ReadoutCalib calib = fConfig.roCalib;
  auto equipIdsTrd2d = setup.GetEquipmentIds();
  for (auto& equip : equipIdsTrd2d) {
    trd2d::UnpackPar par{};
    auto comppars         = setup.CompMap(equip);
    par.fSystemTimeOffset = setup.GetSystemTimeOffset();
    par.fModId            = comppars.moduleId;
    par.fEqAdd            = equip;
    par.fEqId             = comppars.fiberId;
    par.fRefSignal        = calib.GetSystemCalibSignal();

    const size_t numAsics = setup.GetNumAsics(equip);
    auto asics            = setup.GetAsicList(equip);
    for (auto asic : asics) {
      trd2d::UnpackAsicPar asicPar;
      const size_t numChans = setup.GetNumChans(equip, asic);

      for (size_t chan = 0; chan < numChans; chan++) {
        trd2d::UnpackChannelPar chanPar;
        auto pars            = setup.ChanMap(equip, asic, chan);
        chanPar.fPadAddress  = pars.padAddress;  // Pad address for channel
        uint16_t ch          = std::abs(pars.padAddress);
        auto calCh           = calib.GetChannelFeeCalib(par.fModId, ch);
        par.fCalibParams[ch] = {calCh.maskFlag,
                                calCh.tOffset,
                                calCh.baseline,
                                calCh.gainfee,
                                {calCh.noise.tDelay, calCh.noise.tWindow, calCh.noise.lDThres, calCh.noise.lSThres}};
        chanPar.fMask        = pars.maskFlag;    // Flag channel mask
        chanPar.fDaqOffset   = pars.tOffset;     // Time calibration parameter
        par.toff[ch / 2]     = pars.tOffset;
        chanPar.fSignalThres = pars.lThreshold;  // Threshold cut
        asicPar.fChanParams.push_back(chanPar);
      }
      L_(debug) << "--- Configured asic " << (int) asic << " with " << numChans << " channels";
      par.fAsicParams[asic] = asicPar;
    }
    L_(debug) << "--- Configured equipment 0x" << std::hex << (int) equip << " with " << std::dec << numAsics
              << " asics";

    // build all algorithms defined for data unpacking ! Why ?? (AB 25.01.15)
    std::unique_ptr<UnpackMSBase<CbmTrdDigi, UnpackMonitorData, UnpackAuxData>> algo;
    for (auto ver : AlgoVersion) {
      switch (ver) {
        case (int) eMessageVersion::kMessLegacy:
          algo = std::make_unique<UnpackMS<(u8) eMessageVersion::kMessLegacy>>(std::move(par));
          break;
        case (int) eMessageVersion::kMess24:
          algo = std::make_unique<UnpackMS<(u8) eMessageVersion::kMess24>>(std::move(par));
          break;
      }
      // register algorithm
      L_(debug) << "Register algo for ver=" << (int) ver << " eqId=0x" << std::hex << (int) equip;
      fAlgos[{equip, ver}] = std::move(algo);
    }
  }
  L_(info) << "--- Configured " << fAlgos.size() << " unpacker algorithms for TRD2D.";
  //   for (const auto& [key, algo] : fAlgos) {
  //     L_(info) << "eq=0x" << std::hex << key.eqId << " ver=" << int(key.sysVer);
  //     if (key.sysVer ==2 ) continue;
  //     (dynamic_cast<const UnpackMS<3>&>(*algo)).DumpParameters();
  //   }
}

Unpack::Result_t Unpack::operator()(const fles::Timeslice& ts) const { return DoUnpack(Subsystem::TRD2D, ts); }
