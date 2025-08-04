/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

#include "CbmKfTrackFitter.h"

#include "CbmGlobalTrack.h"
#include "CbmKfTrackingSetupBuilder.h"
#include "CbmMuchPixelHit.h"
#include "CbmMuchTrack.h"
#include "CbmMuchTrackingInterface.h"
#include "CbmMvdHit.h"
#include "CbmMvdTrackingInterface.h"
#include "CbmStsAddress.h"
#include "CbmStsHit.h"
#include "CbmStsSetup.h"
#include "CbmStsTrack.h"
#include "CbmStsTrackingInterface.h"
#include "CbmTofHit.h"
#include "CbmTofTrack.h"
#include "CbmTofTrackingInterface.h"
#include "CbmTrdHit.h"
#include "CbmTrdTrack.h"
#include "CbmTrdTrackingInterface.h"
#include "FairRootManager.h"
#include "KFParticleDatabase.h"
#include "KfDefs.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"
#include "TClonesArray.h"
#include "TDatabasePDG.h"

using std::vector;
using namespace std;
using namespace cbm::algo;

void CbmKfTrackFitter::Trajectory::OrderNodesInZ()
{
  // sort the nodes in z
  std::sort(fNodes.begin(), fNodes.end(), [](const TrajectoryNode& a, const TrajectoryNode& b) { return a.fZ < b.fZ; });
}


CbmKfTrackFitter::CbmKfTrackFitter() {}

CbmKfTrackFitter::~CbmKfTrackFitter() {}

void CbmKfTrackFitter::Init()
{
  if (fIsInitialized) {
    return;
  }

  FairRootManager* ioman = FairRootManager::Instance();

  if (!ioman) {
    LOG(fatal) << "CbmKfTrackFitter: no FairRootManager";
  }

  // TODO: better check if all the interfaces are properly initialized
  if (!CbmStsTrackingInterface::Instance() || !CbmTrdTrackingInterface::Instance()
      || !CbmTofTrackingInterface::Instance()) {
    LOG(fatal) << "CbmTrackingDetectorInterface instance was not found. Please, add it as a task to your "
                  "reco macro right before the KF and L1 tasks:\n"
               << "\033[1;30mrun->AddTask(new CbmTrackingDetectorInterfaceInit());\033[0m";
  }

  // Get hits

  fInputMvdHits  = dynamic_cast<TClonesArray*>(ioman->GetObject("MvdHit"));
  fInputStsHits  = dynamic_cast<TClonesArray*>(ioman->GetObject("StsHit"));
  fInputTrdHits  = dynamic_cast<TClonesArray*>(ioman->GetObject("TrdHit"));
  fInputMuchHits = dynamic_cast<TClonesArray*>(ioman->GetObject("MuchHit"));
  fInputTofHits  = dynamic_cast<TClonesArray*>(ioman->GetObject("TofHit"));

  // Get global tracks
  fInputGlobalTracks = dynamic_cast<TClonesArray*>(ioman->GetObject("GlobalTrack"));

  // Get detector tracks
  fInputStsTracks  = dynamic_cast<TClonesArray*>(ioman->GetObject("StsTrack"));
  fInputMuchTracks = dynamic_cast<TClonesArray*>(ioman->GetObject("MuchTrack"));
  fInputTrdTracks  = dynamic_cast<TClonesArray*>(ioman->GetObject("TrdTrack"));
  fInputTofTracks  = dynamic_cast<TClonesArray*>(ioman->GetObject("TofTrack"));

  fKfSetup = cbm::kf::TrackingSetupBuilder::Instance()->GetSharedGeoSetup();

  fIsInitialized = true;
}

void CbmKfTrackFitter::SetParticleHypothesis(int pdg)
{
  TParticlePDG* particlePDG = TDatabasePDG::Instance()->GetParticle(pdg);
  if (!particlePDG) {
    LOG(fatal) << "CbmKfTrackFitter: particle PDG " << pdg << " is not in the data base, please set the mass manually";
    return;
  }
  fMass       = particlePDG->Mass();
  fIsElectron = (abs(pdg) == 11);
}

void CbmKfTrackFitter::SetMassHypothesis(double mass)
{
  assert(mass >= 0.);
  fMass = mass;
}


void CbmKfTrackFitter::SetElectronFlag(bool isElectron) { fIsElectron = isElectron; }


