/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */


#include "CbmTaskUnpack.h"

#include "CbmDefs.h"
#include "CbmDigiBranchBase.h"
#include "CbmDigiEvent.h"
#include "CbmDigiManager.h"
#include "CbmDigiTimeslice.h"
#include "CbmSourceTs.h"
#include "CbmTimeSlice.h"
#include "CbmTrdParFasp.h"
#include "CbmTrdParModAsic.h"
#include "CbmTrdParModDigi.h"
#include "CbmTrdParSetAsic.h"
#include "CbmTrdParSetDigi.h"
#include "CbmTrdParSpadic.h"
#include "CbmTsEventHeader.h"
#include "MicrosliceDescriptor.hpp"
#include "ParFiles.h"
#include "System.hpp"
#include "bmon/ReadoutConfig.h"
#include "sts/ChannelMaskSet.h"
#include "tof/ReadoutConfig.h"
#include "yaml/Yaml.h"

#include <FairParAsciiFileIo.h>
#include <FairParamList.h>
#include <FairRunOnline.h>
#include <Logger.h>

#include <TStopwatch.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <sstream>
#include <vector>

using namespace std;
using namespace cbm::algo;


// -----   Constructor   -----------------------------------------------------
CbmTaskUnpack::CbmTaskUnpack() : FairTask("Unpack")
{
  L_(fatal) << "Instantiated CbmTaskUnpack() without arguments (needs path to parameters and run ID).";
}

// -----   Constructor   -----------------------------------------------------
CbmTaskUnpack::CbmTaskUnpack(fs::path paramsDir, uint32_t runId) : FairTask("Unpack")
{
  ParFiles parFiles(runId);
  L_(info) << "Using parameter files for setup " << parFiles.setup;

  sts::ReadoutSetup stsSetup = yaml::ReadFromFile<sts::ReadoutSetup>(paramsDir / parFiles.sts.readout);
  auto chanMask              = yaml::ReadFromFile<sts::ChannelMaskSet>(paramsDir / parFiles.sts.chanMask);
  auto walkMap               = yaml::ReadFromFile<sts::WalkMap>(paramsDir / parFiles.sts.walkMap);
  sts::ReadoutConfig readout{stsSetup, chanMask};
  sts::Unpack::Config stsCfg{.readout = readout, .walkMap = walkMap, .bCollectAuxData = false};
  fStsUnpack = std::make_unique<sts::Unpack>(stsCfg);

  tof::ReadoutSetup tofSetup = yaml::ReadFromFile<tof::ReadoutSetup>(paramsDir / parFiles.tof.readout);
  tof::ReadoutConfig tofCfg{tofSetup};
  fTofUnpack = std::make_unique<tof::Unpack>(tofCfg);

  bmon::ReadoutSetup bmonSetup = yaml::ReadFromFile<bmon::ReadoutSetup>(paramsDir / parFiles.bmon.readout);
  bmon::ReadoutConfig bmonCfg{bmonSetup};
  fBmonUnpack = std::make_unique<bmon::Unpack>(bmonCfg);

  auto trdCfg = yaml::ReadFromFile<trd::ReadoutConfig>(paramsDir / parFiles.trd.readout);
  fTrdUnpack  = std::make_unique<trd::Unpack>(trdCfg);

  trd2d::ReadoutSetup setup = yaml::ReadFromFile<trd2d::ReadoutSetup>(paramsDir / parFiles.trd.readout2d);
  trd2d::ReadoutCalib calib = yaml::ReadFromFile<trd2d::ReadoutCalib>(paramsDir / parFiles.trd.fee2d);
  trd2d::Unpack::Config trd2dCfg{.roSetup = setup, .roCalib = calib};
  fTrd2dUnpack  = std::make_unique<trd2d::Unpack>(trd2dCfg);

  much::ReadoutConfig muchCfg{};
  fMuchUnpack = std::make_unique<much::Unpack>(muchCfg);

  rich::ReadoutConfig richCfg{};
  fRichUnpack = std::make_unique<rich::Unpack>(richCfg);
}
// ---------------------------------------------------------------------------


// -----   Destructor   ------------------------------------------------------
CbmTaskUnpack::~CbmTaskUnpack()
{
  if (fDigiTimeslice) delete fDigiTimeslice;

  if (fCbmrootFormatOutput) {
    // Clear output vectors
    fBmonDigis->clear();
    fStsDigis->clear();
    fMuchDigis->clear();
    fTrdDigis->clear();
    fTofDigis->clear();
    fRichDigis->clear();
  }
}
// ---------------------------------------------------------------------------


