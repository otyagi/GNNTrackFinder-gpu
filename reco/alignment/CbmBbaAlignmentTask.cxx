/* Copyright (C) 2023-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: S.Gorbunov[committer], N.Bluhme */

/// @file    CbmBbaAlignmentTask.cxx
/// @author  Sergey Gorbunov
/// @author  Nora Bluhme
/// @date    20.01.2023
/// @brief   Task class for alignment
///

// Cbm Headers ----------------------
#include "CbmBbaAlignmentTask.h"


// ROOT headers

#include "FairRootManager.h"
#include "FairRunAna.h"
#include "TClonesArray.h"
#include "TFile.h"
#include "TGeoPhysicalNode.h"
#include "TH1F.h"
#include "TRandom.h"

//
#include "CbmCaTimeSliceReader.h"
#include "CbmGlobalTrack.h"
#include "CbmL1.h"
#include "CbmMuchTrack.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdHit.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmStsHit.h"
#include "CbmStsSetup.h"
#include "CbmStsTrack.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTofTrack.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdTrack.h"
#include "CbmTrdTrackingInterface.h"
#include "TCanvas.h"
#include "TGeoPhysicalNode.h"
#include "bba/BBA.h"

#include <cmath>
#include <iostream>
#include <thread>

namespace
{
  using namespace cbm::algo;
}

std::pair<std::string, TGeoHMatrix> AlignNode(std::string path, double shiftX, double shiftY, double shiftZ,
                                              double rotX, double rotY, double rotZ)
{

  TGeoHMatrix result;
  result.SetDx(shiftX);
  result.SetDy(shiftY);
  result.SetDz(shiftZ);
  result.RotateX(rotX);
  result.RotateY(rotY);
  result.RotateZ(rotZ);

  //std::cout << "Alignment matrix for node " << path << " is: " << std::endl;
  //result.Print();
  //std::cout << std::endl;

  return std::pair<std::string, TGeoHMatrix>(path, result);
}

void CbmBbaAlignmentTask::TrackContainer::MakeConsistent()
{
  fNmvdHits   = 0;
  fNstsHits   = 0;
  fNmuchHits  = 0;
  fNtrd1dHits = 0;
  fNtrd2dHits = 0;
  fNtofHits   = 0;
  for (const auto& n : fUnalignedTrack.fNodes) {
    switch (n.fHitSystemId) {
      case ECbmModuleId::kMvd: fNmvdHits++; break;
      case ECbmModuleId::kSts: fNstsHits++; break;
      case ECbmModuleId::kMuch: fNmuchHits++; break;
      case ECbmModuleId::kTrd: fNtrd1dHits++; break;
      case ECbmModuleId::kTrd2d: fNtrd2dHits++; break;
      case ECbmModuleId::kTof: fNtofHits++; break;
      default: break;
    }
  }
}

// void addVirtualMisalignment()
// {


// }

CbmBbaAlignmentTask::CbmBbaAlignmentTask(const char* name, Int_t iVerbose, TString histoFileName)
  : FairTask(name, iVerbose)
  , fHistoFileName(histoFileName)
{
  TFile* curFile           = gFile;
  TDirectory* curDirectory = gDirectory;

  if (!(fHistoFileName == "")) {
    fHistoFile = new TFile(fHistoFileName.Data(), "RECREATE");
  }
  else {
    fHistoFile = gFile;
  }

  fHistoFile->cd();

  fHistoDir = fHistoFile->mkdir("Bba");
  fHistoDir->cd();

  gFile      = curFile;
  gDirectory = curDirectory;
}


CbmBbaAlignmentTask::~CbmBbaAlignmentTask() {}


