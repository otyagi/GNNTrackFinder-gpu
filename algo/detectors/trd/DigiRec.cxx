/* Copyright (C) 2018-2020 Horia Hulubei National Institute of Physics and Nuclear Engineering, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci[committer] */

#include "DigiRec.h"

#include <cstring>

namespace cbm::algo::trd
{

  //// Taken from CbmTrdFASP. TO DO: Remove these magic numbers!!
  float DigiRec::fgBaseline = 0.25;   //[V]  AB : match HW on mCBM22
  float DigiRec::fgOutGain  = 2.025;  // [V/ 4095 ADC chs]

  //_____________________________________________________________________________
  DigiRec::DigiRec() : CbmTrdDigi(), fStatus(0)
  {
    fG[0] = 1.;
    fG[1] = 1.;
    memset(fT, 0, 3 * sizeof(double));
  }

  //_____________________________________________________________________________
  DigiRec::DigiRec(const CbmTrdDigi& d, double* G, double* T) : CbmTrdDigi(d), fStatus(0)
  {
    if (G) {
      fG[0] = G[0];
      fG[1] = G[1];
    }
    else {
      fG[0] = 1.;
      fG[1] = 1.;
    }
    if (T)
      memcpy(fT, T, 3 * sizeof(double));
    else
      memset(fT, 0, 3 * sizeof(double));

    int dt;
    double t, r = CbmTrdDigi::GetCharge(t, dt);
    if (t > 4094) SETBIT(fStatus, 0);
    if (r > 4094) SETBIT(fStatus, 1);
  }

  //_____________________________________________________________________________
  DigiRec::DigiRec(const CbmTrdDigi& d, const CbmTrdDigi& dr, double* G, double* T) : CbmTrdDigi(d), fStatus(0)
  {
    /** Constructor and RAW digi merger
*/
    if (G) {
      fG[0] = G[0];
      fG[1] = G[1];
    }
    else {
      fG[0] = 1.;
      fG[1] = 1.;
    }
    if (T)
      memcpy(fT, T, 3 * sizeof(double));
    else
      memset(fT, 0, 3 * sizeof(double));

    int dt;
    double t, r = dr.GetCharge(t, dt);
    if (r > 4094) SETBIT(fStatus, 1);
    CbmTrdDigi::GetCharge(t, dt);
    if (t > 4094) SETBIT(fStatus, 0);
    dt = dr.GetTimeDAQ() - GetTimeDAQ();
    SetCharge(t, r, dt);
    int rtrg(dr.GetTriggerType() & 2), ttrg(GetTriggerType() & 1);
    SetTriggerType(rtrg | ttrg);  //merge the triggers
  }

  //_____________________________________________________________________________
  double DigiRec::GetCharge(int typ, bool& on) const
  {
    int dT;
    double T, R = CbmTrdDigi::GetCharge(T, dT);
    on = true;
    if (typ) {  // rectangular pairing
      if (R > 0) {
        R -= GetBaselineCorr();
        return fG[1] * R;
      }
      else
        on = false;
    }
    else {  // tilt pairing
      if (T > 0) {
        T -= GetBaselineCorr();
        return fG[0] * T;
      }
      else
        on = false;
    }
    return 0.;
  }

  //_____________________________________________________________________________
  double DigiRec::GetTime(int typ) const
  {
    int dT;
    double T, R = CbmTrdDigi::GetCharge(T, dT);
    if (typ) {  // rectangular pairing
      double R0 = R - fT[2];
      return GetTimeDAQ() + dT + fT[0] * R0 * R0 + fT[1] * R0;
    }
    else {  // tilt pairing
      double T0 = T - fT[2];
      return GetTimeDAQ() + fT[0] * T0 * T0 + fT[1] * T0;
    }
  }

  //_____________________________________________________________________________
  void DigiRec::Init(double g[2], double t[3])
  {
    fG[0] = g[0];
    fG[1] = g[1];
    memcpy(fT, t, 3 * sizeof(double));
  }

}  // namespace cbm::algo::trd
