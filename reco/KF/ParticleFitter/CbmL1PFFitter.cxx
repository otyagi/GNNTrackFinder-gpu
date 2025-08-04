/* Copyright (C) 2011-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Maksym Zyzak [committer] */

/*
 *=====================================================
 *
 *  CBM Level 1 Reconstruction
 *
 *  Authors: M.Zyzak
 *
 *  e-mail :
 *
 *=====================================================
 *
 *  SIMD Fitter
 *
 */

#include "CbmL1PFFitter.h"

#include "CbmL1.h"
#include "CbmStsAddress.h"
#include "CbmStsHit.h"
#include "CbmStsSetup.h"
#include "CbmStsTrack.h"
#include "TClonesArray.h"

//ca::Framework tools
#include "CaFramework.h"
#include "CaSimd.h"
#include "CaStation.h"
#include "CaToolsField.h"
#include "CbmKFVertex.h"
#include "FairRootManager.h"
#include "KFParticleDatabase.h"
#include "KfFieldRegion.h"
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"
#include "TDatabasePDG.h"

using namespace cbm::algo::ca;
using cbm::algo::kf::TrackParamD;

using std::vector;
using namespace std;

namespace NS_L1TrackFitter
{
  const fvec c_light(0.000299792458), c_light_i = fvec(1.) / c_light;
  const fvec ZERO = fvec(0.);
  const fvec ONE  = fvec(1.);
  const fvec vINF = fvec(0.1);
}  // namespace NS_L1TrackFitter
using namespace NS_L1TrackFitter;


inline void CbmL1PFFitter::PFFieldRegion::setFromL1FieldRegion(const kf::FieldRegion<fvec>& fld, int ind)
{
  assert(fld.GetFieldMode() == kf::EFieldMode::Intrpl);
  const auto& coeff = fld.GetIntrplField()->GetCoefficients();

  int i = 0;
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      fP[i] = coeff[j][k][ind];
      i++;
    }
  }
  fP[9] = fld.GetIntrplField()->GetZfirst()[ind];
}

inline void CbmL1PFFitter::PFFieldRegion::getL1FieldRegion(kf::FieldRegion<fvec>& fld, int ind)
{
  assert(fld.GetFieldMode() == kf::EFieldMode::Intrpl);

  auto coeff = fld.GetIntrplField()->GetCoefficients();
  auto z     = fld.GetIntrplField()->GetZfirst();

  int i = 0;
  for (int j = 0; j < 3; j++) {
    for (int k = 0; k < 3; k++) {
      coeff[j][k][ind] = fP[i];
      i++;
    }
  }
  z[ind] = fP[9];

  kf::detail::FieldRegionBase<fvec, kf::EFieldMode::Intrpl> fldInterpolated;
  fldInterpolated.Set(coeff, fP[9]);
  fld = kf::FieldRegion<fvec>(kf::EFieldType::Normal, fldInterpolated);
}

inline CbmL1PFFitter::PFFieldRegion::PFFieldRegion(const kf::FieldRegion<fvec>& fld, int i)
{
  setFromL1FieldRegion(fld, i);
}


CbmL1PFFitter::CbmL1PFFitter() {}

CbmL1PFFitter::~CbmL1PFFitter() {}

inline void CbmL1PFFitter::Initialize()
{
  if (fIsInitialised) {
    return;
  }

  fNmvdStationsActive = 0;
  fNstsStationsActive = 0;
  fMvdHitArray        = nullptr;
  fStsHitArray        = nullptr;

  FairRootManager* manager = FairRootManager::Instance();

  if (!manager) {
    LOG(fatal) << "CbmL1PFFitter: no FairRootManager";
  }

  if (!CbmL1::Instance() || !CbmL1::Instance()->fpAlgo) {
    LOG(fatal) << "CbmL1PFFitter: no CbmL1 task initialised ";
  }

  using cbm::algo::ca::EDetectorID;
  // TODO: Remove CbmL1::Instance with itterations:
  //  1) replace material with active setup
  //  2) replace field with active setup
  //  3) provide proper initialization of the setup
  fNmvdStationsActive = CbmL1::Instance()->fpAlgo->GetParameters().GetNstationsActive(EDetectorID::kMvd);
  fNstsStationsActive = CbmL1::Instance()->fpAlgo->GetParameters().GetNstationsActive(EDetectorID::kSts);

  if (fNmvdStationsActive > 0) {
    fMvdHitArray = static_cast<TClonesArray*>(manager->GetObject("MvdHit"));
  }
  if (fNstsStationsActive > 0) {
    fStsHitArray = static_cast<TClonesArray*>(manager->GetObject("StsHit"));
  }

  cbm::ca::tools::SetOriginalCbmField();

  fIsInitialised = true;
}