bool CbmKfTrackFitter::CreateMvdStsTrack(CbmKfTrackFitter::Trajectory& kfTrack, int stsTrackIndex)
{
  Init();
  if (!fIsInitialized) {
    return false;
  }

  if (!fInputStsTracks) {
    LOG(error) << "CbmKfTrackFitter: Sts track array not found!";
    return false;
  }
  if (stsTrackIndex >= fInputStsTracks->GetEntriesFast()) {
    LOG(error) << "CbmKfTrackFitter: Sts track index " << stsTrackIndex << " is out of range!";
    return false;
  }
  auto* stsTrack = dynamic_cast<const CbmStsTrack*>(fInputStsTracks->At(stsTrackIndex));
  if (!stsTrack) {
    LOG(error) << "CbmKfTrackFitter: Sts track is null!";
    return false;
  }

  CbmGlobalTrack globalTrack;
  globalTrack.SetStsTrackIndex(stsTrackIndex);
  globalTrack.SetParamFirst(*dynamic_cast<const FairTrackParam*>(stsTrack->GetParamFirst()));

  return CreateGlobalTrack(kfTrack, globalTrack);
}


bool CbmKfTrackFitter::CreateGlobalTrack(CbmKfTrackFitter::Trajectory& kfTrack, int globalTrackIndex)
{
  Init();
  if (!fIsInitialized) {
    return false;
  }

  if (!fInputGlobalTracks) {
    LOG(error) << "CbmKfTrackFitter: Global track array not found!";
    return false;
  }

  if (globalTrackIndex >= fInputGlobalTracks->GetEntriesFast()) {
    LOG(error) << "CbmKfTrackFitter: Global track index " << globalTrackIndex << " is out of range!";
    return false;
  }

  auto* globalTrack = dynamic_cast<const CbmGlobalTrack*>(fInputGlobalTracks->At(globalTrackIndex));
  if (!globalTrack) {
    LOG(error) << "CbmKfTrackFitter: Global track is null!";
    return false;
  }

  return CreateGlobalTrack(kfTrack, *globalTrack);
}