// -----   Execution   -------------------------------------------------------
void CbmTaskUnpack::Exec(Option_t*)
{
  // --- Get FLES timeslice
  assert(fSource);
  fles::Timeslice* timeslice = fSource->GetTimeslice();
  assert(timeslice);

  // --- Timer and counters
  TStopwatch timer;
  timer.Start();

  // --- Unpack the timeslice
  DigiData digis;
  Monitor monitor;

  digis.fBmon  = RunUnpacker(fBmonUnpack, *timeslice, monitor);
  digis.fMuch  = RunUnpacker(fMuchUnpack, *timeslice, monitor);
  digis.fRich  = RunUnpacker(fRichUnpack, *timeslice, monitor);
  digis.fSts   = RunUnpacker(fStsUnpack, *timeslice, monitor);
  digis.fTof   = RunUnpacker(fTofUnpack, *timeslice, monitor);
  digis.fTrd   = RunUnpacker(fTrdUnpack, *timeslice, monitor);
  digis.fTrd2d = RunUnpacker(fTrd2dUnpack, *timeslice, monitor);

  if (fCbmrootFormatOutput) {
    // Clear output vectors
    fBmonDigis->clear();
    fStsDigis->clear();
    fMuchDigis->clear();
    fTrdDigis->clear();
    fTofDigis->clear();
    fRichDigis->clear();

    fTsEventHeader->SetTsIndex(timeslice->index());
    fTsEventHeader->SetTsStartTime(timeslice->start_time());

    fTimeslice->SetStartTime(timeslice->start_time());

    std::move(digis.fBmon.begin(), digis.fBmon.end(), std::back_inserter(*fBmonDigis));
    std::move(digis.fSts.begin(), digis.fSts.end(), std::back_inserter(*fStsDigis));
    std::move(digis.fMuch.begin(), digis.fMuch.end(), std::back_inserter(*fMuchDigis));
    std::move(digis.fTrd2d.begin(), digis.fTrd2d.end(), std::back_inserter(*fTrdDigis));
    std::move(digis.fTrd.begin(), digis.fTrd.end(), std::back_inserter(*fTrdDigis));
    std::move(digis.fTof.begin(), digis.fTof.end(), std::back_inserter(*fTofDigis));
    std::move(digis.fRich.begin(), digis.fRich.end(), std::back_inserter(*fRichDigis));

    // Time-sort the TRD vector as we merged TRD1D and TRD2D (needed for compatibility with legacy unpackers)
    Timesort(fTrdDigis);
  }
  else {
    // --- Reset output branch (CbmDigiTimeslice)
    fDigiTimeslice->Clear();

    // Use lines below to combine TRD 1D and 2D
    //auto& digis1d = digis.fTrd;
    //auto& digis2d = digis.fTrd2d;
    //std::copy(digis2d.begin(), digis2d.end(), std::back_inserter(digis1d));

    fDigiTimeslice->fData = digis.ToStorable();
  }

  // --- Timeslice log
  timer.Stop();
  stringstream logOut;
  logOut << setw(15) << left << GetName() << " [";
  logOut << fixed << setw(8) << setprecision(1) << right << timer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNumTs << " (index " << timeslice->index() << ")";
  logOut << ", components " << monitor.numCompUsed << " / " << timeslice->num_components();
  logOut << ", microslices " << monitor.numMs;
  logOut << ", input rate " << double(monitor.numBytes) / timer.RealTime() / 1.e6 << " MB/s";
  logOut << ", digis " << monitor.numDigis;
  LOG(info) << logOut.str();

#if !defined(__CLING__) && !defined(__ROOTCLING__)
  if (fMonitor) {
    fMonitor->QueueMetric(GetName(), {{"host", fHostname}},
                          {{"realtime", timer.RealTime()},
                           {"cputime", timer.CpuTime()},
                           {"input_size", monitor.numBytes},
                           {"input_rate", double(monitor.numBytes) / timer.RealTime()},
                           {"digis", monitor.numDigis}});
  }
#endif

  // --- Run statistics
  fNumTs++;
  fNumMs += monitor.numMs;
  fNumBytes += monitor.numBytes;
  fNumDigis += monitor.numDigis;
  fTime += timer.RealTime();
}
// ----------------------------------------------------------------------------


// -----   End-of-run action   ------------------------------------------------
void CbmTaskUnpack::Finish()
{
  double timePerTs = 1000. * fTime / double(fNumTs);  // in ms
  double rate      = fNumBytes / 1.e6 / fTime;        // in MB/s
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Timeslices     : " << fNumTs;
  LOG(info) << "Microslices    : " << fNumMs;
  LOG(info) << "Digis          : " << fNumDigis;
  LOG(info) << "Av. input rate : " << fixed << setprecision(2) << rate << " MB/s";
  LOG(info) << "Time / TS      : " << fixed << setprecision(2) << timePerTs << " ms";
  LOG(info) << "=====================================";
}
// ----------------------------------------------------------------------------


