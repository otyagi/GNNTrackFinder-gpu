/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Pierre-Alain Loizeau, Pascal Raisig  */
/** @file CbmRecoUnpack.cxx
 ** @copyright Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
 ** @license SPDX-License-Identifier: GPL-3.0-only
 ** @author Volker Friese [originator]
 **/


#include "CbmRecoUnpack.h"

#include "CbmRecoUnpackConfig.tmpl"
#include "CbmTimeSlice.h"
#include "CbmTrdDigi.h"
#include "CbmTsEventHeader.h"

#include <Monitor.hpp>
#include <System.hpp>

#include <FairRootManager.h>
#include <Logger.h>

#include <RtypesCore.h>
#include <TH1.h>
#include <TStopwatch.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>


using fles::Subsystem;
using fles::Timeslice;
using std::unique_ptr;


// -----   Constructor   ------------------------------------------------------
CbmRecoUnpack::CbmRecoUnpack() {}
// ----------------------------------------------------------------------------

// -----   Destructor   ------------------------------------------------------
CbmRecoUnpack::~CbmRecoUnpack()
{
  /// Need to stay in the cxx file due to destruction of Monitor unique pointer
  LOG(debug) << "CbmRecoUnpack::~CbmRecoUnpack!";
};
// ----------------------------------------------------------------------------


// -----   Finish   -----------------------------------------------------------
void CbmRecoUnpack::Finish()
{
  LOG(info) << "CbmRecoUnpack::Finish() I do let the unpackers talk first :\n";

  if (fPsdConfig) fPsdConfig->GetUnpacker()->Finish();
  if (fMuchConfig) fMuchConfig->GetUnpacker()->Finish();
  if (fRichConfig) fRichConfig->GetUnpacker()->Finish();
  if (fStsConfig) fStsConfig->GetUnpacker()->Finish();
  if (fTofConfig) fTofConfig->GetUnpacker()->Finish();
  if (fTrd1DConfig) fTrd1DConfig->GetUnpacker()->Finish();
  if (fTrd2DConfig) fTrd2DConfig->GetUnpacker()->Finish();
  if (fBmonConfig) fBmonConfig->GetUnpacker()->Finish();

  // Create some default performance profiling histograms and write them to a file
  if (fDoPerfProf) performanceProfiling();
  if (fPublishProfMoni) {
    double dTotalDataSizeIn  = 0.0;
    double dTotalDataSizeOut = 0.0;
    for (auto datait : fDataSizeMapCurrSec) {
      dTotalDataSizeIn += datait.second.first;
      dTotalDataSizeOut += datait.second.second;
    }
    for (auto datait : fDataSizeMapCurrSec) {
      double dUnpRatio = 0 < datait.second.first ? datait.second.second / datait.second.first : 0.0;
      double dShareIn  = 0 < dTotalDataSizeIn ? datait.second.first / dTotalDataSizeIn : 0.0;
      double dShareOut = 0 < dTotalDataSizeOut ? datait.second.second / dTotalDataSizeOut : 0.0;
      fMonitor->QueueMetric(
        "unpack_perf",
        {{"host", fMoniCurrrentHostname}, {"jobid", fMoniJobId}, {"det", fNameMapPerTs[datait.first].first}},
        {{"dataIn", datait.second.first},
         {"dataOut", datait.second.second},
         {"unpRatio", dUnpRatio},
         {"shareIn", dShareIn},
         {"shareOut", dShareOut}},
        fPubMoniProcTime ? std::chrono::system_clock::time_point() : fMonitorSecCurrentTs);
    }
  }
}

// ----------------------------------------------------------------------------

