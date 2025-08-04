/* Copyright (C) 2024 Hulubei National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer], Omveer Singh */

//CBM headers
#include "CbmRecoQaTask.h"

#include "../../core/detectors/rich/utils/CbmRichUtil.h"
#include "CbmDefs.h"
#include "CbmEvent.h"
#include "CbmFsdHit.h"
#include "CbmGlobalTrack.h"
#include "CbmMCDataArray.h"
#include "CbmMCDataManager.h"
#include "CbmMvdHit.h"
#include "CbmPsdHit.h"
#include "CbmRichHit.h"
#include "CbmSetup.h"
#include "CbmStsAddress.h"
#include "CbmStsHit.h"
#include "CbmStsSetup.h"
#include "CbmTimeSlice.h"
#include "CbmTofAddress.h"
#include "CbmTofHit.h"
#include "CbmTrdAddress.h"
#include "CbmTrdHit.h"
// FAIR headers
#include "FairMCPoint.h"

#include <FairRootManager.h>
#include <FairSink.h>
// ROOT headers
#include <TClonesArray.h>
#include <TGeoManager.h>
#include <TGeoNode.h>
#include <TH2D.h>

#include <iterator>
#include <regex>

using namespace std;
using namespace cbm::algo;
std::bitset<CbmRecoQaTask::eRecoConfig::kRecoQaNConfigs> CbmRecoQaTask::fuRecoConfig = {};

//_____________________________________________________________________
CbmRecoQaTask::CbmRecoQaTask() : FairTask("RecoQA", 0) {}

//_____________________________________________________________________
CbmRecoQaTask::Detector* CbmRecoQaTask::AddDetector(ECbmModuleId id)
{
  /* Interface to the user. Check that the det defined in the outside world
   * fulfills the quality criteria.
   */
  if (GetDetector(id)) {
    LOG(warn) << GetName() << "::AddDetector(" << ToString(id) << ")."
              << " already registered. Using "
              << "\"CbmRecoQaTask::GetDetector(ECbmModuleId).\"";
    return GetDetector(id);
  }
  switch (id) {
    case ECbmModuleId::kMvd:
    case ECbmModuleId::kSts:
    case ECbmModuleId::kMuch:
    case ECbmModuleId::kRich:
    case ECbmModuleId::kTrd:
    case ECbmModuleId::kTrd2d:
    case ECbmModuleId::kTof:
    case ECbmModuleId::kFsd:
    case ECbmModuleId::kPsd:
      LOG(debug) << GetName() << "::AddDetector(" << ToString(id) << ").";
      fDetQa.emplace(id, id);
      break;
    default: LOG(warn) << GetName() << "::AddDetector : unsupported det " << ToString(id); return nullptr;
  }

  return &fDetQa[id];
}

//_____________________________________________________________________
CbmRecoQaTask::Detector* CbmRecoQaTask::GetDetector(ECbmModuleId id)
{
  if (fDetQa.find(id) == fDetQa.end()) return nullptr;
  return &fDetQa[id];
}

//_____________________________________________________________________
const CbmTrack* CbmRecoQaTask::GetTrack(ECbmModuleId did, int id) const
{
  if (fTracks.find(did) == fTracks.end()) {
    LOG(warning) << GetName() << " missing tracks for " << ToString(did);
    return nullptr;
  }
  if (id < 0) {
    LOG(warning) << GetName() << " negative trk idx for " << ToString(did);
    return nullptr;
  }
  const CbmTrack* trk = (const CbmTrack*) (fTracks.at(did))->At(id);
  if (!trk) {
    LOG(debug1) << GetName() << " missing trk_" << id << " for " << ToString(did);
    return nullptr;
  }
  return trk;
}

//_____________________________________________________________________
InitStatus CbmRecoQaTask::Init()
{
  fFitter.Init();
  fFitter.SetSkipUnmeasuredCoordinates(true);

  // Get ROOT Manager
  FairRootManager* ioman = FairRootManager::Instance();

  if (!ioman) {
    LOG(fatal) << GetName() << "::Init :: RootManager not instantiated!";
    return kERROR;
  }

  fGTracks = static_cast<TClonesArray*>(ioman->GetObject("GlobalTrack"));
  if (!fGTracks)
    LOG(warn) << GetName() << "::Init: Global track array not found!";
  else
    fuRecoConfig.set(kRecoTracks);

  fEvents = static_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
  if (!fEvents)
    LOG(warn) << GetName() << "::Init: No event found. Some results will be missing.";
  else
    fuRecoConfig.set(kRecoEvents);

  switch (fSetupClass) {
    case kMcbm22:
      LOG(info) << GetName() << "::Init: Setup config for \"mCBM 2022\".";
      InitMcbm22();
      break;
    case kMcbm24:
      LOG(info) << GetName() << "::Init: Setup config for \"mCBM 2024\". ";
      InitMcbm24();
      break;
    case kDefault:
      LOG(info) << GetName() << "::Init: Setup config for \"Default\". ";
      InitDefault();
      break;
  }

  if (fuRecoConfig[kUseMC]) {
    cbm_mc_manager = dynamic_cast<CbmMCDataManager*>(ioman->GetObject("MCDataManager"));
    if (!cbm_mc_manager) {
      LOG(warn) << GetName() << "::Init: MC data manager not available even though asked by user !";
      fuRecoConfig.reset(kUseMC);
    }
    else {
      fTrackMatches = static_cast<TClonesArray*>(ioman->GetObject("GlobalTrackMatch"));
      if (!fTrackMatches) {
        LOG(warn) << GetName() << "::Init: MC info for Global track not available !";
      }
    }
  }

  fOutFolder.Clear();
  for (auto& detp : fDetQa) {  // SYSTEM LOOP
    auto& det = detp.second;

    if (cbm_mc_manager) {
      fPoints[det.id] = cbm_mc_manager->InitBranch(det.point.name.data());
      if (!fPoints[det.id])
        LOG(warn) << GetName() << "::Init: MC Point array for " << ToString(det.id) << " not found!";
      fHitMatch[det.id] = static_cast<TClonesArray*>(ioman->GetObject((det.hit.name + "Match").c_str()));

      if (!fHitMatch[det.id])
        LOG(warn) << GetName() << "::Init: Hit Match array for " << ToString(det.id) << " not found!";
    }

    fHits[det.id] = static_cast<TClonesArray*>(ioman->GetObject(det.hit.name.data()));
    if (!fHits[det.id]) {
      LOG(warn) << GetName() << "::Init: Hit array for " << ToString(det.id) << " not found!";
      continue;
    }

    if (fGTracks && det.trk.id != ECbmDataType::kUnknown) {
      fTracks[det.id] = static_cast<TClonesArray*>(ioman->GetObject(det.trk.name.data()));
      if (!fTracks[det.id]) LOG(warn) << GetName() << "::Init: Track array for " << ToString(det.id) << " not found!";
    }
    if (!det.Init(&fOutFolder, (bool) fuRecoConfig[kUseMC])) continue;
  }

  TDirectoryFile* trgDir = (fuRecoConfig[kRecoEvents] || GetNviews(eViewType::kTrkProj))
                             ? (TDirectoryFile*) fOutFolder.mkdir("TRG", "Target tomography with CA")
                             : nullptr;

  // register track projection on z planes
  if (fPrjPlanes.size() && trgDir) {
    fViews.emplace("prj", View("TrkProj", "", {}));
    fViews["prj"].fType = eViewType::kTrkProj;
    int i(0);
    for (auto plane : fPrjPlanes) {  // add projection planes according to user request
      if (i >= kNtrkProjections) {
        LOG(warn) << GetName() << "::Init: Only " << kNtrkProjections << " are supported. Skipping the rest.";
        fPrjPlanes.erase(fPrjPlanes.begin() + int(kNtrkProjections), fPrjPlanes.end());
        break;
      }

      fViews["prj"].AddProjection(CbmRecoQaTask::eProjectionType(int(CbmRecoQaTask::eProjectionType::kXYt0) + i),
                                  plane.Z());
      i++;
    }
    fViews["prj"].Init("");
    fViews["prj"].Register(trgDir);
  }

  // register primary vertex QA
  if (fuRecoConfig[kRecoEvents] && trgDir) {
    fViews.emplace("vx", View("Vx", "", {}));
    fViews["vx"].fType = eViewType::kPV;
    fViews["vx"].AddProjection(CbmRecoQaTask::eProjectionType::kPVxy);
    fViews["vx"].AddProjection(CbmRecoQaTask::eProjectionType::kPVxz);
    fViews["vx"].AddProjection(CbmRecoQaTask::eProjectionType::kPVyz);
    fViews["vx"].AddProjection(CbmRecoQaTask::eProjectionType::kPVmult);
    fViews["vx"].Init("Prim");
    fViews["vx"].Register(trgDir);
  }
  return kSUCCESS;
}