// -----   Initialisation   ---------------------------------------------------
template<typename TVecobj>
Bool_t CbmTaskUnpack::RegisterVector(FairRootManager* ioman, std::vector<TVecobj>*& vec)
{
  if (ioman->GetObject(TVecobj::GetBranchName())) {
    LOG(fatal) << GetName() << ": Branch " << TVecobj::GetBranchName() << " already exists!";
    return kFALSE;
  }

  ioman->RegisterAny(TVecobj::GetBranchName(), vec, kTRUE);
  LOG(info) << GetName() << ": Registered branch " << TVecobj::GetBranchName() << " at " << vec;

  return kTRUE;
}

InitStatus CbmTaskUnpack::Init()
{
  LOG(info) << "==================================================";
  LOG(info) << GetName() << ": Initialising...";

  // --- Get hostname
  fHostname = fles::system::current_hostname();

  // --- Get source instance
  fSource = dynamic_cast<CbmSourceTs*>(FairRunOnline::Instance()->GetSource());
  if (fSource == nullptr) {
    LOG(error) << GetName() << ": No valid source class registered!";
    return kFATAL;
  }
  LOG(info) << "--- Found CbmSourceTs instance";

  // --- Get FairRootManager instance
  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  // --- Register output array (CbmDigiTimeslice)
  if (ioman->GetObject("DigiTimeslice")) {
    LOG(fatal) << GetName() << ": Branch DigiTimeslice already exists!";
    return kFATAL;
  }

  if (fCbmrootFormatOutput) {

    if (!(fTsEventHeader = dynamic_cast<CbmTsEventHeader*>(FairRun::Instance()->GetEventHeader()))) {
      LOG(fatal) << "CbmSourceDigiTimeslice::Init() no CbmTsEventHeader was added to the run. "
                    "Without it, we can not store the UTC of the Timeslices correctly. "
                    "Hence, this causes a fatal. Please add it to the Run in the steering macro.";
      return kFATAL;
    }

    // TimeSlice. branch initialization
    if (ioman->GetObject("TimeSlice.")) {
      LOG(fatal) << "Source: Branch TimeSlice. already exists!";
      return kFATAL;
    }
    else {
      // NOTE: the max time of timeslice is 1.28e8, taken from CbmRecoUnpack.cxx
      fTimeslice = new CbmTimeSlice(0., 1.28e8 + 1.28e6);
      ioman->Register("TimeSlice.", "DAQ", fTimeslice, kTRUE);
    }

    fBmonDigis = new std::vector<CbmBmonDigi>();
    if (kFALSE == RegisterVector<CbmBmonDigi>(ioman, fBmonDigis)) {
      return kFATAL;
    }


    fStsDigis = new std::vector<CbmStsDigi>();
    if (kFALSE == RegisterVector<CbmStsDigi>(ioman, fStsDigis)) {
      return kFATAL;
    }


    fMuchDigis = new std::vector<CbmMuchDigi>();
    if (kFALSE == RegisterVector<CbmMuchDigi>(ioman, fMuchDigis)) {
      return kFATAL;
    }


    fTrdDigis = new std::vector<CbmTrdDigi>();
    if (kFALSE == RegisterVector<CbmTrdDigi>(ioman, fTrdDigis)) {
      return kFATAL;
    }


    fTofDigis = new std::vector<CbmTofDigi>();
    if (kFALSE == RegisterVector<CbmTofDigi>(ioman, fTofDigis)) {
      return kFATAL;
    }


    fRichDigis = new std::vector<CbmRichDigi>();
    if (kFALSE == RegisterVector<CbmRichDigi>(ioman, fRichDigis)) {
      return kFATAL;
    }
  }
  else {
    fDigiTimeslice = new CbmDigiTimeslice();
    ioman->RegisterAny("DigiTimeslice.", fDigiTimeslice, IsOutputBranchPersistent("DigiTimeslice."));
    LOG(info) << "--- Registered branch DigiTimeslice.";
  }

  return kSUCCESS;
}
// ----------------------------------------------------------------------------


template<class Unpacker>
auto CbmTaskUnpack::RunUnpacker(const std::unique_ptr<Unpacker>& unpacker, const fles::Timeslice& timeslice,
                                Monitor& monitor) -> cbm::algo::algo_traits::Output_t<Unpacker>
{
  auto [digis, detmon, detaux] = (*unpacker)(timeslice);
  monitor.numCompUsed += detmon.numComponents;
  monitor.numMs += detmon.numMs;
  monitor.numBytes += detmon.sizeBytesIn;
  monitor.numDigis += digis.size();
  return digis;
}

ClassImp(CbmTaskUnpack)
