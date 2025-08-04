/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese, Florian Uhlig */

/** @file Run.cxx
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 27.10.2023
 **/

#include "Run.h"

#include "CbmDigitization.h"
#include "CbmSetup.h"

#include <FairEventHeader.h>
#include <FairMonitor.h>
#include <Logger.h>

#include <TFile.h>
#include <TGeoManager.h>
#include <TStopwatch.h>
#include <TString.h>
#include <TTree.h>

#include <cassert>
#include <iostream>

#include <sys/stat.h>

namespace cbm::sim::digitization
{


  // -----   Constructor   ----------------------------------------------------
  Run::Run() : TNamed("Run", "CBM AnalysisTree Converter Run") {}
  // --------------------------------------------------------------------------


  // -----   Destructor   -----------------------------------------------------
  Run::~Run() { LOG(debug) << "Destructing " << fName; }
  // --------------------------------------------------------------------------


  // -----   Check existence of a file   --------------------------------------
  bool Run::CheckFile(const char* fileName)
  {
    struct stat buffer;
    return (stat(fileName, &buffer) == 0);
  }
  // --------------------------------------------------------------------------


  // -----   Execute reconstruction run   -------------------------------------
  void Run::Exec()
  {
    // --- Mirror options and configuration
    LOG(info) << GetName() << ": Output file is         " << fOutput;
    if (CheckFile(fOutput.Data()) && !fOverwrite) throw std::runtime_error("Output file already exists");
    for (auto trafile : fTra) {
      LOG(info) << GetName() << ": Transport input source is      " << trafile;
      if (!CheckFile(trafile.Data())) throw std::runtime_error("Transport file does not exist");
    }
    LOG(info) << GetName() << ": Parameter file is      " << fPar;
    if (!CheckFile(fPar.Data())) throw std::runtime_error("Parameter file does not exist");
    LOG(info) << GetName() << ": Geometry setup is      " << fSetupTag;
    LOG(info) << "Configuration: \n" << fConfig.ToString() << "\n";

    // --- Timer
    TStopwatch timer;
    timer.Start();

    // --- Logger settings
    FairLogger::GetLogger()->SetLogScreenLevel(fConfig.f_glb_logLevel.data());
    FairLogger::GetLogger()->SetLogVerbosityLevel(fConfig.f_glb_logVerbose.data());
    FairLogger::GetLogger()->SetColoredLog(fConfig.f_glb_logColor.data());

    // --- Input sources
    int sourceId = 0;
    for (auto trafile : fTra) {
      if (fConfig.f_src_embedToId.at(sourceId) > 0) {
        LOG(info) << GetName() << ": Embeding input " << trafile << " (Rate will be ignored)";
        fRun.EmbedInput(fConfig.f_src_id.at(sourceId), trafile, fConfig.f_src_embedToId.at(sourceId));
      }
      else {
        if (fConfig.f_glb_mode == cbm::sim::Mode::EventByEvent && fConfig.f_src_id.size() > 1)
          throw std::runtime_error("Event mixing is not possible in event-by-event mode!");

        LOG(info) << GetName() << ": Adding input " << trafile;
        fRun.AddInput(fConfig.f_src_id.at(sourceId), trafile, fConfig.f_ts_timeDist, fConfig.f_src_rate.at(sourceId),
                      fConfig.f_src_treeAccessMode.at(sourceId));
      }
    }
    fRun.SetParameterRootFile(fPar);
    fRun.SetMonitorFile((GetFileName(fOutput) + ".moni_digi.root").c_str());
    fRun.SetOutputFile(fOutput, fOverwrite);

    // --- Set digitization parameters
    fRun.SetMode(fConfig.f_glb_mode);
    if (fConfig.f_glb_mode == cbm::sim::Mode::Timebased) {
      fRun.SetTimeSliceLength(fConfig.f_ts_tslength);
      fRun.SetStartTime(fConfig.f_ts_startTime);
      fRun.StoreAllTimeSlices(fConfig.f_ts_storeAllTS);
    }
    fRun.SetProduceNoise(fConfig.f_bg_produceNoise);

    // --- Geometry setup
    LOG(info) << GetName() << ": Loading setup " << fSetupTag;
    fSetup = CbmSetup::Instance();
    fSetup->LoadSetup(fSetupTag);

    if (fConfig.f_det_deactivateAllBut != ECbmModuleId::kNotExist)
      fRun.DeactivateAllBut(fConfig.f_det_deactivateAllBut);

    for (auto det : fConfig.f_det_deactivate) {
      if (det != ECbmModuleId::kNotExist) fRun.Deactivate(det);
    }

    timer.Stop();
    double timeInit = timer.RealTime();
    timer.Start();

    // --- Execute run
    std::cout << std::endl << std::endl;
    LOG(info) << GetName() << ": Starting run" << std::endl;
    fRun.Run(fConfig.f_glb_firstTs, fConfig.f_glb_numTs);

    // --- Finish
    timer.Stop();
    double timeExec = timer.RealTime();
    FairMonitor::GetMonitor()->Print();
    std::cout << std::endl << std::endl;
    LOG(info) << GetName() << ": Execution successful";
    for (auto trafile : fTra)
      LOG(info) << GetName() << ": Transport file was      " << trafile;
    LOG(info) << GetName() << ": Parameter file was      " << fPar;
    LOG(info) << GetName() << ": Output file is          " << fOutput;
    LOG(info) << GetName() << ": Execution time: Init    " << timeInit << " s, Exec " << timeExec << "s";
  }
  // --------------------------------------------------------------------------


  // -----  Get file name without extension   ---------------------------------
  std::string Run::GetFileName(const TString file)
  {
    std::string temp(file.Data());
    size_t index = temp.find_last_of(".");
    temp         = temp.substr(0, index);
    std::transform(temp.begin(), temp.end(), temp.begin(), [](unsigned char c) { return c; });
    return temp;
  }
  // --------------------------------------------------------------------------


  // -----  Read configuration from YAML file   -------------------------------
  void Run::LoadConfig(const char* fileName)
  {
    TString file(fileName);
    if (file.IsNull()) {
      file = std::getenv("VMCWORKDIR");
      file += "/sim/response/config/DigiConfig_event.yaml";
    }
    LOG(info) << GetName() << ": Loading configuration from " << file;
    fConfig.LoadYaml(file.Data());
  }
  // --------------------------------------------------------------------------


  // ----  Set input sources  -------------------------------------------------
  void Run::SetTraFiles(const std::vector<std::string> files)
  {
    for (auto file : files) {
      fTra.push_back(file.c_str());
    }
  }
  // --------------------------------------------------------------------------

}  // namespace cbm::sim::digitization

ClassImp(cbm::sim::digitization::Run)