/// Hit classification on system and view
template<class Hit>
bool CbmRecoQaTask::View::HasAddress(const CbmHit* h, double&, double&, double&, double&) const
{
  LOG(error) << "Unprocessed hit in view " << name;
  cout << h->ToString();

  return false;
}
// STS specialization
template<>
bool CbmRecoQaTask::View::HasAddress<CbmStsHit>(const CbmHit* h, double& x, double& y, double& dx, double& dy) const
{
  int32_t a      = h->GetAddress();
  uint32_t sel[] = {CbmStsAddress::GetElementId(a, EStsElementLevel::kStsUnit),
                    CbmStsAddress::GetElementId(a, EStsElementLevel::kStsLadder),
                    CbmStsAddress::GetElementId(a, EStsElementLevel::kStsModule)};
  uint idx(0);
  for (auto ii : fSelector) {
    if (uint(ii) != sel[idx]) break;
    idx++;
  }
  if (fSetup == eSetup::kDefault && sel[0] != uint(fSelector[0])) {
    return false;
  }
  else if (fSetup != eSetup::kDefault && idx != fSelector.size()) {
    return false;
  }
  else
    LOG(debug4) << "Accept Sts hit for " << sel[0] << " " << sel[1] << " " << sel[2];

  const CbmStsHit* h0 = dynamic_cast<const CbmStsHit*>(h);
  if (!h0) {
    LOG(error) << "Failed loading STS hit in view " << name;
    return false;
  }
  x  = h0->GetX();
  dx = h0->GetDx();
  y  = h0->GetY();
  dy = h0->GetDy();
  return true;
}
// Rich specialization
template<>
bool CbmRecoQaTask::View::HasAddress<CbmRichHit>(const CbmHit* h, double& x, double& y, double& dx, double& dy) const
{
  int16_t uId = CbmRichUtil::GetDirichId(h->GetAddress());
  // TODO implement at RichUtil level
  int8_t modId = (uId >> 8) & 0xF;

  if (fSetup != eSetup::kDefault) {
    if (modId >= fSelector[0]) return false;
  }

  const CbmRichHit* h0 = dynamic_cast<const CbmRichHit*>(h);
  if (!h0) {
    LOG(error) << "Failed loading RICH hit in view " << name;
    return false;
  }
  x  = h0->GetX();
  dx = h0->GetDx();
  y  = h0->GetY();
  dy = h0->GetDy();
  return true;
}
// TRD specialization
template<>
bool CbmRecoQaTask::View::HasAddress<CbmTrdHit>(const CbmHit* h, double& x, double& y, double& dx, double& dy) const
{
  std::vector<int> sel;

  if (fSetup == eSetup::kDefault) {
    sel = {static_cast<int>(CbmTrdAddress::GetLayerId(CbmTrdAddress::GetModuleAddress(h->GetAddress()))),
           static_cast<int>(CbmTrdAddress::GetModuleId(h->GetAddress())), -1};
  }
  else {
    sel = {static_cast<int>(CbmTrdAddress::GetModuleAddress(h->GetAddress())), -1, -1};
  }

  uint idx = 0;
  for (auto ii : fSelector) {
    if (ii != sel[idx]) break;
    idx++;
  }

  if (idx != fSelector.size()) {
    return false;
  }

  LOG(debug4) << "Accept Sts hit for " << sel[0] << " " << sel[1] << " " << sel[2];

  const CbmTrdHit* h0 = dynamic_cast<const CbmTrdHit*>(h);
  if (!h0) {
    LOG(error) << "Failed loading TRD hit in view " << name;
    return false;
  }
  x  = h0->GetX();
  dx = h0->GetDx();
  y  = h0->GetY();
  dy = h0->GetDy();
  return true;
}
// ToF specialization
template<>
bool CbmRecoQaTask::View::HasAddress<CbmTofHit>(const CbmHit* h, double& x, double& y, double& dx, double& dy) const
{
  int32_t a     = h->GetAddress();
  int32_t sel[] = {CbmTofAddress::GetSmId(a), CbmTofAddress::GetSmType(a), CbmTofAddress::GetRpcId(a)};
  uint idx(0);
  for (auto ii : fSelector) {
    if (ii != sel[idx]) break;
    idx++;
  }
  if (fSetup == eSetup::kDefault && idx != 2) {
    return false;
  }
  else if (fSetup != eSetup::kDefault && idx != fSelector.size()) {
    return false;
  }
  else
    LOG(debug4) << "Accept Tof hit for " << sel[0] << " " << sel[1] << " " << sel[2];

  const CbmTofHit* h0 = dynamic_cast<const CbmTofHit*>(h);
  if (!h0) {
    LOG(error) << "Failed loading ToF hit in view " << name;
    return false;
  }
  x  = h0->GetX();
  dx = h0->GetDx();
  y  = h0->GetY();
  dy = h0->GetDy();
  return true;
}
template<class Hit>
bool CbmRecoQaTask::View::Load(const CbmHit* h, const FairMCPoint* point, const CbmEvent* ev)
{
  double x(0), y(0), dx(0), dy(0);
  if (!HasAddress<Hit>(h, x, y, dx, dy)) {
    LOG(debug1) << "view " << name << " does not own hit " << h->ToString();
    return false;
  }

  // printf("View[%s] z(%d)[%f]\n", name.data(), (int)h->GetType(), h->GetZ());
  int scale(0);
  TH2* hh(nullptr);
  auto fillView = [&](eProjectionType proj, double xx, double yy, bool scaleY = true) {
    auto it = fProjection.find(proj);
    if (it != fProjection.end()) {
      scale = std::get<0>(it->second);
      hh    = std::get<2>(it->second);
      if (hh) hh->Fill(xx, scaleY ? yy * scale : yy);
    }
  };

  if (ev) {
    fillView(eProjectionType::kChdT, x, h->GetTime() - ev->GetStartTime());
    //fillView(eProjectionType::kXYh, x, y, false);
  }

  fillView(eProjectionType::kXYh, x, y, false);

  double event_time = ev ? ev->GetStartTime() : 1000;

  if (point) {
    fillView(eProjectionType::kXYhMC, point->GetX(), point->GetY(), false);
    fillView(eProjectionType::kResidualX, point->GetX(), x - point->GetX());
    fillView(eProjectionType::kResidualY, point->GetY(), y - point->GetY());
    fillView(eProjectionType::kResidualTX, point->GetX(), h->GetTime() - event_time - point->GetTime());
    fillView(eProjectionType::kResidualTY, point->GetY(), h->GetTime() - event_time - point->GetTime());
    fillView(eProjectionType::kPullX, point->GetX(), (x - point->GetX()) / dx);
    fillView(eProjectionType::kPullY, point->GetY(), (y - point->GetY()) / dy);
  }
  fMult++;  // register hit/point multiplicity
  return true;
}

bool CbmRecoQaTask::View::Load(const CbmKfTrackFitter::TrajectoryNode* n, const FairMCPoint* point)
{
  const kf::TrackParamD& t = n->fParamUp;
  double dx = n->fMxy.X() - t.X(), dy = n->fMxy.Y() - t.Y(), dt = n->fMt.T() - t.Time(),
         pullx = dx / sqrt(n->fMxy.Dx2() + t.GetCovariance(0, 0)),
         pully = dy / sqrt(n->fMxy.Dy2() + t.GetCovariance(1, 1));
  // pullt = dt / sqrt(n->fMt.Dt2() + n->fTrack.GetCovariance(5, 5));

  for (auto& projection : fProjection) {
    int scale = get<0>(projection.second);
    TH2* hh   = get<2>(projection.second);
    if (!hh) continue;

    switch (projection.first) {
      case eProjectionType::kXYa: hh->Fill(n->fMxy.X(), n->fMxy.Y()); break;
      case eProjectionType::kXYp: hh->Fill(t.X(), t.Y()); break;
      case eProjectionType::kXdX: hh->Fill(n->fMxy.X(), scale * dx); break;
      case eProjectionType::kYdY: hh->Fill(n->fMxy.Y(), scale * dy); break;
      case eProjectionType::kWdT: hh->Fill(n->fMxy.X(), scale * dt); break;
      case eProjectionType::kXpX: hh->Fill(n->fMxy.X(), pullx); break;
      case eProjectionType::kYpY: hh->Fill(n->fMxy.Y(), pully); break;

      default: break;
    }

    if (point) {
      double dxMC = point->GetX() - t.X(), dyMC = point->GetY() - t.Y();

      switch (projection.first) {
        case eProjectionType::kXdXMC: hh->Fill(point->GetX(), scale * dxMC); break;
        case eProjectionType::kYdYMC: hh->Fill(point->GetY(), scale * dyMC); break;
        default: break;
      }
    }
  }
  return true;
}

bool CbmRecoQaTask::View::Load(TVector3* p)
{
  for (auto& projection : fProjection) {
    int scale = get<0>(projection.second);
    TH2* hh   = get<2>(projection.second);
    if (!hh) continue;
    switch (projection.first) {
      case eProjectionType::kDmult:
        if (int(p->Z()) == -124) hh->Fill(p->X(), p->Y());
        break;
      case eProjectionType::kXYt0:
        if (int(p->Z()) == 0) hh->Fill(scale * p->X(), scale * p->Y());
        break;
      // fall through
      case eProjectionType::kXYt1:
        if (int(p->Z()) == 1) hh->Fill(scale * p->X(), scale * p->Y());
        break;
      // fall through
      case eProjectionType::kXYt2:
        if (int(p->Z()) == 2) hh->Fill(scale * p->X(), scale * p->Y());
        break;
      // fall through
      case eProjectionType::kXYt3:
        if (int(p->Z()) == 3) hh->Fill(scale * p->X(), scale * p->Y());
        break;
      // fall through
      case eProjectionType::kXYt4:
        if (int(p->Z()) == 4) hh->Fill(scale * p->X(), scale * p->Y());
        break;
      // fall through
      case eProjectionType::kXYt5:
        if (int(p->Z()) == 5) hh->Fill(scale * p->X(), scale * p->Y());
        break;
      // fall through
      case eProjectionType::kPVxy: hh->Fill(scale * p->X(), scale * p->Y()); break;
      case eProjectionType::kPVxz: hh->Fill(p->Z(), scale * p->X()); break;
      case eProjectionType::kPVyz: hh->Fill(p->Z(), scale * p->Y()); break;
      case eProjectionType::kPVmult:
        if (int(p->Z()) == -123) hh->Fill(scale * p->X(), scale * p->Y());
        break;
      default: break;
    }
  }
  return true;
}

//_____________________________________________________________________