bool CbmKfTrackFitter::CreateGlobalTrack(CbmKfTrackFitter::Trajectory& kfTrack, const CbmGlobalTrack& globalTrack)
{
  kfTrack = {};
  Init();
  if (!fIsInitialized) {
    return false;
  }

  CbmKfTrackFitter::Trajectory t;

  {
    t.fNodes.resize(fKfSetup->GetNofLayers());
    for (int i = 0; i < fKfSetup->GetNofLayers(); i++) {
      CbmKfTrackFitter::TrajectoryNode& n = t.fNodes[i];
      n                                   = {};
      n.fMaterialLayer                    = i;
      n.fZ                                = fKfSetup->GetMaterial(i).GetZref();
      n.fRadThick                         = 0.;
      n.fIsRadThickFixed                  = false;
      n.fIsFitted                         = false;
      n.fIsXySet                          = false;
      n.fIsTimeSet                        = false;
      n.fHitSystemId                      = ECbmModuleId::kNotExist;
      n.fHitAddress                       = -1;
      n.fHitIndex                         = -1;
    }
  }

  // lambda to set the node from the pixel hit
  auto setNode = [&](const CbmPixelHit& h, int stIdx, int hitIdx,
                     ca::EDetectorID detId) -> CbmKfTrackFitter::TrajectoryNode* {
    int iLayer = fKfSetup->GetIndexMap().LocalToGlobal(detId, stIdx);
    if (iLayer < 0) {
      return nullptr;
    }
    assert(iLayer >= 0 && iLayer < fKfSetup->GetNofLayers());
    CbmKfTrackFitter::TrajectoryNode& n = t.fNodes[iLayer];
    n.fZ                                = h.GetZ();
    n.fMxy.SetX(h.GetX());
    n.fMxy.SetY(h.GetY());
    n.fMxy.SetDx2(h.GetDx() * h.GetDx());
    n.fMxy.SetDy2(h.GetDy() * h.GetDy());
    n.fMxy.SetDxy(h.GetDxy());
    n.fMxy.SetNdfX(1);
    n.fMxy.SetNdfY(1);
    n.fIsXySet = true;
    n.fMt.SetT(h.GetTime());
    n.fMt.SetDt2(h.GetTimeError() * h.GetTimeError());
    n.fMt.SetNdfT(1);
    n.fIsTimeSet       = (detId != ca::EDetectorID::kMvd);
    n.fRadThick        = 0.;
    n.fIsRadThickFixed = false;
    n.fHitSystemId     = ca::ToCbmModuleId(detId);
    n.fHitAddress      = h.GetAddress();
    n.fHitIndex        = hitIdx;
    return &n;
  };

  // Read MVD & STS hits

  if (globalTrack.GetStsTrackIndex() >= 0) {

    int stsTrackIndex = globalTrack.GetStsTrackIndex();

    if (!fInputStsTracks) {
      LOG(error) << "CbmKfTrackFitter: Sts track array not found!";
      return false;
    }
    if (stsTrackIndex >= fInputStsTracks->GetEntriesFast()) {
      LOG(error) << "CbmKfTrackFitter: Sts track index " << stsTrackIndex << " is out of range!";
      return false;
    }
    auto* stsTrack = dynamic_cast<const CbmStsTrack*>(fInputStsTracks->At(stsTrackIndex));
    if (!stsTrack) {
      LOG(error) << "CbmKfTrackFitter: Sts track is null!";
      return false;
    }

    // Read MVD hits

    int nMvdHits = stsTrack->GetNofMvdHits();
    if (nMvdHits > 0) {
      if (!fInputMvdHits) {
        LOG(error) << "CbmKfTrackFitter: Mvd hit array not found!";
        return false;
      }
      if (!CbmMvdTrackingInterface::Instance()) {
        LOG(error) << "CbmKfTrackFitter: Mvd tracking interface not found!";
        return false;
      }
      for (int ih = 0; ih < nMvdHits; ih++) {
        int hitIndex = stsTrack->GetMvdHitIndex(ih);
        if (hitIndex >= fInputMvdHits->GetEntriesFast()) {
          LOG(error) << "CbmKfTrackFitter: Mvd hit index " << hitIndex << " is out of range!";
          return false;
        }
        const auto& hit = *dynamic_cast<const CbmMvdHit*>(fInputMvdHits->At(hitIndex));
        setNode(hit, CbmMvdTrackingInterface::Instance()->GetTrackingStationIndex(&hit), hitIndex,
                ca::EDetectorID::kMvd);
      }
    }

    // Read STS hits

    int nStsHits = stsTrack->GetNofStsHits();
    if (nStsHits > 0) {
      if (!fInputStsHits) {
        LOG(error) << "CbmKfTrackFitter: Sts hit array not found!";
        return false;
      }
      if (!CbmStsTrackingInterface::Instance()) {
        LOG(error) << "CbmKfTrackFitter: Sts tracking interface not found!";
        return false;
      }
      for (int ih = 0; ih < nStsHits; ih++) {
        int hitIndex = stsTrack->GetStsHitIndex(ih);
        if (hitIndex >= fInputStsHits->GetEntriesFast()) {
          LOG(error) << "CbmKfTrackFitter: Sts hit index " << hitIndex << " is out of range!";
          return false;
        }
        const auto& hit = *dynamic_cast<const CbmStsHit*>(fInputStsHits->At(hitIndex));
        setNode(hit, CbmStsTrackingInterface::Instance()->GetTrackingStationIndex(&hit), hitIndex,
                ca::EDetectorID::kSts);
      }
    }
  }  // MVD & STS hits


  // Read Much hits

  if (globalTrack.GetMuchTrackIndex() >= 0) {
    int muchTrackIndex = globalTrack.GetMuchTrackIndex();
    if (!fInputMuchTracks) {
      LOG(error) << "CbmKfTrackFitter: Much track array not found!";
      return false;
    }
    if (muchTrackIndex >= fInputMuchTracks->GetEntriesFast()) {
      LOG(error) << "CbmKfTrackFitter: Much track index " << muchTrackIndex << " is out of range!";
      return false;
    }
    auto* track = dynamic_cast<const CbmMuchTrack*>(fInputMuchTracks->At(muchTrackIndex));
    if (!track) {
      LOG(error) << "CbmKfTrackFitter: Much track is null!";
      return false;
    }
    int nHits = track->GetNofHits();
    if (nHits > 0) {
      if (!fInputMuchHits) {
        LOG(error) << "CbmKfTrackFitter: Much hit array not found!";
        return false;
      }
      if (!CbmMuchTrackingInterface::Instance()) {
        LOG(error) << "CbmKfTrackFitter: Much tracking interface not found!";
        return false;
      }
      for (int ih = 0; ih < nHits; ih++) {
        int hitIndex = track->GetHitIndex(ih);
        if (hitIndex >= fInputMuchHits->GetEntriesFast()) {
          LOG(error) << "CbmKfTrackFitter: Much hit index " << hitIndex << " is out of range!";
          return false;
        }
        const auto& hit = *dynamic_cast<const CbmMuchPixelHit*>(fInputMuchHits->At(hitIndex));
        setNode(hit, CbmMuchTrackingInterface::Instance()->GetTrackingStationIndex(&hit), hitIndex,
                ca::EDetectorID::kMuch);
      }
    }
  }

  // Read TRD hits

  if (globalTrack.GetTrdTrackIndex() >= 0) {
    int trdTrackIndex = globalTrack.GetTrdTrackIndex();
    if (!fInputTrdTracks) {
      LOG(error) << "CbmKfTrackFitter: Trd track array not found!";
      return false;
    }
    if (trdTrackIndex >= fInputTrdTracks->GetEntriesFast()) {
      LOG(error) << "CbmKfTrackFitter: Trd track index " << trdTrackIndex << " is out of range!";
      return false;
    }
    auto* track = dynamic_cast<const CbmTrdTrack*>(fInputTrdTracks->At(trdTrackIndex));
    if (!track) {
      LOG(error) << "CbmKfTrackFitter: Trd track is null!";
      return false;
    }
    int nHits = track->GetNofHits();
    if (nHits > 0) {
      if (!fInputTrdHits) {
        LOG(error) << "CbmKfTrackFitter: Trd hit array not found!";
        return false;
      }
      if (!CbmTrdTrackingInterface::Instance()) {
        LOG(error) << "CbmKfTrackFitter: Trd tracking interface not found!";
        return false;
      }
      for (int ih = 0; ih < nHits; ih++) {
        int hitIndex = track->GetHitIndex(ih);
        if (hitIndex >= fInputTrdHits->GetEntriesFast()) {
          LOG(error) << "CbmKfTrackFitter: Trd hit index " << hitIndex << " is out of range!";
          return false;
        }
        const auto& hit = *dynamic_cast<const CbmTrdHit*>(fInputTrdHits->At(hitIndex));

        auto* node = setNode(hit, CbmTrdTrackingInterface::Instance()->GetTrackingStationIndex(&hit), hitIndex,
                             ca::EDetectorID::kTrd);
        if (node != nullptr && hit.GetClassType() == 1) {
          node->fHitSystemId = ECbmModuleId::kTrd2d;
        }
      }
    }
  }


  // Read TOF hits

  if (globalTrack.GetTofTrackIndex() >= 0) {
    int tofTrackIndex = globalTrack.GetTofTrackIndex();
    if (!fInputTofTracks) {
      LOG(error) << "CbmKfTrackFitter: Trd track array not found!";
      return false;
    }
    if (tofTrackIndex >= fInputTofTracks->GetEntriesFast()) {
      LOG(error) << "CbmKfTrackFitter: Trd track index " << tofTrackIndex << " is out of range!";
      return false;
    }
    auto* track = dynamic_cast<const CbmTofTrack*>(fInputTofTracks->At(tofTrackIndex));
    if (!track) {
      LOG(error) << "CbmKfTrackFitter: Tof track is null!";
      return false;
    }

    int nHits = track->GetNofHits();
    if (nHits > 0) {
      if (!fInputTofHits) {
        LOG(error) << "CbmKfTrackFitter: Tof hit array not found!";
        return false;
      }
      if (!CbmTofTrackingInterface::Instance()) {
        LOG(error) << "CbmKfTrackFitter: Tof tracking interface not found!";
        return false;
      }
      for (int ih = 0; ih < nHits; ih++) {
        int hitIndex = track->GetHitIndex(ih);
        if (hitIndex >= fInputTofHits->GetEntriesFast()) {
          LOG(error) << "CbmKfTrackFitter: Tof hit index " << hitIndex << " is out of range!";
          return false;
        }
        const auto& hit = *dynamic_cast<const CbmTofHit*>(fInputTofHits->At(hitIndex));
        setNode(hit, CbmTofTrackingInterface::Instance()->GetTrackingStationIndex(&hit), hitIndex,
                ca::EDetectorID::kTof);
      }
    }
  }

  t.OrderNodesInZ();

  kfTrack = t;
  return true;

  //  return CreateTrack(kfTrack, *globalTrack.GetParamFirst(), mvdHits, stsHits, muchHits, trdHits, tofHits);
}