inline int CbmL1PFFitter::GetMvdStationIndex(const CbmMvdHit* hit)
{
  using cbm::algo::ca::EDetectorID;
  return CbmL1::Instance()->fpAlgo->GetParameters().GetStationIndexActive(hit->GetStationNr(), EDetectorID::kMvd);
}

inline int CbmL1PFFitter::GetStsStationIndex(const CbmStsHit* hit)
{
  using cbm::algo::ca::EDetectorID;
  return CbmL1::Instance()->fpAlgo->GetParameters().GetStationIndexActive(
    CbmStsSetup::Instance()->GetStationNumber(hit->GetAddress()), EDetectorID::kSts);
}


void FilterFirst(kf::TrackKalmanFilter<fvec>& fit, kf::MeasurementXy<fvec>& mxy, fvec& t, fvec& dt2)
{
  TrackParamV& tr = fit.Tr();
  tr.ResetErrors(mxy.Dx2(), mxy.Dy2(), 1., 1., 1., dt2, 1.e2);
  tr.C10()     = mxy.Dxy();
  tr.X()       = mxy.X();
  tr.Y()       = mxy.Y();
  tr.Time()    = t;
  tr.Vi()      = 0.;
  tr.Ndf()     = mxy.NdfX() + mxy.NdfY() - fvec(5.);
  tr.NdfTime() = -2.;
}