void CbmRecoQaTask::Exec(Option_t*)
{
  LOG(info) << GetName() << "::Exec : Evs[" << (fEvents ? fEvents->GetEntriesFast() : 0) << "] Trks["
            << (fGTracks ? fGTracks->GetEntriesFast() : 0) << "] Hits["
            << (fHits[ECbmModuleId::kSts] ? fHits[ECbmModuleId::kSts]->GetEntriesFast() : 0) << " "
            << (fHits[ECbmModuleId::kTrd] ? fHits[ECbmModuleId::kTrd]->GetEntriesFast() : 0) << " "
            << (fHits[ECbmModuleId::kTof] ? fHits[ECbmModuleId::kTof]->GetEntriesFast() : 0) << "]";

  // fixed momentum no magentic field for mCBM
  fFitter.SetDefaultMomentumForMs(0.5);
  fFitter.FixMomentumForMs(true);
  int iev = 0, itrack = 0, nnodes = 0;
  auto processHits = [&](CbmEvent* ev) {
    for (auto& detp : fDetQa) {
      auto& det = detp.second;
      if (!fHits[det.id]) {
        LOG(error) << GetName() << "::Exec() : Hits for " << ToString(det.id) << " not available. Skip.";
        continue;
      }

      const int nh = (ev) ? max(int(0), int(ev->GetNofData(det.hit.id))) : fHits[det.id]->GetEntriesFast();
      for (int ih = 0; ih < nh; ++ih) {
        const int jh = (ev) ? ev->GetIndex(det.hit.id, ih) : ih;

        const FairMCPoint* mcpoint = nullptr;
        const CbmHit* hit          = dynamic_cast<CbmHit*>(fHits[det.id]->At(jh));
        if (!hit) {
          LOG(warning) << GetName() << "::Exec() : Hit " << jh << " for " << ToString(det.id)
                       << " not available. Skip.";
          continue;
        }

        if (fHitMatch[det.id] && fPoints[det.id]) {
          if (det.id == ECbmModuleId::kRich) {
            Int_t pointID = hit->GetRefId();
            if (pointID >= 0) {
              // mcpoint = dynamic_cast<FairMCPoint*>(fPoints[det.id]->At(pointID));
              if (!mcpoint) continue;
            }
          }
          else {
            const CbmMatch* match = dynamic_cast<CbmMatch*>(fHitMatch[det.id]->At(jh));
            if (match && match->GetNofLinks() > 0) {
              const auto& link = match->GetMatchedLink();
              if (ev) {
                int file_id{0}, event_id{0};
                if (ev && ev->GetMatch() && ev->GetMatch()->GetNofLinks() > 0) {
                  file_id  = ev->GetMatch()->GetMatchedLink().GetFile();
                  event_id = ev->GetMatch()->GetMatchedLink().GetEntry();
                }
                else {
                  event_id = FairRootManager::Instance()->GetEntryNr();
                }
                if (link.GetFile() != file_id || link.GetEntry() != event_id) {
                  LOG(warn) << "match from different event";
                }
              }
              mcpoint = dynamic_cast<FairMCPoint*>(fPoints[det.id]->Get(link));
            }
          }
        }

        bool ret(false);
        for (auto& view : det.fViews) {
          switch (det.id) {
            case ECbmModuleId::kMvd: ret = view.Load<CbmMvdHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kSts: ret = view.Load<CbmStsHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kMuch: ret = view.Load<CbmMuchPixelHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kRich: ret = view.Load<CbmRichHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kTrd: ret = view.Load<CbmTrdHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kTrd2d: ret = view.Load<CbmTrdHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kTof: ret = view.Load<CbmTofHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kFsd: ret = view.Load<CbmFsdHit>(hit, mcpoint, ev); break;
            case ECbmModuleId::kPsd: ret = view.Load<CbmPsdHit>(hit, mcpoint, ev); break;
            default: LOG(fatal) << GetName() << "::Exec : unsupported det " << ToString(det.id); break;
          }
          if (ret) break;
        }
      }
      TVector3 mult;
      for (auto& v : det.fViews) {
        mult.SetXYZ(nh, double(v.fMult) / nh, -124);
        v.Load(&mult);
        v.fMult = 0;
      }
    }
  };

  auto processTracks = [&](CbmEvent* ev) {
    const int ntrk = (ev) ? ev->GetNofData(ECbmDataType::kGlobalTrack) : fGTracks->GetEntriesFast();
    // read in the vertex and the list of tracks used for its definition
    TVector3 pvx, evx;
    int ntrkDet[3] = {0};

    CbmVertex* vx(nullptr);
    int nTrkVx(0);
    if (ev && fViews.find("vx") != fViews.end()) {
      vx      = ev->GetVertex();
      nTrkVx  = vx->GetNTracks();
      auto& v = fViews["vx"];
      if (nTrkVx >= 2) {
        vx->Position(pvx);
        for (int i(0); i < 3; i++) {
          evx[i] = vx->GetCovariance(i, i);
          if (evx[i] > 0.) evx[i] = sqrt(evx[i]);
        }
        v.Load(&pvx);
      }
      pvx.SetXYZ(ntrk, nTrkVx, -123);
      v.Load(&pvx);
    }
    for (int itrk = 0; itrk < ntrk; ++itrk) {
      int trkIdx = ev ? ev->GetIndex(ECbmDataType::kGlobalTrack, itrk) : itrk;
      auto track = dynamic_cast<const CbmGlobalTrack*>((*fGTracks)[trkIdx]);
      // track QA filtering
      if (!FilterTrack(track)) continue;

      if (nTrkVx >= 2) {
        if (vx->FindTrackByIndex(trkIdx)) {
          if (track->GetStsTrackIndex() >= 0) ntrkDet[0]++;
          if (track->GetTrdTrackIndex() >= 0) ntrkDet[1]++;
          if (track->GetTofTrackIndex() >= 0) ntrkDet[2]++;
        }
      }

      CbmKfTrackFitter::Trajectory trkKf;
      if (!fFitter.CreateGlobalTrack(trkKf, *track)) {
        LOG(fatal) << GetName() << "::Exec: can not create the track for the fit!";
        break;
      }
      if (!nnodes) nnodes = trkKf.fNodes.size();
      // minimalistic QA tool for tracks used for target projection
      int nhit[(size_t) ECbmModuleId::kLastModule] = {0};
      for (auto& n : trkKf.fNodes) {
        // check if hit is well defined and detector is registered
        if (n.fHitSystemId == ECbmModuleId::kNotExist || fDetQa.find(n.fHitSystemId) == fDetQa.end()) continue;
        auto& det = fDetQa[n.fHitSystemId];
        // det.Print();
        auto view = det.FindView(n.fMxy.X(), n.fMxy.Y(), n.fZ);
        if (!view) {
          LOG(debug) << GetName() << "::Exec: view for tracking layer " << ToString(det.id) << " not defined.";
          continue;
        }
        n.fIsTimeSet = n.fIsXySet = false;
        fFitter.FitTrajectory(trkKf);

        // match to MC
        const CbmHit* hit = dynamic_cast<CbmHit*>(fHits[n.fHitSystemId]->At(n.fHitIndex));
        if (!hit) continue;
        const FairMCPoint* mcpoint = nullptr;

        if (fHitMatch[n.fHitSystemId] && fPoints[n.fHitSystemId]) {
          if (n.fHitSystemId == ECbmModuleId::kRich) {
            Int_t pointID = hit->GetRefId();
            if (pointID >= 0) {
              // mcpoint = dynamic_cast<FairMCPoint*>(fPoints[n.fHitSystemId]->At(pointID));
            }
          }
          else {
            const CbmMatch* match = dynamic_cast<CbmMatch*>(fHitMatch[n.fHitSystemId]->At(n.fHitIndex));
            if (match && match->GetNofLinks() > 0) {
              const auto& link = match->GetMatchedLink();
              mcpoint          = dynamic_cast<FairMCPoint*>(fPoints[n.fHitSystemId]->Get(link));
            }
          }
        }

        view->Load(&n, mcpoint);
        nhit[(int) n.fHitSystemId]++;
        n.fIsTimeSet = n.fIsXySet = true;
      }

      // Fit track with all hits ON for track projections
      if (!nnodes) nnodes = (int) trkKf.fNodes.size();
      if (fViews.find("prj") != fViews.end()) {
        auto v = fViews["prj"];
        if (v.fType != eViewType::kTrkProj)
          LOG(error) << GetName() << "::Exec: view for track projection with wrong type. Skipping.";
        else {
          int iprj(0);
          for (auto plane : fPrjPlanes) {
            CbmKfTrackFitter::TrajectoryNode n = CbmKfTrackFitter::TrajectoryNode();
            n.fZ                               = plane.Z();
            n.fIsXySet                         = true;
            n.fReference1                      = iprj++;
            trkKf.fNodes.push_back(n);
          }
          trkKf.OrderNodesInZ();
          fFitter.FitTrajectory(trkKf);

          TVector3 xyz;
          for (auto& n : trkKf.fNodes) {
            if (n.fReference1 < 0) continue;
            xyz.SetXYZ(n.fParamUp.X(), n.fParamUp.Y(), n.fReference1);  // to communicate to the Load function
            v.Load(&xyz);
          }
        }
        // if (mod->hdYy.second) mod->hdYy.second->Fill(mod->hdYy.first *
        // n.fTrack.X()[0], n.fTrack.Time()[0]);
      }
      itrack++;
    }
  };

  if (fEvents) {
    for (auto evObj : *fEvents) {
      auto ev = dynamic_cast<CbmEvent*>(evObj);
      if (!FilterEvent(ev)) continue;
      processHits(ev);
      if (fGTracks) processTracks(ev);
      iev++;
    }
    LOG(info) << GetName() << "::Exec : Evs(%)["
              << (fEvents->GetEntriesFast() ? 1.e2 * iev / fEvents->GetEntriesFast() : 0) << "] Trks(%)["
              << (fGTracks && fGTracks->GetEntriesFast() ? 1.e2 * itrack / fGTracks->GetEntriesFast() : 0) << "]";
    return;  // END EbyE QA
  }
  else {

    processHits(nullptr);

    if (!fGTracks) {
      LOG(info) << GetName() << "::Exec : TS local reco only.";
      return;  // END TS QA (no tracking)
    }

    processTracks(nullptr);
  }
  LOG(info) << GetName() << "::Exec : Trks(%)["
            << (fGTracks && fGTracks->GetEntriesFast() ? 1.e2 * itrack / fGTracks->GetEntriesFast() : 0) << "]";
  return;
}

//_____________________________________________________________________
int CbmRecoQaTask::GetNviews(eViewType type) const
{
  int n(0);
  for (auto v : fViews) {
    if (v.second.fType != type) continue;
    n++;
  }
  return n;
}


//_____________________________________________________________________
void CbmRecoQaTask::Finish()
{
  FairSink* sink = FairRootManager::Instance()->GetSink();
  sink->WriteObject(&fOutFolder, nullptr);
}

//_____________________________________________________________________
bool CbmRecoQaTask::FilterEvent(const CbmEvent* ptr)
{
  // sanity checks
  if (ptr->GetStartTime() < 0) return false;  // this is found in MC

  for (auto evCut : fFilterEv) {
    if (!evCut.Accept(ptr, this)) return false;
  }
  return true;
}

//_____________________________________________________________________
bool CbmRecoQaTask::FilterTrack(const CbmGlobalTrack* ptr)
{
  for (auto trkCut : fFilterTrk) {
    if (!trkCut.Accept(ptr, this)) return false;
  }
  return true;
}

//_____________________________________________________________________
TString CbmRecoQaTask::GetGeoTagForDetector(const TString& detector)
{
  gGeoManager->CdTop();
  TGeoNode* cave = gGeoManager->GetCurrentNode();
  if (!cave) {
    LOG(error) << "Error: Could not get the top node in the geometry manager." << std::endl;
    return "";
  }

  for (Int_t iNode = 0; iNode < cave->GetNdaughters(); iNode++) {
    TString name = cave->GetDaughter(iNode)->GetVolume()->GetName();
    if (name.Contains(detector, TString::kIgnoreCase)) {
      return name.Contains("mcbm", TString::kIgnoreCase) ? TString(name(5, name.Length()))
                                                         : TString(name(5, name.Length() - 5));
    }
  }
  return "";
}

