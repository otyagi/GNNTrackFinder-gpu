/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

void SetTrack(CbmTransport*, Double_t, Int_t, Double_t, Double_t, Double_t);

/// @brief Function to run a mCBM transport with a box generator
/// @param nEvents       Number of events
/// @param setupName     Name of the setup
/// @param output        Prefix of the output name
/// @param overwrite     Flag: if overwrite output
/// @param randomSeed    Seed of the random generator
/// @param pdg           PDG code for particles to generate
/// @param multiplicity  Multiplicity of generated particles
// clang-format off
void mcbm_transport_boxgen(Int_t nEvents = 10,
                    //                  const char* setupName = "mcbm_beam_2022_08",
                    //                  const char* setupName = "mcbm_beam_2022_07",
                    //                  const char* setupName = "mcbm_beam_2022_06",
                    //                  const char* setupName = "mcbm_beam_2022_05",
                    //                  const char* setupName = "mcbm_beam_2022_04",
                    //                  const char* setupName = "mcbm_beam_2022_03",
                    //                  const char* setupName = "mcbm_beam_2022_02",
                    //                  const char* setupName = "mcbm_beam_2022_01",
                    //                  const char* setupName = "mcbm_beam_2022_06_16_gold",
                                      const char* setupName = "mcbm_beam_2022_05_23_nickel",
                    //                  const char* setupName = "mcbm_beam_2022_03_28_uranium",
                    //                  const char* setupName = "mcbm_beam_2022_03_27_iron",
                    //                  const char* setupName = "mcbm_beam_2022_03_22_iron",
                    //                  const char* setupName = "mcbm_beam_2022_03_20_iron",
                    //                  const char* setupName = "mcbm_beam_2022_03_09_carbon",
                    //                  const char* setupName = "mcbm_beam_2021_07_surveyed",
                    //                  const char* setupName = "mcbm_beam_2021_04",
                    //                  const char* setupName = "mcbm_beam_2021_03",
                    //                  const char* setupName = "mcbm_beam_2020_03",
                    //                  const char* setupName = "mcbm_beam_2019_11",
                    //                  const char* setupName = "mcbm_beam_2019_03",
                    //                  const char* setupName = "sis18_mcbm_25deg_long",
                    const char* output = "data/test", 
                    Bool_t overwrite = kTRUE, 
                    int randomSeed = 1,
                    int pdg = 2212,
                    int multiplicity = 1
                    )
