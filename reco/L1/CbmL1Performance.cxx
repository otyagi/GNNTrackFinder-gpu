/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel,  Sergey Gorbunov [committer], Igor Kulakov, Valentina Akishina, Grigory Kozlov */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction
 *
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de
 *
 *====================================================================
 *
 *  L1 Fit performance
 *
 *====================================================================
 */

#include "CaFramework.h"
#include "CaToolsDebugger.h"
#include "CbmL1.h"
#include "CbmL1Constants.h"
#include "CbmL1Counters.h"
#include "CbmMatch.h"
#include "CbmMuchPixelHit.h"
#include "CbmMuchPoint.h"
#include "CbmQaTable.h"
#include "CbmStsDigi.h"
#include "CbmStsHit.h"
#include "CbmStsPoint.h"
#include "CbmStsSetup.h"
#include "CbmStsStation.h"
#include "CbmTofHit.h"
#include "CbmTofPoint.h"
#include "CbmTrdHit.h"
#include "CbmTrdPoint.h"
#include "FairField.h"
#include "FairRunAna.h"
#include "FairTrackParam.h"  // for vertex pulls
#include "KfTrackKalmanFilter.h"
#include "KfTrackParam.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TMath.h"
#include "TNtuple.h"
#include "TProfile.h"

#include <Logger.h>

#include <boost/filesystem.hpp>

#include <cmath>
#include <list>
#include <map>
#include <sstream>
#include <vector>

using namespace cbm::algo::ca;
using namespace cbm::algo;

using cbm::algo::kf::TrackParamV;
using cbm::ca::tools::Debugger;
using std::map;
using std::vector;

struct TL1PerfEfficiencies : public TL1Efficiencies {

  TL1PerfEfficiencies()
    : TL1Efficiencies()
    , ratio_killed()
    , ratio_clone()
    , ratio_length()
    , ratio_fakes()
    , killed()
    , clone()
    , reco_length()
    , reco_fakes()
    , mc_length()
    , mc_length_hits()
  {
    // add total efficiency
    AddCounter("long_fast_prim", "LongRPrim efficiency");
    AddCounter("fast_prim", "FastPrim  efficiency");
    AddCounter("fast_sec", "FastSec   efficiency");
    AddCounter("fast", "Fastset   efficiency");
    AddCounter("total", "Allset    efficiency");
    AddCounter("slow_prim", "SlowPrim  efficiency");
    AddCounter("slow_sec", "SlowSec   efficiency");
    AddCounter("slow", "Slow      efficiency");
    AddCounter("d0", "D0        efficiency");
    AddCounter("short", "Short123s efficiency");
    AddCounter("shortPion", "Short123s pion   eff");
    AddCounter("shortProton", "Short123s proton eff");
    AddCounter("shortKaon", "Short123s kaon   eff");
    AddCounter("shortE", "Short123s e      eff");
    AddCounter("shortRest", "Short123s rest   eff");

    AddCounter("fast_sec_e", "FastSec E efficiency");
    AddCounter("fast_e", "Fastset E efficiency");
    AddCounter("total_e", "Allset  E efficiency");
    AddCounter("slow_sec_e", "SlowSec E efficiency");
    AddCounter("slow_e", "Slow    E efficiency");
  }

  virtual ~TL1PerfEfficiencies(){};

  virtual void AddCounter(const TString& shortname, const TString& name)
  {
    TL1Efficiencies::AddCounter(shortname, name);
    ratio_killed.AddCounter();
    ratio_clone.AddCounter();
    ratio_length.AddCounter();
    ratio_fakes.AddCounter();
    killed.AddCounter();
    clone.AddCounter();
    reco_length.AddCounter();
    reco_fakes.AddCounter();
    mc_length.AddCounter();
    mc_length_hits.AddCounter();
  };

  TL1PerfEfficiencies& operator+=(TL1PerfEfficiencies& a)
  {
    TL1Efficiencies::operator+=(a);
    killed += a.killed;
    clone += a.clone;
    reco_length += a.reco_length;
    reco_fakes += a.reco_fakes;
    mc_length += a.mc_length;
    mc_length_hits += a.mc_length_hits;
    return *this;
  };

  void CalcEff()
  {
    TL1Efficiencies::CalcEff();
    ratio_killed                      = killed / mc;
    ratio_clone                       = clone / mc;
    TL1TracksCatCounters<int> allReco = reco + clone;
    ratio_length                      = reco_length / allReco;
    ratio_fakes                       = reco_fakes / allReco;
  };

  void Inc(bool isReco, bool isKilled, double _ratio_length, double _ratio_fakes, int _nclones, int _mc_length,
           int _mc_length_hits, const TString& name)
  {
    TL1Efficiencies::Inc(isReco, name);

    const int index = indices[name];

    if (isKilled) killed.counters[index]++;
    reco_length.counters[index] += _ratio_length;
    reco_fakes.counters[index] += _ratio_fakes;
    clone.counters[index] += _nclones;
    mc_length.counters[index] += _mc_length;
    mc_length_hits.counters[index] += _mc_length_hits;
  };

  void PrintEff(bool ifPrintTableToLog = false, TDirectory* outDir = nullptr,
                const std::string& nameOfTable = "efficiency_table")
  {
    int NCounters = mc.GetNcounters();
    std::vector<std::string> rowNames(NCounters + 2);
    for (int iC = 0; iC < NCounters; ++iC) {
      rowNames[iC] = std::string(names[iC].Data());
    }
    rowNames[NCounters]               = "Ghost prob.";
    rowNames[NCounters + 1]           = "N ghosts";
    std::vector<std::string> colNames = {"Eff.",     "Killed", "Length",    "Fakes",    "Clones",
                                         "All Reco", "All MC", "MCl(hits)", "MCl(MCps)"};

    CbmQaTable* aTable = new CbmQaTable(nameOfTable.c_str(), "Track Efficiency", NCounters + 2, 9);
    aTable->SetColWidth(20);
    aTable->SetNamesOfRows(rowNames);
    aTable->SetNamesOfCols(colNames);
    for (int iC = 0; iC < NCounters; iC++) {
      aTable->SetCell(iC, 0, ratio_reco.counters[iC]);
      aTable->SetCell(iC, 1, ratio_killed.counters[iC]);
      aTable->SetCell(iC, 2, ratio_length.counters[iC]);
      aTable->SetCell(iC, 3, ratio_fakes.counters[iC]);
      aTable->SetCell(iC, 4, ratio_clone.counters[iC]);
      if (nEvents > 0) {
        aTable->SetCell(iC, 5, reco.counters[iC] / double(nEvents));
        aTable->SetCell(iC, 6, mc.counters[iC] / double(nEvents));
      }
      else {
        aTable->SetCell(iC, 5, -1.);
        aTable->SetCell(iC, 6, -1.);
      }
      if (mc.counters[iC] > 0) {
        aTable->SetCell(iC, 7, mc_length_hits.counters[iC] / double(mc.counters[iC]));
        aTable->SetCell(iC, 8, mc_length.counters[iC] / double(mc.counters[iC]));
      }
      else {
        aTable->SetCell(iC, 7, -1.);
        aTable->SetCell(iC, 8, -1.);
      }
    }
    aTable->SetCell(NCounters, 0, ratio_ghosts);
    aTable->SetCell(NCounters + 1, 0, ghosts);

    if (ifPrintTableToLog) {
      LOG(info) << *aTable;  // print a table to log
    }
    if (outDir != nullptr) {
      aTable->SetDirectory(outDir);
    }
    else {
      aTable->SetDirectory(nullptr);
      delete aTable;
    }
  };

  TL1TracksCatCounters<double> ratio_killed;
  TL1TracksCatCounters<double> ratio_clone;
  TL1TracksCatCounters<double> ratio_length;
  TL1TracksCatCounters<double> ratio_fakes;

  TL1TracksCatCounters<int> killed;
  TL1TracksCatCounters<int> clone;
  TL1TracksCatCounters<double> reco_length;
  TL1TracksCatCounters<double> reco_fakes;
  TL1TracksCatCounters<int> mc_length;
  TL1TracksCatCounters<int> mc_length_hits;
};


void CbmL1::EfficienciesPerformance(bool doFinish)
{
  static TL1PerfEfficiencies L1_NTRA;  // average efficiencies

  static int L1_NEVENTS   = 0;
  static double L1_CATIME = 0.0;

  if (doFinish) {
    L1_NTRA.CalcEff();
    L1_NTRA.PrintEff(false, fTableDir);
    return;
  }

  TL1PerfEfficiencies ntra;  // efficiencies for current event

  cbm::ca::tools::Debugger::Instance().AddNtuple("ghost", "it:ih:p:x:y:z:t:dx:dy");
  static int statNghost = 0;

  for (vector<CbmL1Track>::iterator rtraIt = fvRecoTracks.begin(); rtraIt != fvRecoTracks.end(); ++rtraIt) {
    ntra.ghosts += rtraIt->IsGhost();
    if (0 && rtraIt->IsGhost()) {  // Debug.
      TrackParamV tr(*rtraIt);
      std::stringstream ss;
      ss << " L1: ghost track: nhits " << rtraIt->GetNofHits() << " p " << 1. / rtraIt->GetQp() << " purity "
         << rtraIt->GetMaxPurity() << " | hits ";
      for (map<int, int>::iterator posIt = rtraIt->hitMap.begin(); posIt != rtraIt->hitMap.end(); posIt++) {
        ss << " (" << posIt->second << " " << posIt->first << ") ";
      }
      LOG(info) << ss.str();
      for (map<int, int>::iterator posIt = rtraIt->hitMap.begin(); posIt != rtraIt->hitMap.end(); posIt++) {
        const auto& mcTrk = fMCData.GetTrack(posIt->first);
        LOG(info) << "mc " << posIt->first << " pdg " << mcTrk.GetPdgCode() << " mother: " << mcTrk.GetMotherId()
                  << " chain " << mcTrk.GetChainId() << " n mc stations: " << mcTrk.GetNofConsStationsWithPoint();
      }
      for (unsigned int i = 0; i < rtraIt->Hits.size(); i++) {
        const ca::Hit& h           = fpAlgo->GetInputData().GetHit(rtraIt->Hits[i]);
        const CbmL1HitDebugInfo& s = fvHitDebugInfo[rtraIt->Hits[i]];
        LOG(info) << " x y z t " << s.x << " " << s.y << " " << h.Z() << " dx " << s.dx << " dy " << s.dy;
        cbm::ca::tools::Debugger::Instance().FillNtuple("ghost", statNghost, i, fabs(1. / tr.GetQp()[0]), s.x, s.y,
                                                        h.Z(), h.T(), s.dx, s.dy);
      }
      LOG(info) << tr.ToString(0);
      statNghost++;
    }
  }

  int sta_nhits[fpAlgo->GetParameters().GetNstationsActive()];
  int sta_nfakes[fpAlgo->GetParameters().GetNstationsActive()];

  for (int i = 0; i < fpAlgo->GetParameters().GetNstationsActive(); i++) {
    sta_nhits[i]  = 0;
    sta_nfakes[i] = 0;
  }

  for (const auto& mcTrk : fMCData.GetTrackContainer()) {
    //    if( !( mcTrk.GetPdgCode() == -11 && mcTrk.GetMotherId() == -1 ) ) continue; // electrons only
    if (!mcTrk.IsReconstructable() && !mcTrk.IsAdditional()) continue;

    // -- find used constans --
    // is track reconstructed
    const bool reco = (mcTrk.IsReconstructed());
    // is track killed. At least one hit of it belong to some recoTrack
    const bool killed = !reco && mcTrk.IsDisturbed();
    // ration length for current mcTrack
    double ratio_length   = 0;
    double ratio_fakes    = 0;
    double mc_length_hits = mcTrk.GetTotNofStationsWithHit();


    int mc_length = mcTrk.GetTotNofStationsWithPoint();
    if (reco) {
      for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
        const auto& rTrk = fvRecoTracks[irt];
        ratio_length += static_cast<double>(rTrk.GetNofHits()) * rTrk.GetMaxPurity() / mc_length_hits;
        ratio_fakes += 1 - rTrk.GetMaxPurity();
      }
    }


    // number of clones
    int nclones = 0;
    if (reco) nclones = mcTrk.GetNofClones();
    //     if (nclones){ // Debug. Look at clones
    //       for (int irt = 0; irt < rTracks.size(); irt++){
    //         const int ista = fvHitDebugInfo[rTracks[irt]->Hits[0]].iStation;
    //         LOG(info) << rTracks[irt]->GetNOfHits() << "(" << ista << ") ";
    //       }
    //       LOG(info) << mtra.NStations();
    //     }

    if (mcTrk.IsAdditional()) {  // short
      ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "short");
      switch (mcTrk.GetPdgCode()) {
        case 11:
        case -11:
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "shortE");
          break;
        case 211:
        case -211:
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "shortPion");
          break;
        case 321:
        case -321:
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "shortKaon");
          break;
        case 2212:
        case -2212:
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "shortProton");
          break;
        default: ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "shortRest");
      }
    }
    else {  // separate all efficiecies from short eff


      ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "total");

      if (mcTrk.IsSignal()) {  // D0
        ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "d0");
      }

      if (mcTrk.GetP() > CbmL1Constants::MinFastMom) {  // fast tracks
        ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "fast");

        if (mcTrk.IsPrimary()) {                                 // fast primary
          if (mcTrk.GetTotNofStationsWithHit() == fNStations) {  // long fast primary
            ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "long_fast_prim");
          }
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "fast_prim");
        }
        else {  // fast secondary
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "fast_sec");
        }
      }
      else {  // slow set of tracks
        ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "slow");

        if (mcTrk.IsPrimary()) {  // slow primary
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "slow_prim");
        }
        else {
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "slow_sec");
        }
      }  // if slow

      if (mcTrk.GetPdgCode() == 11 || mcTrk.GetPdgCode() == -11) {
        ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "total_e");

        if (mcTrk.GetP() > CbmL1Constants::MinFastMom) {  // fast tracks
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "fast_e");

          if (mcTrk.IsPrimary()) {  // fast primary
          }
          else {  // fast secondary
            ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "fast_sec_e");
          }
        }
        else {  // slow set of tracks
          ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "slow_e");

          if (mcTrk.IsPrimary()) {  // slow primary
          }
          else {
            ntra.Inc(reco, killed, ratio_length, ratio_fakes, nclones, mc_length, mc_length_hits, "slow_sec_e");
          }
        }  // if slow
      }
    }

  }  // for mcTracks

  L1_CATIME += fTrackingTime;
  L1_NEVENTS++;
  ntra.IncNEvents();
  L1_NTRA += ntra;

  ntra.CalcEff();
  L1_NTRA.CalcEff();

  if (fVerbose) {
    if (fVerbose > 1) {
      ntra.PrintEff(true);
      std::stringstream ss;
      ss << "Number of true and fake hits in stations: \n";
      for (int i = 0; i < fpAlgo->GetParameters().GetNstationsActive(); i++) {
        ss << sta_nhits[i] - sta_nfakes[i] << "+" << sta_nfakes[i] << "   ";
      }
      LOG(info) << ss.str();
    }  // fVerbose > 1
    LOG(info) << "\n"
              << "L1 ACCUMULATED STAT    : " << L1_NEVENTS << " EVENTS \n";
    L1_NTRA.PrintEff(true);
    LOG(info) << "Reconstructible MC tracks/event: "
              << (double(L1_NTRA.mc.counters[L1_NTRA.indices["total"]]) / double(L1_NEVENTS));
    LOG(info) << "Reconstructed MC tracks/event: "
              << (double(L1_NTRA.reco.counters[L1_NTRA.indices["total"]]) / double(L1_NEVENTS));
    LOG(info) << "\nCA Track Finder: " << L1_CATIME / L1_NEVENTS << " s/time slice \n";
  }
}  // void CbmL1::Performance()