//_____________________________________________________________________
std::vector<TString> CbmRecoQaTask::GetPath(TGeoNode* node, TString detector, TString activeNodeName, int depth,
                                            const TString& path)
{
  std::vector<TString> nodePaths;

  if (!node) {
    return nodePaths;
  }

  TString nodePath = path;
  if (!nodePath.IsNull()) {
    nodePath += "/";
  }
  nodePath += node->GetName();

  if (TString(node->GetName()).Contains(activeNodeName)) {
    if (nodePath.Contains(detector)) nodePaths.push_back(nodePath);
  }

  // Recursively traverse the daughters
  Int_t numDaughters = node->GetNdaughters();
  for (Int_t i = 0; i < numDaughters; ++i) {
    TGeoNode* daughterNode = node->GetDaughter(i);

    std::vector<TString> result = GetPath(daughterNode, detector, activeNodeName, depth + 1, nodePath);
    nodePaths.insert(nodePaths.end(), result.begin(), result.end());
  }

  return nodePaths;
}
//_____________________________________________________________________
void CbmRecoQaTask::InitMcbm22()
{
  CbmSetup* setup = CbmSetup::Instance();
  if (!setup) {
    LOG(fatal) << GetName() << "::InitMcbm22() : Missing setup definition.";
    return;
  }
  // fixed momentum no magentic field for mCBM
  fFitter.SetDefaultMomentumForMs(0.5);
  fFitter.FixMomentumForMs(true);

  View* v = nullptr;

  std::vector<TString> path;
  TGeoNode* topNode = gGeoManager->GetTopNode();
  if (!topNode) {
    LOG(error) << "Error: Top node not found.";
    return;
  }

  auto processDetector = [&](const std::string& detector, const std::string& component) {
    TString geoTag = GetGeoTagForDetector(detector);

    if (geoTag.Length() > 0) {
      LOG(info) << detector << ": geometry tag is " << geoTag;
    }
    else {
      LOG(warn) << "Warning: No geometry tag found for detector " << detector;
    }

    path = GetPath(topNode, detector, component, 0);
  };

  if (setup->IsActive(ECbmModuleId::kSts)) {

    processDetector("sts", "Sensor");

    std::regex pattern("/Station(\\d+)_(\\d+)/Ladder(\\d+)_(\\d+)/HalfLadder\\d+d_(\\d+)/"
                       "HalfLadder\\d+d_Module(\\d+)_(\\d+)/Sensor(\\d+)_(\\d+)");

    Detector* sts = AddDetector(ECbmModuleId::kSts);
    for (const auto& str : path) {
      std::string Str(str.Data());
      std::smatch match;
      if (std::regex_search(Str, match, pattern)) {
        int station = std::stoi(match[2]);
        int ladder  = std::stoi(match[4]);
        int module  = std::stoi(match[7]);

        const char* fType = Form("U%dL%dM%d", station - 1, ladder - 1, module - 1);

        v = sts->AddView(fType, str.Data(), {station - 1, ladder - 1, module - 1});
        v->SetProjection(eProjectionType::kXdX, (station == 1) ? 1 : 0.75, "mm");
        v->SetProjection(eProjectionType::kYdY, (station == 1) ? 4 : 3, "mm");
        if (fuRecoConfig[kUseMC]) {
          v->SetProjection(eProjectionType::kXdXMC, 1, "mm");
          v->SetProjection(eProjectionType::kYdYMC, 3, "mm");
          v->SetProjection(eProjectionType::kResidualX, 200, "um");
          v->SetProjection(eProjectionType::kResidualY, 500, "um");
        }
      }
      else {
        std::cout << "No match found in string: " << str << std::endl;
      }
    }
  }

  if (setup->IsActive(ECbmModuleId::kTrd)) {
    processDetector("trd", "module");

    // Trd2d
    Detector* trd = AddDetector(ECbmModuleId::kTrd2d);

    for (const auto& str : path) {
      if (!str.Contains("module9")) continue;
      v = trd->AddView("2D", str.Data(), {5});
      v->SetProjection(eProjectionType::kChdT, 400, "ns");
      v->SetProjection(eProjectionType::kXdX, 10, "mm");
      v->SetProjection(eProjectionType::kYdY, 20, "mm");
      if (fuRecoConfig[kUseMC]) {
        v->SetProjection(eProjectionType::kXdXMC, 10, "mm");
        v->SetProjection(eProjectionType::kYdYMC, 20, "mm");
        v->SetProjection(eProjectionType::kResidualX, 1.5, "mm");
        v->SetProjection(eProjectionType::kResidualY, 5.0, "mm");
      }
    }
    // Trd1DxDy
    trd = AddDetector(ECbmModuleId::kTrd);
    for (const auto& str : path) {
      LOG(info) << str;
      if (str.Contains("layer02")) {
        v = trd->AddView("1Dx", str.Data(), {21});
        v->SetProjection(eProjectionType::kChdT, 400, "ns");
        v->SetProjection(eProjectionType::kXdX, 1.5, "cm");
        if (fuRecoConfig[kUseMC]) {
          v->SetProjection(eProjectionType::kXdXMC, 1.5, "cm");
          v->SetProjection(eProjectionType::kResidualX, 1.5, "mm");
        }
      }
      if (str.Contains("layer03")) {
        v = trd->AddView("1Dy", str.Data(), {37});
        v->SetProjection(eProjectionType::kChdT, 400, "ns");
        v->SetProjection(eProjectionType::kYdY, 1.5, "cm");
        if (fuRecoConfig[kUseMC]) {
          v->SetProjection(eProjectionType::kYdYMC, 1.5, "cm");
          v->SetProjection(eProjectionType::kResidualY, 1.5, "mm");
        }
      }
    }
  }


  if (setup->IsActive(ECbmModuleId::kTof)) {
    processDetector("tof", "counter");

    Detector* tof = AddDetector(ECbmModuleId::kTof);
    tof->hit.name = "TofHit";
    std::regex pattern("module_(\\d+)_(\\d+)/gas_box_(\\d+)/counter_(\\d+)");

    for (const auto& str : path) {
      std::string Str(str.Data());
      std::smatch match;
      if (std::regex_search(Str, match, pattern)) {
        int type         = std::stoi(match[1]);
        int smid         = std::stoi(match[2]);
        int rpc          = std::stoi(match[4]);
        const char* name = Form("Sm%d_%dRpc%d", type, smid, rpc);
        if (type == 0) {
          v = tof->AddView(name, str.Data(), {smid, type, rpc});
          v->SetProjection(eProjectionType::kChdT, 15, "ns");
        }
        else {
          v = tof->AddView(name, str.Data(), {smid, type, rpc});
        }
      }
      else {
        std::cout << "No match found in string: " << str << std::endl;
      }
    }
  }
  // ===============================================================================
  // TRG - upstream projections
  float angle = 25., L[] = {14.3, 0, -20, -38 /*-1*/, -50.5 /*-4.1*/};
  //R[]= {2.5, 3, 0.5, 0.6, 0.6}, dx[5], dz[5];
  for (int i(0); i < 5; i++) {
    fPrjPlanes.emplace_back(L[i] * TMath::Sin(angle * TMath::DegToRad()), 0.,
                            L[i] * TMath::Cos(angle * TMath::DegToRad()));
  }
  //     (cm); T_{trk} - T_{ev} (ns)", prj.first),
}

