/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

/** @file run_inspect_digi_timeslice.C
 ** @author Pierre-Alain loizeau <p.-a.loizeau@gsi.de>
 ** @since 21 March 2024
 **/


// --- Includes needed for IDE
#include <RtypesCore.h>
#if !defined(__CLING__)
#include "CbmSourceDigiTimeslice.h"
#include "CbmTaskInspectDigiEvents.h"
#include "CbmTsEventHeader.h"

#include <FairRunAna.h>
#include <FairSystemInfo.h>

#include <TStopwatch.h>
#endif


/** @brief Macro to convert an output from the online binary with raw Digis to a Root file
 ** @author Pierre-Alain loizeau <p.-a.loizeau
 ** @since  21 March 2024
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 **
 ** This macro converts raw Digis produced and archived by online data processing to Root format.
 **/
void run_inspect_digi_timeslice(TString inputFileName, TString outputFileName, UInt_t runid = 2391,
                                size_t numTimeslices = -1)
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  TString myName = "run_convert_online_rawdigis";  // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------


  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------


  // -----   FairRunAna   ---------------------------------------------------
  FairRunOnline* run = new FairRunOnline();
  run->SetEventHeader(new CbmTsEventHeader{});
  FairSource* source = new CbmSourceDigiTimeslice(inputFileName);
  run->SetSource(source);
  auto sink = new FairRootFileSink(outputFileName);
  run->SetSink(sink);
  run->SetGenerateRunInfo(kTRUE);
  // ------------------------------------------------------------------------


  // -----   Logger settings   ----------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel(logLevel.Data());
  FairLogger::GetLogger()->SetLogVerbosityLevel(logVerbosity.Data());
  // ------------------------------------------------------------------------


  // -----   Event inspection   ---------------------------------------------
  FairTask* inspect = new CbmTaskInspectDigiTimeslice();
  LOG(info) << "-I- " << myName << ": Adding task " << inspect->GetName();
  run->AddTask(inspect);
  // ------------------------------------------------------------------------


  // -----   Run initialisation   -------------------------------------------
  std::cout << std::endl;
  std::cout << "-I- " << myName << ": Initialise run" << std::endl;
  run->Init();
  // ------------------------------------------------------------------------


  // -----   Start run   ----------------------------------------------------
  std::cout << std::endl << std::endl;
  std::cout << "-I- " << myName << ": Starting run" << std::endl;
  if (numTimeslices == -1)
    run->Run(-1, 0);
  else
    run->Run(0, numTimeslices);
  // ------------------------------------------------------------------------


  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  FairMonitor::GetMonitor()->Print();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << std::endl << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
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


}  // End of main macro function