InitStatus CbmBbaAlignmentTask::Init()
{

  {
    const char* env = std::getenv("OMP_NUM_THREADS");
    if (env) {
      fNthreads = TString(env).Atoi();
      LOG(info) << "BBA:   found environment variable OMP_NUM_THREADS = \"" << env
                << "\", read as integer: " << fNthreads;
    }
  }

  // ensure that at least one thread is set

  if (fNthreads < 1) {
    fNthreads = 1;
  }

  // fNthreads = 1; // enforce n threads to one

  fFitter.Init();
  fFitter.SetSkipUnmeasuredCoordinates(true);

  //Get ROOT Manager
  FairRootManager* ioman = FairRootManager::Instance();

  if (!ioman) {
    LOG(error) << "CbmBbaAlignmentTask::Init :: RootManager not instantiated!";
    return kERROR;
  }

  // Get global tracks & matches

  if (TrackingMode::kSts != fTrackingMode) {

    fInputGlobalTracks = static_cast<TClonesArray*>(ioman->GetObject("GlobalTrack"));
    if (!fInputGlobalTracks) {
      LOG(error) << "CbmBbaAlignmentTask::Init: Global track array not found!";
      return kERROR;
    }

    fInputGlobalTrackMatches = static_cast<TClonesArray*>(ioman->GetObject("GlobalTrackMatch"));

    if (!fInputGlobalTrackMatches) {
      LOG(error) << "CbmBbaAlignmentTask::Init: Global track matches array not found!";
      //return kERROR;
    }
  }

  fInputStsTracks = static_cast<TClonesArray*>(ioman->GetObject("StsTrack"));
  if (!fInputStsTracks) {
    LOG(error) << "CbmBbaAlignmentTask::Init: Sts track array not found!";
    return kERROR;
  }

  fInputStsTrackMatches = static_cast<TClonesArray*>(ioman->GetObject("StsTrackMatch"));

  if (!fInputStsTrackMatches) {
    LOG(error) << "CbmBbaAlignmentTask::Init: Sts track matches array not found!";
    //return kERROR;
  }

  // MC tracks
  fInputMcTracks = static_cast<TClonesArray*>(ioman->GetObject("MCTrack"));
  if (!fInputMcTracks) {
    Warning("CbmBbaAlignmentTask::Init", "mc track array not found!");
    //return kERROR;
  }

  fTracks.clear();


  ca::Framework* l1                       = CbmL1::Instance()->fpAlgo;
  const ca::Parameters<ca::fvec>& l1Param = l1->GetParameters();
  fNtrackingStations                      = l1Param.GetNstationsActive();
  // TO get rid of weird station counting:
  // fNtrackingStations                      = l1Param.GetNstationsActive() - l1Param. GetNstationsActive(ca::EDetectorID::kMvd);

  TDirectory* curDirectory = gDirectory;

  fHistoDir->cd();

  for (int i = -1; i < fNtrackingStations; i++) {
    const char* n1 = Form("Station%d", i);
    const char* n2 = Form(", station %d", i);
    if (i == -1) {
      n1 = "All";
      n2 = "";
    }
    fHistoDir->mkdir(n1);
    fHistoDir->cd(n1);

    int nBins = 301;

    double sizeX  = 0.1;
    double sizeY  = 0.1;
    double sizePX = 10.;
    double sizePY = 10.;
    if (fTrackingMode == kSts) {
      sizeX = 0.1;
      sizeY = 0.1;
    }
    else if (fTrackingMode == kMcbm) {

      if (i == -1) {
        sizeX  = 5.;
        sizeY  = 5.;
        sizePX = 10.;
        sizePY = 10.;
      }

      if (i == 0 || i == 1) {
        sizeX = 1.0;
        sizeY = 1.0;
      }
      if (i == 2) {
        sizeX = 5.;
        sizeY = 5.;
      }
      if (i == 3) {
        sizeX  = 5.;
        sizeY  = 20.;
        sizePX = 10.;
      }
      if (i == 4) {
        sizeX  = 20.;
        sizeY  = 5.;
        sizePY = 10.;
      }
      if (i >= 5) {
        sizeX = 5.;
        sizeY = 5.;
      }
    }

    hResidualsBeforeAlignmentX.push_back(
      new TH1F(Form("ResBeforeAli%s_X", n1), Form("X-Residuals Before Alignment%s", n2), nBins, -sizeX, sizeX));
    hResidualsBeforeAlignmentY.push_back(
      new TH1F(Form("ResBeforeAli%s_Y", n1), Form("Y-Residuals Before Alignment%s", n2), nBins, -sizeY, sizeY));
    hResidualsAfterAlignmentX.push_back(
      new TH1F(Form("ResAfterAli%s_X", n1), Form("X-Residuals After Alignment%s", n2), nBins, -sizeX, sizeX));
    hResidualsAfterAlignmentY.push_back(
      new TH1F(Form("ResAfterAli%s_Y", n1), Form("Y-Residuals After Alignment%s", n2), nBins, -sizeY, sizeY));

    hPullsBeforeAlignmentX.push_back(
      new TH1F(Form("PullBeforeAli%s_X", n1), Form("X-Pulls Before Alignment%s", n2), nBins, -sizePX, sizePX));
    hPullsBeforeAlignmentY.push_back(
      new TH1F(Form("PullBeforeAli%s_Y", n1), Form("Y-Pulls Before Alignment%s", n2), nBins, -sizePY, sizePY));
    hPullsAfterAlignmentX.push_back(
      new TH1F(Form("PullAfterAli%s_X", n1), Form("X-Pulls After Alignment%s", n2), nBins, -sizePX, sizePX));
    hPullsAfterAlignmentY.push_back(
      new TH1F(Form("PullAfterAli%s_Y", n1), Form("Y-Pulls After Alignment%s", n2), nBins, -sizePY, sizePY));
  }

  for (int i = 0; i < fNtrackingStations + 1; i++) {
    // set line colors
    hResidualsAfterAlignmentX[i]->SetLineColor(kRed);
    hResidualsAfterAlignmentY[i]->SetLineColor(kRed);
    hPullsAfterAlignmentX[i]->SetLineColor(kRed);
    hPullsAfterAlignmentY[i]->SetLineColor(kRed);
    // set histograms to dynamic binning
    hResidualsBeforeAlignmentX[i]->SetCanExtend(TH1::kXaxis);
    hResidualsBeforeAlignmentY[i]->SetCanExtend(TH1::kXaxis);
    hResidualsAfterAlignmentX[i]->SetCanExtend(TH1::kXaxis);
    hResidualsAfterAlignmentY[i]->SetCanExtend(TH1::kXaxis);
    hPullsBeforeAlignmentX[i]->SetCanExtend(TH1::kXaxis);
    hPullsBeforeAlignmentY[i]->SetCanExtend(TH1::kXaxis);
    hPullsAfterAlignmentX[i]->SetCanExtend(TH1::kXaxis);
    hPullsAfterAlignmentY[i]->SetCanExtend(TH1::kXaxis);
  }

  gDirectory = curDirectory;

  return kSUCCESS;
}