//_____________________________________________________________________
void CbmRecoQaTask::InitMcbm24()
{
  CbmSetup* setup = CbmSetup::Instance();
  if (!setup) {
    LOG(fatal) << GetName() << "::InitMcbm24() : Missing setup definition.";
    return;
  }
  // fixed momentum no magentic field for mCBM
  fFitter.SetDefaultMomentumForMs(0.5);
  fFitter.FixMomentumForMs(true);

  TString dtag;
  View* v(nullptr);
  if (setup->IsActive(ECbmModuleId::kSts)) {
    // setup->GetGeoTag(ECbmModuleId::kSts, dtag);
    // LOG(debug) << GetName() << "::InitMcbm24() : found " << dtag;
    dtag          = "v24c_mcbm";
    Detector* sts = AddDetector(ECbmModuleId::kSts);
    // U0 L0 M0
    v = sts->AddView("U0L0M0",
                     Form("/cave_1/sts_%s_0/Station01_1/Ladder13_1/"
                          "HalfLadder13u_1/HalfLadder13u_Module03_1/Sensor03_1",
                          dtag.Data()),
                     {0, 0, 0});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    // U1 L0 M0
    v = sts->AddView("U1L0M0",
                     Form("/cave_1/sts_%s_0/Station02_2/Ladder09_1/"
                          "HalfLadder09d_2/HalfLadder09d_Module03_1/Sensor03_1",
                          dtag.Data()),
                     {1, 0, 0});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    v = sts->AddView("U1L0M1",
                     Form("/cave_1/sts_%s_0/Station02_2/Ladder09_1/"
                          "HalfLadder09d_2/HalfLadder09d_Module03_2/Sensor03_1",
                          dtag.Data()),
                     {1, 0, 1});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    // U1 L1
    v = sts->AddView("U1L1M0",
                     Form("/cave_1/sts_%s_0/Station02_2/Ladder09_2/"
                          "HalfLadder09d_2/HalfLadder09d_Module03_1/Sensor03_1",
                          dtag.Data()),
                     {1, 1, 0});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    v = sts->AddView("U1L1M1",
                     Form("/cave_1/sts_%s_0/Station02_2/Ladder09_2/"
                          "HalfLadder09d_2/HalfLadder09d_Module03_2/Sensor03_1",
                          dtag.Data()),
                     {1, 1, 1});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    // U2 L0
    v = sts->AddView("U2L0M0",
                     Form("/cave_1/sts_%s_0/Station03_3/Ladder10_1/"
                          "HalfLadder10d_2/HalfLadder10d_Module03_1/Sensor03_1",
                          dtag.Data()),
                     {2, 0, 0});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    v = sts->AddView("U2L0M1",
                     Form("/cave_1/sts_%s_0/Station03_3/Ladder10_1/"
                          "HalfLadder10d_2/HalfLadder10d_Module04_2/Sensor04_1",
                          dtag.Data()),
                     {2, 0, 1});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    // U2 L1
    v = sts->AddView("U2L1M0",
                     Form("/cave_1/sts_%s_0/Station03_3/Ladder12_2/"
                          "HalfLadder12d_2/HalfLadder12d_Module03_1/Sensor03_1",
                          dtag.Data()),
                     {2, 1, 0});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    v = sts->AddView("U2L1M1",
                     Form("/cave_1/sts_%s_0/Station03_3/Ladder12_2/"
                          "HalfLadder12d_2/HalfLadder12d_Module04_2/Sensor04_1",
                          dtag.Data()),
                     {2, 1, 1});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    // U2 L2
    v = sts->AddView("U2L2M0",
                     Form("/cave_1/sts_%s_0/Station03_3/Ladder11_3/"
                          "HalfLadder11d_2/HalfLadder11d_Module03_1/Sensor03_1",
                          dtag.Data()),
                     {2, 2, 0});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
    v = sts->AddView("U2L2M1",
                     Form("/cave_1/sts_%s_0/Station03_3/Ladder11_3/"
                          "HalfLadder11d_2/HalfLadder11d_Module03_2/Sensor03_1",
                          dtag.Data()),
                     {2, 2, 1});
    v = sts->AddView("U2L2M2",
                     Form("/cave_1/sts_%s_0/Station03_3/Ladder11_3/"
                          "HalfLadder11d_2/HalfLadder11d_Module03_3/Sensor03_1",
                          dtag.Data()),
                     {2, 2, 2});
    v->SetProjection(eProjectionType::kYdY, 2, "mm");
    v->SetProjection(eProjectionType::kXdX, 500, "um");
  }
  if (setup->IsActive(ECbmModuleId::kTrd)) {
    // setup->GetGeoTag(ECbmModuleId::kTrd, dtag);
    // LOG(debug) << GetName() << "::InitMcbm24() : found " << dtag;
    dtag          = "v24e_mcbm";
    Detector* trd = AddDetector(ECbmModuleId::kTrd2d);
    // Trd2D
    v = trd->AddView("2D", Form("/cave_1/trd_%s_0/layer01_20101/module9_101001001/gas_0", dtag.Data()), {5});
    v->SetProjection(eProjectionType::kChdT, 300, "ns");
    // Trd1Dx
    trd = AddDetector(ECbmModuleId::kTrd);
    v   = trd->AddView("1Dx", Form("/cave_1/trd_%s_0/layer02_10202/module5_101002001", dtag.Data()), {21});
    v->SetProjection(eProjectionType::kChdT, 300, "ns");
    v->SetProjection(eProjectionType::kXdX, 1.5, "cm");
    v->SetProjection(eProjectionType::kYdY, 3.0, "cm");
    // Trd1Dy
    v = trd->AddView("1Dy", Form("/cave_1/trd_%s_0/layer03_11303/module5_101103001", dtag.Data()), {37});
    v->SetProjection(eProjectionType::kChdT, 300, "ns");
    v->SetProjection(eProjectionType::kXdX, 1.5, "cm");
    v->SetProjection(eProjectionType::kYdY, 3.0, "cm");
  }
  if (setup->IsActive(ECbmModuleId::kTof)) {
    // setup->GetGeoTag(ECbmModuleId::kTof, dtag);
    // LOG(debug) << GetName() << "::InitMcbm24() : found " << dtag;
    dtag          = "v24d_mcbm";
    Detector* tof = AddDetector(ECbmModuleId::kTof);
    vector<int> tofSelect(3);
    // add type 0
    for (int ism(0); ism < 6; ism++) {
      tofSelect[0] = ism;
      for (int irpc(0); irpc < 5; irpc++) {
        tofSelect[2] = irpc;
        v            = tof->AddView(Form("Sm%dRpc%d", ism, irpc),
                         Form("/cave_1/tof_%s_0/tof_%sStand_1/module_%d_%d/"
                              "gas_box_0/counter_%d",
                              dtag.Data(), dtag.Data(), tofSelect[1], tofSelect[0], tofSelect[2]),
                         tofSelect);
        v->SetProjection(eProjectionType::kXdX, 3, "cm");
        v->SetProjection(eProjectionType::kYdY, 1, "cm");
      }
    }
    // add type 6 (Buch)
    tofSelect[0] = 0;
    tofSelect[1] = 6;
    for (int irpc(0); irpc < 2; irpc++) {
      tofSelect[2] = irpc;
      tof->AddView(Form("BuchRpc%d", irpc),
                   Form("/cave_1/tof_%s_0/tof_%sStand_1/module_%d_%d/gas_box_0/"
                        "counter_%d",
                        dtag.Data(), dtag.Data(), tofSelect[1], tofSelect[0], tofSelect[2]),
                   tofSelect);
    }
    // add type 9
    tofSelect[1] = 9;
    for (int ism(0); ism < 2; ism++) {
      tofSelect[0] = ism;
      for (int irpc(0); irpc < 2; irpc++) {
        tofSelect[2] = irpc;
        tof->AddView(Form("Test%dRpc%d", ism, irpc),
                     Form("/cave_1/tof_%s_0/tof_%sStand_1/module_%d_%d/"
                          "gas_box_0/counter_%d",
                          dtag.Data(), dtag.Data(), tofSelect[1], tofSelect[0], tofSelect[2]),
                     tofSelect);
      }
    }
    // add type 2
    tofSelect[1] = 2;
    for (int ism(0); ism < 2; ism++) {
      tofSelect[0] = ism;
      for (int irpc(0); irpc < 5; irpc++) {
        tofSelect[2] = irpc;
        tof->AddView(Form("Sm2%dRpc%d", ism, irpc),
                     Form("/cave_1/tof_%s_0/tof_%sStand_1/module_%d_%d/"
                          "gas_box_0/counter_%d",
                          dtag.Data(), dtag.Data(), tofSelect[1], tofSelect[0], tofSelect[2]),
                     tofSelect);
      }
    }
  }
  if (setup->IsActive(ECbmModuleId::kRich)) {
    // setup->GetGeoTag(ECbmModuleId::kRich, dtag);
    // LOG(debug) << GetName() << "::InitMcbm24() : found " << dtag;
    dtag           = "v24a_mcbm";
    Detector* rich = AddDetector(ECbmModuleId::kRich);
    // Aerogel 1
    rich->AddView("Aerogel", Form("/cave_1/rich_%s_0/box_1/Gas_1", dtag.Data()), {4});
  }
  // ===============================================================================
  // TRG - upstream projections
  float angle = 25., L[] = {14.3, 0, -20, -38, -50.5};
  //R[]= {2.5, 3, 0.5, 0.6, 0.6}, dx[5], dz[5];
  for (int i(0); i < 5; i++) {
    fPrjPlanes.emplace_back(L[i] * TMath::Sin(angle * TMath::DegToRad()), 0.,
                            L[i] * TMath::Cos(angle * TMath::DegToRad()));
  }
}
//_____________________________________________________________________
void CbmRecoQaTask::InitDefault()
{
  LOG(info) << "Init Default ....";
  CbmSetup* setup = CbmSetup::Instance();
  if (!setup) {
    LOG(fatal) << GetName() << "::InitDefault() : Missing setup definition.";
    return;
  }

  View* v = nullptr;
  std::vector<TString> path;
  TGeoNode* topNode = gGeoManager->GetTopNode();
  if (!topNode) {
    LOG(error) << "Error: Top node not found.";
    return;
  }

  TString geoTag;
  auto processDetector = [&](const std::string& detector, const std::string& component) {
    geoTag = GetGeoTagForDetector(detector);

    if (geoTag.Length() > 0) {
      LOG(info) << detector << ": geometry tag is " << geoTag;
    }
    else {
      LOG(warn) << "Warning: No geometry tag found for detector " << detector;
    }
    path = GetPath(topNode, detector, component, 0);
  };

  if (setup->IsActive(ECbmModuleId::kSts)) {
    processDetector("sts", "Unit");

    // std::regex pattern("Unit\\d{2}[LR]_\\d+");
    std::regex pattern(R"(Unit(\d{2}[LR])_(\d+))");

    Detector* sts = AddDetector(ECbmModuleId::kSts);

    for (const auto& str : path) {
      std::string Str(str.Data());
      std::smatch match;

      if (std::regex_search(Str, match, pattern)) {
        int unitid           = std::stoi(match[2].str());
        std::string unitname = (Str.find("Unit") != std::string::npos) ? Str.substr(Str.find("Unit")) : "";

        v = sts->AddView(unitname.c_str(), str.Data(), {unitid - 1, -1, -1});
        v->SetSetup(eSetup::kDefault);
      }
      else {
        std::cout << "No match found in string: " << str << std::endl;
      }
    }
  }

  if (setup->IsActive(ECbmModuleId::kTrd)) {
    processDetector("trd", "module");
    std::regex pattern("layer(\\d+)_(\\d+)/module(\\d+)_(\\d+)");
    Detector* trd = AddDetector(ECbmModuleId::kTrd);
    char name[256];
    for (const auto& str : path) {
      std::string Str(str.Data());
      std::smatch match;
      if (std::regex_search(Str, match, pattern)) {
        int layer       = std::stoi(match[1]);
        int layercopyNr = std::stoi(match[2]);

        int module       = std::stoi(match[3]);
        int modulecopyNr = std::stoi(match[4]);

        int fLayer      = ((layercopyNr / 100) % 10);
        int fModuleCopy = (modulecopyNr % 100);

        sprintf(name, "layer%d_%d_module_%d_%d", layer, layercopyNr, module, modulecopyNr);

        v = trd->AddView(name, str.Data(), {fLayer - 1, fModuleCopy - 1, -1});
        v->SetSetup(eSetup::kDefault);
      }
      else {
        std::cout << "No match found in string: " << str << std::endl;
      }
    }
  }

  if (setup->IsActive(ECbmModuleId::kRich)) {
    processDetector("rich", "sens");
    Detector* rich = AddDetector(ECbmModuleId::kRich);
    for (const auto& str : path) {
      v = rich->AddView("rich", str.Data(), {-1, -1, -1});
      v->SetSetup(eSetup::kDefault);
    }
  }

  if (setup->IsActive(ECbmModuleId::kTof)) {
    processDetector("tof", "module");

    Detector* tof = AddDetector(ECbmModuleId::kTof);
    tof->hit.name = "TofHit";
    std::regex pattern("module_(\\d+)_(\\d+)");
    for (const auto& str : path) {
      std::string Str(str.Data());
      std::smatch match;
      if (std::regex_search(Str, match, pattern)) {
        int type = std::stoi(match[1]);
        int smid = std::stoi(match[2]);

        std::string modulename = (Str.find("module") != std::string::npos) ? Str.substr(Str.find("module")) : "";

        v = tof->AddView(modulename.c_str(), str.Data(), {smid, type, -1});
        v->SetSetup(eSetup::kDefault);
      }
      else {
        std::cout << "No match found in string: " << str << std::endl;
      }
    }
  }
}

//========= DETECTOR ======================
CbmRecoQaTask::Detector::Detector(ECbmModuleId did)
{
  id = did;
  switch (id) {
    case ECbmModuleId::kMvd:
      new (&hit) Data(ECbmDataType::kMvdHit, "MvdHit");
      new (&point) Data(ECbmDataType::kMvdPoint, "MvdPoint");
      break;
    case ECbmModuleId::kSts:
      new (&hit) Data(ECbmDataType::kStsHit, "StsHit");
      new (&point) Data(ECbmDataType::kStsPoint, "StsPoint");
      new (&trk) Data(ECbmDataType::kStsTrack, "StsTrack");
      break;
    case ECbmModuleId::kMuch:
      new (&hit) Data(ECbmDataType::kMuchPixelHit, "MuchHit");
      new (&point) Data(ECbmDataType::kMuchPoint, "MuchPoint");
      break;
    case ECbmModuleId::kRich:
      new (&hit) Data(ECbmDataType::kRichHit, "RichHit");
      new (&point) Data(ECbmDataType::kRichPoint, "RichPoint");
      break;
    case ECbmModuleId::kTrd:
    case ECbmModuleId::kTrd2d:
      new (&hit) Data(ECbmDataType::kTrdHit, "TrdHit");
      new (&point) Data(ECbmDataType::kTrdPoint, "TrdPoint");
      new (&trk) Data(ECbmDataType::kTrdTrack, "TrdTrack");
      break;
    case ECbmModuleId::kTof:
      new (&hit) Data(ECbmDataType::kTofHit, "TofHit");
      new (&point) Data(ECbmDataType::kTofPoint, "TofPoint");
      new (&trk) Data(ECbmDataType::kTofTrack, "TofTrack");
      break;
    case ECbmModuleId::kFsd:
      new (&hit) Data(ECbmDataType::kFsdHit, "FsdHit");
      new (&point) Data(ECbmDataType::kFsdHit, "FsdPoint");
      break;
    case ECbmModuleId::kPsd:
      new (&hit) Data(ECbmDataType::kPsdHit, "PsdHit");
      new (&point) Data(ECbmDataType::kPsdPoint, "PsdPoint");
      break;
    default:
      LOG(warn) << "QA unsupported for Detector=" << ToString(did);
      id = ECbmModuleId::kNotExist;
      break;
  }
}

//+++++++++++++++++++++  Detector  ++++++++++++++++++++++++++++++++
//_________________________________________________________________
CbmRecoQaTask::View* CbmRecoQaTask::Detector::AddView(const char* n, const char* p, std::vector<int> set)
{
  View* v(nullptr);
  fViews.emplace_back(n, p, set);
  v = &fViews.back();
  v->AddProjection(CbmRecoQaTask::eProjectionType::kChdT);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kXYh);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kXYhMC);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kDmult);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kDmultMC);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kPullX);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kPullY);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kResidualX);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kResidualY);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kResidualTX);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kResidualTY);

  v->AddProjection(CbmRecoQaTask::eProjectionType::kXYa);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kXYp);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kXdX);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kXdXMC);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kYdY);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kYdYMC);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kWdT);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kXpX);
  v->AddProjection(CbmRecoQaTask::eProjectionType::kYpY);

  return v;
}

CbmRecoQaTask::View* CbmRecoQaTask::Detector::GetView(const char* n)
{
  for (auto& view : fViews)
    if (view.name.compare(n) == 0) return &view;

  return nullptr;
}

CbmRecoQaTask::View* CbmRecoQaTask::Detector::FindView(double x, double y, double z)
{
  for (auto& v : fViews) {
    // printf("Looking for p[%f %f %f] in V[%s-%s] @ %f %f %f (%f %f %f)\n", x,
    // y, z, ToString(id).data(), v.name.data(), v.pos[0], v.pos[1], v.pos[2],
    // v.size[0], v.size[1], v.size[2]);
    if (abs(v.pos[0] - x) > 0.5 * v.size[0]) continue;
    if (abs(v.pos[1] - y) > 0.5 * v.size[1]) continue;
    if (abs(v.pos[2] - z) > 0.5 * v.size[2]) continue;
    return &v;
  }

  return nullptr;
}