void CbmL1::HistoPerformance()  // TODO: check if works correctly. Change vHitFast on match data in CbmL1**Track classes
{

  static TProfile *p_eff_all_vs_mom, *p_eff_prim_vs_mom, *p_eff_sec_vs_mom, *p_eff_d0_vs_mom, *p_eff_prim_vs_theta,
    *p_eff_all_vs_pt, *p_eff_prim_vs_pt, *p_eff_all_vs_nhits, *p_eff_prim_vs_nhits, *p_eff_sec_vs_nhits;

  static TH1F *h_reg_MCmom, *h_acc_MCmom, *h_reco_MCmom, *h_ghost_Rmom;
  static TH1F *h_reg_prim_MCmom, *h_acc_prim_MCmom, *h_reco_prim_MCmom;
  static TH1F *h_reg_sec_MCmom, *h_acc_sec_MCmom, *h_reco_sec_MCmom;

  static TH1F* h_acc_mom_short123s;

  static TH1F *h_reg_mom_prim, *h_reg_mom_sec, *h_reg_nhits_prim, *h_reg_nhits_sec;
  static TH1F *h_acc_mom_prim, *h_acc_mom_sec, *h_acc_nhits_prim, *h_acc_nhits_sec;
  static TH1F *h_reco_mom_prim, *h_reco_mom_sec, *h_reco_nhits_prim, *h_reco_nhits_sec;
  static TH1F *h_rest_mom_prim, *h_rest_mom_sec, *h_rest_nhits_prim, *h_rest_nhits_sec;

  //static TH1F *h_hit_density[10];

  static TH1F *h_ghost_purity, *h_ghost_mom, *h_ghost_nhits, *h_ghost_fstation, *h_ghost_chi2, *h_ghost_prob,
    *h_ghost_tx, *h_ghost_ty;
  static TH1F *h_reco_mom, *h_reco_d0_mom, *h_reco_nhits, *h_reco_station, *h_reco_chi2, *h_reco_prob, *h_rest_prob,
    *h_reco_purity, *h_reco_time;
  static TProfile *h_reco_timeNtr, *h_reco_timeNhit;
  static TProfile *h_reco_fakeNtr, *h_reco_fakeNhit;
  static TH1F *h_tx, *h_ty, *h_sec_r, *h_ghost_r;

  static TH1F *h_notfound_mom, *h_notfound_nhits, *h_notfound_station, *h_notfound_r, *h_notfound_tx, *h_notfound_ty;

  static TH1F *h_mcp, *h_nmchits;
  //  static TH1F *h_chi2, *h_prob, *MC_vx, *MC_vy, *MC_vz, *VtxChiPrim, *VtxChiSec;

  //  static TH2F *P_vs_P ;

  static TH2F *h2_vertex, *h2_prim_vertex, *h2_sec_vertex;
  //static TH3F *h3_vertex, *h3_prim_vertex, *h3_sec_vertex;

  static TH2F *h2_reg_nhits_vs_mom_prim, *h2_reg_nhits_vs_mom_sec, *h2_reg_fstation_vs_mom_prim,
    *h2_reg_fstation_vs_mom_sec, *h2_reg_lstation_vs_fstation_prim, *h2_reg_lstation_vs_fstation_sec;
  static TH2F *h2_acc_nhits_vs_mom_prim, *h2_acc_nhits_vs_mom_sec, *h2_acc_fstation_vs_mom_prim,
    *h2_acc_fstation_vs_mom_sec, *h2_acc_lstation_vs_fstation_prim, *h2_acc_lstation_vs_fstation_sec;
  static TH2F *h2_reco_nhits_vs_mom_prim, *h2_reco_nhits_vs_mom_sec, *h2_reco_fstation_vs_mom_prim,
    *h2_reco_fstation_vs_mom_sec, *h2_reco_lstation_vs_fstation_prim, *h2_reco_lstation_vs_fstation_sec;
  static TH2F *h2_ghost_nhits_vs_mom, *h2_ghost_fstation_vs_mom, *h2_ghost_lstation_vs_fstation;
  static TH2F *h2_rest_nhits_vs_mom_prim, *h2_rest_nhits_vs_mom_sec, *h2_rest_fstation_vs_mom_prim,
    *h2_rest_fstation_vs_mom_sec, *h2_rest_lstation_vs_fstation_prim, *h2_rest_lstation_vs_fstation_sec;

  static TH1F* h_reg_phi_prim;
  static TH1F* h_reg_phi_sec;
  static TH1F* h_acc_phi_prim;
  static TH1F* h_acc_phi_sec;
  static TH1F* h_reco_phi_prim;
  static TH1F* h_reco_phi_sec;
  static TH1F* h_rest_phi_prim;
  static TH1F* h_rest_phi_sec;
  static TH1F* h_ghost_phi;
  static TH1F* h_reco_phi;
  static TH1F* h_notfound_phi;

  static TH2F* h2_fst_hit_yz;  // occupancy of track first hit in y-z plane
  static TH2F* h2_lst_hit_yz;  // occupancy of track last hit in y-z plane
  static TH2F* h2_all_hit_yz;  // occupancy of track all hits in y-z plane

  static bool first_call = 1;

  if (first_call) {
    first_call = 0;

    TDirectory* curdir = gDirectory;
    gDirectory         = fHistoDir;

    h2_fst_hit_yz =
      new TH2F("h2_fst_hit_yz", "Track first hit occupancy in y-z plane;z [cm];y [cm]", 80, 0, 0, 80, 0, 0);
    h2_lst_hit_yz =
      new TH2F("h2_lst_hit_yz", "Track last hit occupancy in y-z plane;z[ cm];y [cm]", 80, 0, 0, 80, 0, 0);
    h2_all_hit_yz = new TH2F("h2_all_hit_yz", "Track hit occupancy in y-z plane;z[ cm];y [cm]", 80, 0, 0, 80, 0, 0);

    p_eff_all_vs_mom = new TProfile("p_eff_all_vs_mom", "AllSet Efficiency vs Momentum", 100, 0.0, 5.0, 0.0, 100.0);
    p_eff_prim_vs_mom =
      new TProfile("p_eff_prim_vs_mom", "Primary Set Efficiency vs Momentum", 100, 0.0, 5.0, 0.0, 100.0);
    p_eff_sec_vs_mom =
      new TProfile("p_eff_sec_vs_mom", "Secondary Set Efficiency vs Momentum", 100, 0.0, 5.0, 0.0, 100.0);
    p_eff_d0_vs_mom =
      new TProfile("p_eff_d0_vs_mom", "D0 Secondary Tracks Efficiency vs Momentum", 150, 0.0, 15.0, 0.0, 100.0);
    p_eff_prim_vs_theta =
      new TProfile("p_eff_prim_vs_theta", "All Primary Set Efficiency vs Theta", 100, 0.0, 30.0, 0.0, 100.0);
    p_eff_all_vs_pt  = new TProfile("p_eff_all_vs_pt", "AllSet Efficiency vs Pt", 100, 0.0, 5.0, 0.0, 100.0);
    p_eff_prim_vs_pt = new TProfile("p_eff_prim_vs_pt", "Primary Set Efficiency vs Pt", 100, 0.0, 5.0, 0.0, 100.0);

    p_eff_all_vs_nhits = new TProfile("p_eff_all_vs_nhits", "AllSet Efficiency vs NMCHits", 8, 3.0, 11.0, 0.0, 100.0);
    p_eff_prim_vs_nhits =
      new TProfile("p_eff_prim_vs_nhits", "PrimSet Efficiency vs NMCHits", 8, 3.0, 11.0, 0.0, 100.0);
    p_eff_sec_vs_nhits = new TProfile("p_eff_sec_vs_nhits", "SecSet Efficiency vs NMCHits", 8, 3.0, 11.0, 0.0, 100.0);

    h_reg_MCmom  = new TH1F("h_reg_MCmom", "Momentum of registered tracks", 100, 0.0, 5.0);
    h_acc_MCmom  = new TH1F("h_acc_MCmom", "Reconstructable tracks", 100, 0.0, 5.0);
    h_reco_MCmom = new TH1F("h_reco_MCmom", "Reconstructed tracks", 100, 0.0, 5.0);

    h_ghost_Rmom      = new TH1F("h_ghost_Rmom", "Ghost tracks", 100, 0.0, 5.0);
    h_reg_prim_MCmom  = new TH1F("h_reg_prim_MCmom", "Momentum of registered tracks", 100, 0.0, 5.0);
    h_acc_prim_MCmom  = new TH1F("h_acc_prim_MCmom", "Reconstructable tracks", 100, 0.0, 5.0);
    h_reco_prim_MCmom = new TH1F("h_reco_prim_MCmom", "Reconstructed tracks", 100, 0.0, 5.0);
    h_reg_sec_MCmom   = new TH1F("h_reg_sec_MCmom", "Momentum of registered tracks", 100, 0.0, 5.0);
    h_acc_sec_MCmom   = new TH1F("h_acc_sec_MCmom", "Reconstructable tracks", 100, 0.0, 5.0);
    h_reco_sec_MCmom  = new TH1F("h_reco_sec_MCmom", "Reconstructed tracks", 100, 0.0, 5.0);

    h_acc_mom_short123s =
      new TH1F("h_acc_mom_short123s", "Momentum of accepted tracks with 3 hits on first stations", 500, 0.0, 5.0);

    h_reg_mom_prim  = new TH1F("h_reg_mom_prim", "Momentum of registered primary tracks", 500, 0.0, 5.0);
    h_reg_mom_sec   = new TH1F("h_reg_mom_sec", "Momentum of registered secondary tracks", 500, 0.0, 5.0);
    h_acc_mom_prim  = new TH1F("h_acc_mom_prim", "Momentum of accepted primary tracks", 500, 0.0, 5.0);
    h_acc_mom_sec   = new TH1F("h_acc_mom_sec", "Momentum of accepted secondary tracks", 500, 0.0, 5.0);
    h_reco_mom_prim = new TH1F("h_reco_mom_prim", "Momentum of reconstructed primary tracks", 500, 0.0, 5.0);
    h_reco_mom_sec  = new TH1F("h_reco_mom_sec", "Momentum of reconstructed secondary tracks", 500, 0.0, 5.0);
    h_rest_mom_prim = new TH1F("h_rest_mom_prim", "Momentum of not found primary tracks", 500, 0.0, 5.0);
    h_rest_mom_sec  = new TH1F("h_rest_mom_sec", "Momentum of not found secondary tracks", 500, 0.0, 5.0);

    h_reg_phi_prim  = new TH1F("h_reg_phi_prim", "Azimuthal angle of registered primary tracks", 1000, -3.15, 3.15);
    h_reg_phi_sec   = new TH1F("h_reg_phi_sec", "Azimuthal angle of registered secondary tracks", 1000, -3.15, 3.15);
    h_acc_phi_prim  = new TH1F("h_acc_phi_prim", "Azimuthal angle of accepted primary tracks", 1000, -3.15, 3.15);
    h_acc_phi_sec   = new TH1F("h_acc_phi_sec", "Azimuthal angle of accepted secondary tracks", 1000, -3.15, 3.15);
    h_reco_phi_prim = new TH1F("h_reco_phi_prim", "Azimuthal angle of reconstructed primary tracks", 1000, -3.15, 3.15);
    h_reco_phi_sec = new TH1F("h_reco_phi_sec", "Azimuthal angle of reconstructed secondary tracks", 1000, -3.15, 3.15);
    h_rest_phi_prim = new TH1F("h_rest_phi_prim", "Azimuthal angle of not found primary tracks", 1000, -3.15, 3.15);
    h_rest_phi_sec  = new TH1F("h_rest_phi_sec", "Azimuthal angle of not found secondary tracks", 1000, -3.15, 3.15);

    h_reg_nhits_prim  = new TH1F("h_reg_nhits_prim", "Number of hits in registered primary track", 51, -0.1, 10.1);
    h_reg_nhits_sec   = new TH1F("h_reg_nhits_sec", "Number of hits in registered secondary track", 51, -0.1, 10.1);
    h_acc_nhits_prim  = new TH1F("h_acc_nhits_prim", "Number of hits in accepted primary track", 51, -0.1, 10.1);
    h_acc_nhits_sec   = new TH1F("h_acc_nhits_sec", "Number of hits in accepted secondary track", 51, -0.1, 10.1);
    h_reco_nhits_prim = new TH1F("h_reco_nhits_prim", "Number of hits in reconstructed primary track", 51, -0.1, 10.1);
    h_reco_nhits_sec  = new TH1F("h_reco_nhits_sec", "Number of hits in reconstructed secondary track", 51, -0.1, 10.1);
    h_rest_nhits_prim = new TH1F("h_rest_nhits_prim", "Number of hits in not found primary track", 51, -0.1, 10.1);
    h_rest_nhits_sec  = new TH1F("h_rest_nhits_sec", "Number of hits in not found secondary track", 51, -0.1, 10.1);

    h_ghost_mom      = new TH1F("h_ghost_mom", "Momentum of ghost tracks", 500, 0.0, 5.0);
    h_ghost_phi      = new TH1F("h_ghost_phi", "Azimuthal angle of ghost tracks", 1000, -3.15, 3.15);
    h_ghost_nhits    = new TH1F("h_ghost_nhits", "Number of hits in ghost track", 51, -0.1, 10.1);
    h_ghost_fstation = new TH1F("h_ghost_fstation", "First station of ghost track", 50, -0.5, 10.0);
    h_ghost_chi2     = new TH1F("h_ghost_chi2", "Chi2/NDF of ghost track", 50, -0.5, 10.0);
    h_ghost_prob     = new TH1F("h_ghost_prob", "Prob of ghost track", 505, 0., 1.01);
    h_ghost_r        = new TH1F("h_ghost_r", "R of ghost track at the first hit", 50, 0.0, 150.0);
    h_ghost_tx       = new TH1F("h_ghost_tx", "tx of ghost track at the first hit", 50, -5.0, 5.0);
    h_ghost_ty       = new TH1F("h_ghost_ty", "ty of ghost track at the first hit", 50, -1.0, 1.0);
    h_ghost_purity   = new TH1F("h_ghost_purity", "Ghost: percentage of correct hits", 100, -0.5, 100.5);

    h_reco_mom      = new TH1F("h_reco_mom", "Momentum of reco track", 50, 0.0, 5.0);
    h_reco_phi      = new TH1F("h_reco_phi", "Azimuthal angle of reco track", 1000, -3.15, 3.15);
    h_reco_nhits    = new TH1F("h_reco_nhits", "Number of hits in reco track", 50, 0.0, 10.0);
    h_reco_station  = new TH1F("h_reco_station", "First station of reco track", 50, -0.5, 10.0);
    h_reco_chi2     = new TH1F("h_reco_chi2", "Chi2/NDF of reco track", 50, -0.5, 10.0);
    h_reco_prob     = new TH1F("h_reco_prob", "Prob of reco track", 505, 0., 1.01);
    h_rest_prob     = new TH1F("h_rest_prob", "Prob of reco rest track", 505, 0., 1.01);
    h_reco_purity   = new TH1F("h_reco_purity", "Percentage of correct hits", 100, -0.5, 100.5);
    h_reco_time     = new TH1F("h_reco_time", "CA Track Finder Time (s/ev)", 20, 0.0, 20.0);
    h_reco_timeNtr  = new TProfile("h_reco_timeNtr", "CA Track Finder Time (s/ev) vs N Tracks", 200, 0.0, 1000.0);
    h_reco_timeNhit = new TProfile("h_reco_timeNhit", "CA Track Finder Time (s/ev) vs N Hits", 200, 0.0, 30000.0);
    h_reco_fakeNtr  = new TProfile("h_reco_fakeNtr", "N Fake hits vs N Tracks", 200, 0.0, 1000.0);
    h_reco_fakeNhit = new TProfile("h_reco_fakeNhit", "N Fake hits vs N Hits", 200, 0.0, 30000.0);

    h_reco_d0_mom = new TH1F("h_reco_d0_mom", "Momentum of reco D0 track", 150, 0.0, 15.0);

    //     h_hit_density[0] = new TH1F("h_hit_density0", "Hit density station 1", 50, 0.0,  5.00);
    //     h_hit_density[1] = new TH1F("h_hit_density1", "Hit density station 2", 100, 0.0, 10.00);
    //     h_hit_density[2] = new TH1F("h_hit_density2", "Hit density station 3", 140, 0.0, 13.99);
    //     h_hit_density[3] = new TH1F("h_hit_density3", "Hit density station 4", 180, 0.0, 18.65);
    //     h_hit_density[4] = new TH1F("h_hit_density4", "Hit density station 5", 230, 0.0, 23.32);
    //     h_hit_density[5] = new TH1F("h_hit_density5", "Hit density station 6", 270, 0.0, 27.98);
    //     h_hit_density[6] = new TH1F("h_hit_density6", "Hit density station 7", 340, 0.0, 34.97);
    //     h_hit_density[7] = new TH1F("h_hit_density7", "Hit density station 8", 460, 0.0, 46.63);
    //     h_hit_density[8] = new TH1F("h_hit_density8", "Hit density station 9", 500, 0.0, 50.00);
    //     h_hit_density[9] = new TH1F("h_hit_density8", "Hit density station 9", 500, 0.0, 50.00);
    //     h_hit_density[10] = new TH1F("h_hit_density8", "Hit density station 9", 500, 0.0, 50.00);

    h_tx = new TH1F("h_tx", "tx of MC track at the vertex", 50, -0.5, 0.5);
    h_ty = new TH1F("h_ty", "ty of MC track at the vertex", 50, -0.5, 0.5);

    h_sec_r = new TH1F("h_sec_r", "R of sec MC track at the first hit", 50, 0.0, 15.0);

    h_notfound_mom     = new TH1F("h_notfound_mom", "Momentum of not found track", 50, 0.0, 5.0);
    h_notfound_phi     = new TH1F("h_notfound_phi", "Momentum of not found track", 50, 0.0, 5.0);
    h_notfound_nhits   = new TH1F("h_notfound_nhits", "Num hits of not found track", 50, 0.0, 10.0);
    h_notfound_station = new TH1F("h_notfound_station", "First station of not found track", 50, 0.0, 10.0);
    h_notfound_r       = new TH1F("h_notfound_r", "R at first hit of not found track", 50, 0.0, 15.0);
    h_notfound_tx      = new TH1F("h_notfound_tx", "tx of not found track at the first hit", 50, -5.0, 5.0);
    h_notfound_ty      = new TH1F("h_notfound_ty", "ty of not found track at the first hit", 50, -1.0, 1.0);

    /*
    h_chi2 = new TH1F("chi2", "Chi^2", 100, 0.0, 10.);
    h_prob = new TH1F("prob", "Prob", 100, 0.0, 1.01);
    VtxChiPrim = new TH1F("vtxChiPrim", "Chi to primary vtx for primary tracks", 100, 0.0, 10.);
    VtxChiSec = new TH1F("vtxChiSec", "Chi to primary vtx for secondary tracks", 100, 0.0, 10.);
*/
    h_mcp = new TH1F("h_mcp", "MC P ", 500, 0.0, 5.0);
    /*
    MC_vx = new TH1F("MC_vx", "MC Vertex X", 100, -.05, .05);
    MC_vy = new TH1F("MC_vy", "MC Vertex Y", 100, -.05, .05);
    MC_vz = new TH1F("MC_vz", "MC Vertex Z", 100, -.05, .05);
*/
    h_nmchits = new TH1F("h_nmchits", "N STS hits on MC track", 50, 0.0, 10.0);

    //    P_vs_P = new TH2F("P_vs_P", "Resolution P/Q vs P", 20, 0., 20.,100, -.05, .05);

    h2_vertex      = new TH2F("h2_vertex", "2D vertex distribution", 105, -5., 100., 100, -50., 50.);
    h2_prim_vertex = new TH2F("h2_primvertex", "2D primary vertex distribution", 105, -5., 100., 100, -50., 50.);
    h2_sec_vertex  = new TH2F("h2_sec_vertex", "2D secondary vertex distribution", 105, -5., 100., 100, -50., 50.);

    //h3_vertex = new TH3F("h3_vertex", "3D vertex distribution", 20, -5., 100., 100, -50., 50., 100, -50., 50.);
    //h3_prim_vertex = new TH3F("h3_primvertex", "3D vertex distribution", 20, -5., 100., 100, -50., 50., 100, -50., 50.);
    //h3_sec_vertex = new TH3F("h3_sec_vertex", "3D vertex distribution", 20, -5., 100., 100, -50., 50., 100, -50., 50.);

    h2_reg_nhits_vs_mom_prim =
      new TH2F("h2_reg_nhits_vs_mom_prim", "NHits vs. Momentum (Reg. Primary Tracks)", 51, -0.05, 5.05, 11, -0.5, 10.5);
    h2_reg_nhits_vs_mom_sec = new TH2F("h2_reg_nhits_vs_mom_sec", "NHits vs. Momentum (Reg. Secondary Tracks)", 51,
                                       -0.05, 5.05, 11, -0.5, 10.5);
    h2_acc_nhits_vs_mom_prim =
      new TH2F("h2_acc_nhits_vs_mom_prim", "NHits vs. Momentum (Acc. Primary Tracks)", 51, -0.05, 5.05, 11, -0.5, 10.5);
    h2_acc_nhits_vs_mom_sec   = new TH2F("h2_acc_nhits_vs_mom_sec", "NHits vs. Momentum (Acc. Secondary Tracks)", 51,
                                       -0.05, 5.05, 11, -0.5, 10.5);
    h2_reco_nhits_vs_mom_prim = new TH2F("h2_reco_nhits_vs_mom_prim", "NHits vs. Momentum (Reco Primary Tracks)", 51,
                                         -0.05, 5.05, 11, -0.5, 10.5);
    h2_reco_nhits_vs_mom_sec  = new TH2F("h2_reco_nhits_vs_mom_sec", "NHits vs. Momentum (Reco Secondary Tracks)", 51,
                                        -0.05, 5.05, 11, -0.5, 10.5);
    h2_ghost_nhits_vs_mom =
      new TH2F("h2_ghost_nhits_vs_mom", "NHits vs. Momentum (Ghost Tracks)", 51, -0.05, 5.05, 11, -0.5, 10.5);
    h2_rest_nhits_vs_mom_prim = new TH2F("h2_rest_nhits_vs_mom_prim", "NHits vs. Momentum (!Found Primary Tracks)", 51,
                                         -0.05, 5.05, 11, -0.5, 10.5);
    h2_rest_nhits_vs_mom_sec  = new TH2F("h2_rest_nhits_vs_mom_sec", "NHits vs. Momentum (!Found Secondary Tracks)", 51,
                                        -0.05, 5.05, 11, -0.5, 10.5);

    h2_reg_fstation_vs_mom_prim =
      new TH2F("h2_reg_fstation_vs_mom_prim", "First Station vs. Momentum (Reg. Primary Tracks)", 51, -0.05, 5.05, 11,
               -0.5, 10.5);
    h2_reg_fstation_vs_mom_sec =
      new TH2F("h2_reg_fstation_vs_mom_sec", "First Station vs. Momentum (Reg. Secondary Tracks)", 51, -0.05, 5.05, 11,
               -0.5, 10.5);
    h2_acc_fstation_vs_mom_prim =
      new TH2F("h2_acc_fstation_vs_mom_prim", "First Station vs. Momentum (Acc. Primary Tracks)", 51, -0.05, 5.05, 11,
               -0.5, 10.5);
    h2_acc_fstation_vs_mom_sec =
      new TH2F("h2_acc_fstation_vs_mom_sec", "First Station vs. Momentum (Acc. Secondary Tracks)", 51, -0.05, 5.05, 11,
               -0.5, 10.5);
    h2_reco_fstation_vs_mom_prim =
      new TH2F("h2_reco_fstation_vs_mom_prim", "First Station vs. Momentum (Reco Primary Tracks)", 51, -0.05, 5.05, 11,
               -0.5, 10.5);
    h2_reco_fstation_vs_mom_sec =
      new TH2F("h2_reco_fstation_vs_mom_sec", "First Station vs. Momentum (Reco Secondary Tracks)", 51, -0.05, 5.05, 11,
               -0.5, 10.5);
    h2_ghost_fstation_vs_mom = new TH2F("h2_ghost_fstation_vs_mom", "First Station vs. Momentum (Ghost Tracks)", 51,
                                        -0.05, 5.05, 11, -0.5, 10.5);
    h2_rest_fstation_vs_mom_prim =
      new TH2F("h2_rest_fstation_vs_mom_prim", "First Station vs. Momentum (!Found Primary Tracks)", 51, -0.05, 5.05,
               11, -0.5, 10.5);
    h2_rest_fstation_vs_mom_sec =
      new TH2F("h2_rest_fstation_vs_mom_sec", "First Station vs. Momentum (!Found Secondary Tracks)", 51, -0.05, 5.05,
               11, -0.5, 10.5);

    h2_reg_lstation_vs_fstation_prim =
      new TH2F("h2_reg_lstation_vs_fstation_prim", "Last vs. First Station (Reg. Primary Tracks)", 11, -0.5, 10.5, 11,
               -0.5, 10.5);
    h2_reg_lstation_vs_fstation_sec =
      new TH2F("h2_reg_lstation_vs_fstation_sec", "Last vs. First Station (Reg. Secondary Tracks)", 11, -0.5, 10.5, 11,
               -0.5, 10.5);
    h2_acc_lstation_vs_fstation_prim =
      new TH2F("h2_acc_lstation_vs_fstation_prim", "Last vs. First Station (Acc. Primary Tracks)", 11, -0.5, 10.5, 11,
               -0.5, 10.5);
    h2_acc_lstation_vs_fstation_sec =
      new TH2F("h2_acc_lstation_vs_fstation_sec", "Last vs. First Station (Acc. Secondary Tracks)", 11, -0.5, 10.5, 11,
               -0.5, 10.5);
    h2_reco_lstation_vs_fstation_prim =
      new TH2F("h2_reco_lstation_vs_fstation_prim", "Last vs. First Station (Reco Primary Tracks)", 11, -0.5, 10.5, 11,
               -0.5, 10.5);
    h2_reco_lstation_vs_fstation_sec =
      new TH2F("h2_reco_lstation_vs_fstation_sec", "Last vs. First Station (Reco Secondary Tracks)", 11, -0.5, 10.5, 11,
               -0.5, 10.5);
    h2_ghost_lstation_vs_fstation = new TH2F("h2_ghost_lstation_vs_fstation", "Last vs. First Station (Ghost Tracks)",
                                             11, -0.5, 10.5, 11, -0.5, 10.5);
    h2_rest_lstation_vs_fstation_prim =
      new TH2F("h2_rest_lstation_vs_fstation_prim", "Last vs. First Station (!Found Primary Tracks)", 11, -0.5, 10.5,
               11, -0.5, 10.5);
    h2_rest_lstation_vs_fstation_sec =
      new TH2F("h2_rest_lstation_vs_fstation_sec", "Last vs. First Station (!Found Secondary Tracks)", 11, -0.5, 10.5,
               11, -0.5, 10.5);

    //maindir->cd();

    // ----- Create list of all histogram pointers

    gDirectory = curdir;

  }  // first_call


  static int NEvents = 0;
  if (NEvents > 0) {
    h_reg_MCmom->Scale(NEvents);
    h_acc_MCmom->Scale(NEvents);
    h_reco_MCmom->Scale(NEvents);
    h_ghost_Rmom->Scale(NEvents);
    h_reg_prim_MCmom->Scale(NEvents);
    h_acc_prim_MCmom->Scale(NEvents);
    h_reco_prim_MCmom->Scale(NEvents);
    h_reg_sec_MCmom->Scale(NEvents);
    h_acc_sec_MCmom->Scale(NEvents);
    h_reco_sec_MCmom->Scale(NEvents);
  }
  NEvents++;

  //   //hit density calculation: h_hit_density[10]
  //
  //   for (vector<CbmL1HitDebugInfo>::iterator hIt = fvHitDebugInfo.begin(); hIt != fvHitDebugInfo.end(); ++hIt){
  //     float x = hIt->x;
  //     float y = hIt->y;
  //     float r = sqrt(x*x+y*y);
  //     h_hit_density[hIt->iStation]->Fill(r, 1.0/(2.0*3.1415*r));
  //   }

  //
  for (vector<CbmL1Track>::iterator rtraIt = fvRecoTracks.begin(); rtraIt != fvRecoTracks.end(); ++rtraIt) {
    CbmL1Track* prtra = &(*rtraIt);
    if ((prtra->Hits).size() < 1) continue;
    {  // fill histos
      if (fabs(prtra->GetQp()) > 1.e-10)
        h_reco_mom->Fill(
          fabs(1.0 / prtra->GetQp()));  // TODO: Is it a right precision? In FairTrackParam it is 1.e-4 (S.Zharko)
      // NOTE: p = (TMath::Abs(fQp) > 1.e-4) ? 1. / TMath::Abs(fQp) : 1.e4; // FairTrackParam::Momentum(TVector3)
      // h_reco_mom->Fill(TMath::Abs(prtra->T[4] > 1.e-4) ? 1. / TMath::Abs(prtra->T[4]) : 1.e+4); // this should be correct

      h_reco_phi->Fill(prtra->GetPhi());  // TODO: What is precision?
      h_reco_nhits->Fill((prtra->Hits).size());
      CbmL1HitDebugInfo& mh = fvHitDebugInfo[prtra->Hits[0]];
      h_reco_station->Fill(mh.iStation);

      int iFstHit  = prtra->GetFirstHitIndex();
      auto& fstHit = fvHitDebugInfo[iFstHit];
      h2_fst_hit_yz->Fill(fpAlgo->GetParameters().GetStation(fstHit.iStation).fZ[0], fstHit.GetY());

      int iLstHit  = prtra->GetLastHitIndex();
      auto& lstHit = fvHitDebugInfo[iLstHit];
      h2_lst_hit_yz->Fill(fpAlgo->GetParameters().GetStation(lstHit.iStation).fZ[0], lstHit.GetY());

      for (int iH : prtra->Hits) {
        const auto& hit = fvHitDebugInfo[iH];
        int y           = hit.GetY();
        int z           = fpAlgo->GetParameters().GetStation(hit.iStation).fZ[0];
        h2_all_hit_yz->Fill(z, y);
      }
    }


    h_reco_purity->Fill(100 * prtra->GetMaxPurity());

    if (prtra->GetNdf() > 0) {
      if (prtra->IsGhost()) {
        h_ghost_chi2->Fill(prtra->GetChiSq() / prtra->GetNdf());
        h_ghost_prob->Fill(TMath::Prob(prtra->GetChiSq(), prtra->GetNdf()));
      }
      else if (prtra->GetNofMCTracks() > 0) {
        const auto& mcTrk = fMCData.GetTrack(prtra->GetMatchedMCTrackIndex());
        if (mcTrk.IsReconstructable()) {
          h_reco_chi2->Fill(prtra->GetChiSq() / prtra->GetNdf());
          h_reco_prob->Fill(TMath::Prob(prtra->GetChiSq(), prtra->GetNdf()));
        }
        else {
          //          h_rest_chi2->Fill(prtra->GetChiSq()/prtra->NDF);
          h_rest_prob->Fill(TMath::Prob(prtra->GetChiSq(), prtra->GetNdf()));
        }
      }
    }


    // fill ghost histos
    if (prtra->IsGhost()) {
      //fMonitor.IncrementCounter(EMonitorKey::kGhostTrack);
      h_ghost_purity->Fill(100 * prtra->GetMaxPurity());
      if (fabs(prtra->GetQp()) > 1.e-10) {
        h_ghost_mom->Fill(prtra->GetP());
        h_ghost_phi->Fill(prtra->GetPhi());  // phi = atan(py / px) = atan(ty / tx)
        h_ghost_Rmom->Fill(prtra->GetP());
      }
      h_ghost_nhits->Fill((prtra->Hits).size());
      CbmL1HitDebugInfo& h1 = fvHitDebugInfo[prtra->Hits[0]];
      CbmL1HitDebugInfo& h2 = fvHitDebugInfo[prtra->Hits[1]];
      h_ghost_fstation->Fill(h1.iStation);
      h_ghost_r->Fill(sqrt(fabs(h1.x * h1.x + h1.y * h1.y)));
      double z1 = fpAlgo->GetParameters().GetStation(h1.iStation).fZ[0];
      double z2 = fpAlgo->GetParameters().GetStation(h2.iStation).fZ[0];
      if (fabs(z2 - z1) > 1.e-4) {
        h_ghost_tx->Fill((h2.x - h1.x) / (z2 - z1));
        h_ghost_ty->Fill((h2.y - h1.y) / (z2 - z1));
      }

      CbmL1HitDebugInfo& hf = fvHitDebugInfo[prtra->Hits[0]];
      CbmL1HitDebugInfo& hl = fvHitDebugInfo[prtra->Hits[(prtra->Hits).size() - 1]];
      if (fabs(prtra->GetQp()) > 1.e-10) {
        h2_ghost_nhits_vs_mom->Fill(prtra->GetP(), prtra->Hits.size());
        h2_ghost_fstation_vs_mom->Fill(prtra->GetP(), hf.iStation + 1);
      }
      if (hl.iStation >= hf.iStation) h2_ghost_lstation_vs_fstation->Fill(hf.iStation + 1, hl.iStation + 1);
    }

  }  // for reco tracks

  int mc_total = 0;  // total amount of reconstructable mcTracks
  for (const auto& mcTrk : fMCData.GetTrackContainer()) {
    //    if( !( mcTrk.GetPdgCode() == -11 && mcTrk.GetPdgCode() == -1 ) ) continue; // electrons only

    // No Sts hits?
    int nmchits = mcTrk.GetNofHits();
    if (nmchits == 0) continue;

    double momentum = mcTrk.GetP();
    double pt       = mcTrk.GetPt();
    double theta    = mcTrk.GetTheta() * 180 / 3.1415;
    double phi      = mcTrk.GetPhi();

    h_mcp->Fill(momentum);
    h_nmchits->Fill(nmchits);

    int nSta = mcTrk.GetTotNofStationsWithHit();

    CbmL1HitDebugInfo& fh = fvHitDebugInfo[*(mcTrk.GetHitIndexes().begin())];
    CbmL1HitDebugInfo& lh = fvHitDebugInfo[*(mcTrk.GetHitIndexes().rbegin())];

    h_reg_MCmom->Fill(momentum);
    if (mcTrk.IsPrimary()) {
      h_reg_mom_prim->Fill(momentum);
      h_reg_phi_prim->Fill(phi);
      h_reg_prim_MCmom->Fill(momentum);
      h_reg_nhits_prim->Fill(nSta);
      h2_reg_nhits_vs_mom_prim->Fill(momentum, nSta);
      h2_reg_fstation_vs_mom_prim->Fill(momentum, fh.iStation + 1);
      if (lh.iStation >= fh.iStation) h2_reg_lstation_vs_fstation_prim->Fill(fh.iStation + 1, lh.iStation + 1);
    }
    else {
      h_reg_mom_sec->Fill(momentum);
      h_reg_phi_sec->Fill(phi);
      h_reg_sec_MCmom->Fill(momentum);
      h_reg_nhits_sec->Fill(nSta);
      if (momentum > 0.01) {
        h2_reg_nhits_vs_mom_sec->Fill(momentum, nSta);
        h2_reg_fstation_vs_mom_sec->Fill(momentum, fh.iStation + 1);
        if (lh.iStation >= fh.iStation) h2_reg_lstation_vs_fstation_sec->Fill(fh.iStation + 1, lh.iStation + 1);
      }
    }

    if (mcTrk.IsAdditional()) {
      h_acc_mom_short123s->Fill(momentum);
    }

    if (!mcTrk.IsReconstructable()) continue;
    mc_total++;

    h_acc_MCmom->Fill(momentum);
    if (mcTrk.IsPrimary()) {
      h_acc_mom_prim->Fill(momentum);
      h_acc_phi_prim->Fill(phi);
      h_acc_prim_MCmom->Fill(momentum);
      h_acc_nhits_prim->Fill(nSta);
      h2_acc_nhits_vs_mom_prim->Fill(momentum, nSta);
      h2_acc_fstation_vs_mom_prim->Fill(momentum, fh.iStation + 1);
      if (lh.iStation >= fh.iStation) h2_acc_lstation_vs_fstation_prim->Fill(fh.iStation + 1, lh.iStation + 1);
    }
    else {
      h_acc_mom_sec->Fill(momentum);
      h_acc_phi_sec->Fill(phi);
      h_acc_sec_MCmom->Fill(momentum);
      h_acc_nhits_sec->Fill(nSta);
      if (momentum > 0.01) {
        h2_acc_nhits_vs_mom_sec->Fill(momentum, nSta);
        h2_acc_fstation_vs_mom_sec->Fill(momentum, fh.iStation + 1);
        if (lh.iStation >= fh.iStation) h2_acc_lstation_vs_fstation_sec->Fill(fh.iStation + 1, lh.iStation + 1);
      }
    }


    // vertex distribution of primary and secondary tracks
    h2_vertex->Fill(mcTrk.GetStartZ(), mcTrk.GetStartY());
    //h3_vertex->Fill(mcTrk.GetStartZ(), mcTrk.GetStartX(), mcTrk.GetStartY());
    if (mcTrk.GetMotherId() < 0) {  // primary
      h2_prim_vertex->Fill(mcTrk.GetStartZ(), mcTrk.GetStartY());
      //h3_prim_vertex->Fill(mcTrk.GetStartZ(), mcTrk.GetStartX(), mcTrk.GetStartY());
    }
    else {
      h2_sec_vertex->Fill(mcTrk.GetStartZ(), mcTrk.GetStartY());
      //h3_sec_vertex->Fill(mcTrk.GetStartZ(), mcTrk.GetStartX(), mcTrk.GetStartY());
    }


    int iph               = mcTrk.GetHitIndexes()[0];
    CbmL1HitDebugInfo& ph = fvHitDebugInfo[iph];

    h_sec_r->Fill(sqrt(fabs(ph.x * ph.x + ph.y * ph.y)));
    if (fabs(mcTrk.GetPz()) > 1.e-8) {
      h_tx->Fill(mcTrk.GetTx());
      h_ty->Fill(mcTrk.GetTy());
    }

    // reconstructed tracks
    bool reco   = (mcTrk.IsReconstructed());
    int nMCHits = mcTrk.GetTotNofStationsWithHit();

    if (reco) {
      p_eff_all_vs_mom->Fill(momentum, 100.0);
      p_eff_all_vs_nhits->Fill(nMCHits, 100.0);
      p_eff_all_vs_pt->Fill(pt, 100.0);
      h_reco_MCmom->Fill(momentum);
      if (mcTrk.GetMotherId() < 0) {  // primary
        p_eff_prim_vs_mom->Fill(momentum, 100.0);
        p_eff_prim_vs_nhits->Fill(nMCHits, 100.0);
        p_eff_prim_vs_pt->Fill(pt, 100.0);
        p_eff_prim_vs_theta->Fill(theta, 100.0);
      }
      else {
        p_eff_sec_vs_mom->Fill(momentum, 100.0);
        p_eff_sec_vs_nhits->Fill(nMCHits, 100.0);
      }
      if (mcTrk.GetMotherId() < 0) {  // primary
        h_reco_mom_prim->Fill(momentum);
        h_reco_phi_prim->Fill(phi);
        h_reco_prim_MCmom->Fill(momentum);
        h_reco_nhits_prim->Fill(nSta);
        h2_reco_nhits_vs_mom_prim->Fill(momentum, nSta);
        h2_reco_fstation_vs_mom_prim->Fill(momentum, fh.iStation + 1);
        if (lh.iStation >= fh.iStation) h2_reco_lstation_vs_fstation_prim->Fill(fh.iStation + 1, lh.iStation + 1);
      }
      else {
        h_reco_mom_sec->Fill(momentum);
        h_reco_phi_sec->Fill(phi);
        h_reco_sec_MCmom->Fill(momentum);
        h_reco_nhits_sec->Fill(nSta);
        if (momentum > 0.01) {
          h2_reco_nhits_vs_mom_sec->Fill(momentum, nSta);
          h2_reco_fstation_vs_mom_sec->Fill(momentum, fh.iStation + 1);
          if (lh.iStation >= fh.iStation) h2_reco_lstation_vs_fstation_sec->Fill(fh.iStation + 1, lh.iStation + 1);
        }
      }
    }
    else {
      h_notfound_mom->Fill(momentum);
      h_notfound_phi->Fill(phi);
      p_eff_all_vs_mom->Fill(momentum, 0.0);
      p_eff_all_vs_nhits->Fill(nMCHits, 0.0);
      p_eff_all_vs_pt->Fill(pt, 0.0);
      if (mcTrk.GetMotherId() < 0) {  // primary
        p_eff_prim_vs_mom->Fill(momentum, 0.0);
        p_eff_prim_vs_nhits->Fill(nMCHits, 0.0);
        p_eff_prim_vs_pt->Fill(pt, 0.0);
        p_eff_prim_vs_theta->Fill(theta, 0.0);
      }
      else {
        p_eff_sec_vs_mom->Fill(momentum, 0.0);
        p_eff_sec_vs_nhits->Fill(nMCHits, 0.0);
      }
      if (mcTrk.GetMotherId() < 0) {  // primary
        h_rest_mom_prim->Fill(momentum);
        h_rest_phi_prim->Fill(phi);
        h_rest_nhits_prim->Fill(nSta);
        h2_rest_nhits_vs_mom_prim->Fill(momentum, nSta);
        h2_rest_fstation_vs_mom_prim->Fill(momentum, fh.iStation + 1);
        if (lh.iStation >= fh.iStation) h2_rest_lstation_vs_fstation_prim->Fill(fh.iStation + 1, lh.iStation + 1);
      }
      else {
        h_rest_mom_sec->Fill(momentum);
        h_rest_phi_sec->Fill(phi);
        h_rest_nhits_sec->Fill(nSta);
        if (momentum > 0.01) {
          h2_rest_nhits_vs_mom_sec->Fill(momentum, nSta);
          h2_rest_fstation_vs_mom_sec->Fill(momentum, fh.iStation + 1);
          if (lh.iStation >= fh.iStation) h2_rest_lstation_vs_fstation_sec->Fill(fh.iStation + 1, lh.iStation + 1);
        }
      }
    }

    // killed tracks. At least one hit of they belong to some recoTrack
    //     bool killed = 0;
    if (!reco) {
      h_notfound_nhits->Fill(nmchits);
      h_notfound_station->Fill(ph.iStation);
      h_notfound_r->Fill(sqrt(fabs(ph.x * ph.x + ph.y * ph.y)));

      if (mcTrk.GetNofPoints() > 0) {
        const auto& pMC = fMCData.GetPoint(mcTrk.GetPointIndexes()[0]);
        h_notfound_tx->Fill(pMC.GetTx());
        h_notfound_ty->Fill(pMC.GetTy());
      }

      //      CbmL1HitDebugInfo &ph21 = fvHitDebugInfo[mtra.Hits[0]];
      //      CbmL1HitDebugInfo &ph22 = fvHitDebugInfo[mtra.Hits[1]];

      //      double z21 = fpAlgo->GetParameters().GetStation(ph21.iStation).fZ[0];
      //      double z22 = fpAlgo->GetParameters().GetStation(ph22.iStation).fZ[0];
      //      if( fabs(z22-z21)>1.e-4 ){
      //        h_notfound_tx->Fill((ph22.x-ph21.x)/(z22-z21));
      //        h_notfound_ty->Fill((ph22.y-ph21.y)/(z22-z21));
      //      }

      //       if( mtra.IsDisturbed() ) killed = 1;
    }


    if (mcTrk.IsSignal()) {  // D0
      h_reco_d0_mom->Fill(momentum);
      if (reco)
        p_eff_d0_vs_mom->Fill(momentum, 100.0);
      else
        p_eff_d0_vs_mom->Fill(momentum, 0.0);
    }

  }  // for mcTracks

  int NFakes = 0;
  for (unsigned int ih = 0; ih < fpAlgo->GetInputData().GetNhits(); ih++) {
    int iMC = fvHitDebugInfo[ih].GetBestMcPointId();  // TODO2: adapt to linking
    if (iMC < 0) NFakes++;
  }

  h_reco_time->Fill(fTrackingTime);
  h_reco_timeNtr->Fill(mc_total, fTrackingTime);
  h_reco_timeNhit->Fill(fpAlgo->GetInputData().GetNhits(), fTrackingTime);

  h_reco_fakeNtr->Fill(mc_total, NFakes);
  h_reco_fakeNhit->Fill(fpAlgo->GetInputData().GetNhits() - NFakes, NFakes);


  // NOTE: Must be called once
  h_reg_MCmom->Scale(1.f / NEvents);
  h_acc_MCmom->Scale(1.f / NEvents);
  h_reco_MCmom->Scale(1.f / NEvents);
  h_ghost_Rmom->Scale(1.f / NEvents);
  h_reg_prim_MCmom->Scale(1.f / NEvents);
  h_acc_prim_MCmom->Scale(1.f / NEvents);
  h_reco_prim_MCmom->Scale(1.f / NEvents);
  h_reg_sec_MCmom->Scale(1.f / NEvents);
  h_acc_sec_MCmom->Scale(1.f / NEvents);
  h_reco_sec_MCmom->Scale(1.f / NEvents);

}  // void CbmL1::HistoPerformance()