void CbmL1PFFitter::Fit(std::vector<CbmStsTrack>& Tracks, const std::vector<CbmMvdHit>& vMvdHits,
                        const std::vector<CbmStsHit>& vStsHits, const std::vector<int>& pidHypo)
{
  // TODO: (!) replace CbmL1::Instance()->fpAlgo->GetParameters() with the cbm::ca::ParametersHandler::Instance()->Get()
  //           everywhere outside cbm::algo
  const auto& activeTrackingSetup = CbmL1::Instance()->fpAlgo->GetParameters().GetActiveSetup();
  Initialize();

  kf::FieldValue<fvec> b0, b1, b2 _fvecalignment;
  kf::FieldRegion<fvec> fld;
  // fld.SetUseOriginalField();

  static int nHits = CbmL1::Instance()->fpAlgo->GetParameters().GetNstationsActive();
  int nTracks_SIMD = fvec::size();

  kf::TrackKalmanFilter<fvec> fit;
  fit.SetParticleMass(CbmL1::Instance()->fpAlgo->GetDefaultParticleMass());

  TrackParamV& T = fit.Tr();  // fitting parametr coresponding to current track

  CbmStsTrack* tr[fvec::size()]{nullptr};

  int ista;
  const ca::Station<fvec>* sta = CbmL1::Instance()->fpAlgo->GetParameters().GetStations().begin();

  fvec x[nHits];
  fvec y[nHits];
  fvec z[nHits];
  fvec t[nHits];
  kf::MeasurementXy<fvec> mxy[nHits];
  fvec dt2[nHits];
  fmask w[nHits];

  //  fvec y_temp;
  fvec x_first, y_first, t_first, x_last, y_last, t_last;
  kf::MeasurementXy<fvec> mxy_first, mxy_last;
  fvec dt2_first, dt2_last;

  fvec z0, z1, z2, dz, z_start, z_end;
  kf::FieldValue<fvec> fB[nHits];
  kf::FieldValue<fvec> fB_temp _fvecalignment;

  unsigned short N_vTracks = Tracks.size();


  for (unsigned short itrack = 0; itrack < N_vTracks; itrack++) {
    Tracks[itrack].SetPidHypo(pidHypo[itrack]);
  }

  fvec mass = 0.000511f;

  for (unsigned short itrack = 0; itrack < N_vTracks; itrack += fvec::size()) {

    T.Time() = 0.;
    T.Vi()   = 0.;
    T.ResetErrors(1.e2, 1.e2, 1., 1., 1., 1.e6, 1.e2);

    if (N_vTracks - itrack < static_cast<unsigned short>(fvec::size())) {
      nTracks_SIMD = N_vTracks - itrack;
    }
    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      tr[iVec]       = &Tracks[itrack + iVec];  // current track
      T.X()[iVec]    = tr[iVec]->GetParamFirst()->GetX();
      T.Y()[iVec]    = tr[iVec]->GetParamFirst()->GetY();
      T.Tx()[iVec]   = tr[iVec]->GetParamFirst()->GetTx();
      T.Ty()[iVec]   = tr[iVec]->GetParamFirst()->GetTy();
      T.Qp()[iVec]   = tr[iVec]->GetParamFirst()->GetQp();
      T.Time()[iVec] = 0.;
      T.Vi()[iVec]   = 0.;
      T.Z()[iVec]    = tr[iVec]->GetParamFirst()->GetZ();

      for (int i = 0; i < 5; i++) {
        for (int j = 0; j <= i; j++) {
          T.C(i, j)[iVec] = tr[iVec]->GetParamFirst()->GetCovariance(i, j);
        }
      }

      int pid = pidHypo[itrack + iVec];
      if (pid == -1) {
        pid = 211;
      }
      //       mass[i] = TDatabasePDG::Instance()->GetParticle(pid)->Mass();
      mass[iVec] = KFParticleDatabase::Instance()->GetMass(pid);
    }

    fit.SetParticleMass(mass);

    // get hits of current track
    for (int i = 0; i < nHits; i++) {
      w[i] = fmask::Zero();
      z[i] = sta[i].fZ;
      x[i] = 0.;
      y[i] = 0.;
      t[i] = 0.;
      mxy[i].SetX(0.);
      mxy[i].SetY(0.);
      mxy[i].SetDx2(1.);
      mxy[i].SetDy2(1.);
      mxy[i].SetDxy(0.);
      mxy[i].SetNdfX(1.);
      mxy[i].SetNdfY(1.);
      dt2[i] = 1.;
      fB[i].Set(0., 0., 0.);
    }

    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      int nHitsTrackMvd = tr[iVec]->GetNofMvdHits();
      int nHitsTrackSts = tr[iVec]->GetNofStsHits();
      int nHitsTrack    = nHitsTrackMvd + nHitsTrackSts;
      for (int i = 0; i < nHitsTrack; i++) {

        const CbmPixelHit* hit;
        if (i < nHitsTrackMvd) {
          int hitIndex            = tr[iVec]->GetMvdHitIndex(i);
          const CbmMvdHit* mvdHit = &(vMvdHits[hitIndex]);
          ista                    = GetMvdStationIndex(mvdHit);
          if (ista < 0) {
            continue;
          }
          hit = mvdHit;
        }
        else {
          int hitIndex            = tr[iVec]->GetStsHitIndex(i - nHitsTrackMvd);
          const CbmStsHit* stsHit = &(vStsHits[hitIndex]);
          ista                    = GetStsStationIndex(stsHit);
          if (ista < 0) {
            continue;
          }
          hit = stsHit;
        }

        x[ista][iVec] = hit->GetX();
        y[ista][iVec] = hit->GetY();
        z[ista][iVec] = hit->GetZ();
        t[ista][iVec] = hit->GetTime();

        mxy[ista].X()[iVec]    = hit->GetX();
        mxy[ista].Y()[iVec]    = hit->GetY();
        mxy[ista].Dx2()[iVec]  = hit->GetDx() * hit->GetDx();
        mxy[ista].Dy2()[iVec]  = hit->GetDy() * hit->GetDy();
        mxy[ista].Dxy()[iVec]  = hit->GetDxy();
        mxy[ista].NdfX()[iVec] = 1.;
        mxy[ista].NdfY()[iVec] = 1.;
        dt2[ista][iVec]        = hit->GetTimeError() * hit->GetTimeError();

        w[ista][iVec] = true;


        fB_temp = sta[ista].fieldSlice.GetFieldValue(x[ista], y[ista]);
        fB[ista].SetSimdEntry(fB_temp.GetBx()[iVec], fB_temp.GetBy()[iVec], fB_temp.GetBz()[iVec], iVec);
        if (i == 0) {
          z_start[iVec]          = z[ista][iVec];
          x_first[iVec]          = x[ista][iVec];
          y_first[iVec]          = y[ista][iVec];
          t_first[iVec]          = t[ista][iVec];
          mxy_first.X()[iVec]    = mxy[ista].X()[iVec];
          mxy_first.Y()[iVec]    = mxy[ista].Y()[iVec];
          mxy_first.Dx2()[iVec]  = mxy[ista].Dx2()[iVec];
          mxy_first.Dy2()[iVec]  = mxy[ista].Dy2()[iVec];
          mxy_first.Dxy()[iVec]  = mxy[ista].Dxy()[iVec];
          mxy_first.NdfX()[iVec] = mxy[ista].NdfX()[iVec];
          mxy_first.NdfY()[iVec] = mxy[ista].NdfY()[iVec];
          dt2_first[iVec]        = dt2[ista][iVec];
        }
        if (i == nHitsTrack - 1) {
          z_end[iVec]           = z[ista][iVec];
          x_last[iVec]          = x[ista][iVec];
          y_last[iVec]          = y[ista][iVec];
          t_last[iVec]          = t[ista][iVec];
          mxy_last.X()[iVec]    = mxy[ista].X()[iVec];
          mxy_last.Y()[iVec]    = mxy[ista].Y()[iVec];
          mxy_last.Dx2()[iVec]  = mxy[ista].Dx2()[iVec];
          mxy_last.Dy2()[iVec]  = mxy[ista].Dy2()[iVec];
          mxy_last.Dxy()[iVec]  = mxy[ista].Dxy()[iVec];
          mxy_last.NdfX()[iVec] = mxy[ista].NdfX()[iVec];
          mxy_last.NdfY()[iVec] = mxy[ista].NdfY()[iVec];
          dt2_last[iVec]        = dt2[ista][iVec];
        }
      }
    }

    // fit forward

    int i = 0;

    FilterFirst(fit, mxy_first, t_first, dt2_first);
    fit.SetQp0(fit.Tr().Qp());

    z1 = z[i];
    b1 = sta[i].fieldSlice.GetFieldValue(T.X(), T.Y());
    b1.SetSimdEntries(fB[i], w[i]);
    z2 = z[i + 2];
    dz = z2 - z1;
    b2 = sta[i].fieldSlice.GetFieldValue(T.X() + T.Tx() * dz, T.Y() + T.Ty() * dz);
    b2.SetSimdEntries(fB[i + 2], w[i + 2]);
    fld.Set(b2, z2, b1, z1, b0, z0);
    for (++i; i < nHits; i++) {
      z0 = z[i];
      dz = (z1 - z0);
      b0 = sta[i].fieldSlice.GetFieldValue(T.X() - T.Tx() * dz, T.Y() - T.Ty() * dz);
      b0.SetSimdEntries(fB[i], w[i]);
      fld.Set(b0, z0, b1, z1, b2, z2);

      fmask initialised = (z[i] <= z_end) & (z_start < z[i]);

      fit.SetMask(initialised);
      fit.Extrapolate(z[i], fld);
      auto radThick = activeTrackingSetup.GetMaterial(i).GetThicknessX0(fit.Tr().X(), fit.Tr().Y());
      fit.MultipleScattering(radThick);
      fit.EnergyLossCorrection(radThick, kf::FitDirection::kDownstream);

      fit.SetMask(initialised && w[i]);
      fit.FilterXY(mxy[i]);
      fit.FilterTime(t[i], dt2[i], fmask(sta[i].timeInfo));

      b2 = b1;
      z2 = z1;
      b1 = b0;
      z1 = z0;
    }

    TrackParamV Tout = T;
    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      FairTrackParam par;
      par.SetX(T.X()[iVec]);
      par.SetY(T.Y()[iVec]);
      par.SetTx(T.Tx()[iVec]);
      par.SetTy(T.Ty()[iVec]);
      par.SetQp(T.Qp()[iVec]);
      par.SetZ(T.Z()[iVec]);

      for (int k = 0; k < 5; k++) {
        for (int j = 0; j <= k; j++) {
          par.SetCovariance(k, j, Tout.C(k, j)[iVec]);
        }
      }

      tr[iVec]->SetParamLast(&par);
    }

    // fit backward

    fit.SetQp0(T.Qp());

    i = nHits - 1;

    FilterFirst(fit, mxy_last, t_last, dt2_last);

    z1 = z[i];
    b1 = sta[i].fieldSlice.GetFieldValue(T.X(), T.Y());
    b1.SetSimdEntries(fB[i], w[i]);

    z2 = z[i - 2];
    dz = z2 - z1;
    b2 = sta[i].fieldSlice.GetFieldValue(T.X() + T.Tx() * dz, T.Y() + T.Ty() * dz);
    b2.SetSimdEntries(fB[i - 2], w[i - 2]);
    fld.Set(b2, z2, b1, z1, b0, z0);
    for (--i; i >= 0; i--) {
      z0 = z[i];
      dz = (z1 - z0);
      b0 = sta[i].fieldSlice.GetFieldValue(T.X() - T.Tx() * dz, T.Y() - T.Ty() * dz);
      b0.SetSimdEntries(fB[i], w[i]);
      fld.Set(b0, z0, b1, z1, b2, z2);

      fmask initialised = (z[i] < z_end) & (z_start <= z[i]);

      fit.SetMask(initialised);
      fit.Extrapolate(z[i], fld);
      auto radThick = activeTrackingSetup.GetMaterial(i).GetThicknessX0(fit.Tr().X(), fit.Tr().Y());
      fit.MultipleScattering(radThick);
      fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);

      fit.SetMask(initialised && w[i]);
      fit.FilterXY(mxy[i]);
      fit.FilterTime(t[i], dt2[i], fmask(sta[i].timeInfo));

      b2 = b1;
      z2 = z1;
      b1 = b0;
      z1 = z0;
    }

    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      FairTrackParam par;
      par.SetX(T.X()[iVec]);
      par.SetY(T.Y()[iVec]);
      par.SetTx(T.Tx()[iVec]);
      par.SetTy(T.Ty()[iVec]);
      par.SetQp(T.Qp()[iVec]);
      par.SetZ(T.Z()[iVec]);

      for (int k = 0; k < 5; k++) {
        for (int j = 0; j <= k; j++) {
          par.SetCovariance(k, j, T.C(k, j)[iVec]);
        }
      }

      tr[iVec]->SetParamFirst(&par);

      tr[iVec]->SetChiSq(T.ChiSq()[iVec]);
      tr[iVec]->SetNDF(static_cast<int>(T.Ndf()[iVec]));
    }
  }
}