void CbmKfTrackFitter::FilterFirstMeasurement(const TrajectoryNode& n)
{
  // a special routine to filter the first measurement.
  // the measurement errors are simply copied to the track covariance matrix

  assert(n.fIsXySet);

  const auto& mxy = n.fMxy;
  const auto& mt  = n.fMt;

  auto& tr = fFit.Tr();

  if (fabs(tr.GetZ() - n.fZ) > 1.e-10) {
    LOG(fatal) << "CbmKfTrackFitter: Z mismatch: fitted track " << tr.GetZ() << " != node " << n.fZ;
  }

  tr.ResetErrors(mxy.Dx2(), mxy.Dy2(), 100., 100., 10., 1.e4, 1.e2);
  tr.SetC10(mxy.Dxy());
  tr.SetX(mxy.X());
  tr.SetY(mxy.Y());
  tr.SetZ(n.fZ);

  if (fSkipUnmeasuredCoordinates) {  // TODO: take C10 correlation into account
    if (mxy.NdfX() == 0) {
      tr.SetX(n.fParamDn.GetX());
      tr.SetC00(1.e4);
    }
    if (mxy.NdfY() == 0) {
      tr.SetY(n.fParamDn.GetY());
      tr.SetC11(1.e4);
      tr.SetC10(0.);
    }
  }

  tr.SetChiSq(0.);
  tr.SetChiSqTime(0.);
  tr.SetNdf(-5. + mxy.NdfX() + mxy.NdfY());
  if (n.fIsTimeSet) {
    tr.SetTime(mt.T());
    tr.SetC55(mt.Dt2());
    tr.SetNdfTime(-2 + 1);
  }
  else {
    tr.SetNdfTime(-2);
  }
  tr.Vi() = cbm::algo::kf::defs::SpeedOfLightInv<>;
  tr.InitVelocityRange(0.5);
}


