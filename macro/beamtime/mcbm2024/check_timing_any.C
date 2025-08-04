/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer], Alexandru Bercuci*/

void check_timing_any(TString fileName, UInt_t uRunId = 0, Int_t nEvents = 0, TString outDir = "data/")
{

  // ========================================================================
  //          Adjust this part according to your requirements

  // Verbosity level (0=quiet, 1=event level, 2=track level, 3=debug)
  Int_t iVerbose = 3;

  // MC file

  TString srcDir      = gSystem->Getenv("VMCWORKDIR");
  TString outFileName = fileName(0, fileName.Index(".digi.root"));

  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------

  // -----  Analysis run   --------------------------------------------------
  FairRunOnline* fRun = new FairRunOnline();
  fRun->ActivateHttpServer(100, 8080);  // refresh each 100 events
  fRun->SetSink(new FairRootFileSink(Form("%s/%s.sink.root", outDir.Data(), outFileName.Data())));
  FairFileSource* inputSource = new FairFileSource(fileName);
  fRun->SetSource(inputSource);
  // ------------------------------------------------------------------------

  CbmMcbmCheckTimingTask* timeChecker = new CbmMcbmCheckTimingTask();
  /// Default is using Bmon as reference
  timeChecker->SetReferenceDetector(ECbmModuleId::kBmon, "Bmon", -1000., 1000., 320., 182, 190);
  /// Remove detectors not present in 03-2024
  timeChecker->RemoveCheckDetector(ECbmModuleId::kMuch);
  timeChecker->RemoveCheckDetector(ECbmModuleId::kPsd);
  /// Add detectors with wider range
  timeChecker->AddCheckDetector(ECbmModuleId::kSts, "Sts", -100., 150., 2500);
  timeChecker->AddCheckDetector(ECbmModuleId::kRich, "Rich", -40., 40., 800);
  timeChecker->AddCheckDetector(ECbmModuleId::kTrd, "Trd", -300., 300., 150);
  timeChecker->AddCheckDetector(ECbmModuleId::kTof, "Tof", -40, 40, 800);
  /// Add extra differential analysis on specific detectors
  /// The modifications have to be accompanied by a parallel implementation of
  /// the template function CbmMcbmCheckTimingAlgo::GetDigiInfo<CbmDetDigi>
  /// TODO Should be implemented as function pointer defined in this macro
  timeChecker->SetDetectorDifferential(ECbmModuleId::kSts,
                                       {"0", "1", "3", "4", "10", "11", "13", "14", "16", "17",
                                        "18"});  // select STS modules based on general expression U*10 + L*3 + M
  timeChecker->SetDetectorDifferential(ECbmModuleId::kTrd, {"5", "21", "37"});  // select modules
  timeChecker->SetDetectorDifferential(
    ECbmModuleId::kTof,
    {"0",   "1",   "2",   "3",   "4",   "10",  "11",  "12",  "13",  "14", "20", "21", "22",
     "23",  "24",  "30",  "31",  "32",  "33",  "34",  "40",  "41",  "42", "43", "44", "200",
     "201", "202", "203", "204", "600", "601", "900", "901", "910", "911"});  // select ToF RPCs based on the general expression Typ*100 + SmId * 10 + RpcId
  if (0 < uRunId) timeChecker->SetOutFilename(Form("%s/%s.tck.root", outDir.Data(), outFileName.Data()));
  fRun->AddTask(timeChecker);


  // -----   Intialise and run   --------------------------------------------
  fRun->Init();
  cout << "Starting run" << endl;
  if (nEvents < 0) {
    fRun->Run(0, 0);  // run until end of input file
  }
  else {
    fRun->Run(0, nEvents);  // process  N Events
  }
  // ------------------------------------------------------------------------


  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  cout << endl << endl;
  cout << "Macro finished successfully." << endl;
  cout << "Real time " << rtime << " s, CPU time " << ctime << " s" << endl;
  cout << endl;
  // ------------------------------------------------------------------------

  // Extract the maximal used memory an add is as Dart measurement
  // This line is filtered by CTest and the value send to CDash
  FairSystemInfo sysInfo;
  Float_t maxMemory = sysInfo.GetMaxMemory();
  cout << "<DartMeasurement name=\"MaxMemory\" type=\"numeric/double\">";
  cout << maxMemory;
  cout << "</DartMeasurement>" << endl;

  Float_t cpuUsage = ctime / rtime;
  cout << "<DartMeasurement name=\"CpuLoad\" type=\"numeric/double\">";
  cout << cpuUsage;
  cout << "</DartMeasurement>" << endl;
  /*
  FairMonitor* tempMon = FairMonitor::GetMonitor();
  tempMon->Print();
*/
  //  RemoveGeoManager();
  cout << " Test passed" << endl;
  cout << " All ok " << endl;
}
