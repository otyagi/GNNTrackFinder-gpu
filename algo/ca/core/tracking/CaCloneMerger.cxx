/* Copyright (C) 2010-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko, Maksym Zyzak [committer]*/

/// \file    CloneMerger.h
/// \authors S.Zharko <s.zharko@gsi.de> (interface), M.Zyzak (original algorithm)
/// \brief   A class wrapper over clones merger algorithm for the Ca track finder (implementation)
/// \since   22.07.2022 (second version)

#include "CaCloneMerger.h"

#include "CaFramework.h"
#include "CaParameters.h"
#include "CaTrack.h"
#include "CaVector.h"
#include "KfTrackKalmanFilter.h"

#include <iostream>

namespace cbm::algo::ca
{
  // -------------------------------------------------------------------------------------------------------------------
  //
  CloneMerger::CloneMerger(const ca::Parameters<fvec>& pars, const fscal mass) : fParameters(pars), fDefaultMass(mass)
  {
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  CloneMerger::~CloneMerger() {}

  // -------------------------------------------------------------------------------------------------------------------
  //
  void CloneMerger::Exec(const ca::InputData& input, WindowData& wData)
  {
    Vector<Track>& extTracks            = wData.RecoTracks();
    Vector<ca::HitIndex_t>& extRecoHits = wData.RecoHitIndices();

    Vector<unsigned short>& firstStation = fTrackFirstStation;
    Vector<unsigned short>& lastStation  = fTrackLastStation;
    Vector<ca::HitIndex_t>& firstHit     = fTrackFirstHit;
    Vector<ca::HitIndex_t>& lastHit      = fTrackLastHit;
    Vector<unsigned short>& neighbour    = fTrackNeighbour;
    Vector<fscal>& trackChi2             = fTrackChi2;
    Vector<char>& isStored               = fTrackIsStored;
    Vector<char>& isDownstreamNeighbour  = fTrackIsDownstreamNeighbour;

    int nTracks = extTracks.size();

    assert(nTracks < std::numeric_limits<unsigned short>::max());

    constexpr unsigned short kNoNeighbour = std::numeric_limits<unsigned short>::max();

    fTracksNew.clear();
    fTracksNew.reserve(nTracks);
    fRecoHitsNew.clear();
    fRecoHitsNew.reserve(extRecoHits.size());

    firstStation.reset(nTracks);
    lastStation.reset(nTracks);
    firstHit.reset(nTracks);
    lastHit.reset(nTracks);
    isStored.reset(nTracks);
    trackChi2.reset(nTracks);
    neighbour.reset(nTracks);
    isDownstreamNeighbour.reset(nTracks);

    ca::HitIndex_t start_hit = 0;

    for (int iTr = 0; iTr < nTracks; iTr++) {
      firstHit[iTr]     = start_hit;
      firstStation[iTr] = input.GetHit(extRecoHits[start_hit]).Station();
      start_hit += extTracks[iTr].fNofHits - 1;
      lastHit[iTr]     = start_hit;
      lastStation[iTr] = input.GetHit(extRecoHits[start_hit]).Station();
      start_hit++;

      isStored[iTr]              = false;
      neighbour[iTr]             = kNoNeighbour;
      trackChi2[iTr]             = 100000.;
      isDownstreamNeighbour[iTr] = false;
    }

    kf::TrackKalmanFilter<fvec> fitB;
    fitB.SetParticleMass(fDefaultMass);
    fitB.SetMask(fmask::One());
    fitB.SetQp0(fvec(0.));

    kf::TrackKalmanFilter<fvec> fitF;
    fitF.SetParticleMass(fDefaultMass);
    fitF.SetMask(fmask::One());
    fitF.SetQp0(fvec(0.));

    TrackParamV& Tb = fitB.Tr();
    TrackParamV& Tf = fitF.Tr();
    kf::FieldValue<fvec> fBm, fBb, fBf _fvecalignment;
    kf::FieldRegion<fvec> fld _fvecalignment;

    // Max length for merging
    unsigned char maxLengthForMerge = static_cast<unsigned char>(fParameters.GetNstationsActive() - 3);

    for (int iTr = 0; iTr < nTracks; iTr++) {
      if (extTracks[iTr].fNofHits > maxLengthForMerge) continue;
      for (int jTr = 0; jTr < nTracks; jTr++) {
        if (extTracks[jTr].fNofHits > maxLengthForMerge) continue;
        if (iTr == jTr) continue;
        if (firstStation[iTr] <= lastStation[jTr]) continue;

        unsigned short stab = firstStation[iTr];

        Tb.Set(extTracks[iTr].fParFirst);

        fitB.SetQp0(fitB.Tr().GetQp());

        unsigned short staf = lastStation[jTr];

        Tf.Set(extTracks[jTr].fParLast);
        fitF.SetQp0(fitF.Tr().GetQp());

        if (Tf.NdfTime()[0] >= 0. && Tb.NdfTime()[0] >= 0.) {
          if (fabs(Tf.GetTime()[0] - Tb.GetTime()[0]) > 3 * sqrt(Tf.C55()[0] + Tb.C55()[0])) continue;
        }

        unsigned short stam;

        fBf = fParameters.GetStation(staf).fieldSlice.GetFieldValue(Tf.X(), Tf.Y());
        fBb = fParameters.GetStation(stab).fieldSlice.GetFieldValue(Tb.X(), Tb.Y());

        unsigned short dist = firstStation[iTr] - lastStation[jTr];

        if (dist > 1)
          stam = staf + 1;
        else
          stam = staf - 1;

        fvec zm = fParameters.GetStation(stam).fZ;
        fvec xm = fvec(0.5) * (Tf.GetX() + Tf.Tx() * (zm - Tf.Z()) + Tb.GetX() + Tb.Tx() * (zm - Tb.Z()));
        fvec ym = fvec(0.5) * (Tf.Y() + Tf.Ty() * (zm - Tf.Z()) + Tb.Y() + Tb.Ty() * (zm - Tb.Z()));
        fBm     = fParameters.GetStation(stam).fieldSlice.GetFieldValue(xm, ym);
        fld.Set(fBb, Tb.Z(), fBm, zm, fBf, Tf.Z());

        fvec zMiddle = fvec(0.5) * (Tb.Z() + Tf.Z());

        fitF.Extrapolate(zMiddle, fld);
        fitB.Extrapolate(zMiddle, fld);

        fvec Chi2Tracks(0.);
        FilterTracks(&(Tf.X()), &(Tf.C00()), &(Tb.X()), &(Tb.C00()), nullptr, nullptr, &Chi2Tracks);
        if (Chi2Tracks[0] > 50) continue;

        if (Chi2Tracks[0] < trackChi2[iTr] || Chi2Tracks[0] < trackChi2[jTr]) {
          if (neighbour[iTr] < kNoNeighbour) {
            neighbour[neighbour[iTr]]             = kNoNeighbour;
            trackChi2[neighbour[iTr]]             = 100000.;
            isDownstreamNeighbour[neighbour[iTr]] = false;
          }
          if (neighbour[jTr] < kNoNeighbour) {
            neighbour[neighbour[jTr]]             = kNoNeighbour;
            trackChi2[neighbour[jTr]]             = 100000.;
            isDownstreamNeighbour[neighbour[jTr]] = false;
          }
          neighbour[iTr]             = jTr;
          neighbour[jTr]             = iTr;
          trackChi2[iTr]             = Chi2Tracks[0];
          trackChi2[jTr]             = Chi2Tracks[0];
          isDownstreamNeighbour[iTr] = true;
          isDownstreamNeighbour[jTr] = false;
        }
      }
    }

    for (int iTr = 0; iTr < nTracks; iTr++) {
      if (isStored[iTr]) continue;

      fTracksNew.push_back(extTracks[iTr]);
      if (!isDownstreamNeighbour[iTr]) {
        for (ca::HitIndex_t HI = firstHit[iTr]; HI <= lastHit[iTr]; HI++) {
          fRecoHitsNew.push_back(extRecoHits[HI]);
        }
      }

      if (neighbour[iTr] < kNoNeighbour) {
        isStored[neighbour[iTr]] = true;
        fTracksNew.back().fNofHits += extTracks[neighbour[iTr]].fNofHits;
        for (ca::HitIndex_t HI = firstHit[neighbour[iTr]]; HI <= lastHit[neighbour[iTr]]; HI++)
          fRecoHitsNew.push_back(extRecoHits[HI]);
      }

      if (isDownstreamNeighbour[iTr]) {
        for (ca::HitIndex_t HI = firstHit[iTr]; HI <= lastHit[iTr]; HI++) {
          fRecoHitsNew.push_back(extRecoHits[HI]);
        }
      }
    }

    // Save the merging results into external vectors
    extTracks   = std::move(fTracksNew);
    extRecoHits = std::move(fRecoHitsNew);
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void CloneMerger::FilterTracks(fvec const r[5], fvec const C[15], fvec const m[5], fvec const V[15], fvec R[5],
                                 fvec W[15], fvec* chi2)
  {
    fvec S[15];
    for (int i = 0; i < 15; i++) {
      if (W) W[i] = C[i];
      S[i] = C[i] + V[i];
    }

    InvertCholesky(S);

    fvec dzeta[5];

    for (int i = 0; i < 5; i++)
      dzeta[i] = m[i] - r[i];

    if (W && R) {
      for (int i = 0; i < 5; i++)
        R[i] = r[i];

      fvec K[5][5];
      MultiplySS(C, S, K);

      fvec KC[15];
      MultiplyMS(K, C, KC);
      for (int i = 0; i < 15; i++)
        W[i] -= KC[i];

      fvec kd;
      for (int i = 0; i < 5; i++) {
        kd = 0.f;
        for (int j = 0; j < 5; j++)
          kd += K[i][j] * dzeta[j];
        R[i] += kd;
      }
    }
    if (chi2) {
      fvec S_dzeta[5];
      MultiplySR(S, dzeta, S_dzeta);
      *chi2 = dzeta[0] * S_dzeta[0] + dzeta[1] * S_dzeta[1] + dzeta[2] * S_dzeta[2] + dzeta[3] * S_dzeta[3]
              + dzeta[4] * S_dzeta[4];
    }
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void CloneMerger::InvertCholesky(fvec a[15])
  {
    fvec d[5], uud, u[5][5];
    for (int i = 0; i < 5; i++) {
      d[i] = 0.f;
      for (int j = 0; j < 5; j++)
        u[i][j] = 0.f;
    }

    for (int i = 0; i < 5; i++) {
      uud = 0.f;
      for (int j = 0; j < i; j++)
        uud += u[j][i] * u[j][i] * d[j];
      uud = a[i * (i + 3) / 2] - uud;

      fvec smallval(1.e-12);
      uud = iif(uud >= smallval, uud, smallval);

      d[i]    = uud / kf::utils::fabs(uud);
      u[i][i] = sqrt(kf::utils::fabs(uud));

      for (int j = i + 1; j < 5; j++) {
        uud = 0.f;
        for (int k = 0; k < i; k++)
          uud += u[k][i] * u[k][j] * d[k];
        uud     = a[j * (j + 1) / 2 + i] /*a[i][j]*/ - uud;
        u[i][j] = d[i] / u[i][i] * uud;
      }
    }

    fvec u1[5];

    for (int i = 0; i < 5; i++) {
      u1[i]   = u[i][i];
      u[i][i] = 1.f / u[i][i];
    }
    for (int i = 0; i < 4; i++) {
      u[i][i + 1] = -u[i][i + 1] * u[i][i] * u[i + 1][i + 1];
    }
    for (int i = 0; i < 3; i++) {
      u[i][i + 2] = u[i][i + 1] * u1[i + 1] * u[i + 1][i + 2] - u[i][i + 2] * u[i][i] * u[i + 2][i + 2];
    }
    for (int i = 0; i < 2; i++) {
      u[i][i + 3] = u[i][i + 2] * u1[i + 2] * u[i + 2][i + 3] - u[i][i + 3] * u[i][i] * u[i + 3][i + 3];
      u[i][i + 3] -= u[i][i + 1] * u1[i + 1] * (u[i + 1][i + 2] * u1[i + 2] * u[i + 2][i + 3] - u[i + 1][i + 3]);
    }
    u[0][4] = u[0][2] * u1[2] * u[2][4] - u[0][4] * u[0][0] * u[4][4];
    u[0][4] += u[0][1] * u1[1] * (u[1][4] - u[1][3] * u1[3] * u[3][4] - u[1][2] * u1[2] * u[2][4]);
    u[0][4] += u[3][4] * u1[3] * (u[0][3] - u1[2] * u[2][3] * (u[0][2] - u[0][1] * u1[1] * u[1][2]));

    for (int i = 0; i < 5; i++)
      a[i + 10] = u[i][4] * d[4] * u[4][4];
    for (int i = 0; i < 4; i++)
      a[i + 6] = u[i][3] * u[3][3] * d[3] + u[i][4] * u[3][4] * d[4];
    for (int i = 0; i < 3; i++)
      a[i + 3] = u[i][2] * u[2][2] * d[2] + u[i][3] * u[2][3] * d[3] + u[i][4] * u[2][4] * d[4];
    for (int i = 0; i < 2; i++)
      a[i + 1] =
        u[i][1] * u[1][1] * d[1] + u[i][2] * u[1][2] * d[2] + u[i][3] * u[1][3] * d[3] + u[i][4] * u[1][4] * d[4];
    a[0] = u[0][0] * u[0][0] * d[0] + u[0][1] * u[0][1] * d[1] + u[0][2] * u[0][2] * d[2] + u[0][3] * u[0][3] * d[3]
           + u[0][4] * u[0][4] * d[4];
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void CloneMerger::MultiplyMS(fvec const C[5][5], fvec const V[15], fvec K[15])
  {
    K[0] = C[0][0] * V[0] + C[0][1] * V[1] + C[0][2] * V[3] + C[0][3] * V[6] + C[0][4] * V[10];

    K[1] = C[1][0] * V[0] + C[1][1] * V[1] + C[1][2] * V[3] + C[1][3] * V[6] + C[1][4] * V[10];
    K[2] = C[1][0] * V[1] + C[1][1] * V[2] + C[1][2] * V[4] + C[1][3] * V[7] + C[1][4] * V[11];

    K[3] = C[2][0] * V[0] + C[2][1] * V[1] + C[2][2] * V[3] + C[2][3] * V[6] + C[2][4] * V[10];
    K[4] = C[2][0] * V[1] + C[2][1] * V[2] + C[2][2] * V[4] + C[2][3] * V[7] + C[2][4] * V[11];
    K[5] = C[2][0] * V[3] + C[2][1] * V[4] + C[2][2] * V[5] + C[2][3] * V[8] + C[2][4] * V[12];

    K[6] = C[3][0] * V[0] + C[3][1] * V[1] + C[3][2] * V[3] + C[3][3] * V[6] + C[3][4] * V[10];
    K[7] = C[3][0] * V[1] + C[3][1] * V[2] + C[3][2] * V[4] + C[3][3] * V[7] + C[3][4] * V[11];
    K[8] = C[3][0] * V[3] + C[3][1] * V[4] + C[3][2] * V[5] + C[3][3] * V[8] + C[3][4] * V[12];
    K[9] = C[3][0] * V[6] + C[3][1] * V[7] + C[3][2] * V[8] + C[3][3] * V[9] + C[3][4] * V[13];

    K[10] = C[4][0] * V[0] + C[4][1] * V[1] + C[4][2] * V[3] + C[4][3] * V[6] + C[4][4] * V[10];
    K[11] = C[4][0] * V[1] + C[4][1] * V[2] + C[4][2] * V[4] + C[4][3] * V[7] + C[4][4] * V[11];
    K[12] = C[4][0] * V[3] + C[4][1] * V[4] + C[4][2] * V[5] + C[4][3] * V[8] + C[4][4] * V[12];
    K[13] = C[4][0] * V[6] + C[4][1] * V[7] + C[4][2] * V[8] + C[4][3] * V[9] + C[4][4] * V[13];
    K[14] = C[4][0] * V[10] + C[4][1] * V[11] + C[4][2] * V[12] + C[4][3] * V[13] + C[4][4] * V[14];
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void CloneMerger::MultiplySR(fvec const C[15], fvec const r_in[5], fvec r_out[5])
  {
    r_out[0] = r_in[0] * C[0] + r_in[1] * C[1] + r_in[2] * C[3] + r_in[3] * C[6] + r_in[4] * C[10];
    r_out[1] = r_in[0] * C[1] + r_in[1] * C[2] + r_in[2] * C[4] + r_in[3] * C[7] + r_in[4] * C[11];
    r_out[2] = r_in[0] * C[3] + r_in[1] * C[4] + r_in[2] * C[5] + r_in[3] * C[8] + r_in[4] * C[12];
    r_out[3] = r_in[0] * C[6] + r_in[1] * C[7] + r_in[2] * C[8] + r_in[3] * C[9] + r_in[4] * C[13];
    r_out[4] = r_in[0] * C[10] + r_in[1] * C[11] + r_in[2] * C[12] + r_in[3] * C[13] + r_in[4] * C[14];
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  void CloneMerger::MultiplySS(fvec const C[15], fvec const V[15], fvec K[5][5])
  {
    K[0][0] = C[0] * V[0] + C[1] * V[1] + C[3] * V[3] + C[6] * V[6] + C[10] * V[10];
    K[0][1] = C[0] * V[1] + C[1] * V[2] + C[3] * V[4] + C[6] * V[7] + C[10] * V[11];
    K[0][2] = C[0] * V[3] + C[1] * V[4] + C[3] * V[5] + C[6] * V[8] + C[10] * V[12];
    K[0][3] = C[0] * V[6] + C[1] * V[7] + C[3] * V[8] + C[6] * V[9] + C[10] * V[13];
    K[0][4] = C[0] * V[10] + C[1] * V[11] + C[3] * V[12] + C[6] * V[13] + C[10] * V[14];

    K[1][0] = C[1] * V[0] + C[2] * V[1] + C[4] * V[3] + C[7] * V[6] + C[11] * V[10];
    K[1][1] = C[1] * V[1] + C[2] * V[2] + C[4] * V[4] + C[7] * V[7] + C[11] * V[11];
    K[1][2] = C[1] * V[3] + C[2] * V[4] + C[4] * V[5] + C[7] * V[8] + C[11] * V[12];
    K[1][3] = C[1] * V[6] + C[2] * V[7] + C[4] * V[8] + C[7] * V[9] + C[11] * V[13];
    K[1][4] = C[1] * V[10] + C[2] * V[11] + C[4] * V[12] + C[7] * V[13] + C[11] * V[14];

    K[2][0] = C[3] * V[0] + C[4] * V[1] + C[5] * V[3] + C[8] * V[6] + C[12] * V[10];
    K[2][1] = C[3] * V[1] + C[4] * V[2] + C[5] * V[4] + C[8] * V[7] + C[12] * V[11];
    K[2][2] = C[3] * V[3] + C[4] * V[4] + C[5] * V[5] + C[8] * V[8] + C[12] * V[12];
    K[2][3] = C[3] * V[6] + C[4] * V[7] + C[5] * V[8] + C[8] * V[9] + C[12] * V[13];
    K[2][4] = C[3] * V[10] + C[4] * V[11] + C[5] * V[12] + C[8] * V[13] + C[12] * V[14];

    K[3][0] = C[6] * V[0] + C[7] * V[1] + C[8] * V[3] + C[9] * V[6] + C[13] * V[10];
    K[3][1] = C[6] * V[1] + C[7] * V[2] + C[8] * V[4] + C[9] * V[7] + C[13] * V[11];
    K[3][2] = C[6] * V[3] + C[7] * V[4] + C[8] * V[5] + C[9] * V[8] + C[13] * V[12];
    K[3][3] = C[6] * V[6] + C[7] * V[7] + C[8] * V[8] + C[9] * V[9] + C[13] * V[13];
    K[3][4] = C[6] * V[10] + C[7] * V[11] + C[8] * V[12] + C[9] * V[13] + C[13] * V[14];

    K[4][0] = C[10] * V[0] + C[11] * V[1] + C[12] * V[3] + C[13] * V[6] + C[14] * V[10];
    K[4][1] = C[10] * V[1] + C[11] * V[2] + C[12] * V[4] + C[13] * V[7] + C[14] * V[11];
    K[4][2] = C[10] * V[3] + C[11] * V[4] + C[12] * V[5] + C[13] * V[8] + C[14] * V[12];
    K[4][3] = C[10] * V[6] + C[11] * V[7] + C[12] * V[8] + C[13] * V[9] + C[14] * V[13];
    K[4][4] = C[10] * V[10] + C[11] * V[11] + C[12] * V[12] + C[13] * V[13] + C[14] * V[14];
  }
}  // namespace cbm::algo::ca