void CbmL1PFFitter::Fit(vector<CbmStsTrack>& Tracks, const vector<int>& pidHypo)
{

  Initialize();

  std::vector<CbmMvdHit> vMvdHits;
  std::vector<CbmStsHit> vStsHits;

  if (fMvdHitArray) {
    for (int ih = 0; ih < fMvdHitArray->GetEntriesFast(); ih++) {
      vMvdHits.push_back(*dynamic_cast<const CbmMvdHit*>(fMvdHitArray->At(ih)));
    }
  }

  if (fStsHitArray) {
    for (int ih = 0; ih < fStsHitArray->GetEntriesFast(); ih++) {
      vStsHits.push_back(*dynamic_cast<const CbmStsHit*>(fStsHitArray->At(ih)));
    }
  }

  Fit(Tracks, vMvdHits, vStsHits, pidHypo);
}


void CbmL1PFFitter::GetChiToVertex(vector<CbmStsTrack>& Tracks, vector<PFFieldRegion>& field, vector<float>& chiToVtx,
                                   CbmKFVertex& primVtx, float chiPrim)
{
  // TODO: (!) replace CbmL1::Instance()->fpAlgo->GetParameters() with the cbm::ca::ParametersHandler::Instance()->Get()
  //           everywhere outside cbm::algo
  const auto& activeTrackingSetup = CbmL1::Instance()->fpAlgo->GetParameters().GetActiveSetup();
  Initialize();

  chiToVtx.reserve(Tracks.size());

  int nTracks_SIMD = fvec::size();

  kf::TrackKalmanFilter<fvec> fit;
  TrackParamV& T = fit.Tr();  // fitting parametr coresponding to current track

  CbmStsTrack* tr[fvec::size()]{nullptr};

  int nStations = fNmvdStationsActive + fNstsStationsActive;

  const ca::Station<fvec>* sta = CbmL1::Instance()->fpAlgo->GetParameters().GetStations().begin();
  fvec* zSta                   = new fvec[nStations];
  for (int iSta = 0; iSta < nStations; iSta++) {
    zSta[iSta] = sta[iSta].fZ;
  }

  field.reserve(Tracks.size());

  kf::FieldRegion<fvec> fld _fvecalignment;
  kf::FieldValue<fvec> fB[3], fB_temp _fvecalignment;
  fvec zField[3];

  unsigned short N_vTracks = Tracks.size();
  int ista;
  for (unsigned short itrack = 0; itrack < N_vTracks; itrack += fvec::size()) {
    if (N_vTracks - itrack < static_cast<unsigned short>(fvec::size())) {
      nTracks_SIMD = N_vTracks - itrack;
    }

    fvec mass2;
    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      tr[iVec]     = &Tracks[itrack + iVec];  // current track
      T.X()[iVec]  = tr[iVec]->GetParamFirst()->GetX();
      T.Y()[iVec]  = tr[iVec]->GetParamFirst()->GetY();
      T.Tx()[iVec] = tr[iVec]->GetParamFirst()->GetTx();
      T.Ty()[iVec] = tr[iVec]->GetParamFirst()->GetTy();
      T.Qp()[iVec] = tr[iVec]->GetParamFirst()->GetQp();
      T.Z()[iVec]  = tr[iVec]->GetParamFirst()->GetZ();

      for (int i = 0; i < 5; i++) {
        for (int j = 0; j <= i; j++) {
          T.C(i, j)[iVec] = tr[iVec]->GetParamFirst()->GetCovariance(i, j);
        }
      }

      //      float mass = TDatabasePDG::Instance()->GetParticle(tr[iVec]->GetPidHypo())->Mass();
      const float mass = KFParticleDatabase::Instance()->GetMass(tr[iVec]->GetPidHypo());
      mass2[iVec]      = mass * mass;

      int nHitsTrackMvd = tr[iVec]->GetNofMvdHits();
      for (int iH = 0; iH < 2; iH++) {
        float posx = 0.f, posy = 0.f, posz = 0.f;

        if (iH < nHitsTrackMvd) {
          if (!fMvdHitArray) {
            continue;
          }
          int hitIndex         = tr[iVec]->GetMvdHitIndex(iH);
          const CbmMvdHit* hit = dynamic_cast<const CbmMvdHit*>(fMvdHitArray->At(hitIndex));

          posx = hit->GetX();
          posy = hit->GetY();
          posz = hit->GetZ();
          ista = GetMvdStationIndex(hit);
          if (ista < 0) {
            continue;
          }
        }
        else {
          if (!fStsHitArray) {
            continue;
          }
          int hitIndex         = tr[iVec]->GetStsHitIndex(iH - nHitsTrackMvd);
          const CbmStsHit* hit = dynamic_cast<const CbmStsHit*>(fStsHitArray->At(hitIndex));

          posx = hit->GetX();
          posy = hit->GetY();
          posz = hit->GetZ();
          ista = GetStsStationIndex(hit);
          if (ista < 0) {
            continue;
          }
        }

        fB_temp = sta[ista].fieldSlice.GetFieldValue(posx, posy);
        fB[iH + 1].SetSimdEntry(fB_temp.GetBx()[iVec], fB_temp.GetBy()[iVec], fB_temp.GetBz()[iVec], iVec);
        zField[iH + 1][iVec] = posz;
      }
    }

    fB[0]     = CbmL1::Instance()->fpAlgo->GetParameters().GetVertexFieldValue();
    zField[0] = CbmL1::Instance()->fpAlgo->GetParameters().GetTargetPositionZ();
    fld.Set(fB[2], zField[2], fB[1], zField[1], fB[0], zField[0]);
    for (int i = 0; i < nTracks_SIMD; i++) {
      field.emplace_back(fld, i);
    }

    fit.SetQp0(fit.Tr().Qp());

    for (int iSt = nStations - 4; iSt >= 0; iSt--) {
      fit.SetMask(T.Z() > zSta[iSt] + fvec(2.5));
      fit.Extrapolate(zSta[iSt], fld);
      auto radThick = activeTrackingSetup.GetMaterial(iSt).GetThicknessX0(fit.Tr().X(), fit.Tr().Y());
      fit.MultipleScattering(radThick);
      fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);
    }
    fit.SetMask(fmask::One());
    fit.Extrapolate(primVtx.GetRefZ(), fld);

    // TODO: get it from parameters
    constexpr float targetRadThick = 3.73e-2f * 2;  // 250 mum Gold

    fit.MultipleScattering(targetRadThick);
    fit.EnergyLossCorrection(targetRadThick, kf::FitDirection::kUpstream);

    Double_t Cv[3] = {primVtx.GetCovMatrix()[0], primVtx.GetCovMatrix()[1], primVtx.GetCovMatrix()[2]};

    fvec dx   = T.X() - fvec(primVtx.GetRefX());
    fvec dy   = T.Y() - fvec(primVtx.GetRefY());
    fvec c[3] = {T.C00(), T.C10(), T.C11()};
    c[0] += fvec(Cv[0]);
    c[1] += fvec(Cv[1]);
    c[2] += fvec(Cv[2]);
    fvec d   = c[0] * c[2] - c[1] * c[1];
    fvec chi = sqrt(kf::utils::fabs(fvec(0.5) * (dx * dx * c[0] - fvec(2.) * dx * dy * c[1] + dy * dy * c[2]) / d));
    chi.setZero(kf::utils::fabs(d) < fvec(1.e-20));

    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      chiToVtx.push_back(chi[iVec]);
    }

    if (chiPrim > 0) {
      for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
        if (chi[iVec] < chiPrim) {
          FairTrackParam par;
          par.SetX(T.X()[iVec]);
          par.SetY(T.Y()[iVec]);
          par.SetTx(T.Tx()[iVec]);
          par.SetTy(T.Ty()[iVec]);
          par.SetQp(T.Qp()[iVec]);
          par.SetZ(T.Z()[iVec]);

          for (int i = 0; i < 5; i++) {
            for (int j = 0; j <= i; j++) {
              par.SetCovariance(i, j, T.C(i, j)[iVec]);
            }
          }

          tr[iVec]->SetParamFirst(&par);
        }
      }
    }
  }
  delete[] zSta;
}