void CbmBbaAlignmentTask::Exec(Option_t* /*opt*/)
{

  LOG(info) << "BBA: exec event N " << fNEvents;

  fNEvents++;

  if (static_cast<int>(fTracks.size()) >= fMaxNtracks) {
    return;
  }

  ca::Framework* l1                     = CbmL1::Instance()->fpAlgo;
  const ca::Parameters<ca::fvec>& l1Par = l1->GetParameters();

  fFitter.SetDoSmooth(true);

  // select tracks for alignment and store them
  unsigned num_rejected_fit = 0;
  if (fTrackingMode == kSts && fInputStsTracks) {
    int numPosMomentum = 0;
    int numNegMomentum = 0;

    fFitter.SetDefaultMomentumForMs(0.1);

    for (int iTr = 0; iTr < fInputStsTracks->GetEntriesFast(); iTr++) {
      if (static_cast<int>(fTracks.size()) >= fMaxNtracks) {
        break;
      }
      const CbmStsTrack* stsTrack = dynamic_cast<CbmStsTrack*>(fInputStsTracks->At(iTr));
      if (!stsTrack) continue;
      TrackContainer t;
      if (!fFitter.CreateMvdStsTrack(t.fUnalignedTrack, iTr)) continue;
      t.MakeConsistent();

      for (auto& n : t.fUnalignedTrack.fNodes) {
        n.fIsTimeSet = false;
        // attempt to switch off MVD
        // if (n.fHitSystemId == ECbmModuleId::kMvd) {
        //   n.fIsXySet = false;
        // }
      }

      if (!fFitter.FitTrajectory(t.fUnalignedTrack)) {
        LOG(debug) << "failed to fit the track! ";
        num_rejected_fit++;
        continue;
      }

      const auto& par = t.fUnalignedTrack.fNodes.front().fParamUp;

      if (t.fNstsHits < l1Par.GetNstationsActive(ca::EDetectorID::kSts)) continue;
      if (!std::isfinite(par.GetQp())) continue;
      if (fabs(par.GetQp()) > 1.) continue;  // select tracks with min 1 GeV momentum
      if (par.GetQp() > 0.) {
        // if (numPosMomentum > numNegMomentum) {
        //   continue;
        // }
        numPosMomentum++;
      }
      else {
        numNegMomentum++;
      }
      t.fAlignedTrack = t.fUnalignedTrack;
      fTracks.push_back(t);
    }
    LOG(info) << "BBA: selected " << fTracks.size() << " tracks for alignment, " << numPosMomentum << " positive and "
              << numNegMomentum << " negative momentum tracks";
  }
  else if (fTrackingMode == kMcbm && fInputGlobalTracks) {

    fFitter.SetDefaultMomentumForMs(0.5);
    fFitter.FixMomentumForMs(true);

    for (int iTr = 0; iTr < fInputGlobalTracks->GetEntriesFast(); iTr++) {
      if (static_cast<int>(fTracks.size()) >= fMaxNtracks) {
        break;
      }
      const CbmGlobalTrack* globalTrack = dynamic_cast<const CbmGlobalTrack*>(fInputGlobalTracks->At(iTr));
      if (!globalTrack) {
        LOG(fatal) << "BBA: null pointer to the global track!";
        break;
      }
      TrackContainer t;
      if (!fFitter.CreateGlobalTrack(t.fUnalignedTrack, *globalTrack)) {
        LOG(fatal) << "BBA: can not create the global track for the fit! ";
        break;
      }
      t.MakeConsistent();

      for (auto& n : t.fUnalignedTrack.fNodes) {
        n.fIsTimeSet = false;
        if (n.fHitSystemId == ECbmModuleId::kTrd) {
          if (n.fMxy.Dx2() < n.fMxy.Dy2()) {
            n.fMxy.SetNdfY(0);
          }
          else {
            n.fMxy.SetNdfX(0);
          }
        }
      }

      if (t.fNstsHits < 2) continue;
      if (t.fNtofHits < 2) continue;
      if (t.fNtrd1dHits + t.fNtrd2dHits < 2) continue;
      if (!fFitter.FitTrajectory(t.fUnalignedTrack)) {
        LOG(debug) << "failed to fit the track! ";
        num_rejected_fit++;
        continue;
      }
      t.fAlignedTrack = t.fUnalignedTrack;
      fTracks.push_back(t);
    }
  }  // end of mcbm tracking mode

  if (num_rejected_fit != 0) {
    LOG(warn) << "failed to fit " << num_rejected_fit << " tracks";
  }

  // fix the material radiation thickness to avoid the chi2 rattling at the material map bin boundaries
  // (to be improved)
  unsigned num_fix_rad_max   = 0;
  unsigned num_fix_rad_min   = 0;
  unsigned num_rejected_fit2 = 0;
  for (TrackContainer& t : fTracks) {
    if (!t.fIsActive) continue;
    for (unsigned int in = 0; in < t.fUnalignedTrack.fNodes.size(); in++) {
      CbmKfTrackFitter::TrajectoryNode& node = t.fUnalignedTrack.fNodes[in];
      if (!node.fIsFitted) {
        t.fIsActive = false;
        num_rejected_fit2++;
        break;
      }
      node.fIsRadThickFixed = true;
      if (node.fRadThick > 0.01) {
        LOG(debug) << "CbmBbaAlignmentTask: radiation thickness is too large: " << node.fRadThick;
        num_fix_rad_max++;
        node.fRadThick = 0.01;
      }
      if (node.fRadThick < 0.001) {
        LOG(debug) << "CbmBbaAlignmentTask: radiation thickness is too small: " << node.fRadThick;
        num_fix_rad_min++;
        node.fRadThick = 0.001;
      }
    }
  }
  if (num_fix_rad_max != 0) {
    LOG(warn) << "CbmBbaAlignmentTask: radiation thickness is too large " << num_fix_rad_max << " times";
  }
  if (num_fix_rad_min != 0) {
    LOG(warn) << "CbmBbaAlignmentTask: radiation thickness is too small " << num_fix_rad_min << " times";
  }
  if (num_rejected_fit2 != 0) {
    LOG(warn) << "Rejected fit 2  " << num_rejected_fit2 << " tracks";
  }

  // ensure that all the hits are not too much deviated from the trajectory
  // (to be improved)
  unsigned num_rejected_traj = 0;
  for (TrackContainer& t : fTracks) {
    if (!t.fIsActive) continue;
    const double cutX = 8.;
    const double cutY = 8.;
    for (unsigned int in = 0; in < t.fUnalignedTrack.fNodes.size(); in++) {
      CbmKfTrackFitter::Trajectory tr        = t.fUnalignedTrack;
      CbmKfTrackFitter::TrajectoryNode& node = tr.fNodes[in];
      if (!node.fIsXySet) {
        continue;
      }
      node.fIsXySet = false;  // exclude the node from the fit
      if (!fFitter.FitTrajectory(tr)) {
        t.fIsActive = false;
        num_rejected_traj++;
        break;
      }
      double x_residual = node.fMxy.X() - node.fParamDn.X();  // this should be the difference vector
      double y_residual = node.fMxy.Y() - node.fParamDn.Y();
      double x_pull     = x_residual / sqrt(node.fMxy.Dx2() + node.fParamDn.GetCovariance(0, 0));
      double y_pull     = y_residual / sqrt(node.fMxy.Dy2() + node.fParamDn.GetCovariance(1, 1));
      if (!node.fIsFitted || (node.fMxy.NdfX() > 0. && fabs(x_pull) > cutX)
          || (node.fMxy.NdfY() > 0. && fabs(y_pull) > cutY)) {
        t.fIsActive = false;
        num_rejected_traj++;
        break;
      }
    }
  }
  if (num_rejected_traj != 0) {
    LOG(warn) << "Rejected  " << num_rejected_traj << " tracks for hits deviating from the trajectory";
  }

  TClonesArray* inputTracks = nullptr;
  if (fTrackingMode == kSts) {
    inputTracks = fInputStsTracks;
  }
  else if (fTrackingMode == kMcbm) {
    inputTracks = fInputGlobalTracks;
  }
  static int statTracks = 0;
  statTracks += inputTracks->GetEntriesFast();

  unsigned active_tracks = 0;
  for (auto& t : fTracks) {
    if (t.fIsActive) active_tracks++;
  }

  LOG(info) << "Bba: " << inputTracks->GetEntriesFast() << " tracks in event, " << statTracks << " tracks in total, "
            << fTracks.size() << " tracks selected for alignment, " << active_tracks << " tracks active";
}