void CbmL1::TrackFitPerformance()
{
  const int Nh_fit = 17;
  static TH1F *h_fit[Nh_fit], *h_fitL[Nh_fit], *h_fitSV[Nh_fit], *h_fitPV[Nh_fit], *h_fit_chi2;

  static TH2F *PRes2D, *PRes2DPrim, *PRes2DSec;

  static TH2F* pion_res_pt_fstDca;
  static TH2F* kaon_res_pt_fstDca;
  static TH2F* pton_res_pt_fstDca;

  static TH2F* pion_res_pt_vtxDca;
  static TH2F* kaon_res_pt_vtxDca;
  static TH2F* pton_res_pt_vtxDca;

  static TH2F* pion_res_p_fstDca;
  static TH2F* kaon_res_p_fstDca;
  static TH2F* pton_res_p_fstDca;

  static TH2F* pion_res_p_vtxDca;
  static TH2F* kaon_res_p_vtxDca;
  static TH2F* pton_res_p_vtxDca;

  static bool first_call = 1;

  kf::TrackKalmanFilter<fvec> fit;
  fit.SetParticleMass(fpAlgo->GetDefaultParticleMass());
  fit.SetMask(fmask::One());
  //fit.SetMaxExtrapolationStep(10.);
  fit.SetDoFitVelocity(true);

  if (first_call) {
    first_call = 0;

    TDirectory* currentDir = gDirectory;
    gDirectory             = fHistoDir;
    gDirectory->cd("Fit");
    {
      PRes2D     = new TH2F("PRes2D", "Resolution P [%] vs P", 100, 0., 20., 100, -15., 15.);
      PRes2DPrim = new TH2F("PRes2DPrim", "Resolution P [%] vs P", 100, 0., 20., 100, -15., 15.);
      PRes2DSec  = new TH2F("PRes2DSec", "Resolution P [%] vs P", 100, 0., 20., 100, -15., 15.);

      pion_res_pt_fstDca = new TH2F("pion_res_pt_fstDca", "", 100, 0, 10, 1000, -500, 500);
      kaon_res_pt_fstDca = new TH2F("kaon_res_pt_fstDca", "", 100, 0, 10, 1000, -500, 500);
      pton_res_pt_fstDca = new TH2F("pton_res_pt_fstDca", "", 100, 0, 10, 1000, -500, 500);

      pion_res_pt_vtxDca = new TH2F("pion_res_pt_vtxDca", "", 100, 0, 10, 1000, -5000, 5000);
      kaon_res_pt_vtxDca = new TH2F("kaon_res_pt_vtxDca", "", 100, 0, 10, 1000, -5000, 5000);
      pton_res_pt_vtxDca = new TH2F("pton_res_pt_vtxDca", "", 100, 0, 10, 1000, -5000, 5000);

      pion_res_p_fstDca = new TH2F("pion_res_p_fstDca", "", 100, 0, 10, 1000, -500, 500);
      kaon_res_p_fstDca = new TH2F("kaon_res_p_fstDca", "", 100, 0, 10, 1000, -500, 500);
      pton_res_p_fstDca = new TH2F("pton_res_p_fstDca", "", 100, 0, 10, 1000, -500, 500);

      pion_res_p_vtxDca = new TH2F("pion_res_p_vtxDca", "", 100, 0, 10, 1000, -5000, 5000);
      kaon_res_p_vtxDca = new TH2F("kaon_res_p_vtxDca", "", 100, 0, 10, 1000, -5000, 5000);
      pton_res_p_vtxDca = new TH2F("pton_res_p_vtxDca", "", 100, 0, 10, 1000, -5000, 5000);

      struct {
        const char* name;
        const char* title;
        Int_t n;
        Double_t l, r;
      } Table[Nh_fit] = {{"x", "Residual X [#mum]", 140, -40., 40.},
                         {"y", "Residual Y [#mum]", 100, -450., 450.},
                         {"tx", "Residual Tx [mrad]", 100, -4., 4.},
                         {"ty", "Residual Ty [mrad]", 100, -3.5, 3.5},
                         {"P", "Resolution P/Q [%]", 100, -15., 15.},
                         {"px", "Pull X [residual/estimated_error]", 100, -6., 6.},
                         {"py", "Pull Y [residual/estimated_error]", 100, -6., 6.},
                         {"ptx", "Pull Tx [residual/estimated_error]", 100, -6., 6.},
                         {"pty", "Pull Ty [residual/estimated_error]", 100, -6., 6.},
                         {"pQP", "Pull Q/P [residual/estimated_error]", 100, -6., 6.},
                         {"QPreco", "Reco Q/P ", 100, -10., 10.},
                         {"QPmc", "MC Q/P ", 100, -10., 10.},
                         {"time", "Residual Time [ns]", 100, -10., 10.},
                         {"ptime", "Pull Time [residual/estimated_error]", 100, -10., 10.},
                         {"vi", "Residual Vi [1/c]", 100, -4., 4.},
                         {"pvi", "Pull Vi [residual/estimated_error]", 100, -10., 10.},
                         {"distrVi", "Vi distribution [1/c]", 100, 0., 4.}};

      if (ca::TrackingMode::kGlobal == fpAlgo->GetTrackingMode()) {
        Table[4].l = -100.;
        Table[4].r = 100.;
      }

      struct Tab {
        const char* name;
        const char* title;
        Int_t n;
        Double_t l, r;
      };
      Tab TableVertex[Nh_fit] = {//{"x",  "Residual X [cm]",                   200, -0.01, 0.01},
                                 {"x", "Residual X [cm]", 2000, -1., 1.},
                                 //{"y",  "Residual Y [cm]",                   200, -0.01, 0.01},
                                 {"y", "Residual Y [cm]", 2000, -1., 1.},
                                 //{"tx", "Residual Tx [mrad]",                  100,   -3.,   3.},
                                 {"tx", "Residual Tx [mrad]", 100, -2., 2.},
                                 //{"ty", "Residual Ty [mrad]",                  100,   -3.,   3.},
                                 {"ty", "Residual Ty [mrad]", 100, -2., 2.},
                                 {"P", "Resolution P/Q [%]", 100, -15., 15.},
                                 {"px", "Pull X [residual/estimated_error]", 100, -10., 10.},
                                 {"py", "Pull Y [residual/estimated_error]", 100, -10., 10.},
                                 {"ptx", "Pull Tx [residual/estimated_error]", 100, -10., 10.},
                                 {"pty", "Pull Ty [residual/estimated_error]", 100, -10., 10.},
                                 {"pQP", "Pull Q/P [residual/estimated_error]", 100, -10., 10.},
                                 {"QPreco", "Reco Q/P ", 100, -10., 10.},
                                 {"QPmc", "MC Q/P ", 100, -10., 10.},
                                 {"time", "Residual Time [ns]", 100, -10., 10.},
                                 {"ptime", "Pull Time [residual/estimated_error]", 100, -10., 10.},
                                 {"vi", "Residual Vi [1/c]", 100, -10., 10.},
                                 {"pvi", "Pull Vi [residual/estimated_error]", 100, -10., 10.},
                                 {"distrVi", "Vi distribution [1/c]", 100, -10., 10.}};

      if (ca::TrackingMode::kGlobal == fpAlgo->GetTrackingMode()) {
        TableVertex[4].l = -1.;
        TableVertex[4].r = 1.;
      }

      for (int i = 0; i < Nh_fit; i++) {
        char n[225], t[255];
        sprintf(n, "fst_%s", Table[i].name);
        sprintf(t, "First point %s", Table[i].title);
        h_fit[i] = new TH1F(n, t, Table[i].n, Table[i].l, Table[i].r);
        sprintf(n, "lst_%s", Table[i].name);
        sprintf(t, "Last point %s", Table[i].title);
        h_fitL[i] = new TH1F(n, t, Table[i].n, Table[i].l, Table[i].r);
        sprintf(n, "svrt_%s", TableVertex[i].name);
        sprintf(t, "Secondary vertex point %s", TableVertex[i].title);
        h_fitSV[i] = new TH1F(n, t, TableVertex[i].n, TableVertex[i].l, TableVertex[i].r);
        sprintf(n, "pvrt_%s", TableVertex[i].name);
        sprintf(t, "Primary vertex point %s", TableVertex[i].title);
        h_fitPV[i] = new TH1F(n, t, TableVertex[i].n, TableVertex[i].l, TableVertex[i].r);

        for (int j = 0; j < 12; j++) {
          sprintf(n, "smth_%s_sta_%i", Table[i].name, j);
          sprintf(t, "Station %i %s", j, Table[i].title);
          //	  h_smoothed[j][i] = new TH1F(n,t, Table[i].n, Table[i].l, Table[i].r);
        }
      }
      h_fit_chi2 = new TH1F("h_fit_chi2", "Chi2/NDF", 50, -0.5, 10.0);
    }
    gDirectory = currentDir;
  }  // if first call


  for (vector<CbmL1Track>::iterator it = fvRecoTracks.begin(); it != fvRecoTracks.end(); ++it) {

    if (it->IsGhost()) continue;

    bool isTimeFitted = 0;

    {
      int nTimeMeasurenments = 0;
      for (unsigned int ih = 0; ih < it->Hits.size(); ih++) {
        int ista = fvHitDebugInfo[it->Hits[ih]].iStation;
        if (fpAlgo->GetParameters().GetStation(ista).timeInfo) {
          nTimeMeasurenments++;
        }
      }
      isTimeFitted = (nTimeMeasurenments > 1);
    }

    do {  // first hit

      if (it->GetNofMCTracks() < 1) {
        break;
      }
      const auto& mcTrk = fMCData.GetTrack(it->GetMCTrackIndexes()[0]);
      int imcPoint      = fvHitDebugInfo[it->Hits.front()].GetBestMcPointId();

      // extrapolate to the first mc point of the mc track !

      imcPoint = mcTrk.GetPointIndexes()[0];

      if (imcPoint < 0) break;

      const auto& mcP = fMCData.GetPoint(imcPoint);

      TrackParamV tr(*it);
      FillFitHistos(tr, mcP, isTimeFitted, h_fit);

      double dx  = tr.GetX()[0] - mcP.GetXIn();
      double dy  = tr.GetY()[0] - mcP.GetYIn();
      double dca = sqrt(dx * dx + dy * dy);
      // make dca distance negative for the half of the tracks to ease the gaussian fit and the rms calculation
      if (mcTrk.GetId() % 2) dca = -dca;
      double pt = mcP.GetPt();
      double dP = (fabs(1. / tr.GetQp()[0]) - mcP.GetP()) / mcP.GetP();

      PRes2D->Fill(mcP.GetP(), 100. * dP);

      if (mcTrk.IsPrimary()) {

        h_fit[4]->Fill(100. * dP);

        PRes2DPrim->Fill(mcP.GetP(), 100. * dP);

        if (abs(mcTrk.GetPdgCode()) == 211) {
          pion_res_p_fstDca->Fill(mcP.GetP(), dca * 1.e4);
          pion_res_pt_fstDca->Fill(pt, dca * 1.e4);
        }
        if (abs(mcTrk.GetPdgCode()) == 321) {
          kaon_res_p_fstDca->Fill(mcP.GetP(), dca * 1.e4);
          kaon_res_pt_fstDca->Fill(pt, dca * 1.e4);
        }
        if (abs(mcTrk.GetPdgCode()) == 2212) {
          pton_res_p_fstDca->Fill(mcP.GetP(), dca * 1.e4);
          pton_res_pt_fstDca->Fill(pt, dca * 1.e4);
        }
      }
      else {
        PRes2DSec->Fill(mcP.GetP(), 100. * dP);
      }
    } while (0);


    {  // last hit

      int iMC = fvHitDebugInfo[it->Hits.back()].GetBestMcPointId();  // TODO2: adapt to linking
      if (iMC < 0) continue;
      const auto& mcP = fMCData.GetPoint(iMC);
      TrackParamV tr(it->TLast);
      FillFitHistos(tr, mcP, isTimeFitted, h_fitL);
    }

    do {  // vertex

      if (it->GetNofMCTracks() < 1) {
        break;
      }

      const auto& mc = fMCData.GetTrack(it->GetMatchedMCTrackIndex());
      fit.SetTrack(*it);

      const TrackParamV& tr = fit.Tr();

      //      if (mc.mother_ID != -1) {  // secondary
      if (!mc.IsPrimary()) {  // secondary

        {  // extrapolate to vertex
          kf::FieldRegion<fvec> fld(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);
          fit.Extrapolate(mc.GetStartZ(), fld);
          // add material
          const int fSta = fvHitDebugInfo[it->Hits[0]].iStation;
          const int dir  = int((mc.GetStartZ() - fpAlgo->GetParameters().GetStation(fSta).fZ[0])
                              / fabs(mc.GetStartZ() - fpAlgo->GetParameters().GetStation(fSta).fZ[0]));
          //         if (abs(mc.z - fpAlgo->GetParameters().GetStation(fSta).fZ[0]) > 10.) continue; // can't extrapolate on large distance
          for (int iSta = fSta /*+dir*/;
               (iSta >= 0) && (iSta < fNStations)
               && (dir * (mc.GetStartZ() - fpAlgo->GetParameters().GetStation(iSta).fZ[0]) > 0);
               iSta += dir) {
            //           LOG(info) << iSta << " " << dir;
            auto radThick = fpAlgo->GetParameters().GetActiveSetup().GetMaterial(iSta).GetThicknessX0(fit.Tr().GetX(),
                                                                                                      fit.Tr().GetY());
            fit.MultipleScattering(radThick);
            fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);
          }
        }
        if (mc.GetStartZ() != tr.GetZ()[0]) continue;

        //       static int good = 0;
        //       static int bad = 0;
        //       if (mc.z != tr.GetZ()[0]){
        //         bad++;
        //         continue;
        //       }
        //       else good++;
        //       LOG(info) << "bad\\good" << bad << " " << good;

        // calculate pulls
        //h_fitSV[0]->Fill( (mc.x-tr.GetX()[0]) *1.e4);
        //h_fitSV[1]->Fill( (mc.y-tr.GetY()[0]) *1.e4);
        h_fitSV[0]->Fill((tr.GetX()[0] - mc.GetStartX()));
        h_fitSV[1]->Fill((tr.GetY()[0] - mc.GetStartY()));
        h_fitSV[2]->Fill((tr.GetTx()[0] - mc.GetTx()) * 1.e3);
        h_fitSV[3]->Fill((tr.GetTy()[0] - mc.GetTy()) * 1.e3);
        h_fitSV[4]->Fill(100. * (fabs(1. / tr.GetQp()[0]) / mc.GetP() - 1.));
        if (std::isfinite((fscal) tr.C00()[0]) && tr.C00()[0] > 0) {
          h_fitSV[5]->Fill((tr.GetX()[0] - mc.GetStartX()) / sqrt(tr.C00()[0]));
        }
        if (std::isfinite((fscal) tr.C11()[0]) && tr.C11()[0] > 0)
          h_fitSV[6]->Fill((tr.GetY()[0] - mc.GetStartY()) / sqrt(tr.C11()[0]));
        if (std::isfinite((fscal) tr.C22()[0]) && tr.C22()[0] > 0)
          h_fitSV[7]->Fill((tr.GetTx()[0] - mc.GetTx()) / sqrt(tr.C22()[0]));
        if (std::isfinite((fscal) tr.C33()[0]) && tr.C33()[0] > 0)
          h_fitSV[8]->Fill((tr.GetTy()[0] - mc.GetTy()) / sqrt(tr.C33()[0]));
        if (std::isfinite((fscal) tr.C44()[0]) && tr.C44()[0] > 0)
          h_fitSV[9]->Fill((tr.GetQp()[0] - mc.GetCharge() / mc.GetP()) / sqrt(tr.C44()[0]));
        h_fitSV[10]->Fill(tr.GetQp()[0]);
        h_fitSV[11]->Fill(mc.GetCharge() / mc.GetP());
        if (isTimeFitted) {
          h_fitSV[12]->Fill(tr.GetTime()[0] - mc.GetStartT());
          if (std::isfinite((fscal) tr.C55()[0]) && tr.C55()[0] > 0.) {
            h_fitSV[13]->Fill((tr.GetTime()[0] - mc.GetStartT()) / sqrt(tr.C55()[0]));
          }
          double dvi =
            tr.GetVi()[0]
            - sqrt(1. + mc.GetMass() * mc.GetMass() / mc.GetP() / mc.GetP()) * constants::phys::SpeedOfLightInvD;
          h_fitSV[14]->Fill(dvi * constants::phys::SpeedOfLightD);
          if (std::isfinite((fscal) tr.C66()[0]) && tr.C66()[0] > 0.) {
            h_fitSV[15]->Fill(dvi / sqrt(tr.C66()[0]));
          }
          h_fitSV[16]->Fill(tr.GetVi()[0] * constants::phys::SpeedOfLightD);
        }
      }
      else {  // primary

        {  // extrapolate to vertex
          kf::FieldRegion<fvec> fld(kf::GlobalField::fgOriginalFieldType, kf::GlobalField::fgOriginalField);

          // add material
          const int fSta = fvHitDebugInfo[it->Hits[0]].iStation;

          const int dir = (mc.GetStartZ() - fpAlgo->GetParameters().GetStation(fSta).fZ[0])
                          / abs(mc.GetStartZ() - fpAlgo->GetParameters().GetStation(fSta).fZ[0]);
          //         if (abs(mc.z - fpAlgo->GetParameters().GetStation(fSta].fZ[0]) > 10.) continue; // can't extrapolate on large distance

          for (int iSta = fSta + dir; (iSta >= 0) && (iSta < fNStations)
                                      && (dir * (mc.GetStartZ() - fpAlgo->GetParameters().GetStation(iSta).fZ[0]) > 0);
               iSta += dir) {

            fit.Extrapolate(fpAlgo->GetParameters().GetStation(iSta).fZ, fld);
            auto radThick = fpAlgo->GetParameters().GetActiveSetup().GetMaterial(iSta).GetThicknessX0(fit.Tr().GetX(),
                                                                                                      fit.Tr().GetY());
            fit.MultipleScattering(radThick);
            fit.EnergyLossCorrection(radThick, kf::FitDirection::kUpstream);
          }
          fit.Extrapolate(mc.GetStartZ(), fld);
        }
        if (mc.GetStartZ() != tr.GetZ()[0]) continue;

        double dx = tr.GetX()[0] - mc.GetStartX();
        double dy = tr.GetY()[0] - mc.GetStartY();
        double dt = sqrt(dx * dx + dy * dy);
        // make dt distance negative for the half of the tracks to ease the gaussian fit and the rms calculation
        if (mc.GetId() % 2) dt = -dt;

        double pt = mc.GetPt();

        if (abs(mc.GetPdgCode()) == 211) {
          pion_res_p_vtxDca->Fill(mc.GetP(), dt * 1.e4);
          pion_res_pt_vtxDca->Fill(pt, dt * 1.e4);
        }
        if (abs(mc.GetPdgCode()) == 321) {
          kaon_res_p_vtxDca->Fill(mc.GetPdgCode(), dt * 1.e4);
          kaon_res_pt_vtxDca->Fill(mc.GetPt(), dt * 1.e4);
        }
        if (abs(mc.GetPdgCode()) == 2212) {
          pton_res_p_vtxDca->Fill(mc.GetP(), dt * 1.e4);
          pton_res_pt_vtxDca->Fill(pt, dt * 1.e4);
        }

        // calculate pulls
        h_fitPV[0]->Fill((mc.GetStartX() - tr.GetX()[0]));
        h_fitPV[1]->Fill((mc.GetStartY() - tr.GetY()[0]));
        h_fitPV[2]->Fill((mc.GetTx() - tr.GetTx()[0]) * 1.e3);
        h_fitPV[3]->Fill((mc.GetTy() - tr.GetTy()[0]) * 1.e3);
        h_fitPV[4]->Fill(100. * (fabs(1 / tr.GetQp()[0]) / mc.GetP() - 1.));
        if (std::isfinite((fscal) tr.C00()[0]) && tr.C00()[0] > 0)
          h_fitPV[5]->Fill((mc.GetStartX() - tr.GetX()[0]) / sqrt(tr.C00()[0]));
        if (std::isfinite((fscal) tr.C11()[0]) && tr.C11()[0] > 0)
          h_fitPV[6]->Fill((mc.GetStartY() - tr.GetY()[0]) / sqrt(tr.C11()[0]));
        if (std::isfinite((fscal) tr.C22()[0]) && tr.C22()[0] > 0)
          h_fitPV[7]->Fill((mc.GetTx() - tr.GetTx()[0]) / sqrt(tr.C22()[0]));
        if (std::isfinite((fscal) tr.C33()[0]) && tr.C33()[0] > 0)
          h_fitPV[8]->Fill((mc.GetTy() - tr.GetTy()[0]) / sqrt(tr.C33()[0]));
        if (std::isfinite((fscal) tr.C44()[0]) && tr.C44()[0] > 0)
          h_fitPV[9]->Fill((mc.GetCharge() / mc.GetP() - tr.GetQp()[0]) / sqrt(tr.C44()[0]));
        h_fitPV[10]->Fill(tr.GetQp()[0]);
        h_fitPV[11]->Fill(mc.GetCharge() / mc.GetP());
        if (isTimeFitted) {
          h_fitPV[12]->Fill(tr.GetTime()[0] - mc.GetStartT());
          if (std::isfinite((fscal) tr.C55()[0]) && tr.C55()[0] > 0.) {
            h_fitPV[13]->Fill((tr.GetTime()[0] - mc.GetStartT()) / sqrt(tr.C55()[0]));
          }
          double dvi =
            tr.GetVi()[0]
            - sqrt(1. + mc.GetMass() * mc.GetMass() / mc.GetP() / mc.GetP()) * constants::phys::SpeedOfLightInvD;
          h_fitPV[14]->Fill(dvi * constants::phys::SpeedOfLightD);
          if (std::isfinite((fscal) tr.C66()[0]) && tr.C66()[0] > 0.) {
            h_fitPV[15]->Fill(dvi / sqrt(tr.C66()[0]));
          }
          h_fitPV[16]->Fill(tr.GetVi()[0] * constants::phys::SpeedOfLightD);
        }
      }
    } while (0);

    h_fit_chi2->Fill(it->GetChiSq() / it->GetNdf());

    // last TRD point
    /*
    do {
      break;  // only produce these plots in debug mode
      if (it->GetNMCTracks() < 1) { break; }

      if (!fpTrdPoints) break;
      const CbmL1MCTrack& mcTrack = *(it->GetMCTracks()[0]);
      int nTrdPoints              = fpTrdPoints->Size(mcTrack.iFile, mcTrack.iEvent);
      int lastP                   = -1;
      double lastZ                = -1.e8;
      for (int iPoint = 0; iPoint < nTrdPoints; iPoint++) {
        const CbmTrdPoint* pt = dynamic_cast<CbmTrdPoint*>(fpTrdPoints->Get(mcTrack.iFile, mcTrack.iEvent, iPoint));
        if (!pt) { continue; }
        if (pt->GetTrackID() != mcTrack.ID) { continue; }
        if (lastP < 0 || lastZ < pt->GetZ()) {
          lastP = iPoint;
          lastZ = pt->GetZ();
        }
      }
      CbmL1MCPoint mcP;
      if (CbmL1::ReadMCPoint(&mcP, lastP, mcTrack.iFile, mcTrack.iEvent, 3)) { break; }
      TrackParamV tr;
      tr.copyFromArrays(it->TLast, it->CLast);
      FillFitHistos(tr, mcP, isTimeFitted, h_fitTrd);
    } while (0);  // Trd point


    // last TOF point
    do {
      break;  // only produce these plots in debug mode

      if (it->GetNMCTracks() < 1) { break; }

      if (!fpTofPoints) break;
      const CbmL1MCTrack& mcTrack = *(it->GetMCTracks()[0]);
      int nTofPoints              = fpTofPoints->Size(mcTrack.iFile, mcTrack.iEvent);
      int lastP                   = -1;
      double lastZ                = -1.e8;
      for (int iPoint = 0; iPoint < nTofPoints; iPoint++) {
        const CbmTofPoint* pt = dynamic_cast<CbmTofPoint*>(fpTofPoints->Get(mcTrack.iFile, mcTrack.iEvent, iPoint));
        if (!pt) { continue; }
        if (pt->GetTrackID() != mcTrack.ID) { continue; }
        if (lastP < 0 || lastZ < pt->GetZ()) {
          lastP = iPoint;
          lastZ = pt->GetZ();
        }
      }
      CbmL1MCPoint mcP;
      if (CbmL1::ReadMCPoint(&mcP, lastP, mcTrack.iFile, mcTrack.iEvent, 4)) { break; }
      TrackParamV tr;
      tr.copyFromArrays(it->TLast, it->CLast);
      FillFitHistos(tr, mcP, isTimeFitted, h_fitTof);
    } while (0);  // tof point
*/
  }  // reco track

}  // void CbmL1::TrackFitPerformance()