void CbmL1PFFitter::CalculateFieldRegion(vector<CbmStsTrack>& Tracks, vector<PFFieldRegion>& field)
{
  Initialize();

  field.reserve(Tracks.size());

  kf::FieldRegion<fvec> fld _fvecalignment;

  int nTracks_SIMD = fvec::size();
  TrackParamV T;  // fitting parametr coresponding to current track

  CbmStsTrack* tr[fvec::size()];

  int ista;
  const ca::Station<fvec>* sta = CbmL1::Instance()->fpAlgo->GetParameters().GetStations().begin();
  kf::FieldValue<fvec> fB[3], fB_temp _fvecalignment;
  fvec zField[3];

  unsigned short N_vTracks = Tracks.size();

  for (unsigned short itrack = 0; itrack < N_vTracks; itrack += fvec::size()) {
    if (N_vTracks - itrack < static_cast<unsigned short>(fvec::size())) {
      nTracks_SIMD = N_vTracks - itrack;
    }

    for (int i = 0; i < nTracks_SIMD; i++) {
      tr[i] = &Tracks[itrack + i];  // current track
    }

    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      int nHitsTrackMvd = tr[iVec]->GetNofMvdHits();
      for (int iH = 0; iH < 2; iH++) {
        float posx = 0.f, posy = 0.f, posz = 0.f;

        if (iH < nHitsTrackMvd) {
          if (!fMvdHitArray) {
            continue;
          }
          int hitIndex         = tr[iVec]->GetMvdHitIndex(iH);
          const CbmMvdHit* hit = dynamic_cast<const CbmMvdHit*>(fMvdHitArray->At(hitIndex));

          posx = hit->GetX();
          posy = hit->GetY();
          posz = hit->GetZ();
          ista = GetMvdStationIndex(hit);
          if (ista < 0) {
            continue;
          }
        }
        else {
          if (!fStsHitArray) {
            continue;
          }
          int hitIndex         = tr[iVec]->GetStsHitIndex(iH - nHitsTrackMvd);
          const CbmStsHit* hit = dynamic_cast<const CbmStsHit*>(fStsHitArray->At(hitIndex));

          posx = hit->GetX();
          posy = hit->GetY();
          posz = hit->GetZ();
          ista = GetStsStationIndex(hit);
          if (ista < 0) {
            continue;
          }
        }

        fB_temp = sta[ista].fieldSlice.GetFieldValue(posx, posy);
        fB[iH + 1].SetSimdEntry(fB_temp.GetBx()[iVec], fB_temp.GetBy()[iVec], fB_temp.GetBz()[iVec], iVec);
        zField[iH + 1][iVec] = posz;
      }
    }

    fB[0]     = CbmL1::Instance()->fpAlgo->GetParameters().GetVertexFieldValue();
    zField[0] = CbmL1::Instance()->fpAlgo->GetParameters().GetTargetPositionZ();
    fld.Set(fB[2], zField[2], fB[1], zField[1], fB[0], zField[0]);
    for (int i = 0; i < nTracks_SIMD; i++) {
      field.emplace_back(fld, i);
    }
  }
}

