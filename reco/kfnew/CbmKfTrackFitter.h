/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

#pragma once  // include this header only once per compilation unit


#include "CbmDefs.h"
#include "KfDefs.h"
#include "KfMeasurementTime.h"
#include "KfMeasurementXy.h"
#include "KfSetup.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"

#include <vector>

class CbmMvdHit;
class CbmStsHit;
class CbmStsTrack;
class CbmGlobalTrack;
class CbmKFVertex;
class CbmMuchPixelHit;
class CbmTrdHit;
class CbmTofHit;

class FairTrackParam;
class TClonesArray;


/// A fitter for the Cbm tracks
///
class CbmKfTrackFitter {

 public:
  /// The class to fit the tracks with the Kalman Filter with explicit linearisation of the equations
  ///
  /// A node on the trajectory where the track parameters are:
  /// a) measured and / or
  /// b) scattered and / or
  /// c) need to be estimated
  /// The nodes are expected to be ordered by increasing Z.
  /// It can be done via CbmKfTrackFitter::Trajectory::MakeConsistent() method.
  ///
  /// When fIsFitted flag is set (e.g. by the fitter), the node track parameters are used
  /// for the trajectory linearisation during the fit.
  ///
  /// When fitting w/o smoother we assume that smoothed parameters are always better than partially fitted parameters
  ///
  /// TODO: proper interface and description
  ///
  struct TrajectoryNode {

    double fZ{0.};  ///< Z coordinate of the node

    cbm::algo::kf::TrackParamD fParamUp{};  ///< fitted track parameters upstream the node
    cbm::algo::kf::TrackParamD fParamDn{};  ///< fitted track parameters downstream the node

    /// == Material information (if present)

    // TODO: change to the material layer index when the material layer is implemented
    int fMaterialLayer{-1};  ///< index of the material layer. Currently equal to the active tracking station index

    /// radiation thickness of the material associated with the node
    /// - taken from the material map or set externally
    double fRadThick{0.};

    /// == Hit information ( if present )

    cbm::algo::kf::MeasurementXy<double> fMxy{};  ///< XY-measurement at fZ

    cbm::algo::kf::MeasurementTime<double> fMt{};  ///< time measurement at fZ

    /// == Flags etc
    bool fIsXySet{false};          ///< true if the XY measurement is set
    bool fIsTimeSet{false};        ///< true if the time measurement is set
    bool fIsRadThickFixed{false};  ///< true if the radiation thickness is fixed to the fRadThick value
    bool fIsFitted{false};         ///< true if the track parameters at the node are fitted

    /// == External references
    ECbmModuleId fHitSystemId{ECbmModuleId::kNotExist};  ///< detector system ID of the hit
    int fHitAddress{-1};                                 ///< detector ID of the hit
    int fHitIndex{-1};                                   ///< hit index in the detector hit array

    int fReference1{-1};  ///< some reference can be set by the user
    int fReference2{-1};  ///< some reference can be set by the user
  };

  /// A trajectory to be fitted
  // TODO: proper interface
  struct Trajectory {
    std::vector<TrajectoryNode> fNodes;  ///< nodes on the trajectory
    bool fIsFitted{false};               ///< true if the trajectory is fitted
    void OrderNodesInZ();                // sort the nodes in Z and set fFirstHitNode and fLastHitNode
  };

  CbmKfTrackFitter();
  ~CbmKfTrackFitter();

  /// initialize the fitter. It must be called in the Init() of the user task.
  /// when called later, it sees track branches always empty for whatever reason
  void Init();

  /// set particle hypothesis (mass and electron flag) via particle PDG
  void SetParticleHypothesis(int pid);

  /// set particle  mass
  void SetMassHypothesis(double mass);

  /// set electron flag (bremmstrallung will be applied)
  void SetElectronFlag(bool isElectron);

  /// skip unmeasured coordinates
  void SetSkipUnmeasuredCoordinates(bool skip = true) { fSkipUnmeasuredCoordinates = skip; }

  /// set the default inverse momentum for the Multiple Scattering calculation
  void SetDefaultMomentumForMs(double p) { fDefaultQpForMs = fabs(1. / p); }

  /// set the default inverse momentum for the Multiple Scattering calculation
  void SetDefaultInverseMomentumForMs(double invP) { fDefaultQpForMs = fabs(invP); }

  /// set the default inverse momentum for the Multiple Scattering calculation
  void SetNoMultipleScattering() { fDefaultQpForMs = 0.; }

  /// fix the inverse momentum for the Multiple Scattering calculation
  void FixMomentumForMs(bool fix = true) { fIsQpForMsFixed = fix; }

  /// set the input data arrays
  bool CreateMvdStsTrack(Trajectory& kfTrack, int stsTrackIndex);
  bool CreateGlobalTrack(Trajectory& kfTrack, int globalTrackIndex);
  bool CreateGlobalTrack(Trajectory& kfTrack, const CbmGlobalTrack& globalTrack);

  /// fit the track
  bool FitTrajectory(CbmKfTrackFitter::Trajectory& t);

  /// do the KF-smoothing to define track pars at all the nodes
  void SetDoSmooth(bool doSmooth) { fDoSmooth = doSmooth; }

  /// set verbosity level
  void SetVerbosityLevel(int level) { fVerbosityLevel = level; }

  /// set information about the track for debug output
  void SetDebugInfo(const std::string& info) { fDebugInfo = info; }

 private:
  struct LinearizationAtNode {
    cbm::algo::kf::TrackParamD fParamUp{};  ///< fitted track parameters upstream the node
    cbm::algo::kf::TrackParamD fParamDn{};  ///< fitted track parameters downstream the node
  };

  void FilterFirstMeasurement(const TrajectoryNode& n);

  void AddMaterialEffects(TrajectoryNode& n, const LinearizationAtNode& l, cbm::algo::kf::FitDirection direction);

  // combine two tracks
  bool Smooth(cbm::algo::kf::TrackParamD& t1, const cbm::algo::kf::TrackParamD& t2);

 private:
  std::shared_ptr<const cbm::algo::kf::Setup<double>> fKfSetup;

  // input data arrays
  TClonesArray* fInputMvdHits{nullptr};
  TClonesArray* fInputStsHits{nullptr};
  TClonesArray* fInputMuchHits{nullptr};
  TClonesArray* fInputTrdHits{nullptr};
  TClonesArray* fInputTofHits{nullptr};

  TClonesArray* fInputGlobalTracks{nullptr};
  TClonesArray* fInputStsTracks{nullptr};
  TClonesArray* fInputMuchTracks{nullptr};
  TClonesArray* fInputTrdTracks{nullptr};
  TClonesArray* fInputTofTracks{nullptr};

  //

  bool fIsInitialized = {false};  // is the fitter initialized
  bool fSkipUnmeasuredCoordinates{false};
  cbm::algo::kf::TrackKalmanFilter<double> fFit;  // track fit object

  /// externally defined inverse momentum for the Multiple Scattering calculation.
  /// It is used for the tracks in field-free regions.
  /// When the momentum can be fitted, the fitted value is used.
  /// the default value is set to 0.1 GeV/c
  double fDefaultQpForMs{1. / 0.1};
  bool fIsQpForMsFixed{false};

  double fMass{cbm::algo::kf::defs::PionMass<double>};  // mass hypothesis for the fit
  bool fIsElectron{false};                              // fit track as an electron (with the bermsstrallung effect)
  bool fDoSmooth{true};                                 // do the KF-smoothing to define track pars at all the nodes
  int fVerbosityLevel{0};                               // verbosity level
  std::string fDebugInfo{};                             // debug info
};
