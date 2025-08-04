/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer]  */

/** @file run_unpack_tsa.C
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since May 2021
 **/


// --- Includes needed for IDE
#include <RtypesCore.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#if !defined(__CLING__)
#include <FairLogger.h>
#include <FairRootFileSink.h>
#include <FairRunOnline.h>
#include <Logger.h>

#include <TStopwatch.h>
#include <TSystem.h>
#endif

std::shared_ptr<CbmTofUnpackMonitor> GetTofMonitor(std::string treefilename, bool bBmonMode = false);
std::string defaultSetupName = "mcbm_beam_2021_07_surveyed";

void monitor_bmon_microspills(std::vector<std::string> inputurl, UInt_t runid = 0, std::int32_t nevents = -1,
                              Int_t serverHttpPort = 8090, Int_t serverRefreshRate = 1,
                              std::string setupName = defaultSetupName, std::string outpath = "data/")
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  TString myName = "monitor_bmon_microspills";     // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------

  // -----   Output filename   ----------------------------------------------
  std::string filename    = Form("online_%05d.digi.root", runid);
  std::string outfilename = outpath + filename;
  if (inputurl[0].npos == inputurl[0].find("tcp://")) {
    outfilename      = inputurl[0];
    auto filenamepos = inputurl[0].find_last_of("/");
    if (inputurl[0].npos != filenamepos) {
      filenamepos++;
    }
    else {
      filenamepos = 0;
    }
    filename = inputurl[0].substr(filenamepos);
    if (filename.find("*") != inputurl[0].npos) filename = std::to_string(runid) + ".tsa";
    if (filename.find(";") != inputurl[0].npos) filename = std::to_string(runid) + "_merged" + ".tsa";
    filename.replace(filename.find(".tsa"), 4, ".digi.root");
    if (outpath.empty()) {
      auto multi_delim = inputurl[0].find_last_of(";");
      if (inputurl[0].npos != multi_delim) {
        multi_delim++;
      }
      else {
        multi_delim = 0;
      }
      outpath = inputurl[0].substr(multi_delim, filenamepos - multi_delim);
    }
    outfilename = outpath + filename;
  }
  std::cout << "-I- " << myName << ": Output file will be " << outfilename << std::endl;
  // ------------------------------------------------------------------------


  // -----   Performance profiling   ----------------------------------------
  // Set to true if you want some minimal performance profiling output
  bool doPerfProfiling = true;
  // Define if you want a special path and name for the performance profiling output file
  std::string perfProfFileName = outpath + filename;
  perfProfFileName.replace(perfProfFileName.find(".digi.root"), 10, ".perf.root");
  // ------------------------------------------------------------------------


  // -----   CbmSetup   -----------------------------------------------------
  /// Do automatic mapping only if not overridden by user or empty
  if (defaultSetupName == setupName || "" == setupName) {
    cbm::mcbm::ToForceLibLoad dummy;  /// Needed to trigger loading of the library as no fct dict in ROOT6 and CLING
    try {
      setupName = cbm::mcbm::GetSetupFromRunId(runid);
    }
    catch (const std::invalid_argument& e) {
      std::cout << "Error in mapping from runID to setup name: " << e.what() << std::endl;
      return;
    }
    if (defaultSetupName != setupName) {
      std::cout << "Automatic setup choice for run " << runid << ": " << setupName << std::endl;
    }
  }
  auto cbmsetup = CbmSetup::Instance();
  cbmsetup->LoadSetup(setupName.c_str());
  // ------------------------------------------------------------------------

  // -----   UnpackerConfigs   ----------------------------------------------

  // ---- BMON ----
  std::shared_ptr<CbmBmonUnpackConfig> bmonconfig = nullptr;

  bmonconfig = std::make_shared<CbmBmonUnpackConfig>("", runid);
  if (bmonconfig) {
    // bmonconfig->SetDebugState();
    // bmonconfig->SetDoWriteOutput();
    // bmonconfig->SetDoWriteOptOutA("CbmBmonErrors");
    std::string parfilesbasepathBmon = Form("%s/macro/beamtime/mcbm2023/", srcDir.Data());
    bmonconfig->SetParFilesBasePath(parfilesbasepathBmon);
    bmonconfig->SetParFileName("mBmonCriPar.par");
    bmonconfig->SetSystemTimeOffset(-1220);  // [ns] value to be updated
    if (2160 <= runid) {
      bmonconfig->SetSystemTimeOffset(-80);  // [ns] value to be updated
    }
    if (2350 <= runid) {
      bmonconfig->SetSystemTimeOffset(0);  // [ns] value to be updated
    }
    /// Enable Monitor plots
    bmonconfig->SetMonitor(GetTofMonitor(outfilename, true));
    if (2337 <= runid) {
      bmonconfig->GetMonitor()->SetSpillThreshold(250);
      bmonconfig->GetMonitor()->SetSpillThresholdNonPulser(100);
    }
  }
  // -------------


  // ------------------------------------------------------------------------

  // In general, the following parts need not be touched
  // ========================================================================

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // -----   CbmSourceTsArchive   -------------------------------------------
  std::unique_ptr<CbmSourceTsArchive> source = std::unique_ptr<CbmSourceTsArchive>(new CbmSourceTsArchive(inputurl));
  //source->SetDebugPrintout();
  auto unpack = source->GetRecoUnpack();
  //unpack->SetDoPerfProfiling(doPerfProfiling);
  //unpack->SetOutputFilename(perfProfFileName);
  // Enable full time sorting instead sorting per FLIM link
  //unpack->SetTimeSorting(true);
  unpack->SetMonitoringOnly(true);

  if (bmonconfig) unpack->SetUnpackConfig(bmonconfig);
  // ------------------------------------------------------------------------

  // -----   FairRunAna   ---------------------------------------------------
  auto run  = new FairRunOnline(source.get());
  auto sink = new FairRootFileSink(outfilename.data());
  run->SetSink(sink);
  auto eventheader = new CbmTsEventHeader();
  run->SetRunId(runid);
  run->SetEventHeader(eventheader);
  // ------------------------------------------------------------------------


  // -----   HttpServer for online monitoring   -----------------------------
  run->ActivateHttpServer(serverRefreshRate, serverHttpPort);
  run->GetHttpServer()->GetSniffer()->SetScanGlobalDir(kFALSE);
  //run->SetAutoFinish(kFALSE);
  // ------------------------------------------------------------------------


  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());
  // ------------------------------------------------------------------------


  // -----   Run initialisation   -------------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Initialise run" << std::endl;
  run->Init();
  // ------------------------------------------------------------------------


  // -----   Start run   ----------------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Starting run" << std::endl;
  if (nevents < 0) {
    std::cout << "-I- " << myName << ": running until end of file" << std::endl;
    run->Run(-1, 0);
  }
  else {
    std::cout << "-I- " << myName << ": running for " << nevents << " timeslices " << std::endl;
    run->Run(0, nevents);
  }
  // ------------------------------------------------------------------------


  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "After CpuTime = " << timer.CpuTime() << " s RealTime = " << timer.RealTime() << " s." << std::endl;
  // ------------------------------------------------------------------------

  // --   Release all shared pointers to config before ROOT destroys things -
  // => We need to destroy things by hand because run->Finish calls (through the FairRootManager) Source->Close which
  //    does call the Source destructor, so due to share pointer things stay alive until out of macro scope...
  run->SetSource(nullptr);
  delete run;

  bmonconfig.reset();
  // ------------------------------------------------------------------------

}  // End of main macro function