void CbmL1PFFitter::CalculateFieldRegionAtLastPoint(vector<CbmStsTrack>& Tracks, vector<PFFieldRegion>& field)
{
  Initialize();

  field.reserve(Tracks.size());

  kf::FieldRegion<fvec> fld _fvecalignment;

  int nTracks_SIMD = fvec::size();
  TrackParamV T;  // fitting parametr coresponding to current track

  CbmStsTrack* tr[fvec::size()];

  int ista;
  const ca::Station<fvec>* sta = CbmL1::Instance()->fpAlgo->GetParameters().GetStations().begin();
  kf::FieldValue<fvec> fB[3], fB_temp _fvecalignment;
  fvec zField[3];

  unsigned short N_vTracks = Tracks.size();

  for (unsigned short itrack = 0; itrack < N_vTracks; itrack += fvec::size()) {
    if (N_vTracks - itrack < static_cast<unsigned short>(fvec::size())) {
      nTracks_SIMD = N_vTracks - itrack;
    }

    for (int i = 0; i < nTracks_SIMD; i++) {
      tr[i] = &Tracks[itrack + i];  // current track
    }

    for (int iVec = 0; iVec < nTracks_SIMD; iVec++) {
      int nHitsTrackMvd = tr[iVec]->GetNofMvdHits();
      int nHits         = tr[iVec]->GetTotalNofHits();
      for (int iH = 0; iH < 3; iH++) {
        float posx = 0.f, posy = 0.f, posz = 0.f;

        int hitNumber = nHits - iH - 1;
        if (hitNumber < nHitsTrackMvd) {
          if (!fMvdHitArray) {
            continue;
          }
          int hitIndex         = tr[iVec]->GetMvdHitIndex(hitNumber);
          const CbmMvdHit* hit = dynamic_cast<const CbmMvdHit*>(fMvdHitArray->At(hitIndex));

          posx = hit->GetX();
          posy = hit->GetY();
          posz = hit->GetZ();
          ista = GetMvdStationIndex(hit);
          if (ista < 0) {
            continue;
          }
        }
        else {
          if (!fStsHitArray) {
            continue;
          }
          int hitIndex         = tr[iVec]->GetStsHitIndex(hitNumber - nHitsTrackMvd);
          const CbmStsHit* hit = dynamic_cast<const CbmStsHit*>(fStsHitArray->At(hitIndex));

          posx = hit->GetX();
          posy = hit->GetY();
          posz = hit->GetZ();
          ista = GetStsStationIndex(hit);
          if (ista < 0) {
            continue;
          }
        }

        fB_temp = sta[ista].fieldSlice.GetFieldValue(posx, posy);

        fB[iH].SetSimdEntry(fB_temp.GetBx()[iVec], fB_temp.GetBy()[iVec], fB_temp.GetBz()[iVec], iVec);
        zField[iH][iVec] = posz;
      }
    }

    fld.Set(fB[0], zField[0], fB[1], zField[1], fB[2], zField[2]);
    for (int i = 0; i < nTracks_SIMD; i++) {
      field.emplace_back(fld, i);
    }
  }
}