void CbmKfTrackFitter::AddMaterialEffects(CbmKfTrackFitter::TrajectoryNode& n, const LinearizationAtNode& l,
                                          kf::FitDirection direction)
{
  // add material effects
  if (n.fMaterialLayer < 0) {
    return;
  }

  double tx   = 0.5 * (l.fParamDn.GetTx() + l.fParamUp.GetTx());
  double ty   = 0.5 * (l.fParamDn.GetTy() + l.fParamUp.GetTy());
  double msQp = 0.5 * (l.fParamDn.GetQp() + l.fParamUp.GetQp());

  if (fIsQpForMsFixed) {
    msQp = fDefaultQpForMs;
  }

  // calculate the radiation thickness from the current track
  if (!n.fIsRadThickFixed) {
    const kf::MaterialMap& map = fKfSetup->GetMaterial(n.fMaterialLayer);
    n.fRadThick                = map.GetThicknessX0(l.fParamDn.GetX(), l.fParamDn.GetY());
  }

  fFit.MultipleScattering(n.fRadThick, tx, ty, msQp);

  if (!fIsQpForMsFixed) {
    if (direction == kf::FitDirection::kDownstream) {
      fFit.SetQp0(l.fParamUp.GetQp());
    }
    else {
      fFit.SetQp0(l.fParamDn.GetQp());
    }
    fFit.EnergyLossCorrection(n.fRadThick, direction);
  }
}