bool CbmRecoQaTask::Detector::Init(TDirectoryFile* fOut, bool mc)
{
  /* Query the geometry and trigger detector views init
   */
  if (!gGeoManager) {
    LOG(fatal) << "CbmRecoQaTask::Detector::Init() " << ToString(id) << " missing geometry.";
    return false;
  }
  //LOG(debug) << "CbmRecoQaTask::Detector::Init() : " << ToString(id);
  LOG(info) << "CbmRecoQaTask::Detector::Init() : " << ToString(id) << " MC = " << mc;

  TDirectoryFile* modDir =
    (TDirectoryFile*) fOut->mkdir(ToString(id).data(), Form("Reco QA for %s", ToString(id).data()));
  bool ret(true);
  for (auto& view : fViews) {
    bool vret = view.Init(ToString(id).data(), mc);
    view.Register(modDir);
    ret = ret && vret;
  }
  return ret;
}

void CbmRecoQaTask::Detector::Print() const
{
  stringstream s;
  s << "D[" << ToString(id) << "] views[" << fViews.size() << "]\n";
  for (auto v : fViews)
    s << v.ToString();
  cout << s.str();
}

//+++++++++++++++++++++  View  ++++++++++++++++++++++++++++++++
//_______________________________________________________________________
bool CbmRecoQaTask::View::AddProjection(eProjectionType prj, float range, const char* unit)
{
  if (fProjection.find(prj) != fProjection.end()) {
    LOG(warn) << "Projection " << ToString(prj) << " already registered";
    return false;
  }
  int scale(1);
  if (strcmp(unit, "mm") == 0)
    scale = 10;
  else if (strcmp(unit, "um") == 0)
    scale = 10000;
  else if (strcmp(unit, "ps") == 0)
    scale = 1000;
  else if (strcmp(unit, "eV") == 0)
    scale = 1000;
  else if (strcmp(unit, "cm") == 0)
    scale = 1;
  else if (strcmp(unit, "ns") == 0)
    scale = 1;
  else if (strcmp(unit, "keV") == 0)
    scale = 1;
  else if (strcmp(unit, "a.u.") == 0)
    scale = 1;
  else
    LOG(warn) << "Projection units " << unit << " not registered. Natural units will be used.";

  fProjection[prj] = make_tuple(scale, range, nullptr);
  return true;
}

bool CbmRecoQaTask::View::SetProjection(eProjectionType prj, float range, const char* unit)
{
  if (fProjection.find(prj) == fProjection.end()) {
    LOG(warn) << "Projection " << ToString(prj)
              << " not initialized. Calling "
                 "\"CbmRecoQaTask::View::AddProjection()\"";
    return AddProjection(prj, range, unit);
  }
  int scale(1);
  if (strcmp(unit, "mm") == 0)
    scale = 10;
  else if (strcmp(unit, "um") == 0)
    scale = 10000;
  else if (strcmp(unit, "ps") == 0)
    scale = 1000;
  else if (strcmp(unit, "eV") == 0)
    scale = 1000;
  else if (strcmp(unit, "cm") == 0)
    scale = 1;
  else if (strcmp(unit, "ns") == 0)
    scale = 1;
  else if (strcmp(unit, "keV") == 0)
    scale = 1;
  else if (strcmp(unit, "a.u.") == 0)
    scale = 1;
  else
    LOG(warn) << "Projection units " << unit << " not registered. Natural units will be used.";

  get<0>(fProjection[prj]) = scale;
  get<1>(fProjection[prj]) = range;
  return true;
}

bool CbmRecoQaTask::View::Init(const char* dname, bool mc)
{
  /** All projections type according to enum eProjectionType are defined here.
   * Additionally the linking with the geometry, in the case of detector view type is also done to check the alignment.
   * If the mc flag is set, histograms relevant for MC truth are also build along side the reconstruction ones.
   */
  if (path.size() && fType == eViewType::kDetUnit) {  // applies only for detection units
    if (!gGeoManager->cd(path.data())) return false;
    TGeoHMatrix* m = gGeoManager->GetCurrentMatrix();
    const double* tr(m->GetTranslation());
    TGeoVolume* v = gGeoManager->GetCurrentVolume();
    TGeoShape* bb = v->GetShape();
    double w_lo, w_hi;
    for (int i(0); i < 3; i++) {  // loop over the spatial dimensions
      size[i] = bb->GetAxisRange(i + 1, w_lo, w_hi);
      pos[i]  = 0.5 * (w_lo + w_hi) + tr[i];
    }

    LOG(info) << "CbmRecoQaTask::Detector(" << dname << ")::View(" << name << ")::Init() : size [" << size[0] << " x "
              << size[1] << " x " << size[2] << "]. mc[" << mc << "].";
  }
  else
    LOG(info) << "CbmRecoQaTask::Detector(" << dname << ")::View(" << name << ")::Init() :  mc[" << mc << "].";


  // TODO short-hand for XY display resolution
  int dscale = 10;
  if (strcmp(dname, "Tof") == 0 || strcmp(dname, "Rich") == 0) dscale = 1;

  int nbinsx, nbinsy;
  string unit;
  for (auto& projection : fProjection) {
    int scale     = get<0>(projection.second);
    float yrange  = get<1>(projection.second);
    char xy_id    = 0;
    char mc_id[2] = {' ', ' '};
    double xlo = pos[0] - 0.5 * size[0], xhi = pos[0] + 0.5 * size[0], ylo = pos[1] - 0.5 * size[1],
           yhi = pos[1] + 0.5 * size[1];
    switch (projection.first) {
      case eProjectionType::kXdXMC:
        if (!mc) break;
        xy_id    = 'M';
        mc_id[0] = 'M';
        mc_id[1] = 'C';
      // fall through
      case eProjectionType::kXdX:
        if (!xy_id) xy_id = 't';
        nbinsx = 10 * ceil(size[0]);  // mm binning
        unit   = makeYrange(scale, yrange);
        nbinsy = 200;  //* ceil(yrange);
        get<2>(projection.second) =
          new TH2D(Form("hxx%s_%s%s", (xy_id == 'M' ? "MC" : ""), dname, name.data()),
                   Form("X resolution %s %s [%s]; X^{%s}_{%s-%s} (cm); #Delta X (%s)", (xy_id == 'M' ? "MC" : ""),
                        name.data(), dname, mc_id, dname, name.data(), unit.data()),
                   nbinsx, xlo, xhi, nbinsy, -yrange, yrange);
        break;

      case eProjectionType::kYdYMC:
        if (!mc) break;
        xy_id    = 'M';
        mc_id[0] = 'M';
        mc_id[1] = 'C';
      // fall through
      case eProjectionType::kYdY:
        if (!xy_id) xy_id = 't';
        nbinsx = 10 * ceil(size[1]);  // mm binning
        unit   = makeYrange(scale, yrange);
        nbinsy = 200;  //* ceil(yrange);
        get<2>(projection.second) =
          new TH2D(Form("hyy%s_%s%s", (xy_id == 'M' ? "MC" : ""), dname, name.data()),
                   Form("Y resolution %s %s [%s]; Y^{%s}_{%s-%s} (cm); #Delta Y (%s)", (xy_id == 'M' ? "MC" : ""),
                        name.data(), dname, mc_id, dname, name.data(), unit.data()),
                   nbinsx, ylo, yhi, nbinsy, -yrange, yrange);
        break;

      case eProjectionType::kXpX:
        nbinsx = 10 * ceil(size[0]);  // mm binning
        if (yrange < 0) {
          LOG(debug) << "ProjectionP[" << name << "] using default range.";
          yrange = 5;  // +- 5 sigma range
        }
        nbinsy = 200;  //* ceil(yrange);
        get<2>(projection.second) =
          new TH2D(Form("hpx_%s%s", dname, name.data()),
                   Form("X pulls %s [%s]; X^{%s}_{%s-%s} (cm); pull(X)", name.data(), dname, mc_id, dname, name.data()),
                   nbinsx, xlo, xhi, nbinsy, -yrange, yrange);
        break;

      case eProjectionType::kYpY:
        nbinsx = 10 * ceil(size[1]);  // mm binning
        if (yrange < 0) {
          LOG(debug) << "ProjectionP[" << name << "] using default range.";
          yrange = 5;  // +- 5 sigma range
        }
        nbinsy = 200;  //* ceil(yrange);
        get<2>(projection.second) =
          new TH2D(Form("hpy_%s%s", dname, name.data()),
                   Form("Y pulls %s [%s]; Y^{%s}_{%s-%s} (cm); pull(Y)", name.data(), dname, mc_id, dname, name.data()),
                   nbinsx, ylo, yhi, nbinsy, -yrange, yrange);
        break;

      case eProjectionType::kWdT:
        nbinsx                    = 10 * ceil(size[0]);  // mm binning
        unit                      = makeTrange(scale, yrange);
        nbinsy                    = 2 * ceil(yrange);
        get<2>(projection.second) = new TH2D(Form("hxt_%s%s", dname, name.data()),
                                             Form("Hit_Trk %s [%s]; X_{%s-%s} (cm); #Delta time_{TRK} (%s)",
                                                  name.data(), dname, dname, name.data(), unit.data()),
                                             nbinsx, xlo, xhi, nbinsy, -yrange, yrange);
        break;

      case eProjectionType::kChdT:
        nbinsx                    = dscale * ceil(size[0]);  // mm binning
        unit                      = makeTrange(scale, yrange);
        nbinsy                    = 10 * ceil(yrange);
        get<2>(projection.second) = new TH2D(Form("hct_%s%s", dname, name.data()),
                                             Form("Hit Event %s [%s]; X_{%s-%s} (cm); #Delta time_{EV} (%s)",
                                                  name.data(), dname, dname, name.data(), unit.data()),
                                             nbinsx, xlo, xhi, nbinsy, -yrange, yrange);
        break;
      case eProjectionType::kDmultMC:
        if (!mc) break;
        xy_id = 'm';
        // fall through
      case eProjectionType::kDmult:
        nbinsx                    = 100;
        xlo                       = -0.5;
        xhi                       = xlo + nbinsx;
        get<2>(projection.second) = new TH2D(
          Form("hdm_%s%s", dname, name.data()),
          Form("%s multiplicity [EbyE] %s [%s]; N_{%s}^{%s}; N_{%s-%s}^{%s} (%%)", (xy_id ? "point" : "hit"),
               name.data(), dname, dname, (xy_id ? "point" : "hit"), dname, name.data(), (xy_id ? "point" : "hit")),
          nbinsx, xlo, xhi, 200, 0, 1);
        break;

      case eProjectionType::kXYa:
        xy_id = 'a';
        // fall through
      case eProjectionType::kXYh:
        if (!xy_id) xy_id = 'h';
        nbinsx = dscale * ceil(size[0]);  // mm binning
        nbinsy = dscale * ceil(size[1]);
        get<2>(projection.second) =
          new TH2D(Form("hxy%c_%s%s", xy_id, dname, name.data()),
                   Form("Hit_{%s} %s [%s]; X_{%s-%s} (cm);  Y_{%s-%s} (cm)", (xy_id == 'h' ? "reco" : "attach"),
                        name.data(), dname, dname, name.data(), dname, name.data()),
                   nbinsx, xlo, xhi, nbinsy, ylo, yhi);
        break;

      case eProjectionType::kXYhMC:
        if (!mc) break;
        if (!xy_id) xy_id = 'h';
        mc_id[0] = 'M';
        mc_id[1] = 'C';
        nbinsx   = dscale * ceil(size[0]);  // mm binning
        nbinsy   = dscale * ceil(size[1]);
        get<2>(projection.second) =
          new TH2D(Form("hxymc%c_%s%s", xy_id, dname, name.data()),
                   Form("Hit_{%s} %s [%s]; X_{%s-%s} (cm);  Y_{%s-%s} (cm)", (xy_id == 'h' ? "mc" : "attach"),
                        name.data(), dname, dname, name.data(), dname, name.data()),
                   nbinsx, xlo, xhi, nbinsy, ylo, yhi);
        break;

      case eProjectionType::kResidualX:
      // fall through
      case eProjectionType::kResidualY:
        if (!mc) break;
        mc_id[0] = 'M';
        mc_id[1] = 'C';
        {
          bool isResidualX = (projection.first == eProjectionType::kResidualX);

          nbinsx = dscale * ceil(size[0]);  // mm binning
          nbinsy = dscale * ceil(size[1]);
          unit   = makeYrange(scale, yrange);

          get<2>(projection.second) = new TH2D(
            Form("h%sR_%s%s", (isResidualX ? "xx" : "yy"), dname, name.data()),
            Form("%s resolution %s [%s]; %s_{%s-%s} (cm); #Delta %s (%s)", (isResidualX ? "X" : "Y"), name.data(),
                 dname, (isResidualX ? "X" : "Y"), dname, name.data(), (isResidualX ? "X" : "Y"), unit.data()),
            nbinsx, (isResidualX ? xlo : ylo), (isResidualX ? xhi : yhi), nbinsy, -yrange, yrange);
          break;
        }

      case eProjectionType::kResidualTX:
      // fall through
      case eProjectionType::kResidualTY:
        if (!mc) break;
        {
          bool isResidualTX = (projection.first == eProjectionType::kResidualTX);

          nbinsx = dscale * ceil(size[0]);  // mm binning
          nbinsy = dscale * ceil(size[1]);

          get<2>(projection.second) = new TH2D(
            Form("h%sR_%s%s", (isResidualTX ? "tx" : "ty"), dname, name.data()),
            Form("%s resolution %s [%s]; %s_{%s-%s} (cm); #Delta %s (ns)", (isResidualTX ? "T" : "T"), name.data(),
                 dname, (isResidualTX ? "X" : "Y"), dname, name.data(), (isResidualTX ? "T" : "T")),
            nbinsx, (isResidualTX ? xlo : ylo), (isResidualTX ? xhi : yhi), nbinsy, -yrange, yrange);
          break;
        }

      case eProjectionType::kPullX:
      // fall through
      case eProjectionType::kPullY:
        if (!mc) break;
        {
          bool isPullX = (projection.first == eProjectionType::kPullX);

          nbinsx = dscale * ceil(size[0]);  // mm binning
          nbinsy = dscale * ceil(size[1]);
          unit   = makeYrange(scale, yrange);

          get<2>(projection.second) =
            new TH2D(Form("h%sP_%s%s", (isPullX ? "xx" : "yy"), dname, name.data()),
                     Form("%s pull %s [%s]; %s_{%s-%s} (cm); pull(%s)", (isPullX ? "X" : "Y"), name.data(), dname,
                          (isPullX ? "X" : "Y"), dname, name.data(), (isPullX ? "X" : "Y")),
                     nbinsx, (isPullX ? xlo : ylo), (isPullX ? xhi : yhi), nbinsy, -yrange, yrange);
          break;
        }

      case eProjectionType::kXYp:
        nbinsx = dscale * ceil(size[0] * 2.);  // mm binning
        nbinsy = dscale * ceil(size[1] * 2.);
        xlo    = pos[0] - size[0];
        xhi    = pos[0] + size[0];
        ylo    = pos[1] - size[1];
        yhi    = pos[1] + size[1];

        get<2>(projection.second) = new TH2D(Form("hxyp_%s%s", dname, name.data()),
                                             Form("Trk_{proj} %s [%s]; X_{%s-%s} (cm);  Y_{%s-%s} (cm)", name.data(),
                                                  dname, dname, name.data(), dname, name.data()),
                                             nbinsx, xlo, xhi, nbinsy, ylo, yhi);
        break;
      case eProjectionType::kXYt0: xy_id = '0';
      // fall through
      case eProjectionType::kXYt1:
        if (!xy_id) xy_id = '1';
      // fall through
      case eProjectionType::kXYt2:
        if (!xy_id) xy_id = '2';
      // fall through
      case eProjectionType::kXYt3:
        if (!xy_id) xy_id = '3';
      // fall through
      case eProjectionType::kXYt4:
        if (!xy_id) xy_id = '4';
      // fall through
      case eProjectionType::kXYt5:
        if (!xy_id) xy_id = '5';
        nbinsx = 4500;  // mm binning
        nbinsy = 1000;
        xlo    = -25;
        xhi    = +20;
        ylo    = -5;
        yhi    = +5;

        get<2>(projection.second) = new TH2D(Form("hxyt%c_%s%s", xy_id, dname, name.data()),
                                             Form("Trk_{proj} z = %+.2f [%s]; X_{%s-%s} (cm);  Y_{%s-%s} (cm)", yrange,
                                                  dname, dname, name.data(), dname, name.data()),
                                             nbinsx, xlo, xhi, nbinsy, ylo, yhi);
        break;

      // Primary vertex plots
      case eProjectionType::kPVmult:  // define primary vertex multiplicity
        nbinsx = 50;
        nbinsy = 20;
        xlo    = -0.5;
        xhi    = xlo + nbinsx;
        ylo    = -0.5;
        yhi    = ylo + nbinsy;

        get<2>(projection.second) =
          new TH2D(Form("hMult_%s%s", dname, name.data()), "Vertex multiplicity; N_{trk}^{event};  N_{trk}^{PV}",
                   nbinsx, xlo, xhi, nbinsy, ylo, yhi);
        break;
      case eProjectionType::kPVxz:  // define X-Z vertex projection
        xy_id    = 'y';
        mc_id[0] = 'z';  // use to store x-axis title
        mc_id[1] = 'x';  // use to store y-axis title
        nbinsx   = 100;
        nbinsy   = 200;
        xlo      = -10;
        xhi      = +10;
        ylo      = -2;
        yhi      = +2;
        // fall through
      case eProjectionType::kPVyz:  // define Y-Z vertex projection
        if (!xy_id) {
          xy_id    = 'x';
          mc_id[0] = 'z';  // use to store x-axis title
          mc_id[1] = 'y';  // use to store y-axis title
          nbinsx   = 100;
          nbinsy   = 200;
          xlo      = -10;
          xhi      = +10;
          ylo      = -2;
          yhi      = +2;
        }
        // fall through
      case eProjectionType::kPVxy:  // define X-Y vertex projection
        if (!xy_id) {
          xy_id    = 'z';
          mc_id[0] = 'x';  // use to store x-axis title
          mc_id[1] = 'y';  // use to store y-axis title
          nbinsx   = 200;
          nbinsy   = 200;
          xlo      = -2;
          xhi      = +2;
          ylo      = -2;
          yhi      = +2;
        }
        get<2>(projection.second) =
          new TH2D(Form("h%c_%s%s", xy_id, dname, name.data()),
                   Form("Vertex_{%c%c}; %c (cm);  %c (cm)", mc_id[0], mc_id[1], mc_id[0], mc_id[1]), nbinsx, xlo, xhi,
                   nbinsy, ylo, yhi);
        break;
    }
  }
  return true;
}