// This function destroys alignment if stations are defined as alignment bodies, do not use
void CbmBbaAlignmentTask::ConstrainStation(std::vector<double>& par, int iSta, int ixyz)
{
  // set overall shifts of the station to zero

  double mean = 0.;
  int n       = 0;
  for (unsigned int i = 0; i < fAlignmentBodies.size(); i++) {
    if (fAlignmentBodies[i].fTrackingStation == iSta) {
      mean += par[3 * i + ixyz];
      n++;
    }
  }
  if (n <= 0) return;
  mean /= n;
  for (unsigned int i = 0; i < fAlignmentBodies.size(); i++) {
    if (fAlignmentBodies[i].fTrackingStation == iSta) {
      par[3 * i + ixyz] -= mean;
    }
  }
}

void CbmBbaAlignmentTask::ApplyConstraints(std::vector<double>& par)
{
  // apply alignment parameters to the hits
  ConstrainStation(par, 0, 2);
  ConstrainStation(par, fNtrackingStations - 1, 0);
  ConstrainStation(par, fNtrackingStations - 1, 1);
  ConstrainStation(par, fNtrackingStations - 1, 2);
}

// This function destroys alignment if stations are defined as alignment bodies, do not use
void CbmBbaAlignmentTask::ApplyAlignment(const std::vector<double>& par)
{
  // apply alignment parameters to the hits

  std::vector<double> parConstrained = par;

  //ApplyConstraints(parConstrained);

  for (TrackContainer& t : fTracks) {

    for (unsigned int in = 0; in < t.fUnalignedTrack.fNodes.size(); in++) {
      CbmKfTrackFitter::TrajectoryNode& node = t.fUnalignedTrack.fNodes[in];
      if (!node.fIsXySet) {
        continue;
      }
      CbmKfTrackFitter::TrajectoryNode& nodeAligned = t.fAlignedTrack.fNodes[in];
      int iSensor                                   = node.fReference1;
      assert(iSensor >= 0 && iSensor < (int) fSensors.size());
      assert(nodeAligned.fReference1 == node.fReference1);

      Sensor& s = fSensors[iSensor];
      if (s.fAlignmentBody < 0) continue;

      double dx = parConstrained[3 * s.fAlignmentBody + 0];
      double dy = parConstrained[3 * s.fAlignmentBody + 1];
      double dz = parConstrained[3 * s.fAlignmentBody + 2];

      // shift the hit
      nodeAligned.fMxy.SetX(node.fMxy.X() + dx);
      nodeAligned.fMxy.SetY(node.fMxy.Y() + dy);
      nodeAligned.fZ = node.fZ + dz;

      // shift the fitted track to the Z position of the hit
      {
        auto& p = nodeAligned.fParamDn;
        p.SetX(p.X() + p.Tx() * dz);
        p.SetY(p.Y() + p.Ty() * dz);
        p.SetZ(nodeAligned.fZ);
      }
      {
        auto& p = nodeAligned.fParamUp;
        p.SetX(p.X() + p.Tx() * dz);
        p.SetY(p.Y() + p.Ty() * dz);
        p.SetZ(nodeAligned.fZ);
      }
    }
  }

  static int statNCalls = 0;
  statNCalls++;
  if (statNCalls % 100 == 0) {
    std::vector<double>& r = parConstrained;
    LOG(info) << "Current parameters: ";
    for (int is = 0; is < fNalignmentBodies; is++) {
      auto& b = fAlignmentBodies[is];
      LOG(info) << "Body " << is << " sta " << b.fTrackingStation << ": x " << r[3 * is + 0] << " y " << r[3 * is + 1]
                << " z " << r[3 * is + 2];
    }
  }
}


