/* Copyright (C) 2023-2023 Warsaw University of Technology, Warsaw
    SPDX-License-Identifier: GPL-3.0-only
    Authors: Daniel Wielanek [committer] */
#include "CbmHelix.h"

#include <TMath.h>

#include <vector>

#include <Hal/Field.h>
// matrix x, y, tx,ty, qp, z
Hal::MagField* CbmHelix::fgField = NULL;
CbmHelix::CbmHelix() {}

void CbmHelix::Build(const CbmGlobalTrack* tr)
{
  const FairTrackParam* parameters = tr->GetParamVertex();
  SetParameters(parameters);
}

void CbmHelix::Build(const CbmStsTrack* tr, Bool_t firstPoint)
{
  if (firstPoint) {
    SetParameters(tr->GetParamFirst());
  }
  else {
    SetParameters(tr->GetParamLast());
  }
}

TVector3 CbmHelix::Eval(Double_t z)
{
  Propagate(z);
  return TVector3(GetTrack()[0], GetTrack()[1], GetTrack()[5]);
}

TVector3 CbmHelix::Eval(Double_t z, TVector3& mom)
{
  Propagate(z);
  Double_t p  = (TMath::Abs(Qp()) > 1.e-4) ? 1. / TMath::Abs(Qp()) : 1.e4;
  Double_t pz = TMath::Sqrt(p * p / (Tx() * Tx() + Ty() * Ty() + 1));
  Double_t px = Tx() * pz;
  Double_t py = Ty() * pz;
  mom.SetXYZ(px, py, pz);
  return TVector3(GetTrack()[0], GetTrack()[1], GetTrack()[5]);
}

void CbmHelix::SetParameters(const FairTrackParam* param)
{
  fTb[0] = param->GetX();
  fTb[1] = param->GetY();
  fTb[2] = param->GetTx();
  fTb[3] = param->GetTy();
  fTb[4] = param->GetQp();
  fTb[5] = param->GetZ();
}

void CbmHelix::Build(const TVector3& pos, const TVector3& mom, Double_t charge)
{
  fTb[0]     = pos.X();
  fTb[1]     = pos.Y();
  Double_t p = mom.Mag();
  fTb[2]     = mom.Px() / mom.Pz();
  fTb[3]     = mom.Py() / mom.Pz();
  fTb[4]     = charge / p;
  fTb[5]     = pos.Z();
}

CbmHelix::~CbmHelix() {}

CbmHelix::CbmHelix(const CbmHelix& other) : TObject()
{
  for (int i = 0; i < 6; i++) {
    fT[i]  = other.fT[i];
    fTb[i] = other.fTb[i];
  }
}

CbmHelix& CbmHelix::operator=(const CbmHelix& other)
{
  if (&other == this) return *this;
  for (int i = 0; i < 6; i++) {
    fT[i]  = other.fT[i];
    fTb[i] = other.fTb[i];
  }
  return *this;
}

Int_t CbmHelix::Propagate(Double_t z)
{
  Bool_t err = 0;
  for (int i = 0; i < 6; i++) {
    fT[i] = fTb[i];
  }
  if (fabs(fT[5] - z) < 1.e-5) return 0;

  Double_t zz = z;
  if (z < 300. && 300 <= fT[5]) ExtrapolateLine(300.);

  if (fT[5] < 300. && 300. < z) {
    zz = 300.;
  }
  Bool_t repeat = 1;
  while (!err && repeat) {
    const Double_t max_step = 5.;
    Double_t zzz;
    if (fabs(fT[5] - zz) > max_step)
      zzz = fT[5] + ((zz > fT[5]) ? max_step : -max_step);
    else {
      zzz    = zz;
      repeat = 0;
    }
    err = err || ExtrapolateALight(zzz);
  }
  if (fT[5] != z) ExtrapolateLine(z);
  return err;
}

void CbmHelix::ExtrapolateLine(Double_t z_out)
{
  Double_t dz = z_out - fT[5];

  fT[0] += dz * fT[2];
  fT[1] += dz * fT[3];
  fT[5] = z_out;
}