bool CbmKfTrackFitter::FitTrajectory(CbmKfTrackFitter::Trajectory& t)
{
  // (re)fit the track
  if (fVerbosityLevel > 0) {
    std::cout << "FitTrajectory ... " << std::endl;
  }
  bool ok = true;

  // ensure that the fitter is initialized
  Init();

  int nNodes = t.fNodes.size();

  if (nNodes <= 0) {
    LOG(warning) << "CbmKfTrackFitter: no nodes found!";
    return false;
  }

  int firstHitNode = -1;
  int lastHitNode  = -1;
  bool isOrdered   = true;

  {  // find the first and the last hit nodes. Check if the nodes are ordered in Z
    double zOld = -1.e10;
    for (int i = 0; i < nNodes; i++) {
      const auto& n = t.fNodes[i];
      if (n.fIsXySet) {
        if (firstHitNode < 0) {
          firstHitNode = i;
        }
        lastHitNode = i;
      }
      if (n.fZ < zOld) {
        isOrdered = false;
      }
      zOld = n.fZ;
    }
  }

  if (firstHitNode < 0 || lastHitNode < 0) {
    LOG(warning) << "CbmKfTrackFitter: no hit nodes found!";
    return false;
  }

  if (!isOrdered) {
    LOG(warning) << "CbmKfTrackFitter: track nodes are not ordered in Z!";
  }

  fFit.SetParticleMass(fMass);

  // kf::GlobalField::fgOriginalFieldType = kf::EFieldType::Null;

  kf::FieldRegion<double> field(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);

  std::vector<LinearizationAtNode> linearization(nNodes);

  if (t.fIsFitted) {  // take the linearization from the previously fitted trajectory
    for (int i = firstHitNode; i <= lastHitNode; i++) {
      if (!t.fNodes[i].fIsFitted) {
        LOG(fatal) << "CbmKfTrackFitter: node " << i << " in the measured region is not fitted";
      }
      linearization[i].fParamDn = t.fNodes[i].fParamDn;
      linearization[i].fParamUp = t.fNodes[i].fParamUp;
    }
  }
  else {  // first approximation with straight line segments connecting the nodes
    for (int i1 = firstHitNode, i2 = firstHitNode + 1; i2 <= lastHitNode; i2++) {
      auto& n1 = t.fNodes[i1];
      auto& n2 = t.fNodes[i2];
      if (!n2.fIsXySet) {
        continue;
      }
      double dz  = n2.fZ - n1.fZ;
      double dzi = (fabs(dz) > 1.e-4) ? 1. / dz : 0.;
      double tx  = (n2.fMxy.X() - n1.fMxy.X()) * dzi;
      double ty  = (n2.fMxy.Y() - n1.fMxy.Y()) * dzi;
      for (int i = i1; i <= i2; i++) {  // fill the nodes inbetween
        auto& n  = t.fNodes[i];
        auto& l  = linearization[i];
        double x = n1.fMxy.X() + tx * (n.fZ - n1.fZ);
        double y = n1.fMxy.Y() + ty * (n.fZ - n1.fZ);
        if (i < i2
            || i == lastHitNode) {  // downstream parameters for i2 will be set later, except for the last hit node
          l.fParamDn.SetX(x);
          l.fParamDn.SetY(y);
          l.fParamDn.SetZ(n.fZ);
          l.fParamDn.SetTx(tx);
          l.fParamDn.SetTy(ty);
          l.fParamDn.SetQp(0.);
          l.fParamDn.SetTime(0.);
          l.fParamDn.SetVi(cbm::algo::kf::defs::SpeedOfLightInv<>);
        }
        if (i > i1 || i == firstHitNode) {  // upstream parameters for i1 are already set, except for the first hit node
          l.fParamUp.SetX(x);
          l.fParamUp.SetY(y);
          l.fParamUp.SetZ(n.fZ);
          l.fParamUp.SetTx(tx);
          l.fParamUp.SetTy(ty);
          l.fParamUp.SetQp(0.);
          l.fParamUp.SetTime(0.);
          l.fParamUp.SetVi(cbm::algo::kf::defs::SpeedOfLightInv<>);
        }
      }
      i1 = i2;
    }
  }

  t.fIsFitted = false;
  for (auto& n : t.fNodes) {
    n.fIsFitted = false;
  }

  auto printNode = [&](std::string str, int node) {
    if (fVerbosityLevel > 0) {
      LOG(info) << str << ": node " << node << " chi2 " << fFit.Tr().GetChiSq() << " x " << fFit.Tr().GetX() << " y "
                << fFit.Tr().GetY() << " z " << fFit.Tr().GetZ() << " tx " << fFit.Tr().GetTx() << " ty "
                << fFit.Tr().GetTy();
    }
  };


  {  // fit downstream up to the last measurement

    {  // initialisation at the first measurement
      TrajectoryNode& n = t.fNodes[firstHitNode];
      assert(n.fIsXySet);
      fFit.SetTrack(linearization[firstHitNode].fParamDn);
      FilterFirstMeasurement(n);
      printNode("fit downstream", firstHitNode);
    }

    for (int iNode = firstHitNode + 1; iNode <= lastHitNode; iNode++) {

      TrajectoryNode& n = t.fNodes[iNode];

      // transport the partially fitted track to the current node
      fFit.SetQp0(linearization[iNode - 1].fParamDn.GetQp());
      fFit.Extrapolate(n.fZ, field);

      // measure the track at the current node
      if (n.fIsXySet) {
        fFit.FilterXY(n.fMxy, fSkipUnmeasuredCoordinates);
      }
      if (n.fIsTimeSet) {
        fFit.FilterTime(n.fMt);
      }
      printNode("fit downstream", iNode);

      // store the partially fitted track before the scattering
      n.fParamUp = fFit.Tr();

      // add material effects
      AddMaterialEffects(n, linearization[iNode], kf::FitDirection::kDownstream);

      // store the partially fitted track after the scattering
      n.fParamDn = fFit.Tr();
    }
  }

  // store the chi2 for debugging purposes
  double dnChi2 = fFit.Tr().GetChiSq();

  {  // fit upstream with the Kalman Filter smoothing

    {  // initialisation at the last measurement
      TrajectoryNode& n = t.fNodes[lastHitNode];
      assert(n.fIsXySet);
      n.fIsFitted = true;
      fFit.SetTrack(linearization[lastHitNode].fParamUp);
      FilterFirstMeasurement(n);
      printNode("fit upstream", lastHitNode);
    }

    for (int iNode = lastHitNode - 1; ok && (iNode > firstHitNode); iNode--) {

      TrajectoryNode& n = t.fNodes[iNode];

      // transport the partially fitted track to the current node
      fFit.SetQp0(linearization[iNode + 1].fParamUp.GetQp());
      fFit.Extrapolate(n.fZ, field);

      // combine partially fitted downstream and upstream tracks
      ok = ok && Smooth(n.fParamDn, fFit.Tr());

      AddMaterialEffects(n, linearization[iNode], kf::FitDirection::kUpstream);

      // combine partially fitted downstream and upstream tracks
      ok = ok && Smooth(n.fParamUp, fFit.Tr());

      n.fIsFitted = true;

      // measure the track at the current node
      if (n.fIsXySet) {
        fFit.FilterXY(n.fMxy, fSkipUnmeasuredCoordinates);
      }
      if (n.fIsTimeSet) {
        fFit.FilterTime(n.fMt);
      }
      printNode("fit upstream", iNode);
    }

    if (!ok) {
      return false;
    }

    if (ok) {
      int iNode = firstHitNode;

      TrajectoryNode& n = t.fNodes[iNode];

      // transport the partially fitted track to the current node
      fFit.SetQp0(linearization[iNode + 1].fParamUp.GetQp());
      fFit.Extrapolate(n.fZ, field);

      // measure the track at the current node
      if (n.fIsXySet) {
        fFit.FilterXY(n.fMxy, fSkipUnmeasuredCoordinates);
      }
      if (n.fIsTimeSet) {
        fFit.FilterTime(n.fMt);
      }
      printNode("fit upstream", iNode);
      n.fParamDn = fFit.Tr();

      AddMaterialEffects(n, linearization[iNode], kf::FitDirection::kUpstream);

      n.fParamUp = fFit.Tr();

      n.fIsFitted = true;
    }
  }

  if (ok) {  // propagate downstream
    fFit.SetTrack(t.fNodes[lastHitNode].fParamDn);
    for (int i = lastHitNode + 1; i < nNodes; i++) {
      auto& n = t.fNodes[i];
      fFit.Extrapolate(n.fZ, field);
      n.fParamDn = fFit.Tr();
      LinearizationAtNode l;
      l.fParamDn = fFit.Tr();
      l.fParamUp = fFit.Tr();
      AddMaterialEffects(n, l, kf::FitDirection::kDownstream);
      n.fParamUp  = fFit.Tr();
      n.fIsFitted = true;
    }
  }

  if (ok) {  // propagate upstream
    fFit.SetTrack(t.fNodes[firstHitNode].fParamUp);
    for (int i = firstHitNode - 1; i >= 0; i--) {
      auto& n = t.fNodes[i];
      fFit.Extrapolate(n.fZ, field);
      n.fParamUp = fFit.Tr();
      LinearizationAtNode l;
      l.fParamDn = fFit.Tr();
      l.fParamUp = fFit.Tr();
      AddMaterialEffects(n, l, kf::FitDirection::kUpstream);
      n.fParamDn  = fFit.Tr();
      n.fIsFitted = true;
    }
  }

  assert(ok);

  if (!fDoSmooth) {
    double upChi2 = fFit.Tr().GetChiSq();
    if (fabs(upChi2 - dnChi2) > 1.e-1) {
      //if (upChi2 > dnChi2 + 1.e-2) {
      LOG(debug) << "CbmKfTrackFitter: " << fDebugInfo << ": chi2 mismatch: dn " << dnChi2 << " != up " << upChi2
                 << " first node " << firstHitNode << " last node " << lastHitNode << " of " << nNodes;
      //if (fVerbosityLevel > 0) {
      //exit(1);
      //}
    }
  }
  // distribute the final chi2, ndf to all nodes

  const auto& tt = fFit.Tr();
  for (int iNode = 0; iNode < nNodes; iNode++) {
    TrajectoryNode& n = t.fNodes[iNode];
    n.fParamDn.SetNdf(tt.GetNdf());
    n.fParamDn.SetNdfTime(tt.GetNdfTime());
    n.fParamDn.SetChiSq(tt.GetChiSq());
    n.fParamDn.SetChiSqTime(tt.GetChiSqTime());
    n.fParamUp.SetNdf(tt.GetNdf());
    n.fParamUp.SetNdfTime(tt.GetNdfTime());
    n.fParamUp.SetChiSq(tt.GetChiSq());
    n.fParamUp.SetChiSqTime(tt.GetChiSqTime());
  }

  return ok;
}

