/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfpV0Finder.cxx
/// \date   01.02.2025
/// \brief  A V0 finding algorithm (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "kfp/KfpV0Finder.h"

#include "global/RecoResults.h"

#include <algorithm>
#include <limits>
#include <sstream>

using cbm::algo::RecoResults;
using cbm::algo::kfp::V0Finder;


// ---------------------------------------------------------------------------------------------------------------------
//
bool V0Finder::AssignMomentum(const PartitionedVector<tof::Hit>& tofHits,
                              const std::vector<RecoResults::HitId_t>& tofHitIds, double t0, ParticleInfo& particleInfo)
{
  if (tofHitIds.empty()) {
    fEventMonitor.IncrementCounter(ECounter::TracksWoTofHits);
    return false;
  }

  double beta{0.};
  if constexpr (kUseAverageSpeed) {
    for (const auto& hitId : tofHitIds) {
      beta += EstimateBeta(tofHits[hitId.first][hitId.second], t0);
    }
    beta /= tofHitIds.size();
  }
  else {
    const auto& hitId = tofHitIds.back();
    beta              = EstimateBeta(tofHits[hitId.first][hitId.second], t0);
  }
  if (beta < 0.) {
    fEventMonitor.IncrementCounter(ECounter::TracksWNegativeTofHitTime);
    return false;
  }
  else if (beta > 1.) {
    fEventMonitor.IncrementCounter(ECounter::TracksWUnphysicalBeta);
    return false;
  }
  double gamma{1. / sqrt(1. - beta * beta)};
  particleInfo.fBeta = beta;
  particleInfo.fQp   = particleInfo.fCharge / (gamma * beta * particleInfo.fMass);
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0Finder::AssignPid(ParticleInfo& particleInfo)
{
  if (std::isnan(particleInfo.fDca)) {
    particleInfo.fPdg = kUndefPdg;
    fEventMonitor.IncrementCounter(ECounter::TracksWoPid);
  }
  else if (particleInfo.fDca > fMinPionDca) {
    // pi-
    particleInfo.fPdg    = -211;
    particleInfo.fMass   = kPionMass;
    particleInfo.fCharge = -1;
    fEventMonitor.IncrementCounter(ECounter::PionsDca);
  }
  else if (particleInfo.fDca > fMinProtonDca) {
    // proton
    particleInfo.fPdg    = 2212;
    particleInfo.fMass   = kProtonMass;
    particleInfo.fCharge = 1;
    fEventMonitor.IncrementCounter(ECounter::ProtonsDca);
  }
  else {
    // primary
    //particleInfo.fPdg = fPrimaryAssignedPdg;
    particleInfo.fPdg = kUndefPdg;
    fEventMonitor.IncrementCounter(ECounter::PrimaryDca);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0Finder::CollectDca(const RecoResults& recoEvent)
{
  const auto& stsHitIndices = recoEvent.trackStsHitIndices;
  for (size_t iTrk = 0; iTrk < stsHitIndices.size(); ++iTrk) {
    const auto& stsHitIndicesInTrack = stsHitIndices[iTrk];
    if (stsHitIndicesInTrack.size() < 2) {  // DCA cannot be estimated
      fEventMonitor.IncrementCounter(ECounter::TracksWoStsHits);
      continue;
    }
    auto& particleInfo     = fvParticleInfo[iTrk];
    auto [iPtFst, iHitFst] = stsHitIndicesInTrack[0];
    auto [iPtSnd, iHitSnd] = stsHitIndicesInTrack[1];
    particleInfo.fDca      = EstimateDca(recoEvent.stsHits[iPtFst][iHitFst], recoEvent.stsHits[iPtSnd][iHitSnd]);
    AssignPid(particleInfo);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0Finder::CollectT0(gsl::span<const bmon::Hit> bmonHits)
{
  fvT0s.clear();
  fvT0s.reserve(bmonHits.size());
  std::transform(bmonHits.begin(), bmonHits.end(), std::back_inserter(fvT0s),
                 [&](const auto& h) { return h.GetTime(); });
}

// ---------------------------------------------------------------------------------------------------------------------
//
double V0Finder::EstimateDca(const sts::Hit& fst, const sts::Hit& snd) const
{
  double factor{(fst.Z() - fOrigin[2]) / (snd.Z() - fst.Z())};
  double dcaX{fst.X() - fOrigin[0] - factor * (snd.X() - fst.X())};
  double dcaY{fst.Y() - fOrigin[1] - factor * (snd.Y() - fst.Y())};
  return std::sqrt(dcaX * dcaX + dcaY * dcaY);
}

// ---------------------------------------------------------------------------------------------------------------------
//
double V0Finder::EstimateBeta(const tof::Hit& hit, double t0) const
{
  double t = hit.Time() - t0 - fTzeroOffset;
  double x{hit.X() - fOrigin[0]};
  double y{hit.Y() - fOrigin[1]};
  double z{hit.Z() - fOrigin[2]};
  double x2{x * x};
  double y2{y * y};
  double z2{z * z};
  double r2{x2 + y2 + z2};
  return std::sqrt(r2) / (t * kSpeedOfLight);
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool V0Finder::FindV0Candidates(const RecoResults& recoEvent, double t0)
{
  const auto& tracks = recoEvent.tracks;

  // Reset temporary data structures
  InitTrackParamVectors(tracks);
  fpTopoReconstructor->Clear();
  fvSelectedTrackIds.clear();
  fvSelectedTrackIds.reserve(tracks.size());
  fEventMonitor.ResetCounter(ECounter::TracksWoMomentum);
  fEventMonitor.ResetCounter(ECounter::TracksSelected);
  fEventMonitor.ResetCounter(ECounter::Pions);
  fEventMonitor.ResetCounter(ECounter::Protons);
  fEventMonitor.ResetCounter(ECounter::EventsLambdaCand);

  // Preselect tracks
  uint32_t nProtonCandidates{0};
  uint32_t nPionCandidates{0};
  uint32_t nSelectedTracks{0};
  fEventMonitor.StartTimer(ETimer::PreselectTracks);
  for (size_t iTrk = 0; iTrk < tracks.size(); ++iTrk) {  // Over all tracks
    auto& particleInfo = fvParticleInfo[iTrk];

    // Cut tracks with undefined dca (by PID == -2)
    // NOTE: if fPdg == kUndefPdg, beta and QP were not estimated for this track on previous iterations as well
    if (particleInfo.fPdg == kUndefPdg) {
      continue;
    }

    // Reset fields of the ParticleInfo, which could be filled on the previous iteration
    particleInfo.fBeta      = std::numeric_limits<double>::quiet_NaN();
    particleInfo.fQp        = std::numeric_limits<double>::quiet_NaN();
    particleInfo.fbSelected = false;

    // Assign momentum to tracks
    if (!AssignMomentum(recoEvent.tofHits, recoEvent.trackTofHitIndices[iTrk], t0, particleInfo)) {
      fEventMonitor.IncrementCounter(ECounter::TracksWoMomentum);
      continue;  // No momentum was assigned
    }

    // Select tracks
    if (!SelectTrack(particleInfo)) {
      continue;
    }
    fvSelectedTrackIds.push_back(iTrk);
    particleInfo.fbSelected = true;

    // Update track parameters
    double qpVar{particleInfo.fQp * fQpAssignedUncertainty};
    qpVar = qpVar * qpVar;
    auto& trkParam{fvTrackParam[iTrk]};
    trkParam.first.SetQp(particleInfo.fQp);
    trkParam.first.SetC44(qpVar);
    trkParam.second.SetQp(particleInfo.fQp);
    trkParam.second.SetC44(qpVar);

    switch (particleInfo.fPdg) {
      case -211: ++nPionCandidates; break;
      case 2212: ++nProtonCandidates; break;
    }
    ++nSelectedTracks;
  }
  fEventMonitor.StopTimer(ETimer::PreselectTracks);
  if (!nPionCandidates || !nProtonCandidates) {
    return false;  // no Lambda can be found
  }
  fEventMonitor.IncrementCounter(ECounter::EventsLambdaCand);
  fEventMonitor.IncrementCounter(ECounter::TracksSelected, nSelectedTracks);
  fEventMonitor.IncrementCounter(ECounter::Pions, nPionCandidates);
  fEventMonitor.IncrementCounter(ECounter::Protons, nProtonCandidates);

  // Initialize and run the KFParticleFinder
  fEventMonitor.StartTimer(ETimer::InitKfp);
  KFPTrackVector kfpTracksFst;
  KFPTrackVector kfpTracksLst;
  kfpTracksFst.Resize(nSelectedTracks);
  kfpTracksLst.Resize(nSelectedTracks);
  for (uint32_t iKfpTrk = 0; iKfpTrk < fvSelectedTrackIds.size(); ++iKfpTrk) {  // Over selected tracks
    uint32_t iCaTrk{fvSelectedTrackIds[iKfpTrk]};
    const auto& trkParam{fvTrackParam[iCaTrk]};
    const auto& particleInfo{fvParticleInfo[iCaTrk]};
    SetKfpTrackParameters(kfpTracksFst, iKfpTrk, iCaTrk, trkParam.first, particleInfo);
    SetKfpTrackParameters(kfpTracksLst, iKfpTrk, iCaTrk, trkParam.second, particleInfo);
  }
  fpTopoReconstructor->Init(kfpTracksFst, kfpTracksLst);
  fpTopoReconstructor->AddPV(MakeKfpPrimaryVertex(fOrigin));
  fpTopoReconstructor->SortTracks();
  fEventMonitor.StopTimer(ETimer::InitKfp);
  fEventMonitor.StartTimer(ETimer::ExecKfp);
  fpTopoReconstructor->ReconstructParticles();
  fEventMonitor.StopTimer(ETimer::ExecKfp);
  const auto& particles{fpTopoReconstructor->GetParticles()};

  // Scan for Lambda-candidates
  uint32_t nLambdaCandidates =
    std::count_if(particles.begin(), particles.end(), [&](const auto& p) { return p.GetPDG() == 3122; });
  fEventMonitor.IncrementCounter(ECounter::KfpLambdaCandidates, nLambdaCandidates);

  return static_cast<bool>(nLambdaCandidates);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0Finder::Init() { fpTopoReconstructor->SetTarget({float(fOrigin[0]), float(fOrigin[1]), float(fOrigin[2])}); }

// ---------------------------------------------------------------------------------------------------------------------
//
void V0Finder::InitTrackParamVectors(const ca::Vector<ca::Track>& tracks)
{
  fvTrackParam.clear();
  fvTrackParam.reserve(tracks.size());
  std::transform(tracks.begin(), tracks.end(), std::back_inserter(fvTrackParam),
                 [&](const auto& t) { return std::make_pair(t.fParFirst, t.fParLast); });
}

// ---------------------------------------------------------------------------------------------------------------------
//
KFVertex V0Finder::MakeKfpPrimaryVertex(const std::array<float, 3>& r)
{
  KFVertex kfVertex;
  kfVertex.X() = r[0];
  kfVertex.Y() = r[1];
  kfVertex.Z() = r[2];
  for (int iC = 0; iC < 6; ++iC) {
    kfVertex.Covariance(iC) = 0.f;
  }
  kfVertex.Chi2() = -100.f;
  return kfVertex;
}

// ---------------------------------------------------------------------------------------------------------------------
//
CbmEventTriggers V0Finder::Process(const RecoResults& recoEvent)
{
  //L_(info) << "----------------------------- EVENT --------------";
  CbmEventTriggers res;
  fEventMonitor.Reset();
  fEventMonitor.StartTimer(ETimer::ProcessEvent);
  fEventMonitor.IncrementCounter(ECounter::EventsTotal);
  fEventMonitor.IncrementCounter(ECounter::TracksTotal, recoEvent.tracks.size());

  // ----- Initialize data-structures
  fvParticleInfo.clear();
  fvParticleInfo.resize(recoEvent.tracks.size());

  // ----- Define T0
  // So far we cannot preselect a hit from multiple ones, we will be using all of them iteratively to find lambdas
  fEventMonitor.StartTimer(ETimer::CollectT0);
  CollectT0(recoEvent.bmonHits[fBmonPartitionIndex]);
  if (fvT0s.empty()) {
    fEventMonitor.IncrementCounter(ECounter::EventsWoTzero);
    return res;
  }
  fEventMonitor.StopTimer(ETimer::CollectT0);

  // ----- Estimate DCA of tracks and assign PID
  // If a track has less then two STS hits, and undefined DCA value is stored
  fEventMonitor.StartTimer(ETimer::CollectDca);
  CollectDca(recoEvent);
  fEventMonitor.StopTimer(ETimer::CollectDca);

  // ----- Try to find lambdas for different T0
  fSelectedT0 = std::numeric_limits<double>::quiet_NaN();
  fEventMonitor.StartTimer(ETimer::FindV0Candidates);
  for (double t0 : fvT0s) {
    if (FindV0Candidates(recoEvent, t0)) {
      fSelectedT0 = t0;
      res.Set(CbmEventTriggers::ETrigger::Lambda);
      fEventMonitor.IncrementCounter(ECounter::KfpEventsLambdaCand);
      break;  // Lambda-candidates were found, there is no sense to scan further
    }
  }
  fEventMonitor.StopTimer(ETimer::FindV0Candidates);
  fEventMonitor.StopTimer(ETimer::ProcessEvent);
  return res;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool V0Finder::SelectTrack(const ParticleInfo& particleInfo) const
{
  // Speed cut
  if (particleInfo.fPdg == -211) {
    if (particleInfo.fBeta < fMinBetaPion || particleInfo.fBeta > fMaxBetaPion) {
      return false;
    }
  }
  else if (particleInfo.fPdg == 2212) {
    if (particleInfo.fBeta < fMinBetaProton || particleInfo.fBeta > fMaxBetaProton) {
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void V0Finder::SetKfpTrackParameters(KFPTrackVector& trackVector, uint32_t iKfpTrk, uint32_t iCaTrk,
                                     const ca::Track::TrackParam_t& trkParam, const ParticleInfo& particleInfo) const
{
  // ----- Parameter definition
  double tx{trkParam.GetTx()};
  double ty{trkParam.GetTy()};
  double qp{trkParam.GetQp()};
  double p{particleInfo.fCharge / qp};
  double p2{p * p};
  double t2inv{1. / (1. + tx * tx + ty * ty)};
  double pz{std::sqrt(t2inv * p2)};
  double px{tx * pz};
  double py{ty * pz};

  trackVector.SetParameter(trkParam.GetX(), 0, iKfpTrk);
  trackVector.SetParameter(trkParam.GetY(), 1, iKfpTrk);
  trackVector.SetParameter(trkParam.GetZ(), 2, iKfpTrk);
  trackVector.SetParameter(px, 3, iKfpTrk);
  trackVector.SetParameter(py, 4, iKfpTrk);
  trackVector.SetParameter(pz, 5, iKfpTrk);

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
  // Position covariance
  trackVector.SetCovariance(trkParam.C00(), 0, iKfpTrk);  // var(x)
  trackVector.SetCovariance(trkParam.C01(), 1, iKfpTrk);  // cov(x, y)
  trackVector.SetCovariance(trkParam.C11(), 2, iKfpTrk);  // var(y)

  // Momentum-position covariances
  auto MomPosCovariance = [&](const int k, const int l) constexpr->double
  {
    double val{0.};
    const auto& JpA = Jp[k];
    for (int i = 0; i < 3; ++i) {
      val += JpA[i] * trkParam.C(i + 2, l);
    }
    return val;
  };
  trackVector.SetCovariance(MomPosCovariance(0, 0), 6, iKfpTrk);   // cov(x, px)
  trackVector.SetCovariance(MomPosCovariance(0, 1), 7, iKfpTrk);   // cov(y, px)
  trackVector.SetCovariance(MomPosCovariance(1, 0), 10, iKfpTrk);  // cov(x, py)
  trackVector.SetCovariance(MomPosCovariance(1, 1), 11, iKfpTrk);  // cov(y, py)
  trackVector.SetCovariance(MomPosCovariance(2, 0), 15, iKfpTrk);  // cov(x, pz)
  trackVector.SetCovariance(MomPosCovariance(2, 1), 16, iKfpTrk);  // cov(y, pz)

  // Momentum covariances
  auto MomentumCovariance = [&](const int k, const int l) constexpr->double
  {
    double val{0.};
    const auto& JpA = Jp[k];
    const auto& JpB = Jp[l];
    for (int i = 0; i < 3; ++i) {
      double factor{0.};
      for (int j = 0; j < 3; ++j) {
        factor += JpB[j] * trkParam.C(i + 2, j + 2);
      }
      val += JpA[i] * factor;
    }
    return val;
  };
  trackVector.SetCovariance(MomentumCovariance(0, 0), 9, iKfpTrk);   // var(px)
  trackVector.SetCovariance(MomentumCovariance(1, 0), 13, iKfpTrk);  // cov(px, py)
  trackVector.SetCovariance(MomentumCovariance(1, 1), 14, iKfpTrk);  // var(py)
  trackVector.SetCovariance(MomentumCovariance(2, 0), 18, iKfpTrk);  // cov(px, pz)
  trackVector.SetCovariance(MomentumCovariance(2, 1), 19, iKfpTrk);  // cov(py, pz)
  trackVector.SetCovariance(MomentumCovariance(2, 2), 20, iKfpTrk);  // var(pz)

  // Zero covariances (with z-coordinate)
  trackVector.SetCovariance(0.f, 3, iKfpTrk);   // cov(x,z)
  trackVector.SetCovariance(0.f, 4, iKfpTrk);   // cov(y,z)
  trackVector.SetCovariance(0.f, 5, iKfpTrk);   // var(z)
  trackVector.SetCovariance(0.f, 8, iKfpTrk);   // cov(z,px)
  trackVector.SetCovariance(0.f, 12, iKfpTrk);  // cov(z,py)
  trackVector.SetCovariance(0.f, 17, iKfpTrk);  // var(z,pz)

  // ----- Other quantities
  // Magnetic field (NOTE: zero fom mCBM)
  // FIXME: Provide a proper initialization for full CBM
  for (int iF = 0; iF < 10; ++iF) {
    trackVector.SetFieldCoefficient(0.f, iF, iKfpTrk);
  }

  trackVector.SetId(iCaTrk, iKfpTrk);
  trackVector.SetPDG(particleInfo.fPdg, iKfpTrk);
  trackVector.SetQ(particleInfo.fCharge, iKfpTrk);
  trackVector.SetNPixelHits(0, iKfpTrk);

  // NOTE: 0 - primary tracks, -1 - secondary tracks. Here for now we assign ALL tracks as secondary
  trackVector.SetPVIndex(-1, iKfpTrk);
}