/**
 * @brief Get the Tof Monitor. Extra function to keep default macro part more silent.
 * @return std::shared_ptr<CbmTofUnpackMonitor>
*/
std::shared_ptr<CbmTofUnpackMonitor> GetTofMonitor(std::string treefilename, bool bBmonMode = false)
{
  // -----   Output filename and path   -------------------------------------
  std::string outpath  = "";
  std::string filename = "";
  auto filenamepos     = treefilename.find_last_of("/");
  if (filenamepos != treefilename.npos) {
    outpath  = treefilename.substr(0, filenamepos);
    filename = treefilename.substr(filenamepos++);
  }
  if (outpath.empty()) outpath = gSystem->GetWorkingDirectory();
  //std::string mydir = "/qa";
  //outpath += mydir;

  auto currentdir = gSystem->GetWorkingDirectory();

  if (!gSystem->cd(outpath.data())) {
    gSystem->MakeDirectory(outpath.data());
  }
  else {
    gSystem->cd(currentdir.data());
  }

  std::string sSystemType = ".mon.tof.root";
  if (bBmonMode) {
    //
    sSystemType = ".mon.bmon.root";
  }

  std::string outfilename = outpath + filename;
  auto filetypepos        = outfilename.find(".digi.root");
  if (filetypepos != outfilename.npos) {
    outfilename.replace(filetypepos, 10, sSystemType);
  }
  else {
    outfilename += sSystemType;
  }
  // ------------------------------------------------------------------------

  auto monitor = std::make_shared<CbmTofUnpackMonitor>();
  monitor->SetHistoFileName(outfilename);
  monitor->SetBmonMicroSpillMode(bBmonMode);
  return monitor;
}

void monitor_bmon_microspills(std::string inputurl = "tcp://localhost:5556", UInt_t runid = 0,
                              std::int32_t nevents = -1, Int_t serverHttpPort = 8090, Int_t serverRefreshRate = 1,
                              std::string setupName = defaultSetupName, std::string outpath = "data/")
{
  std::vector<std::string> vInUrl = {inputurl};
  return monitor_bmon_microspills(vInUrl, runid, nevents, serverHttpPort, serverRefreshRate, setupName, outpath);
}