string CbmRecoQaTask::View::ToString() const
{
  stringstream s;
  s << "V[" << name << "] path[" << path << "] @ [" << pos[0] << ", " << pos[1] << ", " << pos[2] << "] size["
    << size[0] << ", " << size[1] << ", " << size[2] << "] projections[" << fProjection.size() << "] sel[";
  for (auto ii : fSelector)
    s << ii << " ";
  s << "]\n";
  return s.str();
}

uint CbmRecoQaTask::View::Register(TDirectoryFile* fOut)
{
  uint n(0);
  TDirectoryFile* lDir = (TDirectoryFile*) fOut->mkdir(name.data(), Form("QA for vol[%s]", path.data()));
  TDirectoryFile *sDir(nullptr), *tDir(nullptr);
  // run extra directory structure for the case of detector view
  if (fType == eViewType::kDetUnit) {
    if (CbmRecoQaTask::fuRecoConfig[kRecoEvents])
      sDir = (TDirectoryFile*) lDir->mkdir("event", "Local Reco QA");
    else
      sDir = (TDirectoryFile*) lDir->mkdir("TS", "TimeSlice QA");
    tDir = (CbmRecoQaTask::fuRecoConfig[kRecoTracks] ? (TDirectoryFile*) lDir->mkdir("track", "CA QA") : nullptr);
  }

  for (auto& projection : fProjection) {
    TH2* hh = get<2>(projection.second);
    if (!hh) continue;
    switch (projection.first) {
      case eProjectionType::kXYa:
      case eProjectionType::kXYp:
      case eProjectionType::kXdX:
      case eProjectionType::kXdXMC:
      case eProjectionType::kYdY:
      case eProjectionType::kYdYMC:
      case eProjectionType::kWdT:
      case eProjectionType::kXpX:
      case eProjectionType::kYpY:
        if (tDir) tDir->Add(hh);
        break;
      case eProjectionType::kChdT:
      case eProjectionType::kXYh:
      case eProjectionType::kXYhMC:
      case eProjectionType::kDmult:
      case eProjectionType::kDmultMC:
      case eProjectionType::kResidualX:
      case eProjectionType::kResidualY:
      case eProjectionType::kResidualTX:
      case eProjectionType::kResidualTY:
      case eProjectionType::kPullX:
      case eProjectionType::kPullY:
        if (sDir) sDir->Add(hh);
        break;
      case eProjectionType::kXYt0:
      case eProjectionType::kXYt1:
      case eProjectionType::kXYt2:
      case eProjectionType::kXYt3:
      case eProjectionType::kXYt4:
      case eProjectionType::kXYt5:
      case eProjectionType::kPVxz:
      case eProjectionType::kPVyz:
      case eProjectionType::kPVxy:
      case eProjectionType::kPVmult:
        if (lDir) lDir->Add(hh);
        break;
    }
    n++;
  }
  LOG(debug) << "CbmRecoQaTask::View[" << name << "]::Register() : " << n << " projections for " << fOut->GetName();
  return n;
}

string CbmRecoQaTask::View::ToString(eProjectionType prj)
{
  string s;
  switch (prj) {
    case eProjectionType::kXYa: s = "x-y (attach)"; break;
    case eProjectionType::kXYp:
    case eProjectionType::kXYt0:
    case eProjectionType::kXYt1:
    case eProjectionType::kXYt2:
    case eProjectionType::kXYt3:
    case eProjectionType::kXYt4:
    case eProjectionType::kXYt5: s = "x-y (track)"; break;
    case eProjectionType::kXdX: s = "dx-x"; break;
    case eProjectionType::kXdXMC: s = "dx-x (MC)"; break;
    case eProjectionType::kResidualX: s = "residual(x)-x:MC"; break;
    case eProjectionType::kResidualY: s = "residual(y)-y:MC"; break;
    case eProjectionType::kResidualTX: s = "residual(t)-x:MC"; break;
    case eProjectionType::kResidualTY: s = "residual(t)-y:MC"; break;
    case eProjectionType::kPullX: s = "pull(x)-x:MC"; break;
    case eProjectionType::kPullY: s = "pull(y)-y:MC"; break;
    case eProjectionType::kYdY: s = "dy-y"; break;
    case eProjectionType::kYdYMC: s = "dy-y (MC)"; break;
    case eProjectionType::kXpX: s = "pull(x)-x"; break;
    case eProjectionType::kYpY: s = "pull(y)-y"; break;
    case eProjectionType::kWdT: s = "dt(trk)-w"; break;
    case eProjectionType::kChdT: s = "dt(ev)-w"; break;
    case eProjectionType::kXYh: s = "x-y (hit)"; break;
    case eProjectionType::kXYhMC: s = "x-y (point)"; break;
    default: LOG(error) << "View::ToString() : Unknown projection " << int(prj); break;
  }
  return s;
}

