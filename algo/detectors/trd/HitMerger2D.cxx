/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#include "HitMerger2D.h"

#include "AlgoFairloggerCompat.h"

#include <functional>
#include <numeric>


using std::endl;
using std::vector;

namespace cbm::algo::trd
{

  //_______________________________________________________________________________
  HitMerger2D::HitMerger2D(HitFinder2DModPar params) : fParams(params) {}

  //_______________________________________________________________________________
  HitMerger2D::outputType HitMerger2D::operator()(std::vector<inputType>& hitsRow1, std::vector<inputType>& hitsRow2)
  {
    std::vector<inputType*> hitData;

    for (auto& elem : hitsRow1) {
      hitData.push_back(&elem);
    }
    for (auto& elem : hitsRow2) {
      hitData.push_back(&elem);
    }

    // Sort step below is in principle needed, but wasn't present in original implementation
    // TO DO: Clarify
    //std::sort(hitData.begin(), hitData.end(), [](const auto& h0, const auto& h1) { return h0->first.Time() < h1->first.Time(); });

    for (size_t ih = 0; ih < hitData.size(); ih++) {
      Hit* h0                       = &hitData[ih]->first;
      std::vector<DigiRec>* h0digis = &hitData[ih]->second;

      if (h0->IsUsed()) continue;

      for (size_t jh = ih + 1; jh < hitData.size(); jh++) {
        Hit* h1                       = &hitData[jh]->first;
        std::vector<DigiRec>* h1digis = &hitData[jh]->second;

        if (h1->IsUsed()) continue;

        // basic check on Time
        if (h1->Time() < 4000 - h0->Time()) continue;  // TODO check with preoper time calibration
        // skip next hits for being too far (10us) in the future
        if (h1->Time() > 10000 + h0->Time()) break;

        // basic check on Col
        if (std::abs(h1->X() - h0->X()) > 2 * fParams.padSizeX) continue;

        // basic check on Row
        if (std::abs(h1->Y() - h0->Y()) > 2 * fParams.padSizeY) continue;

        // go to topologic checks
        const int a0 = CheckMerge(h0digis, h1digis);
        if (a0 == 0) continue;

        auto [hitFlag, hitF] = ProjectDigis(a0 < 0 ? h0digis : h1digis, a0 < 0 ? h1digis : h0digis);
        // call the working algorithm
        if (MergeHits(h0, a0, hitF)) {
          h0->SetRowCross((bool) h1);
          h1->SetRefId(-1);
        }
      }
    }

    const auto ret1 =
      std::remove_if(hitsRow1.begin(), hitsRow1.end(), [](const auto& obj) { return obj.first.IsUsed(); });
    hitsRow1.erase(ret1, hitsRow1.end());

    const auto ret2 =
      std::remove_if(hitsRow2.begin(), hitsRow2.end(), [](const auto& obj) { return obj.first.IsUsed(); });
    hitsRow2.erase(ret2, hitsRow2.end());

    return std::make_pair(std::move(hitsRow1), std::move(hitsRow2));
  }