void CbmL1::FillFitHistos(TrackParamV& track, const cbm::ca::tools::MCPoint& mcP, bool isTimeFitted, TH1F* h[])
{
  kf::TrackKalmanFilter<fvec> fit;
  fit.SetParticleMass(fpAlgo->GetDefaultParticleMass());
  fit.SetMask(fmask::One());
  //fit.SetMaxExtrapolationStep(10.);
  fit.SetDoFitVelocity(true);
  fit.SetTrack(track);
  kf::FieldRegion<fvec> fld(kf::EFieldType::Normal, kf::GlobalField::fgOriginalField);
  fit.Extrapolate(mcP.GetZOut(), fld);
  track = fit.Tr();

  const TrackParamV& tr = track;

  h[0]->Fill((tr.GetX()[0] - mcP.GetXOut()) * 1.e4);
  h[1]->Fill((tr.GetY()[0] - mcP.GetYOut()) * 1.e4);
  h[2]->Fill((tr.GetTx()[0] - mcP.GetTxOut()) * 1.e3);
  h[3]->Fill((tr.GetTy()[0] - mcP.GetTyOut()) * 1.e3);
  h[4]->Fill(100. * (fabs(1. / tr.GetQp()[0]) / mcP.GetP() - 1.));

  if (std::isfinite((fscal) tr.C00()[0]) && tr.C00()[0] > 0)
    h[5]->Fill((tr.GetX()[0] - mcP.GetXOut()) / sqrt(tr.C00()[0]));
  if (std::isfinite((fscal) tr.C11()[0]) && tr.C11()[0] > 0)
    h[6]->Fill((tr.GetY()[0] - mcP.GetYOut()) / sqrt(tr.C11()[0]));
  if (std::isfinite((fscal) tr.C22()[0]) && tr.C22()[0] > 0)
    h[7]->Fill((tr.GetTx()[0] - mcP.GetTxOut()) / sqrt(tr.C22()[0]));
  if (std::isfinite((fscal) tr.C33()[0]) && tr.C33()[0] > 0)
    h[8]->Fill((tr.GetTy()[0] - mcP.GetTyOut()) / sqrt(tr.C33()[0]));
  if (std::isfinite((fscal) tr.C44()[0]) && tr.C44()[0] > 0)
    h[9]->Fill((tr.GetQp()[0] - mcP.GetQp()) / sqrt(tr.C44()[0]));
  h[10]->Fill(tr.GetQp()[0]);
  h[11]->Fill(mcP.GetQp());
  if (isTimeFitted) {
    h[12]->Fill(tr.GetTime()[0] - mcP.GetTime());
    if (std::isfinite((fscal) tr.C55()[0]) && tr.C55()[0] > 0.) {
      h[13]->Fill((tr.GetTime()[0] - mcP.GetTime()) / sqrt(tr.C55()[0]));
    }

    double dvi = tr.GetVi()[0] - mcP.GetInvSpeed();
    h[14]->Fill(dvi * constants::phys::SpeedOfLightD);
    if (std::isfinite((fscal) tr.C66()[0]) && tr.C66()[0] > 0.) {
      h[15]->Fill(dvi / sqrt(tr.C66()[0]));
    }
    h[16]->Fill(tr.GetVi()[0] * constants::phys::SpeedOfLightD);
  }
}