string CbmRecoQaTask::View::makeYrange(const int scale, float& yrange)
{
  bool kDefaultRange = false;
  string unit        = "cm";
  if (yrange < 0) {
    LOG(debug) << "ProjectionY[" << name << "] using default range.";
    kDefaultRange = true;
    yrange        = 3;  // +- 6 cm default range
  }
  if (scale == 10) {  // mm resolution
    unit = "mm";
    if (kDefaultRange) yrange = 10;  // +- 20 mm default range
  }
  else if (scale == 10000) {  // micron resolution
    unit = "#mu m";
    if (kDefaultRange) yrange = 150;  // +- 300 um default range
  }
  return unit;
}

string CbmRecoQaTask::View::makeTrange(const int scale, float& yrange)
{
  bool kDefaultRange = false;
  string unit        = "ns";
  if (yrange < 0) {
    LOG(debug) << "ProjectionT[" << name << "] using default range.";
    kDefaultRange = true;
    yrange        = 40;  // +- 80 ns default range
  }
  if (scale == 1000) {  // ps resolution
    unit = "ps";
    if (kDefaultRange) yrange = 300;  // +- 600 ps default range
  }
  return unit;
}

CbmRecoQaTask::EventFilter* CbmRecoQaTask::AddEventFilter(CbmRecoQaTask::EventFilter::eEventCut type)
{
  for (auto cut : fFilterEv) {
    if (cut.fType == type) {
      LOG(warning) << GetName() << "::AddEventFilter event filter : " << cut.ToString() << " already on the list.";
      return nullptr;
    }
  }
  fFilterEv.emplace_back(type);
  return &fFilterEv.back();
}

CbmRecoQaTask::TrackFilter* CbmRecoQaTask::AddTrackFilter(CbmRecoQaTask::TrackFilter::eTrackCut type)
{
  for (auto cut : fFilterTrk) {
    if (cut.fType == type) {
      LOG(warning) << GetName() << "::AddTrackFilter track filter : " << cut.ToString() << " already on the list.";
      return nullptr;
    }
  }
  fFilterTrk.emplace_back(type);
  return &fFilterTrk.back();
}

//+++++++++++++++++++++  EventFilter  ++++++++++++++++++++++++++++++++
//____________________________________________________________________
bool CbmRecoQaTask::EventFilter::Accept(const CbmEvent* ptr, const CbmRecoQaTask* /* lnk*/)
{
  stringstream ss;
  bool ret(true);
  size_t val(0);
  switch (fType) {
    case eEventCut::kMultTrk:
      val = ptr->GetNofData(ECbmDataType::kGlobalTrack);
      if (fMinTrack > 0 && val < size_t(fMinTrack)) {
        ss << "NofTrack[" << val << "] < min[" << fMinTrack << "].";
        ret = false;
        break;
      }
      if (ret && fMaxTrack > 0 && val > size_t(fMaxTrack)) {
        ss << "NofTrack[" << val << "] > max[" << fMaxTrack << "].";
        ret = false;
      }
      break;
    case eEventCut::kMultHit:
      val = ptr->GetNofData(ECbmDataType::kStsHit);
      if (fMultHit[0] > 0 && val > size_t(fMultHit[0])) {
        ss << "Sts hits [" << val << "] > max[" << fMultHit[0] << "].";
        ret = false;
        break;
      }
      val = ptr->GetNofData(ECbmDataType::kTrdHit);
      if (fMultHit[1] > 0 && val > size_t(fMultHit[1])) {
        ss << "Trd hits [" << val << "] > max[" << fMultHit[1] << "].";
        ret = false;
        break;
      }
      val = ptr->GetNofData(ECbmDataType::kTofHit);
      if (fMultHit[2] > 0 && val > size_t(fMultHit[2])) {
        ss << "Tof hits [" << val << "] > max[" << fMultHit[2] << "].";
        ret = false;
      }
      break;
    default: break;
  }
  if (!ret) LOG(debug2) << "Event reject for : " << ss.str();
  return ret;
}
bool CbmRecoQaTask::EventFilter::SetFilter(std::vector<float> cuts)
{
  switch (fType) {
    case eEventCut::kMultTrk:
      if (cuts.size() < 2 || cuts[1] < cuts[0]) {
        LOG(warning) << "Improper definition for event filter :\n\t" << ToString() << endl;
        HelpMess();
        return false;
      }
      fMinTrack = int(cuts[0]);
      fMaxTrack = int(cuts[1]);
      break;
    case eEventCut::kMultHit:
      if (cuts.size() < 3) {
        LOG(warning) << "Improper definition for event filter :\n\t" << ToString() << endl;
        HelpMess();
        return false;
      }
      for (int i(0); i < int(CbmRecoQaTask::EventFilter::eEventDef::kNofDetHit); i++)
        fMultHit[i] = int(cuts[i]);
      break;
    case eEventCut::kTrigger: break;
    case eEventCut::kVertex: break;
    default: break;
  }
  return true;
}
std::string CbmRecoQaTask::EventFilter::ToString() const
{
  stringstream ss;
  switch (fType) {
    case eEventCut::kMultTrk: ss << "kMultTrk : \"cut on track multiplicity"; break;
    case eEventCut::kMultHit: ss << "kMultHit : \"cut on hit multiplicity"; break;
    case eEventCut::kTrigger: ss << "kTrigger : \"cut on trigger conditions"; break;
    case eEventCut::kVertex: ss << "kVertex : \"cut on vertex definition"; break;
    default: break;
  }
  return ss.str();
}
void CbmRecoQaTask::EventFilter::HelpMess() const
{
  LOG(info) << "CbmRecoQaTask::EventFilter : Usage";
  switch (fType) {
    case eEventCut::kMultTrk:
      LOG(info) << ToString();
      LOG(info) << "\tDepends one two values : min and max of the no of global "
                   "tracks / event.";
      LOG(info) << "\tValue should follow the relation max >= min.";
      break;
    case eEventCut::kMultHit:
      LOG(info) << ToString();
      LOG(info) << "\tDepends one three values : total hits in the following "
                   "systems (in this order) : STS TRD TOF.";
      LOG(info) << "\tNegative values are interpreted as no-cut.";
      break;
    default:
      LOG(info) << ToString();
      LOG(info) << "\tNo user info.";
      break;
  }
}
//+++++++++++++++++++++  TrackFilter  ++++++++++++++++++++++++++++++++
//____________________________________________________________________
bool CbmRecoQaTask::TrackFilter::Accept(const CbmGlobalTrack* ptr, const CbmRecoQaTask* lnk)
{
  bool ret(true);
  stringstream ss;
  int idx(-1);
  const CbmTrack* trk(nullptr);
  switch (fType) {
    case eTrackCut::kSts:
      if (fNSts <= 0 && !fStsHits.size()) break;
      if ((idx = ptr->GetStsTrackIndex()) < 0) {
        ss << "Sts trk index missing";
        ret = false;
        break;
      }
      if (!(trk = lnk->GetTrack(ECbmModuleId::kSts, idx))) {
        ss << "Sts trk nullptr";
        ret = false;
        break;
      }
      // cut on the total no of STS hits/trk
      if (trk->GetNofHits() < fNSts) {
        ss << "Sts hits/trk [" << trk->GetNofHits() << "] < min[" << fNSts << "].";
        ret = false;
        break;
      }
      // cut on the distribution of STS hits/trk/station
      for (auto stsStation : fStsHits) {
        if (!stsStation) continue;
        // check hit in this particular station
      }
      break;
    case eTrackCut::kTrd:
      if (fNTrd <= 0) break;
      if ((idx = ptr->GetTrdTrackIndex()) < 0) {
        ss << "Trd trk index missing";
        ret = false;
        break;
      }
      if (!(trk = lnk->GetTrack(ECbmModuleId::kTrd, idx))) {
        ss << "Trd trk nullptr";
        ret = false;
        break;
      }
      // cut on the total no of TRD hits/trk
      if (trk->GetNofHits() < fNTrd) {
        ss << "Trd` hits/trk [" << trk->GetNofHits() << "] < min[" << fNTrd << "].";
        ret = false;
        break;
      }
      break;
    case eTrackCut::kTof:
      if (fNTof <= 0) break;
      if ((idx = ptr->GetTofTrackIndex()) < 0) {
        ss << "Tof trk index missing";
        ret = false;
        break;
      }
      if (!(trk = lnk->GetTrack(ECbmModuleId::kTof, idx))) {
        ss << "Tof trk nullptr";
        ret = false;
        break;
      }
      // cut on the total no of ToF hits/trk
      if (trk->GetNofHits() < fNTof) {
        ss << "Tof` hits/trk [" << trk->GetNofHits() << "] < min[" << fNTof << "].";
        ret = false;
        break;
      }
      break;
    default: break;
  }

  if (!ret) LOG(debug2) << "Track reject for : " << ss.str();
  return ret;
}
bool CbmRecoQaTask::TrackFilter::SetFilter(std::vector<float> cuts)
{
  switch (fType) {
    case eTrackCut::kSts:
      if (cuts.size() < 1) {
        LOG(warning) << "Improper definition for track filter :\n\t" << ToString() << endl;
        // HelpMess();
        return false;
      }
      fNSts = int(cuts[0]);
      break;
    case eTrackCut::kTrd:
      if (cuts.size() < 1) {
        LOG(warning) << "Improper definition for track filter :\n\t" << ToString() << endl;
        // HelpMess();
        return false;
      }
      fNTrd = int(cuts[0]);
      break;
    case eTrackCut::kTof:
      if (cuts.size() < 1) {
        LOG(warning) << "Improper definition for track filter :\n\t" << ToString() << endl;
        // HelpMess();
        return false;
      }
      fNTof = int(cuts[0]);
      break;
    default: break;
  }
  return true;
}
std::string CbmRecoQaTask::TrackFilter::ToString() const
{
  stringstream ss;
  switch (fType) {
    case eTrackCut::kSts: ss << "kSts : \"cut on no of STS hits / track\""; break;
    case eTrackCut::kMuch: ss << "kMuch : \"cut on no of Much hits / track\""; break;
    case eTrackCut::kRich: ss << "kRich : \"cut on no of Rich hits / track\""; break;
    case eTrackCut::kTrd: ss << "kTrd : \"cut on no of TRD hits / track\""; break;
    case eTrackCut::kTof: ss << "kTof : \"cut on no of Tof hits / track\""; break;
    default: break;
  }
  return ss.str();
}
/* clang-format off */
ClassImp(CbmRecoQaTask)
ClassImp(CbmRecoQaTask::Detector)
ClassImp(CbmRecoQaTask::View)
ClassImp(CbmRecoQaTask::TrackFilter)
ClassImp(CbmRecoQaTask::EventFilter)
  /* clang-format on */
