/* Copyright (C) 2023-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: S.Gorbunov[committer], N.Bluhme */

/// @file    CbmBbaAlignmentTask.h
/// @author  Sergey Gorbunov
/// @author  Nora Bluhme
/// @date    20.01.2023
/// @brief   Task class for alignment
///

#ifndef CbmBbaAlignmentTask_H
#define CbmBbaAlignmentTask_H


#include "CbmDefs.h"
#include "CbmKfTrackFitter.h"
#include "FairTask.h"
#include "TString.h"

#include <vector>

class TObject;
class TH1F;
class TClonesArray;
class TFile;
class TDirectory;
class TH1F;


///
/// an example of alignment using BBA package

class CbmBbaAlignmentTask : public FairTask {
 public:
  // Constructors/Destructors ---------
  CbmBbaAlignmentTask(const char* name = "CbmBbaAlignmentTask", Int_t iVerbose = 0,
                      TString histoFileName = "CbmBbaAlignmentHisto.root");
  ~CbmBbaAlignmentTask();

  Int_t GetZtoNStation(Double_t getZ);

  InitStatus Init();
  void Exec(Option_t* opt);
  void Finish();

  void SetMcbmTrackingMode() { fTrackingMode = TrackingMode::kMcbm; }
  void SetStsTrackingMode() { fTrackingMode = TrackingMode::kSts; }

  void SetSimulatedMisalignmentRange(double range) { fSimulatedMisalignmentRange = range; }
  void SetRandomSeed(int seed) { fRandomSeed = seed; }

 public:
  enum TrackingMode
  {
    kSts,
    kMcbm
  };

  /// information about a track
  /// aligned and unaligned hits are stored in the corresponding CbmKfTrackFitter::Track objects
  struct TrackContainer {

    CbmKfTrackFitter::Trajectory fUnalignedTrack;  // track before alignment
    CbmKfTrackFitter::Trajectory fAlignedTrack;    // track after alignment

    int fNmvdHits{0};    // number of MVD hits
    int fNstsHits{0};    // number of STS hits
    int fNmuchHits{0};   // number of MUCH hits
    int fNtrd1dHits{0};  // number of TRD hits
    int fNtrd2dHits{0};  // number of TRD hits
    int fNtofHits{0};    // number of TOF hits

    bool fIsActive{1};  // is the track active
    void MakeConsistent();
  };

  struct AlignmentBody {
    int fTrackingStation{-1};
  };

  struct Sensor {
    ECbmModuleId fSystemId{0};
    int fSensorId{-1};
    int fTrackingStation{-1};
    int fAlignmentBody{-1};
    std::string fNodePath{""};
    bool operator<(const Sensor& other) const
    {
      if (fSystemId < other.fSystemId) return true;
      if (fSystemId > other.fSystemId) return false;
      if (fTrackingStation < other.fTrackingStation) return true;
      if (fTrackingStation > other.fTrackingStation) return false;
      return (fSensorId < other.fSensorId);
    };

    bool operator==(const Sensor& other) const
    {
      return (fSystemId == other.fSystemId) && (fSensorId == other.fSensorId);
    };
  };

 private:
  const CbmBbaAlignmentTask& operator=(const CbmBbaAlignmentTask&);
  CbmBbaAlignmentTask(const CbmBbaAlignmentTask&);

  void WriteHistosCurFile(TObject* obj);

  void ApplyAlignment(const std::vector<double>& par);

  double CostFunction(const std::vector<double>& par);

  void ApplyConstraints(std::vector<double>& par);

  void ConstrainStation(std::vector<double>& par, int iSta, int ixyz);


  TrackingMode fTrackingMode = TrackingMode::kMcbm;

  // input data arrays

  TClonesArray* fInputGlobalTracks{nullptr};
  TClonesArray* fInputStsTracks{nullptr};

  TClonesArray* fInputMcTracks{nullptr};            // Mc info for debugging
  TClonesArray* fInputGlobalTrackMatches{nullptr};  // Mc info for debugging
  TClonesArray* fInputStsTrackMatches{nullptr};     // Mc info for debugging

  int fNthreads = 1;

  CbmKfTrackFitter fFitter;

  // collection of selected tracks and hits
  std::vector<TrackContainer> fTracks;

  //output file with histograms
  TString fHistoFileName{"CbmBbaAlignmentHisto.root"};
  TFile* fHistoFile{nullptr};
  TDirectory* fHistoDir{nullptr};

  Int_t fNEvents{0};

  Int_t fMaxNtracks{100000};

  int fNtrackingStations{0};
  int fNalignmentBodies{0};

  double fCostIdeal{1.e10};
  double fCostInitial{0.};

  double fSimulatedMisalignmentRange{0.};  // misalignment range for simulated misalignment

  int fRandomSeed{1};

  double fChi2Total{0.};
  long fNdfTotal{0};
  long fFixedNdf{-1};

  std::vector<Sensor> fSensors;
  std::vector<AlignmentBody> fAlignmentBodies;

  //histograms

  std::vector<TH1F*> hResidualsBeforeAlignmentX{};
  std::vector<TH1F*> hResidualsBeforeAlignmentY{};
  std::vector<TH1F*> hResidualsAfterAlignmentX{};
  std::vector<TH1F*> hResidualsAfterAlignmentY{};

  std::vector<TH1F*> hPullsBeforeAlignmentX{};
  std::vector<TH1F*> hPullsBeforeAlignmentY{};
  std::vector<TH1F*> hPullsAfterAlignmentX{};
  std::vector<TH1F*> hPullsAfterAlignmentY{};

  ClassDef(CbmBbaAlignmentTask, 1);
};

#endif