  //_______________________________________________________________________________
  int HitMerger2D::CheckMerge(std::vector<DigiRec>* cid, std::vector<DigiRec>* cjd)
  {
    /** Check topologic conditions if the 2 clusters (in digi representation) can be merged.
 * The first pair is always from the bottom row
 * return the anode candidate wrt boundary 1, 2, 3 for the first 3 anodes of the upper row; -1, -2, -3 for the bottom row (r0) or 0 if the check fails
 */

    bool on(kFALSE);
    int vc[2] = {-1, -1}, vm[2] = {0};
    double M[2] = {-1., -1.}, S[2] = {0.};
    vector<DigiRec>::const_iterator jp[2];

    for (int rowId(0); rowId < 2; rowId++) {
      double rtMax                        = 0.;
      double mdMax                        = 0.;
      const std::vector<DigiRec>* vcid[2] = {cid, cjd};
      for (auto id = vcid[rowId]->begin(); id != vcid[rowId]->end(); id++) {
        int col;
        GetPadRowCol((*id).GetAddressChannel(), col);

        // mark max position and type
        const double t = (*id).GetTiltCharge(on);
        if (on && t > rtMax) {
          vc[rowId] = col;
          vm[rowId] = 0;
          rtMax     = t;
        }
        const double r = (*id).GetRectCharge(on);
        if (on && r > rtMax) {
          vc[rowId] = col;
          vm[rowId] = 1;
          rtMax     = r;
        }

        double m = 0.;
        double d = 0.;
        if (!rowId) {  // compute TR pairs on the bottom row
          m = 0.5 * (t + r);
          d = r - t;
        }
        else {  // compute RT pairs on the upper row
          auto jd  = std::next(id);
          double T = 0;
          if (jd != vcid[rowId]->end()) T = (*jd).GetTiltCharge(on);
          m = 0.5 * (r + T);
          d = r - T;
        }
        if (std::abs(m) > 0.) d = 1.e2 * d / m;
        if (m > mdMax) {
          mdMax     = m;
          M[rowId]  = m;
          S[rowId]  = d;
          jp[rowId] = id;
        }
      }
    }
    int rowMax = M[0] > M[1] ? 0 : 1;

    // basic check on col of the max signal
    const int dc = vc[1] - vc[0];
    if (dc < 0 || dc > 1) return 0;

    // special care for both tilt maxima :  the TT case
    // recalculate values on the min row on neighbor column
    if (!vm[0] && !vm[1]) {
      if (rowMax == 0) {  // modify r=1
        double r = 0.;
        double T = 0.;
        if (M[1] >= 0) {
          if (jp[1] != cjd->end()) jp[1]++;
          if (jp[1] != cjd->end()) {
            r = (*jp[1]).GetRectCharge(on);
            jp[1]++;
            if (jp[1] != cjd->end()) T = (*jp[1]).GetTiltCharge(on);
          }
        }
        M[1] = 0.5 * (r + T);
        S[1] = r - T;
      }
      else {  // modify r=0
        double r = 0.;
        double t = 0.;
        if (M[0] >= 0) {
          if (jp[0] != cid->begin()) jp[0]--;
          if (jp[0] != cid->begin()) {
            r = (*jp[0]).GetRectCharge(on);
            t = (*jp[0]).GetTiltCharge(on);
          }
        }
        M[0] = 0.5 * (t + r);
        S[0] = r - t;
      }
    }
    rowMax = M[0] > M[1] ? 0 : 1;

    // Build the ratio of the charge
    const float mM = M[rowMax ? 0 : 1] / M[rowMax];
    const float mS = std::abs(S[rowMax]), mM_l[3] = {0.15, 0.5, 1}, mM_r[3] = {0, 0.28, 1}, mS_r[3] = {43, 27, 20};
    float dSdM[2], S0[2];

    for (int i(0); i < 2; i++) {
      dSdM[i] = (mS_r[i + 1] - mS_r[i]) / (mM_r[i + 1] - mM_r[i]);
      S0[i]   = mS_r[i] - dSdM[i] * mM_r[i];
    }
    const int irange = mM < mM_r[1] ? 0 : 1;
    if (mS > S0[irange] + dSdM[irange] * mM) return 0;

    for (int ia(0); ia < 3; ia++) {
      if (mM < mM_l[ia]) return (rowMax ? 1 : -1) * (3 - ia);
    }
    return 0;
  }