// clang-format on
{
  // --- Define the beam angle ----------------------------------------------
  Double_t beamRotY = 25.;
  // ------------------------------------------------------------------------

  // --- Define the target geometry -----------------------------------------
  //
  // The target is not part of the setup, since one and the same setup can
  // and will be used with different targets.
  // The target is constructed as a tube in z direction with the specified
  // diameter (in x and y) and thickness (in z). It will be placed at the
  // specified position as daughter volume of the volume present there. It is
  // in the responsibility of the user that no overlaps or extrusions are
  // created by the placement of the target.
  //
  TString targetElement = "Gold";
  Double_t targetPosX   = 0.;  // target x position in global c.s. [cm]
  Double_t targetPosY   = 0.;  // target y position in global c.s. [cm]
  Double_t targetPosZ   = 0.;  // target z position in global c.s. [cm]

  //  Double_t targetThickness = 0.1;    // full thickness in cm
  //  Double_t targetDiameter  = 0.5;    // diameter in cm
  //  Double_t targetRotY      = 25.;    // target rotation angle around the y axis [deg]

  Double_t targetThickness = 0.025;  // mCBM thin gold target 0.25 mm = 0.025 cm thickness
  Double_t targetDiameter  = 1.5;    // mCBM target width 15 mm = 1.5 cm
  //  Double_t targetDiameter  = 0.1;      // set small target for window acceptance plots
  Double_t targetRotY = beamRotY;  // target rotation angle around the y axis [deg]
  // ------------------------------------------------------------------------

  // --- Logger settings ----------------------------------------------------
  FairLogger::GetLogger()->SetLogScreenLevel("INFO");
  FairLogger::GetLogger()->SetLogVerbosityLevel("VERYHIGH");
  // ------------------------------------------------------------------------


  // -----   Environment   --------------------------------------------------
  TString myName = "mcbm_transport_boxgen";        // this macro's name for screen output
  TString srcDir = gSystem->Getenv("VMCWORKDIR");  // top source directory
  // ------------------------------------------------------------------------


  // -----   In- and output file names   ------------------------------------
  TString dataset(output);
  TString outFile = dataset + ".tra.root";
  TString parFile = dataset + ".par.root";
  TString geoFile = dataset + ".geo.root";
  std::cout << "Output file:     " << outFile << '\n';
  std::cout << "Parameters file: " << parFile << '\n';
  std::cout << "Geometry file:   " << geoFile << '\n';
  std::cout << '\n';
  // ------------------------------------------------------------------------


  // -----   Timer   --------------------------------------------------------
  TStopwatch timer;
  timer.Start();
  // ------------------------------------------------------------------------


  // ----- Transport run   --------------------------------------------------
  CbmTransport run;

  // ----- Box generator of a single -----------------------
  FairBoxGenerator* boxGen = new FairBoxGenerator(pdg, multiplicity);
  boxGen->SetXYZ(targetPosX, targetPosY, targetPosZ);
  boxGen->SetPRange(100., 100.);
  boxGen->SetPhiRange(0., 360.);
  boxGen->SetThetaRange(0., 50.);
  boxGen->SetCosTheta();
  boxGen->Init();

  run.SetEngine(kGeant3);
  run.AddInput(boxGen);

  CbmGeant3Settings g3set;

  g3set.SetProcessPairProduction(0);
  g3set.SetProcessComptonScattering(0);
  g3set.SetProcessPhotoEffect(0);
  g3set.SetProcessPhotoFission(0);
  g3set.SetProcessDeltaRay(0);
  g3set.SetProcessAnnihilation(0);
  g3set.SetProcessBremsstrahlung(0);
  g3set.SetProcessHadronicInteraction(0);
  g3set.SetProcessMuonNuclearInteraction(0);
  g3set.SetProcessDecay(0);
  //g3set.SetProcessEnergyLossModel(0); // energy loss is required by the STS digitization
  g3set.SetProcessMultipleScattering(0);

  g3set.SetProcessRayleighScattering(0);
  g3set.SetProcessCherenkovProduction(0);
  g3set.SetProcessEneryLossStraggling(0);

  run.SetGeant3Settings(&g3set);


  // ----- Box generator of a single proton per event -----------------------

  run.SetOutFileName(outFile, overwrite);
  run.SetParFileName(parFile);
  run.SetGeoFileName(geoFile);
  run.LoadSetup(setupName);

  CbmSetup* geoSetup = CbmSetup::Instance();

  // You can modify the pre-defined setup
  // geoSetup->SetActive(ECbmModuleId::kMuch, kFALSE);

  run.SetField(new CbmFieldConst());
  run.SetTarget(targetElement, targetThickness, targetDiameter, targetPosX, targetPosY, targetPosZ,
                targetRotY * TMath::DegToRad());
  run.SetBeamPosition(0., 0., 0.1, 0.1);  // Beam width 1 mm is assumed
  run.SetBeamAngle(beamRotY * TMath::DegToRad(), 0.);
  if (nEvents <= 10)  // store only for small number of events
    run.StoreTrajectories();
  run.SetRandomSeed(randomSeed);


  run.Run(nEvents);
  // ------------------------------------------------------------------------


  // -----   Finish   -------------------------------------------------------
  timer.Stop();
  Double_t rtime = timer.RealTime();
  Double_t ctime = timer.CpuTime();
  std::cout << std::endl << std::endl;
  std::cout << "Macro finished successfully." << std::endl;
  std::cout << "Output file is " << outFile << std::endl;
  std::cout << "Parameter file is " << parFile << std::endl;
  std::cout << "Real time " << rtime << " s, CPU time " << ctime << "s" << std::endl << std::endl;
  // ------------------------------------------------------------------------


  // -----   Resource monitoring   ------------------------------------------
  FairSystemInfo sysInfo;
  Float_t maxMemory = sysInfo.GetMaxMemory();
  std::cout << "<DartMeasurement name=\"MaxMemory\" type=\"numeric/double\">";
  std::cout << maxMemory;
  std::cout << "</DartMeasurement>" << std::endl;

  Float_t cpuUsage = ctime / rtime;
  std::cout << "<DartMeasurement name=\"CpuLoad\" type=\"numeric/double\">";
  std::cout << cpuUsage;
  std::cout << "</DartMeasurement>" << std::endl;


  std::cout << " Test passed" << std::endl;
  std::cout << " All ok " << std::endl;
  // ------------------------------------------------------------------------
}


void SetTrack(CbmTransport* run, Double_t beamRotY, Int_t pdgid, Double_t x, Double_t y, Double_t z)
{
  TVector3 v;
  v.SetXYZ(x, y, z);
  v.RotateY(-beamRotY * acos(-1.) / 180.);
  cout << "X " << v.X() << " Y " << v.Y() << " Z " << v.Z() << endl;

  run->AddInput(new FairParticleGenerator(pdgid, 1, v.X(), v.Y(), v.Z()));  // single electron along beam axis
}