double CbmBbaAlignmentTask::CostFunction(const std::vector<double>& par)
{
  // apply new parameters to the hits

  for (auto& t : fTracks) {
    t.fAlignedTrack = t.fUnalignedTrack;
  }

  ApplyAlignment(par);

  std::vector<double> chi2Thread(fNthreads, 0.);
  std::vector<long> ndfThread(fNthreads, 0);

  auto fitThread = [&](int iThread) {
    CbmKfTrackFitter fit(fFitter);
    fit.SetDoSmooth(false);
    fit.SetNoMultipleScattering();

    for (unsigned int itr = iThread; itr < fTracks.size(); itr += fNthreads) {
      auto& t = fTracks[itr];
      if (!t.fIsActive) continue;

      // fit.SetDebugInfo(" track " + std::to_string(itr));

      for (unsigned int in = 0; in < t.fAlignedTrack.fNodes.size(); in++) {
        CbmKfTrackFitter::TrajectoryNode& node = t.fAlignedTrack.fNodes[in];
        if (!node.fIsFitted) {
          LOG(fatal) << "BBA: Cost function: aligned node is not fitted! ";
        }
      }
      bool ok     = fit.FitTrajectory(t.fAlignedTrack);
      double chi2 = t.fAlignedTrack.fNodes.back().fParamDn.GetChiSq();
      double ndf  = t.fAlignedTrack.fNodes.back().fParamDn.GetNdf();
      if (!ok || !std::isfinite(chi2) || chi2 <= 0. || ndf <= 0.) {
        // TODO: deactivate the track and continue
        LOG(fatal) << "BBA: fit failed! ";
        chi2 = 0.;
        ndf  = 0.;
      }
      chi2Thread[iThread] += chi2;
      ndfThread[iThread] += (int) ndf;
    }
  };

  std::vector<std::thread> threads(fNthreads);

  // run n threads
  for (int i = 0; i < fNthreads; i++) {
    threads[i] = std::thread(fitThread, i);
  }

  // wait for the threads to finish
  for (auto& th : threads) {
    th.join();
  }

  fChi2Total = 0.;
  fNdfTotal  = 0;
  for (int i = 0; i < fNthreads; i++) {
    fChi2Total += chi2Thread[i];
    fNdfTotal += ndfThread[i];
  }

  double cost = fChi2Total / (fNdfTotal + 1);
  if (fFixedNdf > 0 && fNdfTotal != fFixedNdf) {
    cost = 1.e30;
  }
  //LOG(info) << "BBA: calculate cost function:  ndf " << fNdfTotal << ", cost " << cost
  //          << ", diff to ideal cost: " << cost - fCostIdeal << ", diff to initial cost: " << cost - fCostInitial;

  return cost;
}

