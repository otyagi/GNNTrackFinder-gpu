/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmKFV0FinderTask.cxx
/// \brief  A FairTask for V0 candidates finding in mCBM (implementation)
/// \since  10.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CbmKFV0FinderTask.h"

#include "CbmEvent.h"
#include "CbmEventTriggers.h"
#include "CbmGlobalTrack.h"
#include "CbmKfTarget.h"
#include "CbmStsHit.h"
#include "CbmStsTrack.h"
#include "CbmTofAddress.h"
#include "CbmTofHit.h"
#include "CbmTofTrack.h"
#include "CbmTrdHit.h"
#include "CbmTrdTrack.h"
#include "FairRunAna.h"
#include "KFPTrackVector.h"
#include "KFVertex.h"
#include "KfpV0FinderConfig.h"
#include "Logger.h"
#include "TClonesArray.h"
#include "TMatrixTSym.h"
#include "yaml/Yaml.h"

#include <boost/filesystem.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

// Log macros:
#define ERR_ LOG(error) << fName << ": "
#define LOG_(SEVERITY, VERBOSITY) LOG_IF(SEVERITY, fVerbose >= VERBOSITY) << fName << ": "

using cbm::kfp::V0FinderTask;

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderTask::ApplyConfiguration()
{
  namespace fs        = boost::filesystem;
  namespace yml       = cbm::algo::yaml;
  fs::path configPath = fsConfigName.Data();
  if (!fs::exists(configPath)) {
    std::stringstream msg;
    msg << fName << ": configuration file " << configPath.string() << " does not exist";
    throw std::runtime_error(msg.str());
  }

  LOG_(info, 1) << "applying configuration from " << configPath.string();
  auto config = yml::ReadFromFile<cbm::algo::kfp::V0FinderConfig>(configPath);

  LOG_(info, 1) << config.ToString();


  // -- Read the config
  if (config.reconstructPdg != 3122) {  // At the moment only Lambda analysis is possible
    std::stringstream msg;
    msg << fName << ": at the moment only lambda finding is possible. Provided PDG: " << config.reconstructPdg;
    throw std::runtime_error(msg.str());
  }

  // Check daughter particles:
  auto& particles = config.cuts.particles;
  int iPion       = -1;
  int iProton     = -1;
  for (int iPart = 0; iPart < int(particles.size()); ++iPart) {
    const auto& particle = particles[iPart];
    if (particle.pdg == -211) {
      if (iPion == -1) {
        iPion = iPart;
      }
      else {
        std::stringstream msg;
        msg << fName << ": pion entry is defined more then one time in the config.cuts.particles";
        throw std::runtime_error(msg.str());
      }
    }
    else if (particle.pdg == 2212) {
      if (iProton == -1) {
        iProton = iPart;
      }
      else {
        std::stringstream msg;
        msg << fName << ": proton entry is defined more then one time in the config.cuts.particles";
        throw std::runtime_error(msg.str());
      }
    }
  }
  if (iProton == -1 || iPion == -1) {
    std::stringstream msg;
    msg << fName << ": config cuts/particles: either pion or proton settings are not found";
    throw std::runtime_error(msg.str());
  }

  const auto& pion{particles[iPion]};
  const auto& proton{particles[iProton]};

  SetMinPionDca(pion.minDca);
  SetPionVelocityRange(pion.minVelocity, pion.maxVelocity);
  SetMinProtonDca(proton.minDca);
  SetProtonVelocityRange(proton.minVelocity, proton.maxVelocity);

  SetTzeroOffset(config.tZeroOffset);
  SetQpAssignedUncertainty(config.qpAssignedUncertainty);
  AddDecayToReconstructionList(config.reconstructPdg);  // Lambda

  fPrimaryAssignedPdg = config.primaryAssignedPdg;

  // KFParticleFinder cuts:
  auto& kfpCuts = config.cuts.kfp;
  SetChiPrimaryCut2D(kfpCuts.maxChi2NdfPrim);
  SetLdLCut2D(kfpCuts.minDecayLDL);
  SetLCut(kfpCuts.minDecayLength);  // 5cm cut
  SetChi2Cut2D(kfpCuts.maxChi2NdfGeo);
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool V0FinderTask::AssignMomentum(CbmGlobalTrack* pTrack, int pdg)
{
  const int iTofTrk = pTrack->GetTofTrackIndex();
  if (iTofTrk < 0) {
    ++fCounters[ECounter::TracksWoTofHits];
    return false;  // Skip tracks, which do not have hits in TOF
  }

  const auto* pTofTrack{static_cast<const CbmTofTrack*>(fpBrTofTracks->At(iTofTrk))};
  assert(pTofTrack);

  int nTofHits = pTofTrack->GetNofTofHits();
  if (nTofHits < 1) {
    // Must not be called
    return false;
  }
  ++fCounters[ECounter::TracksWAtLeastOneTofHit];

  //int iFstTofHit = pTofTrack->GetTofHitIndex(0);
  int iLstTofHit = pTofTrack->GetTofHitIndex(nTofHits - 1);  // last hit in TOF

  const auto* pLstTofHit{static_cast<CbmTofHit*>(fpBrTofHits->At(iLstTofHit))};
  if (fbRunQa) {
    fpQa->fph_tof_lst_hit_time->Fill(pLstTofHit->GetTime());
  }
  assert(pLstTofHit);

  if (pLstTofHit->GetTime() < 0) {  //< wrongly calibrated T0
    ++fCounters[ECounter::TracksWNegativeTofHitTime];
    return false;
  }

  auto qpAndBeta = EstimateQp(pLstTofHit, pdg);
  if (fbRunQa) {
    fpQa->fph_beta_all->Fill(qpAndBeta.fBeta);
  }

  auto& beta = qpAndBeta.fBeta;
  if (qpAndBeta.fBeta > 1) {
    ++fCounters[ECounter::TracksWithUnphysicalBeta];
    return false;  // unphysical track
  }

  // Extra cut on pion and proton velocity (TODO: maybe put this cut earlier)
  if ((pdg == -211 && (beta > fMaxBetaPion || beta < fMinBetaPion))
      || (pdg == 2212 && (beta > fMaxBetaProton || beta < fMinBetaProton))) {
    return false;
  }


  // Update the track with the PID hypothesis and momentum
  FairTrackParam parFst(*pTrack->GetParamFirst());
  FairTrackParam parLst(*pTrack->GetParamLast());

  parFst.SetQp(qpAndBeta.fQp);
  parLst.SetQp(qpAndBeta.fQp);
  parFst.SetCovariance(4, 4, qpAndBeta.fQpVar);
  parFst.SetCovariance(4, 4, qpAndBeta.fQpVar);

  pTrack->SetParamFirst(&parFst);
  pTrack->SetParamLast(&parLst);
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
template<bool UseEvent>
bool V0FinderTask::ProcessEvent(const CbmEvent* pEvent)
{
  int nTracks = UseEvent ? pEvent->GetNofData(ECbmDataType::kGlobalTrack) : fpBrGlobalTracks->GetEntriesFast();
  fCounters[ECounter::TracksTotal] += nTracks;


  // Local event counters
  int nProtons{0};
  int nPions{0};
  int nTracksSelected{0};

  // Local containers (per event)
  std::vector<const CbmGlobalTrack*> vpSelectedTracks;  // Selected tracks [n selected tracks]
  vpSelectedTracks.reserve(nTracks);
  std::vector<int> vSelectedTrackIds;  // Indices of selected tracks [n selected tracks]
  vSelectedTrackIds.reserve(nTracks);

  // ----- Shift TOF hit times to t0
  if constexpr (UseEvent) {
    auto t0 = ShiftTofHitsToTzero(pEvent);
    if (std::isnan(t0)) {
      ++fCounters[ECounter::EventsWoTzero];
      return false;
    }
  }

  // ----- Select tracks
  for (int iTrkEvt{0}; iTrkEvt < nTracks; ++iTrkEvt) {
    const int iTrk = UseEvent ? pEvent->GetIndex(ECbmDataType::kGlobalTrack, iTrkEvt) : iTrkEvt;
    auto* pGlobalTrack{static_cast<CbmGlobalTrack*>(fpBrGlobalTracks->At(iTrk))};
    if (!SelectTrack(pGlobalTrack, iTrk)) {
      continue;
    }
    ++nTracksSelected;
    switch (pGlobalTrack->GetPidHypo()) {
      case -211:  // pions
        ++nPions;
        break;
      case 2212:  // protons
        ++nProtons;
        break;
    }
    vpSelectedTracks.push_back(pGlobalTrack);
    vSelectedTrackIds.push_back(iTrk);
  }  // END LOOP: tracks in event

  if (nProtons < 1 || nPions < 1) {
    return false;  // Nothing to search for in the event
  }

  fCounters[ECounter::TracksSelected] += nTracksSelected;
  fCounters[ECounter::Pions] += nPions;
  fCounters[ECounter::Protons] += nProtons;

  ++fCounters[ECounter::EventsLambdaCand];

  // NOTE: The variable is used to assign a KfParticle to a corresponding primary vertex:
  //        a) pvChi2 < 3 => primary particle
  //        b) pvChi2 > 3 => secondary particle
  // Here we assign all particles as secondary (TODO: test and try to separate primary and secondary particles)
  std::vector<float> vChi2ToPv;             // Chi-square to primary vertex (NOTE: not used)
  vChi2ToPv.resize(nTracksSelected, 999.);  // Assign all tracks as secondary

  KFPTrackVector kfpTracksFst = MakeKfpTrackVector(vpSelectedTracks, vSelectedTrackIds, vChi2ToPv, true);
  KFPTrackVector kfpTracksLst = MakeKfpTrackVector(vpSelectedTracks, vSelectedTrackIds, vChi2ToPv, false);

  // ----- Reconstruct topology in the event
  fpTopoReconstructorEvent->Clear();
  fpTopoReconstructorEvent->Init(kfpTracksFst, kfpTracksLst);
  if (EPvUsageMode::Target == fPvUsageMode) {
    float x{float(fpOrigin->GetX())};
    float y{float(fpOrigin->GetY())};
    float z{float(fpOrigin->GetZ())};
    // NOTE: the target size can be applied as a covariance matrix, now it has all nulls
    fpTopoReconstructorEvent->AddPV(MakeKfpPrimaryVertex(x, y, z), std::vector<int>{});
  }
  fpTopoReconstructorEvent->SortTracks();
  fpTopoReconstructorEvent->ReconstructParticles();

  // ----- Count number of found lambda-candidates
  int nLambda = 0;
  //std::count_if(particles.begin(), particles.end(), [](const auto& p) { return p.GetPDG() == 3122; });
  for (const auto& particle : fpTopoReconstructorEvent->GetParticles()) {
    if (particle.GetPDG() == 3122) {
      ++nLambda;
      if (fbRunQa) {
        fpQa->fph_lambda_cand_mass->Fill(particle.GetMass());
      }
    }
  }
  fCounters[ECounter::KfpLambdaCandidates] += nLambda;
  if (nLambda > 0) {
    auto& triggers = (*fpBrEventTriggers)[pEvent ? pEvent->GetNumber() : 0];
    triggers.Set(CbmEventTriggers::ETrigger::Lambda);
    ++fCounters[ECounter::KfpEventsLambdaCand];
  }

  // ----- Store particles to the run topology
  StoreParticles();

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool V0FinderTask::CheckTrackParam(const FairTrackParam* pParam)
{
  bool bOk{true};
  bOk &= std::isfinite(pParam->GetX());
  bOk &= std::isfinite(pParam->GetY());
  bOk &= std::isfinite(pParam->GetZ());
  bOk &= std::isfinite(pParam->GetTx());
  bOk &= std::isfinite(pParam->GetTy());
  bOk &= std::isfinite(pParam->GetQp());
  std::array<double, 15> covMatrix = {0.};
  if (bOk) {
    for (int i = 0, iCov = 0; i < 5; ++i) {
      for (int j = i; j < 5; j++, iCov++) {
        // NOTE: In the FairTrackParam::GetCovariance(i, j) one can exchange the i and j indices, but for more
        //       efficient calculation it is recommended to keep i < j.
        covMatrix[iCov] = pParam->GetCovariance(i, j);
        bOk &= std::isfinite(covMatrix[iCov]);
      }
    }
  }
  return bOk;
}

// ---------------------------------------------------------------------------------------------------------------------
//
V0FinderTask::DcaVector V0FinderTask::EstimateDcaToOrigin(const CbmStsTrack* pStsTrack) const
{
  DcaVector res;
  if (pStsTrack->GetNofStsHits() < 2) {
    // Too few STS hits
    return res;
  }
  const auto* pHitFst{static_cast<const CbmStsHit*>(fpBrStsHits->At(pStsTrack->GetStsHitIndex(0)))};  // first hit
  const auto* pHitSnd{static_cast<const CbmStsHit*>(fpBrStsHits->At(pStsTrack->GetStsHitIndex(1)))};  // second hit
  assert(pHitFst);
  assert(pHitSnd);
  double factor{(pHitFst->GetZ() - fpOrigin->GetZ()) / (pHitSnd->GetZ() - pHitFst->GetZ())};
  res.fX   = pHitFst->GetX() - fpOrigin->GetX() - factor * (pHitSnd->GetX() - pHitFst->GetX());
  res.fY   = pHitFst->GetY() - fpOrigin->GetY() - factor * (pHitSnd->GetY() - pHitFst->GetY());
  res.fAbs = std::sqrt(res.fX * res.fX + res.fY * res.fY);
  res.fX /= res.fAbs;
  res.fY /= res.fAbs;
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
V0FinderTask::QpAndBeta V0FinderTask::EstimateQp(const CbmTofHit* pTofHit, int pdg) const
{
  // FIXME: For now we consider the origin as a precise measurement, which is not correct. The corresponding
  //        uncertainty of the origin must be accounted here.
  //        Also: redefine origin with reconstructed or MC PV (in other function).
  //        Also: the t0 uncertainty should be estimated and accounted in the momentum uncertainty
  QpAndBeta res;
  double x{pTofHit->GetX() - fpOrigin->GetX()};
  double y{pTofHit->GetY() - fpOrigin->GetY()};
  double z{pTofHit->GetZ() - fpOrigin->GetZ()};
  double x2{x * x};
  double y2{y * y};
  double z2{z * z};
  double t2{pTofHit->GetTime() * pTofHit->GetTime()};
  double r2{x2 + y2 + z2};
  double r4{r2 * r2};
  // FIXME: In general case, the x, y, z coordinates of the origin can have non-zero covariances, which should be
  //        accounted in the calculation.
  double xVar{pTofHit->GetDx() * pTofHit->GetDx() + fpOrigin->GetCovariance(0, 0)};
  double yVar{pTofHit->GetDy() * pTofHit->GetDy() + fpOrigin->GetCovariance(1, 1)};
  double zVar{pTofHit->GetDz() * pTofHit->GetDz() + fpOrigin->GetCovariance(2, 2)};
  double tVar{pTofHit->GetTimeError() * pTofHit->GetTimeError()};
  double factor{(x2 * xVar + y2 * yVar + z2 * zVar) / r4 + tVar / t2};
  res.fBeta    = std::sqrt(r2) / (pTofHit->GetTime() * kSpeedOfLight);
  res.fBetaVar = res.fBeta * factor;
  if (res.fBeta >= 1) {
    return res;  //< Non-physical beta
    // FIXME: I would set beta to fixed large value, like 0.999999 for this case
  }
  double gamma{1. / std::sqrt(1. - res.fBeta * res.fBeta)};
  double m{0};
  double q{0};
  switch (pdg) {
    case -211:
      m = kPionMass;
      q = -1;
      break;
    case 2212:
      m = kProtonMass;
      q = 1;
      break;
    default:
      res.fQp    = -1. / 99999.;  //< Give small momentum to primary tracks
      res.fQpVar = res.fQp * res.fQp * fQpAssignedUncertainty * fQpAssignedUncertainty;
      return res;
      // NOTE: it would be more straight forward to assign the primary tracks a mass of pion and keep it
  }
  res.fQp = q / (gamma * res.fBeta * m);
  if (fQpAssignedUncertainty > 0) {
    res.fQpVar = res.fQp * res.fQp * fQpAssignedUncertainty * fQpAssignedUncertainty;
  }
  else {
    res.fQpVar = q * q / (m * m * res.fBeta * res.fBeta) * factor;
  }
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderTask::Exec(Option_t*)
{
  fvTrackDca.clear();
  fvTrackDca.resize(fpBrGlobalTracks->GetEntriesFast());

  // ----- Reset data containers per timeslice
  fpTopoReconstructorRun->Clear();

  // ----- Process timeslice
  if (EProcessingMode::EventBased == fProcessingMode) {
    ++fCounters[ECounter::EventsTotal];
    fpBrEventTriggers->clear();
    fpBrEventTriggers->resize(1);
    ProcessEvent</*UseEvent = */ false>(nullptr);
  }
  else {
    const int nEvents{fpBrRecoEvents->GetEntriesFast()};

    fCounters[ECounter::EventsTotal] += nEvents;
    fpBrEventTriggers->clear();
    fpBrEventTriggers->resize(nEvents);
    for (int iEvent = 0; iEvent < nEvents; ++iEvent) {
      const auto* pEvent = static_cast<CbmEvent*>(fpBrRecoEvents->At(iEvent));
      ProcessEvent</*UseEvent = */ true>(pEvent);
    }
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderTask::Finish()
{
  // ----- Store QA histograms
  if (fbRunQa) {
    fpQa->WriteHistograms(fsQaOutputName);
  }

  // ----- Counters summary log
  std::stringstream msg;
  msg << "*** Counters summary ***\n";
  msg << "\n  tracks total:                         " << fCounters[ECounter::TracksTotal];
  msg << "\n  tracks selected:                      " << fCounters[ECounter::TracksSelected];
  msg << "\n  tracks w/ infinit parameters:         " << fCounters[ECounter::TracksInfiniteParam];
  msg << "\n  tracks w/o TOF hits:                  " << fCounters[ECounter::TracksWoTofHits];
  msg << "\n  tracks w/ last TOF hit having t<0:    " << fCounters[ECounter::TracksWNegativeTofHitTime];
  msg << "\n  tracks w/o STS hits:                  " << fCounters[ECounter::TracksWoStsHits];
  msg << "\n  tracks w/o PID:                       " << fCounters[ECounter::TracksWoPid];
  msg << "\n  tracks w/o momentum:                  " << fCounters[ECounter::TracksWoMomentum];
  msg << "\n  tracks w/ at least one TOF hit:       " << fCounters[ECounter::TracksWAtLeastOneTofHit];
  msg << "\n  tracks w/ at least two TOF hits:      " << fCounters[ECounter::TracksWAtLeastTwoTofHits];
  msg << "\n  tracks w/ beta > 1                    " << fCounters[ECounter::TracksWithUnphysicalBeta];
  msg << "\n  pion candidates:                      " << fCounters[ECounter::Pions];
  msg << "\n  proton candidates:                    " << fCounters[ECounter::Protons];
  msg << "\n  events total:                         " << fCounters[ECounter::EventsTotal];
  msg << "\n  events w/o t-zero:                    " << fCounters[ECounter::EventsWoTzero];
  msg << "\n  events w/ potential Lambda-candidate: " << fCounters[ECounter::EventsLambdaCand];
  msg << "\n  events w/ Lambda-candidate from KFPF: " << fCounters[ECounter::KfpEventsLambdaCand];
  msg << "\n  Lambda-candidates:                    " << fCounters[ECounter::KfpLambdaCandidates];
  LOG_(info, 1) << msg.str();
}

// ---------------------------------------------------------------------------------------------------------------------
//
int V0FinderTask::InferTrackPidTopo(double dca) const
{
  if (std::isnan(dca)) {
    return -2;  //< Track PID cannot be infered
  }
  if (dca > fMinPionDca) {
    return -211;  // pi-
  }
  else if (dca > fMinProtonDca) {
    return 2212;  // proton
  }
  return kPrimaryPdg;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InitStatus V0FinderTask::Init()
{
  LOG_(info, 1) << "initializing the task ... ";

  LOG_(info, 1) << "using the following configuration:\n  processing mode: " << ToString(fProcessingMode)
                << "\n  PID approach: " << ToString(fPidApproach) << "\n  " << ToString(fPvUsageMode);

  // Temporary not implemented:
  if (EPidApproach::Mc == fPidApproach || EPvUsageMode::Mc == fPvUsageMode
      || EPvUsageMode::Reconstructed == fPvUsageMode || EPvUsageMode::ReconstructSingle == fPvUsageMode
      || EPvUsageMode::ReconstructMultiple == fPvUsageMode) {
    ERR_ << "Desirable configuration of PID or PV handling was not implemented yet";
    return kFATAL;
  }

  if (!fsConfigName.IsNull()) {
    try {
      ApplyConfiguration();
    }
    catch (const std::exception& err) {
      ERR_ << "configuration from a config was required, but failed. Reason: " << err.what();
      return kFATAL;
    }
  }

  // ----- Input data branches initialization
  const auto* pTarget = kf::Target::Instance();  // CBM target info

  auto* pFairManager = FairRootManager::Instance();
  if (!pFairManager) {
    ERR_ << "FairRootManager not found";
    return kERROR;
  }

  auto InitBranch = [&](TClonesArray*& branch, const char* name) -> bool {
    branch = dynamic_cast<TClonesArray*>(pFairManager->GetObject(name));
    if (!branch) {
      ERR_ << "branch \"" << name << "\" not found";
      return false;
    }
    return true;
  };

  bool bBranchesInitialized{true};
  if (EProcessingMode::TimeBased == fProcessingMode) {
    bBranchesInitialized = InitBranch(fpBrRecoEvents, "CbmEvent") && bBranchesInitialized;
  }
  bBranchesInitialized = InitBranch(fpBrGlobalTracks, "GlobalTrack") && bBranchesInitialized;
  bBranchesInitialized = InitBranch(fpBrStsTracks, "StsTrack") && bBranchesInitialized;
  bBranchesInitialized = InitBranch(fpBrTrdTracks, "TrdTrack") && bBranchesInitialized;
  bBranchesInitialized = InitBranch(fpBrTofTracks, "TofTrack") && bBranchesInitialized;
  bBranchesInitialized = InitBranch(fpBrStsHits, "StsHit") && bBranchesInitialized;
  bBranchesInitialized = InitBranch(fpBrTrdHits, "TrdHit") && bBranchesInitialized;
  bBranchesInitialized = InitBranch(fpBrTofHits, "TofHit") && bBranchesInitialized;

  if (EPvUsageMode::Reconstructed == fPvUsageMode) {
    fpBrPrimaryVertex = dynamic_cast<CbmVertex*>(pFairManager->GetObject("PrimaryVertex."));
    if (!fpBrPrimaryVertex) {
      ERR_ << "branch \"PrimaryVertex.\" not found";
      bBranchesInitialized = false;
    }
  }
  else if (EPvUsageMode::Target == fPvUsageMode) {
    double x{pTarget->GetX()};
    double y{pTarget->GetY()};
    double z{pTarget->GetZ()};
    LOG_(info, 1) << "using target center as origin: r = (" << x << ", " << y << ", " << z << ") [cm]";
    TMatrixFSym covMatrix(3);
    covMatrix(1, 0) = 0.;
    covMatrix(2, 0) = 0.;
    covMatrix(2, 1) = 0.;
    //double transverseVariance{pTarget->GetRmax() * pTarget->GetRmax() / 12.};
    //covMatrix(0, 0) = transverseVariance;
    //covMatrix(1, 1) = transverseVariance;
    //covMatrix(2, 2) = pTarget->GetDz() * pTarget->GetDz() / 3.;
    covMatrix(0, 0) = 0.;
    covMatrix(1, 1) = 0.;
    covMatrix(2, 2) = 0.;
    fpOrigin->SetVertex(x, y, z, 0., 1, 0, covMatrix);
  }

  if (!bBranchesInitialized) {
    return kERROR;
  }

  // ----- Output branches initialization
  fpBrEventTriggers = new std::vector<CbmEventTriggers>();
  if (pFairManager->GetObject("CbmEventTriggers")) {
    LOG(error) << "Branch \"CbmEventTriggers\" already exists!";
    return kFATAL;
  }
  pFairManager->RegisterAny("CbmEventTriggers", fpBrEventTriggers, true);
  LOG_(info, 1) << "registering branch \"CbmEventTriggers\"";

  // ----- Topology reconstructor initialization
  fpTopoReconstructorRun->SetTarget({float(pTarget->GetX()), float(pTarget->GetY()), float(pTarget->GetZ())});
  fpTopoReconstructorEvent->SetTarget(fpTopoReconstructorRun->GetTargetPosition());
  fpTopoReconstructorEvent->CopyCuts(fpTopoReconstructorRun.get());
  fpTopoReconstructorEvent->GetKFParticleFinder()->SetReconstructionList(
    fpTopoReconstructorRun->GetKFParticleFinder()->GetReconstructionList());

  // ----- Auxilary variables initialization
  std::fill(fCounters.begin(), fCounters.end(), 0);

  // ----- QA initialization
  if (fbRunQa) {
    fpQa = std::make_unique<V0FinderQa>(fbUseMc);
    fpQa->InitHistograms();
  }

  LOG_(info, 1) << "initializing the task ... done";
  return kSUCCESS;
}

// ---------------------------------------------------------------------------------------------------------------------
//
KFPTrackVector V0FinderTask::MakeKfpTrackVector(const std::vector<const CbmGlobalTrack*>& vpTracks,
                                                const std::vector<int>& vTrackIds, const std::vector<float>& vChi2ToPv,
                                                bool bAtFirstPoint) const
{
  // TODO: Cross-check with the CbmKFParticleFinder results
  // NOTE: Actually tracks themselves are not needed here, so one could fill and pass a vector of FairTrackParam*,
  //       put in KFPTrackVector there is an option to set number of Pixel hits, so we will keep the signature of
  //       the function as it is.
  KFPTrackVector trackVector;
  int nTracks = vpTracks.size();
  trackVector.Resize(nTracks);
  for (int iTrk = 0; iTrk < nTracks; ++iTrk) {
    const auto* pTrack{vpTracks[iTrk]};
    const auto* pParam{bAtFirstPoint ? pTrack->GetParamFirst() : pTrack->GetParamLast()};

    // ----- Parameters definition
    int pdg{pTrack->GetPidHypo()};
    const double& tx{pParam->GetTx()};
    const double& ty{pParam->GetTy()};
    const double& qp{pParam->GetQp()};
    int q{qp > 0. ? 1 : -1};
    if (std::fabs(pdg) == 10000020030 || std::fabs(pdg == 1000020040)) {  // He3 and He4
      // NOTE: at the moment not called, but never the less leave it here for the future pid procedures
      q *= 2;
    }
    double p{q / qp};
    double p2{p * p};
    double t2inv{1. / (1. + tx * tx + ty * ty)};
    double pz{std::sqrt(t2inv * p2)};
    double px{tx * pz};
    double py{ty * pz};

    trackVector.SetParameter(pParam->GetX(), 0, iTrk);
    trackVector.SetParameter(pParam->GetY(), 1, iTrk);
    trackVector.SetParameter(pParam->GetZ(), 2, iTrk);
    trackVector.SetParameter(px, 3, iTrk);
    trackVector.SetParameter(py, 4, iTrk);
    trackVector.SetParameter(pz, 5, iTrk);

    // Jacobian matrix for (tx, ty, qp) -> (px, py, pz)
    std::array<std::array<double, 3>, 3> Jp;
    Jp[2][0] = -t2inv * px;         // d(pz)/d(tx)
    Jp[2][1] = -t2inv * py;         // d(pz)/d(ty)
    Jp[2][2] = -pz / qp;            // d(pz)/d(qp)
    Jp[0][0] = tx * Jp[2][0] + pz;  // d(px)/d(tx)
    Jp[0][1] = tx * Jp[2][1];       // d(px)/d(ty)
    Jp[0][2] = tx * Jp[2][2];       // d(px)/d(qp)
    Jp[1][0] = ty * Jp[2][0];       // d(py)/d(tx)
    Jp[1][1] = ty * Jp[2][1] + pz;  // d(py)/d(ty)
    Jp[1][2] = ty * Jp[2][2];       // d(py)/d(qp)

    // ----- Covariance matrix definition
    // Position covariances
    trackVector.SetCovariance(pParam->GetCovariance(0, 0), 0, iTrk);  // var(x)
    trackVector.SetCovariance(pParam->GetCovariance(0, 1), 1, iTrk);  // cov(x, y)
    trackVector.SetCovariance(pParam->GetCovariance(1, 1), 2, iTrk);  // var(y)

    // Momentum-position covariances
    auto MomPosCovariance = [&](const int k, const int l) constexpr->double
    {
      double val{0.};
      const auto& JpA = Jp[k];
      for (int i = 0; i < 3; ++i) {
        val += JpA[i] * pParam->GetCovariance(i + 2, l);
      }
      return val;
    };
    trackVector.SetCovariance(MomPosCovariance(0, 0), 6, iTrk);   // cov(x, px)
    trackVector.SetCovariance(MomPosCovariance(0, 1), 7, iTrk);   // cov(y, px)
    trackVector.SetCovariance(MomPosCovariance(1, 0), 10, iTrk);  // cov(x, py)
    trackVector.SetCovariance(MomPosCovariance(1, 1), 11, iTrk);  // cov(y, py)
    trackVector.SetCovariance(MomPosCovariance(2, 0), 15, iTrk);  // cov(x, pz)
    trackVector.SetCovariance(MomPosCovariance(2, 1), 16, iTrk);  // cov(y, pz)

    // Momentum covariances
    auto MomentumCovariance = [&](const int k, const int l) constexpr->double
    {
      double val{0.};
      const auto& JpA = Jp[k];
      const auto& JpB = Jp[l];
      for (int i = 0; i < 3; ++i) {
        double factor{0.};
        for (int j = 0; j < 3; ++j) {
          factor += JpB[j] * pParam->GetCovariance(i + 2, j + 2);
        }
        val += JpA[i] * factor;
      }
      return val;
    };
    trackVector.SetCovariance(MomentumCovariance(0, 0), 9, iTrk);   // var(px)
    trackVector.SetCovariance(MomentumCovariance(1, 0), 13, iTrk);  // cov(px, py)
    trackVector.SetCovariance(MomentumCovariance(1, 1), 14, iTrk);  // var(py)
    trackVector.SetCovariance(MomentumCovariance(2, 0), 18, iTrk);  // cov(px, pz)
    trackVector.SetCovariance(MomentumCovariance(2, 1), 19, iTrk);  // cov(py, pz)
    trackVector.SetCovariance(MomentumCovariance(2, 2), 20, iTrk);  // var(pz)

    // Zero covariances
    // NOTE: from the tracking point of view a z-coordinate in known precisely, so all the corresponding covariances
    //       should be set to null.
    trackVector.SetCovariance(0.f, 3, iTrk);   // cov(x,z)
    trackVector.SetCovariance(0.f, 4, iTrk);   // cov(y,z)
    trackVector.SetCovariance(0.f, 5, iTrk);   // var(z)
    trackVector.SetCovariance(0.f, 8, iTrk);   // cov(z,px)
    trackVector.SetCovariance(0.f, 12, iTrk);  // cov(z,py)
    trackVector.SetCovariance(0.f, 17, iTrk);  // var(z,pz)

    // ----- Other parameters
    // Magnetic field
    // NOTE: In mCBM there is no magnetic field, so here we define the coefficients with zeros
    for (int iF = 0; iF < 10; ++iF) {
      trackVector.SetFieldCoefficient(0.f, iF, iTrk);
    }

    trackVector.SetId(vTrackIds[iTrk], iTrk);
    trackVector.SetPDG(pdg, iTrk);
    trackVector.SetQ(q, iTrk);
    trackVector.SetNPixelHits(0, iTrk);  // Number of MVD hits in track (used to define PV inside the KFPFinder)

    if (EPvUsageMode::Reconstructed == fPvUsageMode || EPvUsageMode::Mc == fPvUsageMode) {
      if (vChi2ToPv[iTrk] < kChi2PvPrimThrsh) {
        trackVector.SetPVIndex(0, iTrk);  // Primary track
      }
      else {
        trackVector.SetPVIndex(-1, iTrk);  // Secondary track
      }
    }
    else {
      trackVector.SetPVIndex(-1, iTrk);  // Secondary track
    }
  }
  return trackVector;
}

// ---------------------------------------------------------------------------------------------------------------------
//
KFVertex V0FinderTask::MakeKfpPrimaryVertex(float x, float y, float z) const
{
  KFVertex kfVertex;
  kfVertex.X() = x;
  kfVertex.Y() = y;
  kfVertex.Z() = z;
  for (int iC = 0; iC < 6; ++iC) {
    kfVertex.Covariance(iC) = 0.f;
  }
  kfVertex.Chi2() = -100.f;
  return kfVertex;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool V0FinderTask::SelectTrack(CbmGlobalTrack* pTrack, int iTrk)
{
  const int iStsTrk = pTrack->GetStsTrackIndex();
  if (iStsTrk < 0) {
    ++fCounters[ECounter::TracksWoStsHits];
    return false;  // Skip tracks, which do not have hits in TOF
  }
  const auto* pStsTrack{static_cast<const CbmStsTrack*>(fpBrStsTracks->At(iStsTrk))};

  // Get PID hypothesis for the track
  int pdgHypo{kUndefPdg};
  if (EPidApproach::Topo == fPidApproach) {
    auto& trkDca{fvTrackDca[iTrk]};
    trkDca  = EstimateDcaToOrigin(pStsTrack);
    pdgHypo = InferTrackPidTopo(trkDca.fAbs);
    if (fbRunQa) {
      fpQa->fph_dca->Fill(trkDca.fAbs);
      fpQa->fph_dca2D->Fill(trkDca.fAbs * trkDca.fX, trkDca.fAbs * trkDca.fY);
      fpQa->fph_dca_projectionX->Fill(trkDca.fAbs * trkDca.fX);
      fpQa->fph_dca_projectionY->Fill(trkDca.fAbs * trkDca.fY);
    }
  }

  pTrack->SetPidHypo(pdgHypo);
  if (pdgHypo == kUndefPdg) {
    ++fCounters[ECounter::TracksWoPid];
    return false;  // Tracks, which do not obey the PID criteria
  }

  // Assign a momentum to the track using the last hit in TOF, reject the track, if it has no TOF hits
  if (!AssignMomentum(pTrack, pdgHypo)) {
    ++fCounters[ECounter::TracksWoMomentum];
    return false;
  }

  // Check track parameters for validity
  // FIXME: understand the reason, why the parameters are infinite
  if (!CheckTrackParam(pTrack->GetParamFirst())) {
    ++fCounters[ECounter::TracksInfiniteParam];
    return false;
  }

  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
double V0FinderTask::ShiftTofHitsToTzero(const CbmEvent* pEvent)
{
  // Define t0 from the Bmon hits
  double t0{std::numeric_limits<double>::signaling_NaN()};
  int nTofHits = pEvent->GetNofData(ECbmDataType::kTofHit);
  for (int iTofHitEvt{0}; iTofHitEvt < nTofHits; ++iTofHitEvt) {
    int iTofHit = pEvent->GetIndex(ECbmDataType::kTofHit, iTofHitEvt);
    const auto* pTofHit{static_cast<const CbmTofHit*>(fpBrTofHits->At(iTofHit))};
    //if (5 == CbmTofAddress::GetSmType(pTofHit->GetAddress())) {   // selection by the supermodule type
    if (pTofHit->GetZ() == 0) {  // Take some small z-coordinate to identify BMon (TODO: provide a flag by some task)
      t0 = pTofHit->GetTime();
    }
  }
  // NOTE: t0 must be defined for each event, since the Bmon digis are used to seed the digi event builder. Basically,
  //       the tZero must be defined as a field of CbmEvent, e.g. in a separate FairTask, or directly
  //       in CbmAlgoBuildRawEvent
  if (std::isnan(t0)) {
    return t0;
  }

  // Shift TOF times to found t0:
  for (int iTofHitEvt{0}; iTofHitEvt < nTofHits; ++iTofHitEvt) {
    int iTofHit = pEvent->GetIndex(ECbmDataType::kTofHit, iTofHitEvt);
    auto* pTofHit{static_cast<CbmTofHit*>(fpBrTofHits->At(iTofHit))};
    pTofHit->SetTime(pTofHit->GetTime() - t0 - fTzeroOffset);
  }
  return t0;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0FinderTask::StoreParticles()
{
  int indexOffset = fpTopoReconstructorRun->GetParticles().size();
  for (int iPv = 0; iPv < fpTopoReconstructorEvent->NPrimaryVertices(); ++iPv) {
    std::vector<int> vPVTrackIndexArray = fpTopoReconstructorEvent->GetPVTrackIndexArray(iPv);
    std::for_each(vPVTrackIndexArray.begin(), vPVTrackIndexArray.end(), [&](auto& item) { item += indexOffset; });
    fpTopoReconstructorRun->AddPV(fpTopoReconstructorEvent->GetPrimKFVertex(iPv), vPVTrackIndexArray);
  }

  for (size_t iP = 0; iP < fpTopoReconstructorEvent->GetParticles().size(); ++iP) {
    const KFParticle& particleEvt{fpTopoReconstructorEvent->GetParticles()[iP]};
    KFParticle particle{particleEvt};
    particle.CleanDaughtersId();
    int nDaughters{particleEvt.NDaughters()};
    if (nDaughters == 1) {
      particle.AddDaughterId(particleEvt.DaughterIds()[0]);
    }
    else if (nDaughters > 1) {
      for (int iD = 0; iD < particleEvt.NDaughters(); ++iD) {
        particle.AddDaughterId(particleEvt.DaughterIds()[iD] + indexOffset);
      }
    }
    particle.SetId(particleEvt.Id() + indexOffset);
    fpTopoReconstructorRun->AddParticle(particle);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string V0FinderTask::ToString(EProcessingMode mode)
{
  switch (mode) {
    case EProcessingMode::EventBased: return "event-based";
    case EProcessingMode::TimeBased: return "time-based";
    default: return "";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string V0FinderTask::ToString(EPidApproach pid)
{
  switch (pid) {
    case EPidApproach::Topo: return "track topology";
    case EPidApproach::Mc: return "MC-truth";
    default: return "";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string V0FinderTask::ToString(EPvUsageMode pvMode)
{
  switch (pvMode) {
    case EPvUsageMode::Reconstructed: return "reconstructed primary vertex is used";
    case EPvUsageMode::ReconstructSingle: return "a single primary vertex will be reconstructed";
    case EPvUsageMode::ReconstructMultiple: return "multiple primary vertices will be reconstructed";
    case EPvUsageMode::Target: return "target center is used for primary vertex";
    case EPvUsageMode::Mc: return "MC-true primary vertex is used";
    default: return "";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string V0FinderTask::ToString(const FairTrackParam* pParam)
{
  std::stringstream msg;
  msg << "Track parameters: r=(" << pParam->GetX() << ", " << pParam->GetY() << ", " << pParam->GetZ()
      << "), tx=" << pParam->GetTx() << ", ty=" << pParam->GetTy() << ", q/p=" << pParam->GetQp()
      << ", Covariance matrix:";
  for (int i = 0; i < 5; ++i) {
    msg << '\n';
    for (int j = 0; j <= i; ++j) {
      msg << std::setw(15) << pParam->GetCovariance(i, j) << ' ';
    }
  }
  return msg.str();
}