Int_t CbmHelix::ExtrapolateALight(Double_t z_out)
{
  //
  //  Part of the analytic extrapolation formula with error (c_light*B*dz)^4/4!
  //
  {
    bool ok = 1;
    for (int i = 0; i < 6; i++)
      ok = ok && !TMath::IsNaN(fT[i]) && (fT[i] < 1.e5);
  }
  const Double_t c_light = 0.000299792458;

  //Double_t qp_in = fT[4];
  Double_t z_in = fT[5];
  Double_t dz   = z_out - z_in;

  // construct coefficients

  Double_t x = fT[2],  // tx !!
    y        = fT[3],  // ty !!

    xx = x * x, xy = x * y, yy = y * y, xx31 = xx * 3 + 1, xx159 = xx * 15 + 9;

  const Double_t Ax = xy, Ay = -xx - 1, Az = y, Ayy = x * (xx * 3 + 3), Ayz = -2 * xy,
                 Ayyy = -(15 * xx * xx + 18 * xx + 3), Bx = yy + 1, By = -xy, Bz = -x, Byy = y * xx31, Byz = 2 * xx + 1,
                 Byyy = -xy * xx159;

  // end of coefficients calculation

  Double_t t2 = 1. + xx + yy;
  if (t2 > 1.e4) return 1;
  Double_t t = sqrt(t2), h = Qp() * c_light, ht = h * t;

  Double_t sx = 0, sy = 0, sz = 0, syy = 0, syz = 0, syyy = 0, Sx = 0, Sy = 0, Sz = 0, Syy = 0, Syz = 0, Syyy = 0;

  {  // get field integrals

    Double_t B[3][3];
    Double_t r0[3], r1[3], r2[3];

    // first order track approximation

    r0[0] = fT[0];
    r0[1] = fT[1];
    r0[2] = fT[5];

    r2[0] = fT[0] + fT[2] * dz;
    r2[1] = fT[1] + fT[3] * dz;
    r2[2] = z_out;

    r1[0] = 0.5 * (r0[0] + r2[0]);
    r1[1] = 0.5 * (r0[1] + r2[1]);
    r1[2] = 0.5 * (r0[2] + r2[2]);

    fgField->GetFieldValue(r0, B[0]);
    fgField->GetFieldValue(r1, B[1]);
    fgField->GetFieldValue(r2, B[2]);

    Sy    = (7 * B[0][1] + 6 * B[1][1] - B[2][1]) * dz * dz / 96.;
    r1[0] = fT[0] + x * dz / 2 + ht * Sy * Ay;
    r1[1] = fT[1] + y * dz / 2 + ht * Sy * By;

    Sy    = (B[0][1] + 2 * B[1][1]) * dz * dz / 6.;
    r2[0] = fT[0] + x * dz + ht * Sy * Ay;
    r2[1] = fT[1] + y * dz + ht * Sy * By;

    Sy = 0;

    // integrals

    fgField->GetFieldValue(r0, B[0]);
    fgField->GetFieldValue(r1, B[1]);
    fgField->GetFieldValue(r2, B[2]);

    sx = (B[0][0] + 4 * B[1][0] + B[2][0]) * dz / 6.;
    sy = (B[0][1] + 4 * B[1][1] + B[2][1]) * dz / 6.;
    sz = (B[0][2] + 4 * B[1][2] + B[2][2]) * dz / 6.;

    Sx = (B[0][0] + 2 * B[1][0]) * dz * dz / 6.;
    Sy = (B[0][1] + 2 * B[1][1]) * dz * dz / 6.;
    Sz = (B[0][2] + 2 * B[1][2]) * dz * dz / 6.;

    Double_t c2[3][3] = {{5, -4, -1}, {44, 80, -4}, {11, 44, 5}};    // /=360.
    Double_t C2[3][3] = {{38, 8, -4}, {148, 208, -20}, {3, 36, 3}};  // /=2520.
    for (Int_t n = 0; n < 3; n++)
      for (Int_t m = 0; m < 3; m++) {
        syz += c2[n][m] * B[n][1] * B[m][2];
        Syz += C2[n][m] * B[n][1] * B[m][2];
      }

    syz *= dz * dz / 360.;
    Syz *= dz * dz * dz / 2520.;

    syy  = (B[0][1] + 4 * B[1][1] + B[2][1]) * dz;
    syyy = syy * syy * syy / 1296;
    syy  = syy * syy / 72;

    Syy = (B[0][1] * (38 * B[0][1] + 156 * B[1][1] - B[2][1]) + B[1][1] * (208 * B[1][1] + 16 * B[2][1])
           + B[2][1] * (3 * B[2][1]))
          * dz * dz * dz / 2520.;
    Syyy = (B[0][1]
              * (B[0][1] * (85 * B[0][1] + 526 * B[1][1] - 7 * B[2][1]) + B[1][1] * (1376 * B[1][1] + 84 * B[2][1])
                 + B[2][1] * (19 * B[2][1]))
            + B[1][1] * (B[1][1] * (1376 * B[1][1] + 256 * B[2][1]) + B[2][1] * (62 * B[2][1]))
            + B[2][1] * B[2][1] * (3 * B[2][1]))
           * dz * dz * dz * dz / 90720.;
  }

  const Double_t

    sA1 = sx * Ax + sy * Ay + sz * Az,

    sB1 = sx * Bx + sy * By + sz * Bz,

    SA1 = Sx * Ax + Sy * Ay + Sz * Az,

    SB1 = Sx * Bx + Sy * By + Sz * Bz,

    sA2 = syy * Ayy + syz * Ayz, sB2 = syy * Byy + syz * Byz,

    SA2 = Syy * Ayy + Syz * Ayz, SB2 = Syy * Byy + Syz * Byz,

    sA3 = syyy * Ayyy, sB3 = syyy * Byyy,

    SA3 = Syyy * Ayyy, SB3 = Syyy * Byyy;
  fT[0] = fT[0] + x * dz + ht * (SA1 + ht * (SA2 + ht * SA3));
  fT[1] = fT[1] + y * dz + ht * (SB1 + ht * (SB2 + ht * SB3));
  fT[2] = fT[2] + ht * (sA1 + ht * (sA2 + ht * sA3));
  fT[3] = fT[3] + ht * (sB1 + ht * (sB2 + ht * sB3));
  fT[5] = z_out;
  return 0;
}
