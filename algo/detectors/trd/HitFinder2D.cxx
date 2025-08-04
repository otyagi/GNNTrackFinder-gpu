/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Alexandru Bercuci */

#include "HitFinder2D.h"

#include "AlgoFairloggerCompat.h"

#include <numeric>

using std::endl;
using std::vector;

namespace cbm::algo::trd
{

  //////////// TO DO: fdEdep is not used!!! Will be needed at some point.
  double HitFinder2D::fgDT[] = {4.181e-6, 1586, 24};

  //_______________________________________________________________________________
  HitFinder2D::HitFinder2D(HitFinder2DModPar params) : fParams(params) {}

  //_______________________________________________________________________________
  std::vector<trd::HitFinder2D::resultType> HitFinder2D::operator()(std::vector<Cluster2D>* clusters)
  {
    const int nclusters = clusters->size();
    std::vector<resultType> hitData;

    for (int icluster = 0; icluster < nclusters; icluster++) {
      auto& cluster = clusters->at(icluster);
      auto hit      = MakeHit(icluster, &cluster);
      if (hit.Address() == -1) continue;
      hitData.emplace_back(hit, cluster.GetRecDigis());
    }
    return hitData;
  }

  //_______________________________________________________________________________
  Hit HitFinder2D::MakeHit(int ic, const Cluster2D* cluster)
  {
    Hit hit;
    if (cluster->GetRecDigis().size() == 0) return hit;

    auto [hitFlag, hitF] = ProjectDigis(cluster);
    if (!hitFlag) return hit;

    hit.SetAddress(fParams.address);
    hit.SetRefId(ic);
    BuildHit(&hit, hitF);
    return hit;
  }


  //_______________________________________________________________________________
  bool HitFinder2D::BuildHit(Hit* h, HitFactory2D& hitF)
  {
    const int n0  = hitF.fSignal.size() - 2;
    auto [dx, dy] = hitF.GetDxDy(n0);

    // get position correction
    const double xcorr = hitF.GetXcorr(dx, hitF.GetHitClass()) / fParams.padSizeX;
    std::tie(dx, dy)   = hitF.CorrectPosition(dx, dy, xcorr, fParams.padSizeX, fParams.padSizeY);

    // get anode wire offset
    for (int ia = 0; ia <= NANODE; ia++) {
      const float ya = -1.35 + ia * 0.3;  //anode position in local pad coordinates
      if (dy > ya + 0.15) continue;
      break;
    }

    // Error parametrization X : parabolic model on cl size
    const double parX[] = {0.713724, -0.318667, 0.0366036};
    const double parY[] = {0.0886413, 0., 0.0435141};
    const int nex       = std::min(n0, 7);
    const double edx    = (n0 < 3) ? 1. : parX[0] + parX[1] * nex + parX[2] * nex * nex;
    const double edy    = (n0 < 3) ? 1. : parY[0] + parY[2] * dy * dy;
    const double edt    = (n0 < 3) ? 60. : 26.33;  // should be parametrized as function of da TODO

    // COMPUTE TIME
    double t_avg = 0.;
    for (int idx = 1; idx <= n0; idx++) {
      HitFactory2D::signal& sig = hitF.fSignal[idx];
      const double dtFEE        = fgDT[0] * (sig.s - fgDT[1]) * (sig.s - fgDT[1]) / fClk;
      t_avg += (sig.t - dtFEE);
      if (sig.xe > 0) sig.x += dy / fParams.padSizeY;
    }

    const double time   = (n0 > 1) ? t_avg / n0 : -21.;
    const double tdrift = 100.;  // should depend on Ua

    // COMPUTE ENERGY (conversion to GeV)
    const double e_avg = 1.e-9
                         * std::accumulate(hitF.fSignal.begin(), hitF.fSignal.end(), 0,
                                           [](int sum, const auto& sig) { return sum + sig.s; });

    CalibrateHit(h, dx, dy, edx, edy, edt, time, tdrift, e_avg, hitF);
    return kTRUE;
  }

  //_______________________________________________________________________________
  void HitFinder2D::CalibrateHit(Hit* h, const double dx, const double dy, const double edx, const double edy,
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
  std::pair<int, HitFactory2D> HitFinder2D::ProjectDigis(const Cluster2D* cluster)
  {
    /** Load digis information in working vectors Digis are represented in the normal coordinate system of
   * (pad width [pw], DAQ time [clk], signal [ADC chs]) with rectangular signals at integer
   * positions.
   */

    if (cluster->GetRecDigis().empty()) {
      L_(debug) << "TrdModuleRec2D::ProjectDigis : Requested cluster not found.";
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

    int j(0), col(-1), col0(0);

    for (auto i = cluster->GetRecDigis().begin(); i != cluster->GetRecDigis().end(); i++, j++) {
      const DigiRec* dg = &(*i);

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
    }

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

    hitF.SetBiasX(0);
    hitF.SetBiasY(0);

    if (ovf < 0) hitF.SetOvf();
    return std::make_pair(ovf * (hitSig.size() - 2), hitF);
  }


  int HitFinder2D::GetPadRowCol(int address, int& c)
  {
    c = address % fParams.rowPar[0].padPar.size();
    return address / fParams.rowPar[0].padPar.size();
  }

}  // namespace cbm::algo::trd