void CbmL1::FieldApproxCheck()
{
  TDirectory* curr   = gDirectory;
  TFile* currentFile = gFile;
  TFile* fout        = new TFile("CaFieldApproxQa.root", "RECREATE");
  fout->cd();

  assert(FairRunAna::Instance());
  FairField* MF = FairRunAna::Instance()->GetField();
  assert(MF);

  for (int ist = 0; ist < fpAlgo->GetParameters().GetNstationsActive(); ist++) {

    const ca::Station<fvec>& st = fpAlgo->GetParameters().GetStation(ist);

    double z    = st.fZ[0];
    double Xmax = st.Xmax[0];
    double Ymax = st.Ymax[0];

    int NbinsX = 101;
    int NbinsY = 101;

    TH2F* hdB[4] = {
      new TH2F(Form("station_%i_dB", ist), Form("station %i, dB, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax),
      new TH2F(Form("station_%i_dBx", ist), Form("station %i, dBx, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax),
      new TH2F(Form("station_%i_dBy", ist), Form("station %i, dBy, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax),
      new TH2F(Form("station_%i_dBz", ist), Form("station %i, dBz, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax)                                   //
    };

    TH2F* hB[4] = {
      new TH2F(Form("station_%i_B", ist), Form("station %i, B, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax),
      new TH2F(Form("station_%i_Bx", ist), Form("station %i, Bx, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax),
      new TH2F(Form("station_%i_By", ist), Form("station %i, By, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax),
      new TH2F(Form("station_%i_Bz", ist), Form("station %i, Bz, z = %0.f cm", ist, z),  //
               NbinsX, -Xmax, Xmax, NbinsY, -Ymax, Ymax)                                 //
    };

    for (int i = 0; i < 4; i++) {
      hdB[i]->GetXaxis()->SetTitle("X, cm");
      hdB[i]->GetYaxis()->SetTitle("Y, cm");
      hdB[i]->GetXaxis()->SetTitleOffset(1);
      hdB[i]->GetYaxis()->SetTitleOffset(1);
      hdB[i]->GetZaxis()->SetTitleOffset(1.3);
      hB[i]->GetXaxis()->SetTitle("X, cm");
      hB[i]->GetYaxis()->SetTitle("Y, cm");
      hB[i]->GetXaxis()->SetTitleOffset(1);
      hB[i]->GetYaxis()->SetTitleOffset(1);
      hB[i]->GetZaxis()->SetTitleOffset(1.3);
    }

    hdB[0]->GetZaxis()->SetTitle("B_map - B_L1, kGauss");
    hdB[1]->GetZaxis()->SetTitle("Bx_map - Bx_L1, kGauss");
    hdB[2]->GetZaxis()->SetTitle("By_map - By_L1, kGauss");
    hdB[3]->GetZaxis()->SetTitle("Bz_map - Bz_L1, kGauss");

    hdB[0]->GetZaxis()->SetTitle("B_map, kGauss");
    hdB[1]->GetZaxis()->SetTitle("Bx_map, kGauss");
    hdB[2]->GetZaxis()->SetTitle("By_map, kGauss");
    hdB[3]->GetZaxis()->SetTitle("Bz_map, kGauss");


    for (int i = 1; i <= hdB[0]->GetXaxis()->GetNbins(); i++) {
      double x = hdB[0]->GetXaxis()->GetBinCenter(i);
      for (int j = 1; j <= hdB[0]->GetYaxis()->GetNbins(); j++) {
        double y   = hdB[0]->GetYaxis()->GetBinCenter(j);
        double r[] = {x, y, z};
        double B[] = {0., 0., 0.};
        MF->GetFieldValue(r, B);

        double bbb = sqrt(B[0] * B[0] + B[1] * B[1] + B[2] * B[2]);

        kf::FieldValue<fvec> B_L1 = st.fieldSlice.GetFieldValue(x, y);

        hdB[0]->SetBinContent(i, j, (bbb - B_L1.GetAbs()[0]));
        hdB[1]->SetBinContent(i, j, (B[0] - B_L1.GetBx()[0]));
        hdB[2]->SetBinContent(i, j, (B[1] - B_L1.GetBy()[0]));
        hdB[3]->SetBinContent(i, j, (B[2] - B_L1.GetBz()[0]));

        hB[0]->SetBinContent(i, j, bbb);
        hB[1]->SetBinContent(i, j, B[0]);
        hB[2]->SetBinContent(i, j, B[1]);
        hB[3]->SetBinContent(i, j, B[2]);
      }
    }

    for (int i = 0; i < 4; i++) {
      hdB[i]->Write();
      hB[i]->Write();
    }

  }  // for ista

  fout->Close();
  fout->Delete();
  gFile      = currentFile;
  gDirectory = curr;
}  // void CbmL1::FieldApproxCheck()


#include "TMath.h"
void CbmL1::FieldIntegralCheck()
{
  TDirectory* curr   = gDirectory;
  TFile* currentFile = gFile;
  TFile* fout        = new TFile("FieldApprox.root", "RECREATE");
  fout->cd();

  assert(FairRunAna::Instance());
  FairField* MF = FairRunAna::Instance()->GetField();
  assert(MF);

  int nPointsZ     = 1000;
  int nPointsPhi   = 100;
  int nPointsTheta = 100;
  double startZ = 0, endZ = 100.;
  double startPhi = 0, endPhi = 2 * TMath::Pi();
  double startTheta = -30. / 180. * TMath::Pi(), endTheta = 30. / 180. * TMath::Pi();

  double DZ = endZ - startZ;
  double DP = endPhi - startPhi;
  double DT = endTheta - startTheta;

  float ddp = endPhi / nPointsPhi;
  float ddt = 2 * endTheta / nPointsTheta;

  TH2F* hSb =
    new TH2F("Field Integral", "Field Integral", static_cast<int>(nPointsPhi), -(startPhi + ddp / 2.),
             (endPhi + ddp / 2.), static_cast<int>(nPointsTheta), (startTheta - ddt / 2.), (endTheta + ddt / 2.));

  for (int iP = 0; iP < nPointsPhi; iP++) {
    double phi = startPhi + iP * DP / nPointsPhi;
    for (int iT = 0; iT < nPointsTheta; iT++) {
      double theta = startTheta + iT * DT / nPointsTheta;

      double Sb = 0;
      for (int iZ = 1; iZ < nPointsZ; iZ++) {
        double z    = startZ + DZ * iZ / nPointsZ;
        double x    = z * TMath::Tan(theta) * TMath::Cos(phi);
        double y    = z * TMath::Tan(theta) * TMath::Sin(phi);
        double r[3] = {x, y, z};
        double b[3];
        MF->GetFieldValue(r, b);
        double B = sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
        Sb += B * DZ / nPointsZ / 100. / 10.;
      }
      hSb->SetBinContent(iP + 1, iT + 1, Sb);
    }
  }

  hSb->GetXaxis()->SetTitle("#phi [rad]");
  hSb->GetYaxis()->SetTitle("#theta [rad]");
  hSb->GetXaxis()->SetTitleOffset(1);
  hSb->GetYaxis()->SetTitleOffset(1);
  hSb->GetZaxis()->SetTitle("Field Integral [T#dotm]");
  hSb->GetZaxis()->SetTitleOffset(1.3);

  hSb->Write();


  fout->Close();
  fout->Delete();
  gFile      = currentFile;
  gDirectory = curr;
}  // void CbmL1::FieldApproxCheck()

// ---------------------------------------------------------------------------------------------------------------------
//
void CbmL1::DumpMCTripletsToTree()
{
  if (!fpMcTripletsTree) {
    TDirectory* currentDir = gDirectory;
    TFile* currentFile     = gFile;


    // Get prefix and directory
    boost::filesystem::path p = (FairRunAna::Instance()->GetUserOutputFileName()).Data();
    std::string dir           = p.parent_path().string();
    if (dir.empty()) dir = ".";
    std::string filename = dir + "/" + fsMcTripletsOutputFilename + "." + p.filename().string();
    LOG(info) << "\033[1;31mSAVING A TREE: " << filename << "\033[0m";

    fpMcTripletsOutFile = new TFile(filename.c_str(), "RECREATE");
    fpMcTripletsOutFile->cd();
    fpMcTripletsTree = new TTree("t", "MC Triplets");
    // motherId   - id of mother particle (< 0, if primary)
    // pdg        - PDG code of particle
    // processId  - id of the process (ROOT::TMCProcessID)
    // pq         - absolute value of track momentum [GeV/c], divided by charge [e]
    // zv         - z-component of track vertex [cm]
    // s          - global index of station
    // x0, y0, z0 - position of the left MC point in a triplet [cm]
    // x1, y1, z1 - position of the middle MC point in a triplet [cm]
    // x2, y2, z2 - position of the right MC point in a triplet [cm]

    gFile      = currentFile;
    gDirectory = currentDir;
  }

  // Variables for tree branches
  int brMotherId        = -1;                 ///< mother ID of track
  int brPdg             = -1;                 ///< PDG code of track
  unsigned int brProcId = (unsigned int) -1;  ///< process ID (see ROOT::TMCProcessID)
  float brP             = 0.f;                ///< abs. momentum of track [GeV/c]
  int brQ               = 0;                  ///< charge of track [e]
  float brVertexZ       = 0.f;                ///< z-component of the MC track vertex [cm]
  int brStation         = -1;                 ///< global index of the left MC point station

  float brX0 = 0.f;  ///< x-component of the left MC point in a triplet [cm]
  float brY0 = 0.f;  ///< y-component of the left MC point in a triplet [cm]
  float brZ0 = 0.f;  ///< z-component of the left MC point in a triplet [cm]
  float brX1 = 0.f;  ///< x-component of the middle MC point in a triplet [cm]
  float brY1 = 0.f;  ///< y-component of the middle MC point in a triplet [cm]
  float brZ1 = 0.f;  ///< z-component of the middle MC point in a triplet [cm]
  float brX2 = 0.f;  ///< x-component of the right MC point in a triplet [cm]
  float brY2 = 0.f;  ///< y-component of the right MC point in a triplet [cm]
  float brZ2 = 0.f;  ///< z-component of the right MC point in a triplet [cm]

  // Register branches
  fpMcTripletsTree->Branch("brMotherId", &brMotherId, "motherId/I");
  fpMcTripletsTree->Branch("brPdg", &brPdg, "pdg/I");
  fpMcTripletsTree->Branch("brProcId", &brProcId, "processId/i");
  fpMcTripletsTree->Branch("brP", &brP, "p/F");
  fpMcTripletsTree->Branch("brQ", &brQ, "q/I");
  fpMcTripletsTree->Branch("brVertexZ", &brVertexZ, "zv/F");
  fpMcTripletsTree->Branch("brStation", &brStation, "s/I");
  fpMcTripletsTree->Branch("brX0", &brX0, "x0/F");
  fpMcTripletsTree->Branch("brY0", &brY0, "y0/F");
  fpMcTripletsTree->Branch("brZ0", &brZ0, "z0/F");
  fpMcTripletsTree->Branch("brX1", &brX1, "x1/F");
  fpMcTripletsTree->Branch("brY1", &brY1, "y1/F");
  fpMcTripletsTree->Branch("brZ1", &brZ1, "z1/F");
  fpMcTripletsTree->Branch("brX2", &brX2, "x2/F");
  fpMcTripletsTree->Branch("brY2", &brY2, "y2/F");
  fpMcTripletsTree->Branch("brZ2", &brZ2, "z2/F");

  struct ReducedMcPoint {
    int s;    ///< global active index of tracking station
    float x;  ///< x-component of point position [cm]
    float y;  ///< y-component of point position [cm]
    float z;  ///< z-component of point position [cm]
  };

  for (const auto& tr : fMCData.GetTrackContainer()) {
    // Use only reconstructable tracks
    if (!tr.IsReconstructable()) {
      continue;
    }

    std::vector<ReducedMcPoint> vPoints;
    vPoints.reserve(tr.GetNofPoints());
    for (unsigned int iP : tr.GetPointIndexes()) {
      const auto& point = fMCData.GetPoint(iP);
      vPoints.emplace_back(
        ReducedMcPoint{point.GetStationId(), float(point.GetX()), float(point.GetY()), float(point.GetZ())});
    }

    std::sort(vPoints.begin(), vPoints.end(),
              [](const ReducedMcPoint& lhs, const ReducedMcPoint& rhs) { return lhs.s < rhs.s; });

    for (unsigned int i = 0; i + 2 < vPoints.size(); ++i) {
      // Condition to collect only triplets without gaps in stations
      // TODO: SZh 20.10.2022 Add cases for jump iterations
      if (vPoints[i + 1].s == vPoints[i].s + 1 && vPoints[i + 2].s == vPoints[i].s + 2) {
        // Fill MC-triplets tree
        brMotherId = tr.GetMotherId();
        brPdg      = tr.GetPdgCode();
        brProcId   = tr.GetProcessId();
        brP        = tr.GetP();
        brQ        = tr.GetCharge();
        brVertexZ  = tr.GetStartZ();
        brStation  = vPoints[i].s;
        brX0       = vPoints[i].x;
        brY0       = vPoints[i].y;
        brZ0       = vPoints[i].z;
        brX1       = vPoints[i + 1].x;
        brY1       = vPoints[i + 1].y;
        brZ1       = vPoints[i + 1].z;
        brX2       = vPoints[i + 2].x;
        brY2       = vPoints[i + 2].y;
        brZ2       = vPoints[i + 2].z;

        fpMcTripletsTree->Fill();
      }
    }
  }
}
