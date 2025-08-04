/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file run_inspect_digievents.C
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 19 March 2024
 **/


// --- Includes needed for IDE
#include <RtypesCore.h>
#if !defined(__CLING__)
#include "CbmSourceDigiEvents.h"
#include "CbmTaskInspectDigiEvents.h"

#include <FairRunAna.h>
#include <FairSystemInfo.h>

#include <TStopwatch.h>
#endif


/** @brief Macro for simple inspection of DigiEvents
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since  219 March 2024
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 **
 ** This macro performs a simple inspection of DigiEvents produced an archived by online data processing.
 ** It prints statistics of events on the console. The purpose is to test CbmSourceDigiEvents and to
 ** demonstrate the event loop within a timeslice and the access to digi data in the event.
 **/
void run_inspect_digievents(TString inputFileName, TString outputFileName, size_t numTimeslices = -1)
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // --- Logger settings ----------------------------------------------------
  TString logLevel     = "INFO";
  TString logVerbosity = "LOW";
  // ------------------------------------------------------------------------

  // -----   Environment   --------------------------------------------------
  TString myName = "run_inspect_digievents";       // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------


  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------


  // -----   FairRunAna   ---------------------------------------------------
  FairRunOnline* run = new FairRunOnline();
  FairSource* source = new CbmSourceDigiEvents(inputFileName);
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
  FairTask* inspect = new CbmTaskInspectDigiEvents();
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
