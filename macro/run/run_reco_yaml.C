/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Dominik Smith */

/** @file run_reco_yaml.C
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 3 June 2023
 **/


// --- Includes needed for IDE
#include <RtypesCore.h>
#if !defined(__CLING__)
#include <FairMonitor.h>
#include <FairSystemInfo.h>

#include <TStopwatch.h>

#include "reco/offline/steer/Run.h"
#endif


/** @brief Macro for CBM offline reconstruction, configured from YAML
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since  3 June 2023
 ** @param input          Name of input file (w/o extension .raw.root)
 ** @param numTs          Number of time-slices to process
 ** @param config         YAML configuration file
 ** @param output         Name of output file (w/o extension .rec.root)
 ** @param setup          Name of predefined geometry setup
 ** @param paramFile      Parameter ROOT file (w/o extension .par.root)
 **
 ** This macro performs reconstruction from digis. It can be used
 ** for simulated data (result of run_digi.C) or real data after unpacking.
 **
 ** The macro covers both time-based reconstruction and event-based
 ** reconstruction using raw events build from digis.
 **
 ** The file names must be specified without extensions. The convention is
 ** that the raw (input) file is [input].raw.root. The output file
 ** will be [input].rec.root if not specified by the user. The parameter file
 ** has the extension .par.root. It is assumed to be [input].par.root if
 ** not specified by the user.
 **
 ** If no argument is specified, the input will be set to "test". This allows
 ** to execute the macro chain (run_tra_file.C, run_digi.C and run_reco_yaml.C)
 ** from the ROOT prompt without user intervention.
 **
 **/
void run_reco_yaml(TString input = "", Int_t numTs = 0, TString config = "", TString output = "",
                   TString setup = "sis100_electron", TString paramFile = "")
{

  // -----   Environment   --------------------------------------------------
  TString myName = "run_reco_yaml";                // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------


  // -----   In- and output file names   ------------------------------------
  if (input.IsNull()) input = "test";
  TString inFile = input + ".raw.root";
  if (output.IsNull()) output = input;
  TString outFile = output + ".reco.root";
  TString monFile = output + ".moni_reco.root";
  if (paramFile.IsNull()) paramFile = input;
  TString parFile = paramFile + ".par.root";
  std::cout << myName << ": Input file     " << inFile << std::endl;
  std::cout << myName << ": Output file    " << outFile << std::endl;
  std::cout << myName << ": Parameter file " << parFile << std::endl;
  std::cout << myName << ": Configuration  " << (config.IsNull() ? "default" : config);
  std::cout << std::endl;
  std::cout << myName << ": Geometry setup " << setup << std::endl;
  std::cout << myName << ": Number of Ts   ";
  if (numTs == 0) std::cout << "max";
  else
    std::cout << numTs;
  std::cout << std::endl;
  // ------------------------------------------------------------------------


  // -----   Configure run   ------------------------------------------------
  // TODO: The run should created on the stack, but then there is a segmentation fault
  // when destructing it at the end of the main function. This is caused by
  // the destructor of CbmStsReco (StsHitfinderBase). Until this is resolved,
  // the run is created on the heap.
  std::cout << myName << ": Configuring run" << std::endl;
  cbm::reco::offline::Run* run = new cbm::reco::offline::Run();
  run->SetInput(inFile.Data());
  run->SetOutput(outFile.Data());
  run->SetParams(parFile.Data());
  run->SetGeoSetupTag(setup.Data());
  run->SetNumTs(numTs);
  run->LoadConfig(config.Data());
  // ------------------------------------------------------------------------


  // -----   Start run   ----------------------------------------------------
  TStopwatch timer;
  timer.Start();
  std::cout << myName << ": Starting run" << std::endl;
  run->Exec();
  timer.Stop();
  // ------------------------------------------------------------------------


  // -----   Finish   -------------------------------------------------------
  FairMonitor::GetMonitor()->Print();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << std::endl << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "Output file is    " << outFile << std::endl;
  std::cout << "Parameter file is " << parFile << std::endl;
  std::cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << std::endl;
  FairSystemInfo sysInfo;
  Float_t maxMemory = sysInfo.GetMaxMemory();
  std::cout << "<DartMeasurement name=\"MaxMemory\" type=\"numeric/double\">";
  std::cout << maxMemory;
  std::cout << "</DartMeasurement>" << std::endl;
  Float_t cpuUsage = ctime / rtime;
  std::cout << "<DartMeasurement name=\"CpuLoad\" type=\"numeric/double\">";
  std::cout << cpuUsage;
  std::cout << "</DartMeasurement>" << std::endl;
  // ------------------------------------------------------------------------


  // -----   This is to prevent a malloc error when exiting ROOT   ----------
  // The source of the error is unknown. Related to TGeoManager.
  RemoveGeoManager();
  // ------------------------------------------------------------------------

}  // End of main macro function