  //_______________________________________________________________________________
  bool HitMerger2D::MergeHits(Hit* h, int a0, HitFactory2D& hitF)
  {
    int n0(hitF.fSignal.size() - 2);
    auto [dx, dy] = hitF.GetDxDy(n0);

    int typ(hitF.GetHitClass());
    // get position correction [pw]
    double xcorr = hitF.GetXcorr(dx, typ, 1) / fParams.padSizeX, xcorrBias(xcorr);
    if (hitF.IsBiasX()) {
      typ      = hitF.GetHitRcClass(a0);
      int xmap = hitF.vyM & 0xff;
      switch (n0) {
        case 4:
          if (dx < 0)
            xcorrBias += (hitF.IsBiasXleft() ? -0.12 : 0.176);
          else
            xcorrBias += (xmap == 53 || xmap == 80 || xmap == 113 || xmap == 117 ? -0.176 : 0.12);
          break;
        case 5:
        case 7:
          if (typ < 0) break;
          if (xmap == 50 || xmap == 58 || xmap == 146 || xmap == 154) {
            if (typ == 2)
              xcorr += 0.0813;
            else if (typ == 3) {
              xcorr -= 0.0813;
              typ = 2;
            }
            dx -= xcorr;
            hitF.RecenterXoffset(dx);
            xcorrBias = hitF.GetXcorr(dx, typ, 2) / fParams.padSizeX;
          }
          else {
            dx -= xcorr;
            hitF.RecenterXoffset(dx);
            if (typ < 2)
              xcorrBias = hitF.GetXcorr(dx, typ, 2) / fParams.padSizeX;
            else if (typ == 2)
              xcorrBias = 0.12;
            else if (typ == 3)
              xcorrBias = -0.12;
          }
          break;
        default:
          if (typ < 0)
            break;
          else if (typ == 2)
            xcorr += 0.0813;
          else if (typ == 3) {
            xcorr -= 0.0813;
            typ = 2;
          }
          dx -= xcorr;
          hitF.RecenterXoffset(dx);
          xcorrBias = hitF.GetXcorr(dx, typ, 2) / fParams.padSizeX;
          break;
      }
    }
    else {
      if (typ) xcorrBias += (dx < 0 ? 1 : -1) * 0.0293;
    }
    std::tie(dx, dy) = hitF.CorrectPosition(dx, dy, xcorrBias, fParams.padSizeX, fParams.padSizeY);

    double edx(1), edy(1), edt(60), time(-21), tdrift(100), e(200);
    CalibrateHit(h, dx, dy, edx, edy, edt, time, tdrift, e, hitF);

    return kTRUE;
  }

  //_______________________________________________________________________________
  void HitMerger2D::CalibrateHit(Hit* h, const double dx, const double dy, const double edx, const double edy,
                                 const double edt, const double time, const double tdrift, const double eloss,
                                 const HitFactory2D& hitF)
  {
    const ROOT::Math::XYZVector& local_pad = fParams.rowPar[hitF.vrM].padPar[hitF.vcM].pos;
    const ROOT::Math::XYZVector local(local_pad.X() + dx, local_pad.Y() + dy, local_pad.Z());
    const ROOT::Math::XYZVector global = fParams.translation + fParams.rotation(local);
    h->SetX(global.X());
    h->SetY(global.Y());
    h->SetZ(global.Z());
    h->SetDx(edx);
    h->SetDy(edy);
    h->SetDz(0.);
    h->SetDxy(0.);
    h->SetTime(fClk * (hitF.vt0 + time) - tdrift + 30.29 + fHitTimeOff, edt);
    h->SetELoss(eloss);
    h->SetClassType();
    h->SetMaxType(hitF.IsMaxTilt());
    h->SetOverFlow(hitF.HasOvf());
  }