void CbmBbaAlignmentTask::Finish()
{
  // perform the alignment

  LOG(info) << "BBA: start the alignment procedure with " << fTracks.size() << " tracks ...";

  // collect the sensors from the hits
  // TODO: read it from the detector setup

  if (fTrackingMode == kSts) {
    fNtrackingStations = CbmStsTrackingInterface::Instance()->GetNtrackingStations();
  }

  std::set<Sensor> sensorSet;
  for (auto& t : fTracks) {
    for (auto& n : t.fUnalignedTrack.fNodes) {
      if (!n.fIsXySet) {
        continue;
      }
      Sensor s;
      s.fSystemId = n.fHitSystemId;
      s.fSensorId = n.fHitAddress;
      // TODO: get the station index from n.fHitSystemId, n.fHitAddress
      s.fTrackingStation = n.fMaterialLayer;

      if (fTrackingMode == kSts && s.fSystemId == ECbmModuleId::kSts) {
        s.fTrackingStation = CbmStsTrackingInterface::Instance()->GetTrackingStationIndex(n.fHitAddress);
      }

      if (s.fSystemId == ECbmModuleId::kTrd || s.fSystemId == ECbmModuleId::kTrd2d) {
        s.fSensorId = s.fTrackingStation;
        // hardcode path to physical node in geometry
        if (s.fTrackingStation == 2) {
          s.fNodePath = "/cave_1/trd_v22h_mcbm_0/layer01_20101/module9_101001001";
        }
        else if (s.fTrackingStation == 3) {
          s.fNodePath = "/cave_1/trd_v22h_mcbm_0/layer02_10202/module8_101002001";
        }
        else if (s.fTrackingStation == 4) {
          s.fNodePath = "/cave_1/trd_v22h_mcbm_0/layer03_11303/module8_101303001";
        }
      }
      else if (s.fSystemId == ECbmModuleId::kTof) {
        s.fSensorId = CbmTofAddress::GetRpcFullId(n.fHitAddress);
      }

      sensorSet.insert(s);
    }
  }

  fSensors.clear();
  for (auto& s : sensorSet) {  // convert set to a vector
    fSensors.push_back(s);
  }
  std::sort(fSensors.begin(), fSensors.end(), [](const Sensor& a, const Sensor& b) { return a < b; });

  for (auto& t : fTracks) {
    for (auto& n : t.fUnalignedTrack.fNodes) {
      if (!n.fIsXySet) {
        continue;
      }
      Sensor s;
      s.fSystemId        = n.fHitSystemId;
      s.fSensorId        = n.fHitAddress;
      s.fTrackingStation = n.fMaterialLayer;

      if (fTrackingMode == kSts && s.fSystemId == ECbmModuleId::kSts) {
        s.fTrackingStation = CbmStsTrackingInterface::Instance()->GetTrackingStationIndex(n.fHitAddress);
      }

      if (s.fSystemId == ECbmModuleId::kTrd || s.fSystemId == ECbmModuleId::kTrd2d) {
        s.fSensorId = s.fTrackingStation;
      }
      else if (s.fSystemId == ECbmModuleId::kTof) {
        s.fSensorId = CbmTofAddress::GetRpcFullId(n.fHitAddress);
      }
      auto iter = std::lower_bound(  // find the sensor in the vector
        fSensors.begin(), fSensors.end(), s, [](const Sensor& a, const Sensor& b) { return a < b; });
      assert(iter != fSensors.end());
      assert(*iter == s);
      int iSensor   = std::distance(fSensors.begin(), iter);
      n.fReference1 = iSensor;
    }
    t.fAlignedTrack = t.fUnalignedTrack;
  }

  if (1) {  // one alignment body per tracking station

    fNalignmentBodies = fNtrackingStations;
    fAlignmentBodies.resize(fNalignmentBodies);
    for (int i = 0; i < fNalignmentBodies; i++) {
      fAlignmentBodies[i].fTrackingStation = i;
    }
    for (auto& s : fSensors) {
      s.fAlignmentBody = s.fTrackingStation;
    }
  }
  else {  // one alignment body per sensor

    fNalignmentBodies = fSensors.size();
    fAlignmentBodies.resize(fNalignmentBodies);
    for (int unsigned i = 0; i < fSensors.size(); i++) {
      fSensors[i].fAlignmentBody           = i;
      fAlignmentBodies[i].fTrackingStation = fSensors[i].fTrackingStation;
    }
  }

  LOG(info) << "BBA: " << fSensors.size() << " sensors, " << fNalignmentBodies << " alignment bodies";

  LOG(info) << "\n Sensors: ";
  {
    for (unsigned int is = 0; is < fSensors.size(); is++) {
      auto& s = fSensors[is];
      LOG(info) << "Sensor " << is << " sys " << s.fSystemId << " sta " << s.fTrackingStation << " body "
                << s.fAlignmentBody;
    }
  }

  LOG(info) << "\n Alignment bodies: ";
  {
    for (int is = 0; is < fNalignmentBodies; is++) {
      auto& b = fAlignmentBodies[is];
      LOG(info) << "Body: " << is << " station: " << b.fTrackingStation;
    }
  }

  int nParameters = 3 * fNalignmentBodies;  // x, y, z

  std::vector<bba::Parameter> par(nParameters);

  for (int ip = 0; ip < nParameters; ip++) {
    bba::Parameter& p = par[ip];
    p.SetActive(0);
    p.SetValue(0.);
    p.SetBoundaryMin(-2.);
    p.SetBoundaryMax(2.);
    p.SetStepMin(1.e-4);
    p.SetStepMax(0.1);
    p.SetStep(1.e-2);
  }

  // set active parameters
  // fix first and last station
  for (unsigned int ib = 0; ib < fAlignmentBodies.size(); ib++) {
    auto& b = fAlignmentBodies[ib];
    if ((fTrackingMode == kSts && b.fTrackingStation == 0)
        || (fTrackingMode == kSts && b.fTrackingStation == fNtrackingStations - 1)) {
      par[3 * ib + 0].SetActive(0);
      par[3 * ib + 1].SetActive(0);
      par[3 * ib + 2].SetActive(0);
    }
    else {
      par[3 * ib + 0].SetActive(1);
      par[3 * ib + 1].SetActive(1);
      par[3 * ib + 2].SetActive(1);
    }
    // fix x in the middle station
    if (fTrackingMode == kSts && b.fTrackingStation == fNtrackingStations / 2) {
      par[3 * ib + 0].SetActive(0);
    }
  }


  // set parameter ranges for different subsystems
  for (const auto& s : fSensors) {
    int ib = s.fAlignmentBody;
    if (ib < 0 || ib >= fNalignmentBodies) continue;
    //auto& b = fAlignmentBodies[ib];
    for (int j = 0; j < 3; j++) {
      bba::Parameter& p = par[ib * 3 + j];
      p.SetStepMin(1.e-4);
      if (s.fSystemId == ECbmModuleId::kTrd || s.fSystemId == ECbmModuleId::kTrd2d) {
        p.SetStepMin(10.e-4);
      }
      else if (s.fSystemId == ECbmModuleId::kTof) {
        p.SetStepMin(10.e-4);
      }
    }
  }

  gRandom->SetSeed(1);
  LOG(info) << "Simulated misalignment range: " << fSimulatedMisalignmentRange;
  double d = fSimulatedMisalignmentRange;

  for (int is = 0; is < fNalignmentBodies; is++) {
    bba::Parameter& px = par[3 * is + 0];
    bba::Parameter& py = par[3 * is + 1];
    bba::Parameter& pz = par[3 * is + 2];

    // +- d cm misalignment
    if (px.IsActive()) {
      px.SetValue(gRandom->Uniform(2. * d) - d);
    }
    if (py.IsActive()) {
      py.SetValue(gRandom->Uniform(2. * d) - d);
    }
    if (pz.IsActive()) {
      pz.SetValue(gRandom->Uniform(2. * d) - d);
    }
  }

  if (0) {  //  test
    LOG(info) << "STS sensor paths: ";
    for (auto& s : fSensors) {
      if (s.fSystemId != ECbmModuleId::kSts) {
        continue;
      }
      const CbmStsElement* el = CbmStsSetup::Instance()->GetElement(s.fSensorId, kStsSensor);
      if (!el) {
        LOG(error) << "Element not found: " << s.fSensorId;
        continue;
      }
      el->Print();
      const auto* n = el->GetPnode();
      if (n) {
        LOG(info) << "Node: ";
        n->Print();
      }
    }
  }

  bba::BBA alignment;

  alignment.setParameters(par);

  auto lambdaCostFunction = [this](const std::vector<double>& p) { return CostFunction(p); };

  alignment.setChi2(lambdaCostFunction);
  alignment.printInfo();

  std::vector<double> parIdeal;
  std::vector<double> parMisaligned;
  {
    const std::vector<double>& r = alignment.getResult();
    for (unsigned int i = 0; i < r.size(); i++) {
      parMisaligned.push_back(r[i]);
      parIdeal.push_back(0.);
    }
  }

  {
    std::cout << "initial cost function..." << std::endl;

    fCostInitial = CostFunction(parMisaligned);

    unsigned tracks_rejected = 0;
    for (auto& t : fTracks) {
      double ndf  = t.fAlignedTrack.fNodes.back().fParamDn.GetNdf();
      double chi2 = t.fAlignedTrack.fNodes.back().fParamDn.GetChiSq();
      if (ndf < 0. || chi2 < 0. || !std::isfinite(ndf) || !std::isfinite(chi2)) {
        t.fIsActive = false;
        tracks_rejected++;
      }
    }
    std::cout << "reject " << tracks_rejected << " bad tracks and recalculate the initial cost function..."
              << std::endl;
    fCostInitial = CostFunction(parMisaligned);
    fFixedNdf    = -1;  //fNdfTotal;

    // plot the residuals before alignment

    for (const auto& t : fTracks) {
      if (!t.fIsActive) continue;
      // calculate the residuals for all tracks at each TrajectoryNode before alignment
      for (unsigned int in = 0; in < t.fAlignedTrack.fNodes.size(); in++) {
        CbmKfTrackFitter::Trajectory tr        = t.fAlignedTrack;
        CbmKfTrackFitter::TrajectoryNode& node = tr.fNodes[in];

        if (!node.fIsXySet) {
          continue;
        }
        node.fIsXySet = false;

        fFitter.FitTrajectory(tr);

        if (!node.fIsFitted) {
          continue;
        }

        Sensor& s = fSensors[node.fReference1];

        double x_residual = node.fMxy.X() - node.fParamDn.X();  // this should be the difference vector
        double y_residual = node.fMxy.Y() - node.fParamDn.Y();
        double x_pull     = x_residual / sqrt(node.fMxy.Dx2() + node.fParamDn.GetCovariance(0, 0));
        double y_pull     = y_residual / sqrt(node.fMxy.Dy2() + node.fParamDn.GetCovariance(1, 1));

        if (node.fMxy.NdfX() > 0) {
          hResidualsBeforeAlignmentX[0]->Fill(x_residual);
          hPullsBeforeAlignmentX[0]->Fill(x_pull);
        }
        if (node.fMxy.NdfY() > 0) {
          hResidualsBeforeAlignmentY[0]->Fill(y_residual);
          hPullsBeforeAlignmentY[0]->Fill(y_pull);
        }

        hResidualsBeforeAlignmentX[s.fTrackingStation + 1]->Fill(x_residual);
        hResidualsBeforeAlignmentY[s.fTrackingStation + 1]->Fill(y_residual);

        hPullsBeforeAlignmentX[s.fTrackingStation + 1]->Fill(x_pull);
        hPullsBeforeAlignmentY[s.fTrackingStation + 1]->Fill(y_pull);
      }
    }
  }

  std::cout << "calculate ideal cost function..." << std::endl;
  fCostIdeal = CostFunction(parIdeal);


  LOG(info) << " Cost function for the true parameters: " << fCostIdeal;
  LOG(info) << " Cost function for the initial parameters: " << fCostInitial;

  // run the alignment
  alignment.align();

  double costResult = CostFunction(alignment.getResult());

  CostFunction(alignment.getResult());
  CostFunction(alignment.getResult());

  // Create alignment matrices:
  std::vector<double> result = alignment.getResult();
  std::map<std::string, TGeoHMatrix> alignmentMatrices;

  for (auto& s : fSensors) {
    int i = s.fAlignmentBody;
    assert(i < fNalignmentBodies);
    if (i < 0) continue;

    // For STS sensors
    if (s.fSystemId == ECbmModuleId::kSts) {
      // get sensor for sensorID
      const CbmStsElement* el = CbmStsSetup::Instance()->GetElement(s.fSensorId, kStsSensor);
      if (!el) {
        LOG(error) << "Element not found: " << s.fSensorId;
        continue;
      }
      const auto* n = el->GetPnode();
      LOG(debug) << "Node: ";
      LOG(debug) << n->GetName();
      alignmentMatrices.insert(
        AlignNode(n->GetName(), result[i * 3 + 0], result[i * 3 + 1], result[i * 3 + 2], 0., 0., 0.));
    }
    // for Trd stations
    else if (s.fSystemId == ECbmModuleId::kTrd) {
      LOG(debug) << "Node: ";
      LOG(debug) << s.fNodePath;
      alignmentMatrices.insert(
        AlignNode(s.fNodePath, result[i * 3 + 0], result[i * 3 + 1], result[i * 3 + 2], 0., 0., 0.));
    }
    // for Trd2D station
    else if (s.fSystemId == ECbmModuleId::kTrd2d) {
      LOG(debug) << "Node: ";
      LOG(debug) << s.fNodePath;
      alignmentMatrices.insert(
        AlignNode(s.fNodePath, result[i * 3 + 0], result[i * 3 + 1], result[i * 3 + 2], 0., 0., 0.));
    }
    // TODO:  for TOF stations
    // else {
    //   LOG(info) << "fSystemId: " << s.fSystemId << "\tfTrackongStation: " << s.fTrackingStation
    //             << "\t SensorId: " << s.fSensorId;
    //   alignmentMatrices.insert(AlignNode(Form("fTrackingStation %i", s.fTrackingStation), result[i * 3 + 0],
    //                                      result[i * 3 + 1], result[i * 3 + 2], 0., 0., 0.));
    // }
  }  // sensors

  // save matrices to disk
  TFile* misalignmentMatrixRootfile =
    new TFile("AlignmentMatrices_mcbm_beam_2022_05_23_nickel_finetuning.root", "RECREATE");
  if (misalignmentMatrixRootfile->IsOpen()) {
    gDirectory->WriteObject(&alignmentMatrices, "AlignmentMatrices");
    misalignmentMatrixRootfile->Write();
    misalignmentMatrixRootfile->Close();
  }

  LOG(info) << "\n Misaligned parameters: ";
  for (int is = 0; is < fNalignmentBodies; is++) {
    const std::vector<double>& r = parMisaligned;
    LOG(info) << "Body " << is << ": x " << r[3 * is + 0] << " y " << r[3 * is + 1] << " z " << r[3 * is + 2];
  }

  LOG(info) << "\n Alignment results: ";
  {
    const std::vector<double>& r = alignment.getResult();
    for (int is = 0; is < fNalignmentBodies; is++) {
      auto& b = fAlignmentBodies[is];
      LOG(info) << "Body " << is << " sta " << b.fTrackingStation << ": x " << r[3 * is + 0] << " y " << r[3 * is + 1]
                << " z " << r[3 * is + 2];
    }
  }

  LOG(info) << "\n Alignment results, constrained: ";
  {
    std::vector<double> r = alignment.getResult();
    //ApplyConstraints(r);
    for (int is = 0; is < fNalignmentBodies; is++) {
      auto& b = fAlignmentBodies[is];
      LOG(info) << "Body " << is << " sta " << b.fTrackingStation << ": x " << r[3 * is + 0] << " y " << r[3 * is + 1]
                << " z " << r[3 * is + 2];
    }
  }

  unsigned active_tracks = 0;
  for (auto& t : fTracks) {
    if (t.fIsActive) active_tracks++;
  }

  LOG(info) << "\n";

  LOG(info) << "Bba: " << fTracks.size() << " tracks used for alignment\n";
  LOG(info) << "     " << active_tracks << " tracks active\n";

  LOG(info) << " Cost function for the true parameters: " << fCostIdeal;
  LOG(info) << " Cost function for the initial parameters: " << fCostInitial;
  LOG(info) << " Cost function for the aligned parameters: " << costResult;

  for (auto& t : fTracks) {
    if (!t.fIsActive) continue;
    // calculate the residuals for all tracks at each TrajectoryNode after alignment
    // TODO: put into function
    for (unsigned int in = 0; in < t.fAlignedTrack.fNodes.size(); in++) {
      CbmKfTrackFitter::Trajectory tr        = t.fAlignedTrack;
      CbmKfTrackFitter::TrajectoryNode& node = tr.fNodes[in];
      if (!node.fIsXySet) {
        continue;
      }
      node.fIsXySet = false;
      fFitter.FitTrajectory(tr);

      Sensor& s = fSensors[node.fReference1];

      double x_residual = node.fMxy.X() - node.fParamDn.X();
      double y_residual = node.fMxy.Y() - node.fParamDn.Y();
      double x_pull     = x_residual / sqrt(node.fMxy.Dx2() + node.fParamDn.GetCovariance(0, 0));
      double y_pull     = y_residual / sqrt(node.fMxy.Dy2() + node.fParamDn.GetCovariance(1, 1));

      if (node.fMxy.NdfX() > 0) {
        hResidualsAfterAlignmentX[0]->Fill(x_residual);
        hPullsAfterAlignmentX[0]->Fill(x_pull);
      }
      if (node.fMxy.NdfY() > 0) {
        hResidualsAfterAlignmentY[0]->Fill(y_residual);
        hPullsAfterAlignmentY[0]->Fill(y_pull);
      }

      hResidualsAfterAlignmentX[s.fTrackingStation + 1]->Fill(x_residual);
      hResidualsAfterAlignmentY[s.fTrackingStation + 1]->Fill(y_residual);

      hPullsAfterAlignmentX[s.fTrackingStation + 1]->Fill(x_pull);
      hPullsAfterAlignmentY[s.fTrackingStation + 1]->Fill(y_pull);
    }
  }

  // store the histograms

  TDirectory* curr   = gDirectory;
  TFile* currentFile = gFile;
  // Open output file and write histograms

  fHistoFile->cd();
  WriteHistosCurFile(fHistoDir);
  if (!(fHistoFileName == "")) {
    fHistoFile->Close();
    fHistoFile->Delete();
  }
  gFile      = currentFile;
  gDirectory = curr;
}


void CbmBbaAlignmentTask::WriteHistosCurFile(TObject* obj)
{
  if (!obj->IsFolder())
    obj->Write();
  else {
    TDirectory* cur    = gDirectory;
    TFile* currentFile = gFile;

    TDirectory* sub = cur->GetDirectory(obj->GetName());
    sub->cd();
    TList* listSub = (static_cast<TDirectory*>(obj))->GetList();
    TIter it(listSub);
    while (TObject* obj1 = it()) {
      WriteHistosCurFile(obj1);
    }
    cur->cd();
    gFile      = currentFile;
    gDirectory = cur;
  }
}

ClassImp(CbmBbaAlignmentTask);