// -----   Initialisation   ---------------------------------------------------
Bool_t CbmRecoUnpack::Init()
{

  FairRootManager* ioman = FairRootManager::Instance();
  assert(ioman);

  auto eh = FairRun::Instance()->GetEventHeader();
  if (eh->IsA() == CbmTsEventHeader::Class())
    fCbmTsEventHeader = static_cast<CbmTsEventHeader*>(eh);
  else
    LOG(fatal)
      << "CbmRecoUnpack::Init() no CbmTsEventHeader was added to the run. Without it, we can not store the UTC of the "
         "Timeslices correctly. Hence, this causes a fatal. Please add it in the steering macro to the Run.";

  fTimeSlice = new CbmTimeSlice(0., 1.28e8 + 1.28e6);  // FIXME: hardcoded TS length + overlap of mCBM 2022 becnhmark
  ioman->Register("TimeSlice.", "DAQ", fTimeSlice, kTRUE);

  // --- Psd
  if (fPsdConfig) {
    fPsdConfig->InitOutput();
    RegisterOutputs(ioman, fPsdConfig);  /// Framework bound work = kept in this Task
    fPsdConfig->SetAlgo();
    initParContainers(fPsdConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fPsdConfig->InitAlgo();
    initPerformanceMaps(Subsystem::PSD, "PSD");
  }
  // --- Rich
  if (fRichConfig) {
    fRichConfig->InitOutput();
    RegisterOutputs(ioman, fRichConfig);  /// Framework bound work = kept in this Task
    fRichConfig->SetAlgo();
    initParContainers(fRichConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fRichConfig->InitAlgo();
    initPerformanceMaps(Subsystem::RICH, "RICH");
  }

  // --- Sts
  if (fStsConfig) {
    fStsConfig->InitOutput();
    RegisterOutputs(ioman, fStsConfig);  /// Framework bound work = kept in this Task
    fStsConfig->SetAlgo();
    initParContainers(fStsConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fStsConfig->InitAlgo();
    initPerformanceMaps(Subsystem::STS, "STS");
  }

  // --- Much
  if (fMuchConfig) {
    fMuchConfig->InitOutput();
    RegisterOutputs(ioman, fMuchConfig);  /// Framework bound work = kept in this Task
    fMuchConfig->SetAlgo();
    initParContainers(fMuchConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fMuchConfig->InitAlgo();
    initPerformanceMaps(Subsystem::MUCH, "MUCH");
  }


  // --- Tof
  if (fTofConfig) {
    fTofConfig->InitOutput();
    RegisterOutputs(ioman, fTofConfig);  /// Framework bound work = kept in this Task
    fTofConfig->SetAlgo();
    fTofConfig->LoadParFileName();  /// Needed to change the Parameter file name before it is used!!!
    initParContainers(fTofConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fTofConfig->InitAlgo();
    initPerformanceMaps(Subsystem::TOF, "TOF");
  }
  // --- Trd
  if (fTrd1DConfig) {
    fTrd1DConfig->InitOutput();
    RegisterOutputs(ioman, fTrd1DConfig);  /// Framework bound work = kept in this Task
    fTrd1DConfig->SetAlgo();
    initParContainers(fTrd1DConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fTrd1DConfig->InitAlgo();
    initPerformanceMaps(Subsystem::TRD, "TRD1D");
  }
  // --- TRD2D
  if (fTrd2DConfig) {
    if (fTrd1DConfig && (fTrd2DConfig->GetOutputBranchName() == fTrd1DConfig->GetOutputBranchName())) {
      LOG(info) << fTrd2DConfig->GetName() << "::Init() ---------------------------------";
      fTrd2DConfig->SetOutputVec(fTrd1DConfig->GetOutputVec());
    }
    else {
      fTrd2DConfig->InitOutput();
      RegisterOutputs(ioman, fTrd2DConfig);  /// Framework bound work = kept in this Task
    }
    fTrd2DConfig->SetAlgo();
    initParContainers(fTrd2DConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fTrd2DConfig->InitAlgo();
    initPerformanceMaps(Subsystem::TRD2D, "TRD2D");
  }
  // This is an ugly work around, because the TRD and TRD2D want to access the same vector and there is no
  // function to retrieve a writeable vector<obj> from the FairRootManager, especially before the branches
  // are created, as far as I am aware.
  // The second option workaround is in in Init() to look for the fasp config and create a separate branch
  // for fasp created CbmTrdDigis PR 072021

  // --- Bmon
  if (fBmonConfig) {
    fBmonConfig->InitOutput();
    RegisterOutputs(ioman, fBmonConfig);  /// Framework bound work = kept in this Task
    fBmonConfig->SetAlgo();
    fBmonConfig->LoadParFileName();  /// Needed to change the Parameter file name before it is used!!!
    initParContainers(fBmonConfig->GetParContainerRequest());  /// Framework bound work = kept in this Task
    fBmonConfig->InitAlgo();
    initPerformanceMaps(Subsystem::BMON, "Bmon");
  }

  if (fDoPerfProfPerTs) {
    /// Timer for complete processing and histograming perTS
    fTimerTs = new TStopwatch();

    fhCpuTimePerTs  = new TH1D("hCpuTimePerTs", "CPU Processing time of TS vs TS; Ts; CPU time [ms]", 6000, 0, 6000);
    fhRealTimePerTs = new TH1D("hRealTimePerTs", "Real Processing time of TS vs TS; Ts; Real time [ms]", 6000, 0, 6000);

    fhCpuTimePerTsHist =
      new TH1D("hCpuTimePerTsHist", "CPU Histo filling time of TS vs TS; Ts; CPU time [ms]", 6000, 0, 6000);
    fhRealTimePerTsHist =
      new TH1D("hRealTimePerTsHist", "Real Histo filling time of TS vs TS; Ts; Real time [ms]", 6000, 0, 6000);

    fhUnpackingRatioPerTs =
      new TH1D("hUnpackingRatioPerTs", "ratio of tot. unp. digi size to tot. input raw size vs TS; TS; Size Ratio []",
               6000, 0, 6000);
  }

  if (fPublishProfMoni) {  //
    fMonitor              = std::make_unique<cbm::Monitor>(fUriPublishProfMoni);
    fMoniCurrrentHostname = fles::system::current_hostname();
    LOG(info) << "Unpack: Publishing monitoring metrics to time-series DB at " << fUriPublishProfMoni << " as "
              << fMoniCurrrentHostname << " with job ID " << fMoniJobId;
  }

  return kTRUE;
}
// ----------------------------------------------------------------------------


// -----   initPerformanceMaps   ----------------------------------------------
void CbmRecoUnpack::initPerformanceMaps(Subsystem subsystem, std::string name)
{
  if (fDoPerfProf) {
    fNameMap.emplace(std::make_pair(subsystem, std::make_pair(name, 0)));
    fTimeMap.emplace(std::make_pair(subsystem, std::make_pair(0, 0)));
    fDataSizeMap.emplace(std::make_pair(subsystem, std::make_pair(0, 0)));
  }
  if (fDoPerfProfPerTs) {
    fNameMapPerTs.emplace(std::make_pair(subsystem, std::make_pair(name, 0)));
    fTimeMapPerTs.emplace(std::make_pair(subsystem, std::make_pair(0, 0)));
    fDataSizeMapPerTs.emplace(std::make_pair(subsystem, std::make_pair(0, 0)));
    if (fPublishProfMoni) {  //
      fDataSizeMapCurrSec.emplace(std::make_pair(subsystem, std::make_pair(0, 0)));
    }

    fvhInpRatioPerTs.emplace(std::make_pair(
      subsystem,
      new TH1D(Form("hInpRatioPerTs%s", name.c_str()),
               Form("ratio of input data size in total input data size vs TS for %s; TS; Size Ratio []", name.c_str()),
               6000, 0, 6000)));
    fvhOutRatioPerTs.emplace(std::make_pair(
      subsystem,
      new TH1D(Form("hOutRatioPerTs%s", name.c_str()),
               Form("ratio of unpacked digi size in total output size vs TS for %s; TS; Size Ratio []", name.c_str()),
               6000, 0, 6000)));
    fvhUnpRatioPerTs.emplace(std::make_pair(
      subsystem,
      new TH1D(Form("hUnpRatioPerTs%s", name.c_str()),
               Form("ratio of unpacked digi size to raw data size vs TS for %s; TS; O/I Size Ratio []", name.c_str()),
               6000, 0, 6000)));
  }
}
// ----------------------------------------------------------------------------


// -----   performanceProfiling   ---------------------------------------------
void CbmRecoUnpack::performanceProfiling()
{
  std::unique_ptr<TH1D> hProducedDigis =
    std::unique_ptr<TH1D>(new TH1D("ProducedDigis", "ProducedDigis", fNameMap.size(), -0.5, fNameMap.size() - 0.5));
  hProducedDigis->SetXTitle("Subsystem");
  hProducedDigis->SetYTitle("N-Digis");
  std::unique_ptr<TH1D> hSpeedPerf = std::unique_ptr<TH1D>(
    new TH1D("SpeedPerformance", "SpeedPerformance", fNameMap.size() * 2, -0.5, fNameMap.size() * 2 - 0.5));
  hSpeedPerf->SetXTitle("Subsystem");
  hSpeedPerf->SetYTitle("Unpacking performance [#mus/Digi]");
  std::unique_ptr<TH1D> hDataPerf = std::unique_ptr<TH1D>(
    new TH1D("DataPerformance", "DataPerformance", fNameMap.size() * 2, -0.5, fNameMap.size() * 2 - 0.5));
  hDataPerf->SetXTitle("Subsystem");
  hDataPerf->SetYTitle("Data [MB]");
  size_t iunpackerbin = 1;
  for (auto namepair : fNameMap) {

    // Speed performance
    auto timeit = fTimeMap.find(namepair.first);
    double cpu  = 0 < namepair.second.second ? timeit->second.first / namepair.second.second : 0.0;
    double wall = 0 < namepair.second.second ? timeit->second.second / namepair.second.second : 0.0;

    // Data performance
    auto datait    = fDataSizeMap.find(namepair.first);
    double indata  = datait->second.first;
    double outdata = datait->second.second;


    // N-Digis
    std::string label = namepair.second.first;
    hProducedDigis->GetXaxis()->SetBinLabel(iunpackerbin, label.data());
    hProducedDigis->SetBinContent(iunpackerbin, namepair.second.second);

    // Cpu time
    label = namepair.second.first;
    label += " cpu";
    hSpeedPerf->GetXaxis()->SetBinLabel(iunpackerbin * 2 - 1, label.data());
    hSpeedPerf->SetBinContent(iunpackerbin * 2 - 1, cpu);
    // Wall time
    label = namepair.second.first;
    label += " wall";
    hSpeedPerf->GetXaxis()->SetBinLabel(iunpackerbin * 2, label.data());
    hSpeedPerf->SetBinContent(iunpackerbin * 2, wall);

    // In data
    label = namepair.second.first;
    label += " in";
    hDataPerf->GetXaxis()->SetBinLabel(iunpackerbin * 2 - 1, label.data());
    hDataPerf->SetBinContent(iunpackerbin * 2 - 1, indata);

    // Out data
    label = namepair.second.first;
    label += " out";
    hDataPerf->GetXaxis()->SetBinLabel(iunpackerbin * 2, label.data());
    hDataPerf->SetBinContent(iunpackerbin * 2, outdata);

    ++iunpackerbin;
  }

  /// Save old global file and folder pointer to avoid messing with FairRoot
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  /// (Re-)Create ROOT file to store the histos
  TFile histofile(fOutfilename.data(), "RECREATE");

  histofile.cd();
  hProducedDigis->Write();
  hSpeedPerf->Write();
  hDataPerf->Write();

  if (fDoPerfProfPerTs) {
    fhCpuTimePerTs->Write();
    fhRealTimePerTs->Write();
    fhCpuTimePerTsHist->Write();
    fhRealTimePerTsHist->Write();
    for (auto detHist : fvhInpRatioPerTs) {
      fvhInpRatioPerTs[detHist.first]->Write();
      fvhOutRatioPerTs[detHist.first]->Write();
      fvhUnpRatioPerTs[detHist.first]->Write();
    }
    fhUnpackingRatioPerTs->Write();
  }

  /// Restore old global file and folder pointer to avoid messing with FairRoot
  gFile      = oldFile;
  gDirectory = oldDir;

  // histofile->Close();
  histofile.Close();
}
void CbmRecoUnpack::performanceProfilingPerTs()
{
  /// Speed performance
  double dTotalCpuTime  = fTimerTs->CpuTime() * 1000.;
  double dTotalRealTime = fTimerTs->RealTime() * 1000.;
  fTimerTs->Start();

  /*
  for (auto timeit : fTimeMapPerTs) {
    // Speed performance
    dTotalCpuTime     += timeit->second.first
    dTotalRealTime    += timeit->second.second
  }
  */

  double dTotalDataSizeIn  = 0.0;
  double dTotalDataSizeOut = 0.0;

  using sctp   = std::chrono::system_clock::time_point;
  sctp tsStart = sctp(std::chrono::seconds(static_cast<uint64_t>(fCbmTsEventHeader->GetTsStartTime() * 1e-9)));
  if (fMonitorSecCurrentTs == sctp()) {  //
    fMonitorSecCurrentTs = tsStart;
  }
  else if (fPubMoniProcTime || fMonitorSecCurrentTs < tsStart) {
    for (auto datait : fDataSizeMapCurrSec) {
      dTotalDataSizeIn += datait.second.first;
      dTotalDataSizeOut += datait.second.second;
    }
    for (auto datait : fDataSizeMapCurrSec) {
      double dUnpRatio = 0 < datait.second.first ? datait.second.second / datait.second.first : 0.0;
      double dShareIn  = 0 < dTotalDataSizeIn ? datait.second.first / dTotalDataSizeIn : 0.0;
      double dShareOut = 0 < dTotalDataSizeOut ? datait.second.second / dTotalDataSizeOut : 0.0;
      fMonitor->QueueMetric(
        "unpack_perf",
        {{"host", fMoniCurrrentHostname}, {"jobid", fMoniJobId}, {"det", fNameMapPerTs[datait.first].first}},
        {{"dataIn", datait.second.first},
         {"dataOut", datait.second.second},
         {"unpRatio", dUnpRatio},
         {"shareIn", dShareIn},
         {"shareOut", dShareOut}},
        fPubMoniProcTime ? sctp() : fMonitorSecCurrentTs);
    }
    fMonitorSecCurrentTs = tsStart;
    for (auto datait = fDataSizeMapCurrSec.begin(); datait != fDataSizeMapCurrSec.end(); ++datait) {
      datait->second.first  = 0.0;
      datait->second.second = 0.0;
    }
    dTotalDataSizeIn  = 0.0;
    dTotalDataSizeOut = 0.0;
  }

  /// Data performance
  for (auto datait : fDataSizeMapPerTs) {
    dTotalDataSizeIn += datait.second.first;
    dTotalDataSizeOut += datait.second.second;

    if (fPublishProfMoni) {
      fDataSizeMapCurrSec[datait.first].first += datait.second.first;
      fDataSizeMapCurrSec[datait.first].second += datait.second.second;
    }
  }
  for (auto datait : fDataSizeMapPerTs) {
    fvhInpRatioPerTs[datait.first]->Fill(fCbmTsEventHeader->GetTsIndex(), datait.second.first / dTotalDataSizeIn);
    fvhOutRatioPerTs[datait.first]->Fill(fCbmTsEventHeader->GetTsIndex(), datait.second.second / dTotalDataSizeOut);
    if (datait.second.first) {  //
      fvhUnpRatioPerTs[datait.first]->Fill(fCbmTsEventHeader->GetTsIndex(), datait.second.second / datait.second.first);
    }
  }
  fhUnpackingRatioPerTs->Fill(fCbmTsEventHeader->GetTsIndex(), dTotalDataSizeOut / dTotalDataSizeIn);
  fhCpuTimePerTs->Fill(fCbmTsEventHeader->GetTsIndex(), dTotalCpuTime);
  fhRealTimePerTs->Fill(fCbmTsEventHeader->GetTsIndex(), dTotalRealTime);

  for (auto timeit = fTimeMapPerTs.begin(); timeit != fTimeMapPerTs.end(); ++timeit) {
    // Speed performance
    timeit->second.first  = 0.0;
    timeit->second.second = 0.0;
  }
  for (auto datait = fDataSizeMapPerTs.begin(); datait != fDataSizeMapPerTs.end(); ++datait) {
    datait->second.first  = 0.0;
    datait->second.second = 0.0;
  }

  fTimerTs->Stop();
  fhCpuTimePerTsHist->Fill(fCbmTsEventHeader->GetTsIndex(), fTimerTs->CpuTime() * 1000.);
  fhRealTimePerTsHist->Fill(fCbmTsEventHeader->GetTsIndex(), fTimerTs->RealTime() * 1000.);
}
// ----------------------------------------------------------------------------

// -----   Reset   ------------------------------------------------------------
void CbmRecoUnpack::Reset()
{
  // Reset the event header for a new timeslice
  fCbmTsEventHeader->Reset();
  fTimeSlice->Reset(0., 1.28e8 + 1.28e6);  // FIXME: hardcoded TS length + overlap of mCBM 2022 becnhmark

  // Reset the unpackers for a new timeslice, e.g. clear the output vectors

  // ----Much ----
  if (fMuchConfig) fMuchConfig->Reset();
  // ---- Psd ----
  if (fPsdConfig) fPsdConfig->Reset();
  // ---- Rich ----
  if (fRichConfig) fRichConfig->Reset();
  // ---- Sts ----
  if (fStsConfig) fStsConfig->Reset();
  // ---- Tof ----
  if (fTofConfig) fTofConfig->Reset();
  // ---- Trd ----
  if (fTrd1DConfig) fTrd1DConfig->Reset();
  // ---- Trd2D ----
  if (fTrd2DConfig) fTrd2DConfig->Reset();
  // ---- Bmon ----
  if (fBmonConfig) fBmonConfig->Reset();
}

// ----------------------------------------------------------------------------

// -----   Unpacking   --------------------------------------------------------
void CbmRecoUnpack::Unpack(unique_ptr<Timeslice> ts)
{
  if (fDoPerfProfPerTs) {
    /// Start time for complete processing perTS
    fTimerTs->Start();
  }

  // Prepare timeslice
  const fles::Timeslice& timeslice = *ts;

  fCbmTsEventHeader->SetTsIndex(ts->index());
  fCbmTsEventHeader->SetTsStartTime(ts->start_time());

  fTimeSlice->SetStartTime(ts->start_time());

  uint64_t nComponents = ts->num_components();
  if (fDoDebugPrints || 0 == ts->index() % 100) {
    LOG(info) << "Unpack: TS index " << ts->index() << " components " << nComponents;
  }

  for (uint64_t component = 0; component < nComponents; component++) {

    auto subsystem = static_cast<Subsystem>(ts->descriptor(component, 0).sys_id);

    switch (subsystem) {
      case Subsystem::MUCH: {
        if (fMuchConfig) {
          fCbmTsEventHeader->AddNDigisMuch(unpack(subsystem, &timeslice, component, fMuchConfig,
                                                  fMuchConfig->GetOptOutAVec(), fMuchConfig->GetOptOutBVec()));
        }
        break;
      }

      case Subsystem::PSD: {
        if (fPsdConfig) {
          fCbmTsEventHeader->AddNDigisPsd(unpack(subsystem, &timeslice, component, fPsdConfig,
                                                 fPsdConfig->GetOptOutAVec(), fPsdConfig->GetOptOutBVec()));
        }
        break;
      }
      case Subsystem::RICH: {
        if (fRichConfig) {
          fCbmTsEventHeader->AddNDigisRich(unpack(subsystem, &timeslice, component, fRichConfig,
                                                  fRichConfig->GetOptOutAVec(), fRichConfig->GetOptOutBVec()));
        }
        break;
      }
      case Subsystem::STS: {
        if (fStsConfig) {
          fCbmTsEventHeader->AddNDigisSts(unpack(subsystem, &timeslice, component, fStsConfig,
                                                 fStsConfig->GetOptOutAVec(), fStsConfig->GetOptOutBVec()));
        }
        break;
      }
      case Subsystem::TOF: {
        if (fTofConfig) {
          fCbmTsEventHeader->AddNDigisTof(unpack(subsystem, &timeslice, component, fTofConfig,
                                                 fTofConfig->GetOptOutAVec(), fTofConfig->GetOptOutBVec()));
        }
        break;
      }
      case Subsystem::TRD: {
        if (fTrd1DConfig) {
          fCbmTsEventHeader->AddNDigisTrd1D(unpack(subsystem, &timeslice, component, fTrd1DConfig,
                                                   fTrd1DConfig->GetOptOutAVec(), fTrd1DConfig->GetOptOutBVec()));
        }
        break;
      }
      case Subsystem::TRD2D: {
        if (fTrd2DConfig) {
          fCbmTsEventHeader->AddNDigisTrd2D(unpack(subsystem, &timeslice, component, fTrd2DConfig,
                                                   fTrd2DConfig->GetOptOutAVec(), fTrd2DConfig->GetOptOutBVec()));
        }
        break;
      }
      case Subsystem::BMON: {
        if (fBmonConfig) {
          fCbmTsEventHeader->AddNDigisBmon(unpack(subsystem, &timeslice, component, fBmonConfig,
                                                  fBmonConfig->GetOptOutAVec(), fBmonConfig->GetOptOutBVec()));
        }
        break;
      }
      default: {
        if (fDoDebugPrints)
          LOG(error) << "Unpack: Unknown subsystem " << fles::to_string(subsystem) << " for component " << component;
        break;
      }
    }
  }

  if (!bMonitoringOnly && bOutputFullTimeSorting) {
    /// Time sort the output vectors of all unpackers present
    if (fMuchConfig && fMuchConfig->GetOutputVec()) {
      timesort(fMuchConfig->GetOutputVec());
    }
    if (fPsdConfig && fPsdConfig->GetOutputVec()) {
      timesort(fPsdConfig->GetOutputVec());
    }
    if (fRichConfig && fRichConfig->GetOutputVec()) {
      timesort(fRichConfig->GetOutputVec());
    }
    if (fStsConfig && fStsConfig->GetOutputVec()) {
      timesort(fStsConfig->GetOutputVec());
    }
    if (fTofConfig && fTofConfig->GetOutputVec()) {
      timesort(fTofConfig->GetOutputVec());
    }
    if (fTrd1DConfig && fTrd1DConfig->GetOutputVec()) {
      timesort(fTrd1DConfig->GetOutputVec());
    }
    if (fTrd2DConfig && fTrd2DConfig->GetOutputVec()) {
      timesort(fTrd2DConfig->GetOutputVec());
    }
    if (fBmonConfig && fBmonConfig->GetOutputVec()) {
      timesort(fBmonConfig->GetOutputVec());
    }

    /// Time sort the output vectors of all unpackers present
    if (fMuchConfig && fMuchConfig->GetOptOutAVec()) {
      timesort(fMuchConfig->GetOptOutAVec());
    }
    if (fPsdConfig && fPsdConfig->GetOptOutAVec()) {
      timesort(fPsdConfig->GetOptOutAVec());
    }
    if (fRichConfig && fRichConfig->GetOptOutAVec()) {
      timesort(fRichConfig->GetOptOutAVec());
    }
    if (fStsConfig && fStsConfig->GetOptOutAVec()) {
      timesort(fStsConfig->GetOptOutAVec());
    }
    if (fTofConfig && fTofConfig->GetOptOutAVec()) {
      timesort(fTofConfig->GetOptOutAVec());
    }
    if (fTrd1DConfig && fTrd1DConfig->GetOptOutAVec()) {
      timesort(fTrd1DConfig->GetOptOutAVec());
    }
    if (fTrd2DConfig && fTrd2DConfig->GetOptOutAVec()) {
      timesort(fTrd2DConfig->GetOptOutAVec());
    }
    if (fBmonConfig && fBmonConfig->GetOptOutAVec()) {
      timesort(fBmonConfig->GetOptOutAVec());
    }
  }

  if (fBmonConfig && fBmonConfig->GetMonitor()) {  //
    fBmonConfig->GetMonitor()->FinalizeTsBmonMicroSpillHistos();
    fBmonConfig->GetMonitor()->FinalizeTsBmonQfactorHistos(ts->start_time(), fBmonConfig->GetOutputVec());
  }

  if (fDoPerfProfPerTs) {
    fTimerTs->Stop();
    performanceProfilingPerTs();
  }
}
// ----------------------------------------------------------------------------


ClassImp(CbmRecoUnpack)