  //_______________________________________________________________________________
  std::pair<int, HitFactory2D> HitMerger2D::ProjectDigis(std::vector<DigiRec>* cid, std::vector<DigiRec>* cjd)
  {
    /** Load digis information in working vectors Digis are represented in the normal coordinate system of
   * (pad width [pw], DAQ time [clk], signal [ADC chs]) with rectangular signals at integer
   * positions.
   */

    if (cid->empty()) {
      L_(debug) << "TrdModuleRec2D::ProjectDigis : Request cl id " << cid << " not found.";
      return std::make_pair(0, HitFactory2D(0));
    }
    HitFactory2D hitF(fParams.rowPar[0].padPar.size());

    std::vector<HitFactory2D::signal>& hitSig = hitF.fSignal;
    hitF.reset();

    bool on(0);              // flag signal transmition
    int n0(0), nsr(0),       // no of signals in the cluster (all/rect)
      NR(0),                 // no of rect signals/channel in case of RC
      NT(0),                 // no of tilt signals/channel in case of RC
      ovf(1);                // over-flow flag for at least one of the digis
    Char_t dt0(0);           // cluster time offset wrt arbitrary t0
    double err,              // final error parametrization for signal
      xc(-0.5),              // running signal-pad position
      max(0.);               // max signal
    const double re = 100.;  // rect signal
    const double te = 100.;  // tilt signal

    int j(0), col(-1), col0(0), col1(0), step(0), row1;

    // check integrity of input
    if (cid == nullptr || cjd == nullptr || cid->empty() || cjd->empty()) {
      L_(debug) << "TrdModuleRec2D::ProjectDigis : Requested cluster not found.";
      return std::make_pair(0, HitFactory2D(0));
    }

    vector<DigiRec>::const_iterator i1 = cjd->begin();

    for (auto i = cid->begin(); i != cid->end(); i++, j++) {
      const DigiRec* dg  = &(*i);
      const DigiRec* dg0 = nullptr;

      //  initialize
      if (col < 0) {
        hitF.vrM = GetPadRowCol(dg->GetAddressChannel(), col);
        hitF.vt0 = dg->GetTimeDAQ();  // set arbitrary t0 to avoid double digis loop
      }
      GetPadRowCol(dg->GetAddressChannel(), col0);
      int nt = 0;  // no of tilt signals/channel in case of RC
      int nr = 0;  // no of rect signals/channel in case of RC

      // read calibrated signals
      double t = dg->GetTiltCharge(on);
      if (on) nt = 1;
      double r = dg->GetRectCharge(on);
      if (on) nr = 1;
      // look for matching neighbor digis in case of pad row cross
      if ((dg0 = (i1 != cjd->end()) ? &(*i1) : nullptr)) {
        row1 = GetPadRowCol(dg0->GetAddressChannel(), col1);
        if (!step) step = hitF.vrM - row1;
        if (col1 == col0) {
          r += dg0->GetRectCharge(on);
          if (on) nr++;
        }
        else
          dg0 = nullptr;
      }
      if (step == 1 && i1 != cjd->begin()) {
        const auto dg1 = &(*(i1 - 1));
        GetPadRowCol(dg1->GetAddressChannel(), col1);
        if (col1 == col0 - 1) {
          t += dg1->GetTiltCharge(on);
          if (on) nt++;
        }
      }
      if (step == -1 && i1 != cjd->end() && i1 + 1 != cjd->end()) {
        const auto dg1 = &(*(i1 + 1));
        GetPadRowCol(dg1->GetAddressChannel(), col1);
        if (col1 == col0 + 1) {
          t += dg1->GetTiltCharge(on);
          if (on) nt++;
        }
      }
      if (dg0) i1++;

      //TO DO: two blocks below might be mergable
      // process tilt signal/time
      char ddt = dg->GetTiltTime() - hitF.vt0;  // signal time offset wrt prompt
      if (ddt < dt0) dt0 = ddt;
      if (abs(t) > 0) {
        if (nt > 1) t *= 0.5;
        err = te * (nt > 1 ? 0.707 : 1);
        if (dg->HasTiltOvf()) {
          ovf = -1;
          err = 150.;
        }
        if (t > max) {
          max      = t;
          hitF.vcM = j;
          hitF.SetMaxTilt(1);
          hitF.viM = hitSig.size();
        }
      }
      else
        err = 300.;

      hitSig.emplace_back(t, err, ddt, xc, 0.035);
      xc += 0.5;

      // process rect signal/time
      ddt = dg->GetRectTime() - hitF.vt0;  // signal time offset wrt prompt
      if (ddt < dt0) dt0 = ddt;
      if (abs(r) > 0) {
        nsr++;
        if (nr > 1) r *= 0.5;
        err = re * (nr > 1 ? 0.707 : 1);
        if (dg->HasRectOvf()) {
          ovf = -1;
          err = 150.;
        }
        if (r > max) {
          max      = r;
          hitF.vcM = j;
          hitF.SetMaxTilt(0);
          hitF.viM = hitSig.size();
        }
      }
      else
        err = 300.;

      hitSig.emplace_back(r, err, ddt, xc, 0.);
      xc += 0.5;
      NR += nr;
      NT += nt;
    }  // for (auto i = cid->begin(); i != cid->end(); i++, j++)


    //TO DO: two blocks below might be mergable
    // add front and back anchor points if needed
    if (std::abs(hitSig[0].s) > 1.e-3) {
      xc       = hitSig[0].x;
      char ddt = hitSig[0].t;
      hitSig.emplace(hitSig.begin(), 0., 300., ddt, xc - 0.5, 0.);
      hitF.viM++;
    }
    int n(hitSig.size() - 1);
    if (std::abs(hitSig[n].s) > 1.e-3) {
      xc       = hitSig[n].x + 0.5;
      char ddt = hitSig[n].t;
      hitSig.emplace_back(0., 300., ddt, xc, 0.035);
    }

    n0 = hitSig.size() - 2;
    // compute cluster asymmetry
    int nR = n0 + 1 - hitF.viM;
    if (nR == hitF.viM) {
      hitF.SetSymmHit(1);
      if (hitSig.size() % 2) {
        double LS(0.), RS(0.);
        for (UChar_t idx(0); idx < hitF.viM; idx++)
          LS += hitSig[idx].s;
        for (uint idx(hitF.viM + 1); idx < hitSig.size(); idx++)
          RS += hitSig[idx].s;
        hitF.SetLeftSgn(LS < RS ? 0 : 1);
      }
    }
    else {
      hitF.SetSymmHit(0);
      if (hitF.viM < nR)
        hitF.SetLeftHit(0);
      else if (hitF.viM > nR)
        hitF.SetLeftHit(1);
    }
    // recenter time and space profile
    hitF.vt0 += dt0;
    for (auto& sig : hitSig) {
      sig.t -= dt0;
      sig.x -= hitF.vcM;
    }
    hitF.vcM += col;

    // check if all signals have same significance
    const int nmissX = 2 * nsr - NR;
    if (nmissX) {
      hitF.SetBiasX(1);
      for (UChar_t idx(1); idx < hitF.viM; idx++) {
        if (hitSig[idx].xe > 0.) continue;  //  select rect signals
        if (hitSig[idx].se > re * 0.8) hitF.SetBiasXleft(1);
      }
      if (hitSig[hitF.viM].xe <= 0. && hitSig[hitF.viM].se > re * 0.8) hitF.SetBiasXmid(1);
      for (UChar_t idx(hitF.viM + 1); idx < hitSig.size() - 1; idx++) {
        if (hitSig[idx].xe > 0.) continue;  //  select rect signals
        if (hitSig[idx].se > re * 0.8) hitF.SetBiasXright(1);
      }
    }
    else {
      hitF.SetBiasX(0);
    }

    const int nmissY = 2 * n0 - 2 * nsr - NT;
    if (nmissY) {
      hitF.SetBiasY();
      for (UChar_t idx(1); idx < hitF.viM; idx++) {
        if (hitSig[idx].xe > 0. && hitSig[idx].se > te * 0.8) hitF.SetBiasYleft(1);  //  select tilt signals
      }
      if (hitSig[hitF.viM].xe > 0. && hitSig[hitF.viM].se > te * 0.8) hitF.SetBiasYmid(1);
      for (UChar_t idx(hitF.viM + 1); idx < hitSig.size() - 1; idx++) {
        if (hitSig[idx].xe > 0. && hitSig[idx].se > te * 0.8) hitF.SetBiasYright(1);  //  select tilt signals
      }
    }
    else {
      hitF.SetBiasY(0);
    }

    if (ovf < 0) hitF.SetOvf();
    return std::make_pair(ovf * (hitSig.size() - 2), hitF);
  }

  int HitMerger2D::GetPadRowCol(int address, int& c)
  {
    c = address % fParams.rowPar[0].padPar.size();
    return address / fParams.rowPar[0].padPar.size();
  }

}  // namespace cbm::algo::trd