bool CbmKfTrackFitter::Smooth(kf::TrackParamD& t1, const kf::TrackParamD& t2)
{
  // TODO: move to the CaTrackFit class

  constexpr int nPar = kf::TrackParamV::kNtrackParam;
  constexpr int nCov = kf::TrackParamV::kNcovParam;

  double r[nPar] = {t1.X(), t1.Y(), t1.Tx(), t1.Ty(), t1.Qp(), t1.Time(), t1.Vi()};
  double m[nPar] = {t2.X(), t2.Y(), t2.Tx(), t2.Ty(), t2.Qp(), t2.Time(), t2.Vi()};

  double S[nCov], S1[nCov], Tmp[nCov];
  for (int i = 0; i < nCov; i++) {
    S[i] = (t1.GetCovMatrix()[i] + t2.GetCovMatrix()[i]);
  }

  int nullty = 0;
  int ifault = 0;
  cbm::algo::kf::utils::math::SymInv(S, nPar, S1, Tmp, &nullty, &ifault);

  if ((0 != ifault) || (0 != nullty)) {
    return false;
  }

  double dzeta[nPar];

  for (int i = 0; i < nPar; i++) {
    dzeta[i] = m[i] - r[i];
  }

  double C[nPar][nPar];
  double Si[nPar][nPar];
  for (int i = 0, k = 0; i < nPar; i++) {
    for (int j = 0; j <= i; j++, k++) {
      C[i][j]  = t1.GetCovMatrix()[k];
      Si[i][j] = S1[k];
      C[j][i]  = C[i][j];
      Si[j][i] = Si[i][j];
    }
  }

  // K = C * S^{-1}
  double K[nPar][nPar];
  for (int i = 0; i < nPar; i++) {
    for (int j = 0; j < nPar; j++) {
      K[i][j] = 0.;
      for (int k = 0; k < nPar; k++) {
        K[i][j] += C[i][k] * Si[k][j];
      }
    }
  }

  for (int i = 0, k = 0; i < nPar; i++) {
    for (int j = 0; j <= i; j++, k++) {
      double kc = 0.;  // K*C[i][j]
      for (int l = 0; l < nPar; l++) {
        kc += K[i][l] * C[l][j];
      }
      t1.CovMatrix()[k] = t1.CovMatrix()[k] - kc;
    }
  }

  for (int i = 0; i < nPar; i++) {
    double kd = 0.;
    for (int j = 0; j < nPar; j++) {
      kd += K[i][j] * dzeta[j];
    }
    r[i] += kd;
  }
  t1.X()    = r[0];
  t1.Y()    = r[1];
  t1.Tx()   = r[2];
  t1.Ty()   = r[3];
  t1.Qp()   = r[4];
  t1.Time() = r[5];
  t1.Vi()   = r[6];

  double chi2     = 0.;
  double chi2Time = 0.;
  for (int i = 0; i < nPar; i++) {
    double SiDzeta = 0.;
    for (int j = 0; j < nPar; j++) {
      SiDzeta += Si[i][j] * dzeta[j];
    }
    if (i < 5) {
      chi2 += dzeta[i] * SiDzeta;
    }
    else {
      chi2Time += dzeta[i] * SiDzeta;
    }
  }
  t1.SetChiSq(chi2 + t1.GetChiSq() + t2.GetChiSq());
  t1.SetChiSqTime(chi2Time + t1.GetChiSqTime() + t2.GetChiSqTime());
  t1.SetNdf(5 + t1.GetNdf() + t2.GetNdf());
  t1.SetNdfTime(2 + t1.GetNdfTime() + t2.GetNdfTime());
  return true;
}
