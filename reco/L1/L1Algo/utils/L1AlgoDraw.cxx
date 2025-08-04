/* Copyright (C) 2010-2021 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov [committer] */

   #include "L1AlgoDraw.h"

   #include "CaFramework.h"
   #include "CaSimd.h"
   #include "CbmL1.h"
   #include "TApplication.h"
   #include "TCanvas.h"
   #include "TEllipse.h"
   #include "TFrame.h"
   #include "TGraph.h"
   #include "TLatex.h"
   #include "TLegend.h"
   #include "TLine.h"
   #include "TMarker.h"
   #include "TMultiGraph.h"
   #include "TPad.h"
   #include "TPaveStats.h"
   #include "TPaveText.h"
   #include "TPolyLine.h"
   #include "TPolyLine3D.h"
   #include "TPolyMarker.h"
   #include "TStyle.h"
   #include "TText.h"
   #include "TView3D.h"
   // #include "TAxis.h"
   
   #include "EmbedNet.h"
   
   
   const float OT_RAD_TO_DEGREE = 57.2958f;  // 180/PI
   
   using namespace cbm::algo::ca;
   
   // NOTE: using std::cout, std::endl were removed from one of the headers, and it
   //       caused errors. Please, don't use "using" in headers
   using cbm::algo::ca::Track;
   using std::cout;
   using std::endl;
   using std::map;
   using std::vector;
   
   /*
   Station Z Pos:
                   0     -36
                   1     -32
                   2     -28
                   3     -24  
                   4     -14
                   5     -3.5
                   6     7
                   7     19.5
                   8     30.0
                   9     40.5
                   10    51.0
                   11    61.5
   */
   
   L1AlgoDraw::L1AlgoDraw()
   {
   
     gStyle->SetCanvasBorderMode(0);
     gStyle->SetCanvasBorderSize(1);
     gStyle->SetCanvasColor(0);
   
     YZ = new TCanvas("YZ", "YZ Side View", -1, 0, 500, 500);  // set pixels
     YZ->Range(-50.0, -50.0, 70.0, 50.0);                      // set coords frame in canvas. bottom left and top right
     YZ->Draw();
     YZ->Update();
   
     XZ = new TCanvas("XZ", "XZ Top View", -1, 500, 500, 500);
     XZ->Range(-50.0, -50.0, 70.0, 50.0);
     XZ->Draw();
     XZ->Update();
   
     YX = new TCanvas("YX", "YX Front View", -506, 0, 500, 500);
     YX->Range(-50.0, -50.0, 50.0, 50.0);
     YX->Draw();
     YX->Update();
   
     XYZ = new TCanvas("XYZ", "XYZ 3D View", -500, 500, 500, 500);
     XYZ->Range(-50.0, -50.0, 70.0, 50.0);
     XYZ->Draw();
     XYZ->Update();
   
     fVerbose = CbmL1::Instance()->fVerbose;
     ask      = true;
   
     LOG(info) << "L1AlgoDraw: Constructor.";
   }
   
   void L1AlgoDraw::InitL1Draw(ca::Framework* algo_)
   {
     //   algo = CbmL1::Instance()->algo;
     algo = algo_;
   
     vHits.clear();
     vHitsQa.clear();
     vHits.reserve(algo->fWindowHits.size());
     vHitsQa.reserve(algo->fWindowHits.size());
     for (unsigned int i = 0; i < algo->fWindowHits.size(); i++) {
       vHits.push_back(algo->fWindowHits[i]);
       int iQaHit = algo->fWindowHits[i].Id();
       vHitsQa.push_back(CbmL1::Instance()->GetQaHits()[iQaHit]);
     }
     NStations = algo->GetParameters().GetNstationsActive();
     for (int i = 0; i < NStations; i++) {
       HitsStartIndex[i] = algo->fStationHitsStartIndex[i];
       HitsStopIndex[i]  = algo->fStationHitsStartIndex[i] + algo->fStationNhits[i];
       vStations[i]      = algo->GetParameters().GetStation(i);
     }
     LOG(info) << "L1AlgoDraw: InitL1Draw.";
   }
   
   void L1AlgoDraw::DrawMCTracks()
   {
     int nLongPrim = 0;
     std::map<int, int> trkHitCount;
   
     int NRegMCTracks = 0;
     CbmL1& L1        = *CbmL1::Instance();
     TPolyLine pline;
     if (fVerbose >= 10) {
       cout << "Only reconstructable tracks are shown." << endl;
       cout << "Red - primary p > 0.5 - (first iteration)" << endl;
       cout << "Red - primary p < 0.5 - (second iteration)" << endl;
       cout << "Red - secondary p > 0.5 - (third\\first iteration)" << endl;
       cout << "Red - secondary p < 0.5 - (third\\second iteration)" << endl;
     };
   
     int nRecoTracks = 0;  // associated reco tracks
   
     const auto& mcData = L1.GetMCData();
     for (const auto& mcTrk : mcData.GetTrackContainer()) {
       // draw reconstructable tracks only
       if (!mcTrk.IsReconstructable()) continue;
       if (mcTrk.GetNofHits() < 4) continue;
       // if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
       // if (mcTrk.GetP() < 0.2) continue;
   
       // if (mcTrk.IsPrimary()) continue;
   
       // if (mcTrk.IsReconstructed()) continue;
   
       /// skip tracks not found in GNN iteration
       // bool found_gnn_iter = true;
       // for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
       //   const auto& rTrk = L1.fvRecoTracks[irt];
       //   if (rTrk.fFoundInIteration != 3) {
       //     found_gnn_iter = false;
       //     break;
       //   }
       // }
       // if (!found_gnn_iter) continue;
   
   
       nRecoTracks += mcTrk.GetRecoTrackIndexes().size();
   
       /// select tracks of interest
       // std::vector<int> ids = {114, 427, 438, 777, 882};
       // bool interest = false;
       // for ( auto id : ids ) {
       //   if (mcTrk.GetId() == id) {
       //     interest = true;
       //     break;
       //   }
       // }
       // if (!interest) continue;
   
       pline.SetLineColor(kRed);
       // set different colours for different iterations
       if (mcTrk.GetP() < 0.5) pline.SetLineColor(kRed);
       if (mcTrk.GetMotherId() != -1) pline.SetLineColor(kRed);
       if ((mcTrk.GetMotherId() != -1) && (mcTrk.GetMotherId() < 0.5)) pline.SetLineColor(kRed);
   
       LOG_IF(info, fVerbose > 4) << "MC Track: p = " << mcTrk.GetP() << "  mother_ID = " << mcTrk.GetMotherId()
                                  << "  PDG = " << mcTrk.GetPdgCode() << " x,y,z = (" << mcTrk.GetStartX() << ", "
                                  << mcTrk.GetStartY() << ", " << mcTrk.GetStartZ() << ")";
   
       double par[6];
       par[0] = mcTrk.GetStartX();
       par[1] = mcTrk.GetStartY();
       //if( fabs(mcTrk.GetPz())<0.1 ) continue;
       par[2] = mcTrk.GetTx();
       par[3] = mcTrk.GetTy();
       par[4] = mcTrk.GetCharge() / mcTrk.GetP();
       par[5] = mcTrk.GetStartZ();
   
       int npoints = mcTrk.GetNofPoints();
       LOG_IF(info, fVerbose >= 10) << " NMCPoints = " << npoints;
       if (npoints < 1) continue;
   
       vector<double> lx, ly, lz;
       vector<int> matchedWithHit;  // 1 - MC point is matched with hit, 0 - not matched
       vector<int> hitUsedEarlier;  // 1 - hit was used earlier, 0 - not used
       // start track from its vertex
       // lx.push_back(par[0]);
       // ly.push_back(par[1]);
       // lz.push_back(par[5]);
   
       bool ok = true;
   
       for (int ip : mcTrk.GetPointIndexes()) {
         auto& point = mcData.GetPoint(ip);
   
         int isMatched = point.GetHitIndexes().size() > 0 ? 1 : 0;
         matchedWithHit.push_back(isMatched);
   
         // OT: Next block commented out. I dont properly understand what its trying to do. Smoothing?
         // OT: Keeping this in makes run crash early without error.
         // double par1[6];
         // par1[0] = point.GetX();
         // par1[1] = point.GetY();
         // par1[2] = point.GetTx();
         // par1[3] = point.GetTy();
         // par1[4] = point.GetQp();
         // par1[5] = point.GetZ();
         // if (fVerbose >= 5) {
         //   static fscal pz = -1;
         //   LOG(info) << (fabs(pz - point.GetZ()) > 1.0 ? "-- " : "") << "point z = " << point.GetZ();
         //   pz = point.GetZ();
         // }
   
         // double Zfrst = par[5];
         // double Zlast = par1[5];
         // double step  = .5;
         // if (step > fabs(Zfrst - Zlast) / 5) step = fabs(Zfrst - Zlast) / 5;
         // if (Zlast < par[5]) step = -step;
         // while (fabs(par[5] - Zlast) > fabs(step)) {
         //   double znxt = par[5] + step;
         //   //TODO: replace this part with some L1 utility
         //   // CbmKF::Instance()->Propagate(par1, 0, znxt, par1[4]);
         //   // CbmKF::Instance()->Propagate(par, 0, znxt, par[4]);
         //   double w  = fabs(znxt - Zfrst);
         //   double w1 = fabs(znxt - Zlast);
         //   if (w + w1 < 1.e-3) {
         //     w  = 1;
         //     w1 = 0;
         //   }
         //   double xl = (w1 * par[0] + w * par1[0]) / (w + w1);
         //   double yl = (w1 * par[1] + w * par1[1]) / (w + w1);
         //   double zl = (w1 * par[5] + w * par1[5]) / (w + w1);
         //   if ((fabs(xl) > 50.0) || (fabs(yl) > 50.0)) {
         //     //cout << "*** track " << NRegMCTracks+1 << " xl = " << xl << ", zl = " << zl << endl;
         //     //cout << "*** track " << NRegMCTracks+1 << " yl = " << yl << ", zl = " << zl << endl;
         //     ok = false;
         //     continue;
         //   }
         //   lx.push_back(xl);
         //   ly.push_back(yl);
         //   lz.push_back(zl);
         // }
   
   
         par[0] = point.GetX();
         par[1] = point.GetY();
         par[2] = point.GetTx();
         par[3] = point.GetTy();
         par[4] = point.GetQp();
         par[5] = point.GetZ();
   
         // if (matchedWithHit.back() == 1) {
         // }
         // else {
         //   hitUsedEarlier.push_back(0);
         // }
   
         lx.push_back(par[0]);
         ly.push_back(par[1]);
         lz.push_back(par[5]);
       }
   
       if (ok) {
         NRegMCTracks++;
         TMarker marker;
         marker.SetMarkerColor(kBlack);
         marker.SetMarkerStyle(24);  // 26 - open triangle, 24 - open circle
         marker.SetMarkerSize(HitSize);
   
         TMarker markerUsed;
         markerUsed.SetMarkerColor(kGreen);
         markerUsed.SetMarkerStyle(20);  // closed circle
         markerUsed.SetMarkerSize(HitSize);
   
         YZ->cd();
         pline.DrawPolyLine(lx.size(), &(lz[0]), &(ly[0]));
         for (int i = 0; i < int(lx.size()); i++) {
           if (matchedWithHit[i] == 1) {
             marker.SetMarkerColor(kBlack);
             // if (hitUsedEarlier[i] == 1) {
             //   markerUsed.DrawMarker(lz[i], ly[i]);
             // }
           }
           else {  // no match of point with hit
             // marker.SetMarkerColor(kRed);
           }
           marker.DrawMarker(lz[i], ly[i]);
         }
         XZ->cd();
         pline.DrawPolyLine(lx.size(), &(lz[0]), &(lx[0]));
         for (int i = 0; i < int(lx.size()); i++) {
           if (matchedWithHit[i] == 1) {
             marker.SetMarkerColor(kBlack);
             // if (hitUsedEarlier[i] == 1) {
             //   markerUsed.DrawMarker(lz[i], lx[i]);
             // }
           }
           else {
             // marker.SetMarkerColor(kRed);
           }
           marker.DrawMarker(lz[i], lx[i]);
         }
         YX->cd();
         pline.DrawPolyLine(lx.size(), &(lx[0]), &(ly[0]));
         for (int i = 0; i < int(lx.size()); i++) {
           if (matchedWithHit[i] == 1) {
             marker.SetMarkerColor(kBlack);
           }
           else {
             // marker.SetMarkerColor(kRed);
           }
           // marker.DrawMarker(lx[i], ly[i]);
         }
       }
   
       /// debugging
       // {
       //   // print track parameters
       //   std::cout << std::string(80, '-') << std::endl;
       //   std::cout << "Track momentum: " << mcTrk.GetP() << std::endl;
       //   std::cout << "Track PDG code: " << mcTrk.GetPdgCode() << std::endl;
       //   std::cout << "Track mother ID: " << mcTrk.GetMotherId() << std::endl;
       //   // std::cout << "Number of points: " << mcTrk.GetNofPoints() << std::endl;
       //   std::cout << "Number of hits: " << mcTrk.GetNofHits() << std::endl;
       //   // std::cout << "Is Additional: " << mcTrk.IsAdditional() << std::endl;
       //   // start coordinates
       //   std::cout << "Start coordinates: x = " << mcTrk.GetStartX() << " y = " << mcTrk.GetStartY()
       //             << " z = " << mcTrk.GetStartZ() << std::endl;
       //   // slope
       //   std::cout << "Slope: tx = " << mcTrk.GetTx() << " ty = " << mcTrk.GetTy() << std::endl;
       //   // num touch tracks
       //   std::cout << "Number of touch tracks: " << mcTrk.GetNofTouchTracks() << std::endl;
       //   // num clones
       //   std::cout << "Number of clones: " << mcTrk.GetNofClones() << std::endl;
       //   // id
       //   std::cout << "ID: " << mcTrk.GetId() << std::endl;
       //   // print hit indexes
       //   std::cout << "Hit indexes: ";
       //   for (auto ih : mcTrk.GetHitIndexes()) {
       //     std::cout << ih << " ";
       //   }
       //   std::cout << std::endl;
       // }
   
     }  // over MC tracks
   
     /// draw associated reco tracks
     // for (const auto& mcTrk : mcData.GetTrackContainer()) {
     //   // select tracks of interest
     //   std::vector<int> ids = {114, 427, 438, 777, 882};
     //   bool interest = false;
     //   for ( auto id : ids ) {
     //     if (mcTrk.GetId() == id) {
     //       interest = true;
     //       break;
     //     }
     //   }
     //   if (!interest) continue;
   
     //   pline.SetLineColor(kBlue);
     //   std::cout << "Number of associated reco tracks: " << mcTrk.GetRecoTrackIndexes().size() << std::endl;
     //   for (auto ireco : mcTrk.GetRecoTrackIndexes()) {
     //     vector<double> lx1, ly1, lz1;
     //     const auto& recoTrack = L1.fvRecoTracks[ireco];
     //     for (unsigned int i = 0; i < recoTrack.Hits.size(); i++) {
     //       const ca::Hit& h = algo->GetInputData().GetHit(recoTrack.Hits[i]);
     //       lx1.push_back(h.X());
     //       ly1.push_back(h.Y());
     //       lz1.push_back(h.Z());
     //     }
   
     //     YZ->cd();
     //     pline.DrawPolyLine(lx1.size(), &(lz1[0]), &(ly1[0]));
     //     XZ->cd();
     //     pline.DrawPolyLine(lx1.size(), &(lz1[0]), &(lx1[0]));
     //     YX->cd();
     //     pline.DrawPolyLine(lx1.size(), &(lx1[0]), &(ly1[0]));
     //   }
     // }
   
     // debugging : print map
     // for (auto& it : trkHitCount) {
     //   std::cout << "Number of tracks with " << it.first << " hits: " << it.second << std::endl;
     // }
   
     LOG(info) << "L1CADraw: number of registered MC tracks: " << NRegMCTracks;
     LOG(info) << "nRecotracks associated: " << nRecoTracks;
   
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   }
   
   /// draw hits which have been used by reco tracks found in first two iterations
   void L1AlgoDraw::DrawUsedHits()
   {
     LOG(info) << "L1AlgoDraw: DrawUsedHits";
     int nUsedHits = 0;
   
     TMarker marker;
     marker.SetMarkerColor(9);
     marker.SetMarkerStyle(20);  // closed circle
     marker.SetMarkerSize(2 * HitSize);
   
     const auto& remainingHitsWindow = algo->remainingHitsIds;  // fWindowHits
     std::vector<int> remainingHits;                            // in fInputData
     for (auto ih : remainingHitsWindow) {
       remainingHits.push_back(algo->fWindowHits[ih].Id());
     }
   
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
     for (const auto& mcTrk : mcData.GetTrackContainer()) {
       // draw reconstructable tracks only
       if (!mcTrk.IsReconstructable()) continue;
       if (mcTrk.GetNofHits() < 4) continue;
       if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
       if (mcTrk.GetP() < 0.2) continue;
   
       /// skip tracks not found in GNN iteration
       bool found_gnn_iter = true;
       for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
         const auto& rTrk = L1.fvRecoTracks[irt];
         if (rTrk.fFoundInIteration != 3) {
           found_gnn_iter = false;
           break;
         }
       }
       if (!found_gnn_iter) continue;
   
       /// draw all hits corresponding to points of MC tracks which have been used before GNN iteration
       for (int ip : mcTrk.GetPointIndexes()) {
         auto& point     = mcData.GetPoint(ip);
         auto hitIndexes = point.GetHitIndexes();  // in fInputData
         for (auto ih : hitIndexes) {
           auto it = std::find(remainingHits.begin(), remainingHits.end(), ih);
           if (it != remainingHits.end()) continue;
           nUsedHits++;
           const auto& h = algo->GetInputData().GetHit(ih);
           YZ->cd();
           marker.DrawMarker(h.Z(), h.Y());
           XZ->cd();
           marker.DrawMarker(h.Z(), h.X());
           YX->cd();
           marker.DrawMarker(h.X(), h.Y());
         }
       }  // points in track
     }  // tracks
   
     LOG(info) << "L1AlgoDraw: number of used hits: " << nUsedHits;
   }
   
   
   /// difference between angle difference of two overlapping triplets in a track
   void L1AlgoDraw::drawOverlapTripletAngleDiffDistMC()
   {
     std::cout << "L1AlgoDraw: OverlapTripletAngleDiffDistMCLastIter" << std::endl;
     const std::string baseDir = "/u/otyagi/cbmroot/macro/run/L1CADraw/distributions/";
     std::vector<double> angleDiffYZ;
     std::vector<double> angleDiffXZ;
     std::vector<double> trackMom;
     std::vector<int> particleCharge;
   
     /// read data from file
     const std::string fileName = baseDir + "data/gnn_iter_overlap_triplet_angle_diff_data_ev_0_100.txt";
     // const std::string fileName = baseDir + "data/fastPrim_overlap_triplet_angle_diff_data_ev_0_100.txt";
     std::ifstream file(fileName);
     if (!file.is_open()) {
       std::cerr << "Error: unable to open file: " << fileName << std::endl;
       return;
     }
   
     // file format: Event angleDiffYZ angleDiffXZ trackMom particleCharge
     // read until end of file and store data
     // angles in radians
     int event;
     double angleYZ, angleXZ, mom;
     int charge;
     while (file >> event >> angleYZ >> angleXZ >> mom >> charge) {
       // radians to degrees
       angleYZ *= OT_RAD_TO_DEGREE;
       angleXZ *= OT_RAD_TO_DEGREE;
       angleDiffYZ.push_back(angleYZ);
       angleDiffXZ.push_back(angleXZ);
       trackMom.push_back(mom);
       particleCharge.push_back(charge);
     }
     file.close();
   
     /// print max and min values
     std::cout << "Max angle diff XZ: " << *std::max_element(angleDiffXZ.begin(), angleDiffXZ.end()) << std::endl;
     std::cout << "Min angle diff XZ: " << *std::min_element(angleDiffXZ.begin(), angleDiffXZ.end()) << std::endl;
   
     double yMin     = -20.0f;  //-5.0 * OT_RAD_TO_DEGREE;
     double yMax     = 20.0f;   //5.0 * OT_RAD_TO_DEGREE;
     double xMin     = -20.0f;  //-5.0 * OT_RAD_TO_DEGREE;
     double xMax     = 20.0f;   //5.0 * OT_RAD_TO_DEGREE;
     const int nBins = 100;
   
     /// Find dependance of angle difference on track momentum.
     /// Plot YZ angles - p
     {
       TCanvas* c1 = new TCanvas("c1", "Y-Z Triplet diff Angle Diff vs Track Momentum", 0, 0, 800, 800);
       c1->SetGrid();
       c1->SetFillColor(0);
       c1->SetFrameFillColor(0);
       c1->SetFrameBorderMode(0);
       c1->SetFrameBorderSize(1);
       c1->SetFrameFillColor(0);
       c1->SetLeftMargin(0.1);
   
       /// create two graphs with positive and negative charges
       std::vector<double> angleDiffYZ_pos;
       std::vector<double> mom_pos;
       std::vector<double> angleDiffYZ_neg;
       std::vector<double> mom_neg;
       for (unsigned int i = 0; i < angleDiffYZ.size(); i++) {
         if (particleCharge[i] > 0) {
           angleDiffYZ_pos.push_back(angleDiffYZ[i]);
           mom_pos.push_back(trackMom[i]);
         }
         else {
           angleDiffYZ_neg.push_back(angleDiffYZ[i]);
           mom_neg.push_back(trackMom[i]);
         }
       }
   
       // positive charge graph
       TGraph* gr1_pos = new TGraph(angleDiffYZ_pos.size(), &(mom_pos[0]), &(angleDiffYZ_pos[0]));
       gr1_pos->SetMarkerStyle(20);
       gr1_pos->SetMarkerSize(0.5);
       gr1_pos->SetMarkerColor(kRed);
       gr1_pos->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr1_pos->GetYaxis()->SetTitle("Triplet diff Angle Diff Y-Z (degree)");
       gr1_pos->GetYaxis()->SetTitleOffset(1.5);
       gr1_pos->SetLineColor(kRed);
       gr1_pos->SetTitle("");
   
       // negative charge graph
       TGraph* gr1_neg = new TGraph(angleDiffYZ_neg.size(), &(mom_neg[0]), &(angleDiffYZ_neg[0]));
       gr1_neg->SetMarkerStyle(20);
       gr1_neg->SetMarkerSize(0.5);
       gr1_neg->SetMarkerColor(kBlue);
       gr1_neg->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr1_neg->GetYaxis()->SetTitle("Triplet diff Angle Diff Y-Z (degree)");
       gr1_neg->GetYaxis()->SetTitleOffset(1.5);
       gr1_neg->SetLineColor(kBlue);
       gr1_neg->SetTitle("");
   
       /// set y range
       gr1_pos->GetYaxis()->SetRangeUser(yMin, yMax);
       gr1_neg->GetYaxis()->SetRangeUser(yMin, yMax);
   
       c1->cd();
       gr1_pos->Draw("AP");
       gr1_neg->Draw("P same");
       c1->Update();  // Force the canvas to update and redraw the stats box
   
       // Add a custom title
       TPaveText* title = new TPaveText(0.1, 0.92, 0.9, 0.98, "brNDC");
       title->AddText("Triplet diff Angle Diff Y-Z vs Track Momentum");
       title->SetFillColor(0);   // Transparent background
       title->SetTextAlign(22);  // Center-align
       title->SetTextSize(0.04);
       title->Draw();
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.7, 0.7, 0.9, 0.9);
       legend->AddEntry(gr1_pos, "Positive", "l");
       legend->AddEntry(gr1_neg, "Negative", "l");
       legend->SetTextSize(0.03);
       legend->SetBorderSize(1);
       legend->Draw();
   
       const TString tmp = baseDir + "figs/gnn_iter_YZ_Triplet_diff_Angle_Diff_vs_Track_Momentum.pdf";
       c1->SaveAs(tmp);
       std::cout << "Triplet diff Angle diff vs track momentum saved to: " << tmp << std::endl;
   
       delete c1;
       delete gr1_pos;
       delete gr1_neg;
       delete legend;
       delete title;
     }
   
     /// Plot XZ angles - p
     {
       TCanvas* c2 = new TCanvas("c2", "X-Z Triplet diff Angle Diff vs Track Momentum", 0, 0, 800, 800);
       c2->SetGrid();
       c2->SetFillColor(0);
       c2->SetFrameFillColor(0);
       c2->SetFrameBorderMode(0);
       c2->SetFrameBorderSize(1);
       c2->SetFrameFillColor(0);
       c2->SetLeftMargin(0.1);
   
       /// create two graphs with positive and negative charges
       std::vector<double> angleDiffXZ_pos;
       std::vector<double> mom_pos;
       std::vector<double> angleDiffXZ_neg;
       std::vector<double> mom_neg;
       for (unsigned int i = 0; i < angleDiffXZ.size(); i++) {
         if (particleCharge[i] > 0) {
           angleDiffXZ_pos.push_back(angleDiffXZ[i]);
           mom_pos.push_back(trackMom[i]);
         }
         else {
           angleDiffXZ_neg.push_back(angleDiffXZ[i]);
           mom_neg.push_back(trackMom[i]);
         }
       }
   
       // positive charge graph
       TGraph* gr2_pos = new TGraph(angleDiffXZ_pos.size(), &(mom_pos[0]), &(angleDiffXZ_pos[0]));
       gr2_pos->SetMarkerStyle(20);
       gr2_pos->SetMarkerSize(0.5);
       gr2_pos->SetMarkerColor(kRed);
       gr2_pos->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr2_pos->GetYaxis()->SetTitle("Triplet diff Angle Diff X-Z (degree)");
       gr2_pos->GetYaxis()->SetTitleOffset(1.5);
       gr2_pos->SetLineColor(kRed);
       gr2_pos->SetTitle("");
   
       // negative charge graph
       TGraph* gr2_neg = new TGraph(angleDiffXZ_neg.size(), &(mom_neg[0]), &(angleDiffXZ_neg[0]));
       gr2_neg->SetMarkerStyle(20);
       gr2_neg->SetMarkerSize(0.5);
       gr2_neg->SetMarkerColor(kBlue);
       gr2_neg->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr2_neg->GetYaxis()->SetTitle("Triplet diff Angle Diff X-Z (degree)");
       gr2_neg->GetYaxis()->SetTitleOffset(1.5);
       gr2_neg->SetLineColor(kBlue);
       gr2_neg->SetTitle("");
   
       /// set y range
       gr2_pos->GetYaxis()->SetRangeUser(yMin, yMax);
       gr2_neg->GetYaxis()->SetRangeUser(yMin, yMax);
   
       c2->cd();
       gr2_pos->Draw("AP");
       gr2_neg->Draw("P same");
       c2->Update();  // Force the canvas to update and redraw the stats box
   
       // Add a custom title
       TPaveText* title = new TPaveText(0.1, 0.92, 0.9, 0.98, "brNDC");
       title->AddText("Triplet diff Angle Diff X-Z vs Track Momentum");
       title->SetFillColor(0);   // Transparent background
       title->SetTextAlign(22);  // Center-align
       title->SetTextSize(0.04);
       title->Draw();
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.7, 0.7, 0.9, 0.9);
       legend->AddEntry(gr2_pos, "Positive", "l");
       legend->AddEntry(gr2_neg, "Negative", "l");
       legend->SetTextSize(0.03);
       legend->SetBorderSize(1);
       legend->Draw();
   
       const TString tmp = baseDir + "figs/gnn_iter_XZ_Triplet_diff_Angle_Diff_vs_Track_Momentum.pdf";
       c2->SaveAs(tmp);
   
       std::cout << "triplet diff Angle diff vs track momentum saved to: " << tmp << std::endl;
   
       delete c2;
       delete gr2_pos;
       delete gr2_neg;
       delete legend;
       delete title;
     }
   
     // Configure the statistics box of histogram
     gStyle->SetOptStat("nemou");  // https://root.cern.ch/doc/master/classTPaveStats.html
   
     /// Plot YZ angle difference histogram
     {
       TCanvas* c = new TCanvas("c3", "YZ Triplet diff Angle Diff Histogram", 0, 0, 1600, 1200);
       c->SetFillColor(0);
       c->SetFrameFillColor(0);
       c->SetFrameBorderMode(0);
       c->SetFrameBorderSize(1);
       c->SetFrameFillColor(0);
   
       TH1F* h1     = new TH1F("Total", "YZ Triplet diff Angle Diff", nBins, xMin, xMax);
       TH1F* h1_pos = new TH1F("Pos", "YZ Triplet diff Angle Diff (Positive Charge)", nBins, xMin, xMax);
       TH1F* h1_neg = new TH1F("Neg", "YZ Triplet diff Angle Diff (Negative Charge)", nBins, xMin, xMax);
       h1->SetLineColor(kBlack);
       h1_pos->SetLineColor(kRed);
       h1_neg->SetLineColor(kBlue);
   
       for (unsigned int i = 0; i < angleDiffYZ.size(); i++) {
         h1->Fill(angleDiffYZ[i]);
         if (particleCharge[i] > 0) {
           h1_pos->Fill(angleDiffYZ[i]);
         }
         else {
           h1_neg->Fill(angleDiffYZ[i]);
         }
       }
   
       // log scale y axis
       c->SetLogy();
       // c->SetLogx();
   
       c->cd();
       h1->Draw();
       h1_pos->Draw("same");
       h1_neg->Draw("same");
       c->Update();  // Force the canvas to update and redraw the stats box
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.1, 0.8, 0.2, 0.9);
       legend->AddEntry(h1, "Total", "l");
       legend->AddEntry(h1_pos, "Positive", "l");
       legend->AddEntry(h1_neg, "Negative", "l");
       legend->SetTextSize(0.02);
       legend->SetBorderSize(1);
       legend->Draw();
   
       h1->GetXaxis()->SetTitle("Triplet diff Angle Diff Y-Z (degree)");
       h1->GetYaxis()->SetTitle("Number of Entries");
   
       const TString tmp = baseDir + "figs/gnn_iter_YZ_Triplet_diff_Angle_Diff_Histogram.pdf";
       c->SaveAs(tmp);
   
       std::cout << "Angle diff histogram saved to: " << tmp << std::endl;
   
       delete c;
       delete h1;
       delete h1_pos;
       delete h1_neg;
       delete legend;
     }
   
     /// Plot XZ angle difference histogram
     {
       TCanvas* c = new TCanvas("c4", "XZ Triplet diff Angle Diff Histogram", 0, 0, 1600, 1200);
       c->SetFillColor(0);
       c->SetFrameFillColor(0);
       c->SetFrameBorderMode(0);
       c->SetFrameBorderSize(1);
       c->SetFrameFillColor(0);
   
       TH1F* h2     = new TH1F("Total", "XZ Triplet diff Angle Diff", nBins, xMin, xMax);
       TH1F* h2_pos = new TH1F("Pos", "XZ Triplet diff Angle Diff (Positive Charge)", nBins, xMin, xMax);
       TH1F* h2_neg = new TH1F("Neg", "XZ Triplet diff Angle Diff (Negative Charge)", nBins, xMin, xMax);
       h2->SetLineColor(kBlack);
       h2_pos->SetLineColor(kRed);
       h2_neg->SetLineColor(kBlue);
   
       for (unsigned int i = 0; i < angleDiffXZ.size(); i++) {
         h2->Fill(angleDiffXZ[i]);
         if (particleCharge[i] > 0) {
           h2_pos->Fill(angleDiffXZ[i]);
         }
         else {
           h2_neg->Fill(angleDiffXZ[i]);
         }
       }
   
       // log scale
       c->SetLogy();
       // c->SetLogx();
   
       c->cd();
       h2->Draw();
       h2_pos->Draw("same");
       h2_neg->Draw("same");
       c->Update();  // Force the canvas to update and redraw the stats box
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.1, 0.8, 0.2, 0.9);
       legend->AddEntry(h2, "Total", "l");
       legend->AddEntry(h2_pos, "Positive", "l");
       legend->AddEntry(h2_neg, "Negative", "l");
       legend->SetTextSize(0.02);
       legend->SetBorderSize(1);
       legend->Draw();
   
       h2->GetXaxis()->SetTitle("Triplet diff Angle Diff X-Z (degree)");
       h2->GetYaxis()->SetTitle("Number of Entries");
   
       const TString tmp = baseDir + "figs/gnn_iter_XZ_Triplet_diff_Angle_Diff_Histogram.pdf";
       c->SaveAs(tmp);
   
       std::cout << "Triplet diff Angle diff histogram saved to: " << tmp << std::endl;
   
       delete c;
       delete h2;
       delete h2_pos;
       delete h2_neg;
       delete legend;
     }
   
     /// back to default
     gStyle->SetOptStat(1111);
   }
   
   
   /// similar to SlopeDistMCLastIter, but for angles not slopes
   void L1AlgoDraw::drawAngleDiffDistMC()
   {
     std::cout << "L1AlgoDraw: AngleDistMCLastIter" << std::endl;
     const std::string baseDir = "/u/otyagi/cbmroot/macro/run/L1CADraw/distributions/";
     std::vector<double> angleDiffYZ;
     std::vector<double> angleDiffXZ;
     std::vector<double> trackMom;
     std::vector<int> particleCharge;
   
     /// read data from file
     // const std::string fileName = baseDir + "data/gnn_iter_angle_diff_data_ev_0_100.txt";
     const std::string fileName = baseDir + "data/fastPrim_angle_diff_data_ev_0_100.txt";
   
     std::ifstream file(fileName);
     if (!file.is_open()) {
       std::cerr << "Error: unable to open file: " << fileName << std::endl;
       return;
     }
   
     // file format: Event angleDiffYZ angleDiffXZ trackMom particleCharge
     // read until end of file and store data
     int event;
     double angleYZ, angleXZ, mom;
     int charge;
     while (file >> event >> angleYZ >> angleXZ >> mom >> charge) {
       angleDiffYZ.push_back(angleYZ);
       angleDiffXZ.push_back(angleXZ);
       trackMom.push_back(mom);
       particleCharge.push_back(charge);
     }
     file.close();
   
     /// Find dependance of angle difference on track momentum.
     /// Plot YZ angles - p
     {
       TCanvas* c1 = new TCanvas("c1", "Y-Z Angle Diff vs Track Momentum", 0, 0, 800, 800);
       c1->SetGrid();
       c1->SetFillColor(0);
       c1->SetFrameFillColor(0);
       c1->SetFrameBorderMode(0);
       c1->SetFrameBorderSize(1);
       c1->SetFrameFillColor(0);
       c1->SetLeftMargin(0.1);
   
       /// create two graphs with positive and negative charges
       std::vector<double> angleDiffYZ_pos;
       std::vector<double> mom_pos;
       std::vector<double> angleDiffYZ_neg;
       std::vector<double> mom_neg;
       for (unsigned int i = 0; i < angleDiffYZ.size(); i++) {
         if (particleCharge[i] > 0) {
           angleDiffYZ_pos.push_back(angleDiffYZ[i]);
           mom_pos.push_back(trackMom[i]);
         }
         else {
           angleDiffYZ_neg.push_back(angleDiffYZ[i]);
           mom_neg.push_back(trackMom[i]);
         }
       }
   
       // positive charge graph
       TGraph* gr1_pos = new TGraph(angleDiffYZ_pos.size(), &(mom_pos[0]), &(angleDiffYZ_pos[0]));
       gr1_pos->SetMarkerStyle(20);
       gr1_pos->SetMarkerSize(0.5);
       gr1_pos->SetMarkerColor(kRed);
       gr1_pos->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr1_pos->GetYaxis()->SetTitle("Angle Diff Y-Z (rad)");
       gr1_pos->GetYaxis()->SetTitleOffset(1.5);
       gr1_pos->SetLineColor(kRed);
   
       // negative charge graph
       TGraph* gr1_neg = new TGraph(angleDiffYZ_neg.size(), &(mom_neg[0]), &(angleDiffYZ_neg[0]));
       gr1_neg->SetMarkerStyle(20);
       gr1_neg->SetMarkerSize(0.5);
       gr1_neg->SetMarkerColor(kBlue);
       gr1_neg->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr1_neg->GetYaxis()->SetTitle("Angle Diff Y-Z (rad)");
       gr1_neg->GetYaxis()->SetTitleOffset(1.5);
       gr1_neg->SetLineColor(kBlue);
       gr1_neg->SetTitle("");
   
       /// set y range
       double yMin = -0.4;  // [-3.2, 3.2] for gnn_iter
       double yMax = 0.4;
       gr1_pos->GetYaxis()->SetRangeUser(yMin, yMax);
       gr1_neg->GetYaxis()->SetRangeUser(yMin, yMax);
   
       c1->cd();
       gr1_pos->Draw("AP");
       gr1_neg->Draw("P same");
       c1->Update();  // Force the canvas to update and redraw the stats box
   
       // Add a custom title
       TPaveText* title = new TPaveText(0.1, 0.92, 0.9, 0.98, "brNDC");
       title->AddText("Angle Diff Y-Z vs Track Momentum");
       title->SetFillColor(0);   // Transparent background
       title->SetTextAlign(22);  // Center-align
       title->SetTextSize(0.04);
       title->Draw();
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.7, 0.7, 0.9, 0.9);
       legend->AddEntry(gr1_pos, "Positive", "l");
       legend->AddEntry(gr1_neg, "Negative", "l");
       legend->SetTextSize(0.03);
       legend->SetBorderSize(1);
       legend->Draw();
   
       const TString tmp = baseDir + "figs/fastPrim_YZ_Angle_Diff_vs_Track_Momentum.pdf";
       c1->SaveAs(tmp);
       std::cout << "Angle diff vs track momentum saved to: " << tmp << std::endl;
   
       delete c1;
       delete gr1_pos;
       delete gr1_neg;
       delete legend;
       delete title;
     }
   
     /// Plot XZ angles - p
     {
       TCanvas* c2 = new TCanvas("c2", "X-Z Angle Diff vs Track Momentum", 0, 0, 800, 800);
       c2->SetGrid();
       c2->SetFillColor(0);
       c2->SetFrameFillColor(0);
       c2->SetFrameBorderMode(0);
       c2->SetFrameBorderSize(1);
       c2->SetFrameFillColor(0);
       c2->SetLeftMargin(0.1);
   
       /// create two graphs with positive and negative charges
       std::vector<double> angleDiffXZ_pos;
       std::vector<double> mom_pos;
       std::vector<double> angleDiffXZ_neg;
       std::vector<double> mom_neg;
       for (unsigned int i = 0; i < angleDiffXZ.size(); i++) {
         if (particleCharge[i] > 0) {
           angleDiffXZ_pos.push_back(angleDiffXZ[i]);
           mom_pos.push_back(trackMom[i]);
         }
         else {
           angleDiffXZ_neg.push_back(angleDiffXZ[i]);
           mom_neg.push_back(trackMom[i]);
         }
       }
   
       // positive charge graph
       TGraph* gr2_pos = new TGraph(angleDiffXZ_pos.size(), &(mom_pos[0]), &(angleDiffXZ_pos[0]));
       gr2_pos->SetMarkerStyle(20);
       gr2_pos->SetMarkerSize(0.5);
       gr2_pos->SetMarkerColor(kRed);
       gr2_pos->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr2_pos->GetYaxis()->SetTitle("Angle Diff X-Z (rad)");
       gr2_pos->GetYaxis()->SetTitleOffset(1.5);
       gr2_pos->SetLineColor(kRed);
   
       // negative charge graph
       TGraph* gr2_neg = new TGraph(angleDiffXZ_neg.size(), &(mom_neg[0]), &(angleDiffXZ_neg[0]));
       gr2_neg->SetMarkerStyle(20);
       gr2_neg->SetMarkerSize(0.5);
       gr2_neg->SetMarkerColor(kBlue);
       gr2_neg->GetXaxis()->SetTitle("Track Momentum (GeV/c)");
       gr2_neg->GetYaxis()->SetTitle("Angle Diff X-Z (rad)");
       gr2_neg->GetYaxis()->SetTitleOffset(1.5);
       gr2_neg->SetLineColor(kBlue);
       gr2_neg->SetTitle("");
   
   
       /// set y range
       double yMin = -0.4;  // [-3.2, 3.2] for gnn_iter
       double yMax = 0.4;
       gr2_pos->GetYaxis()->SetRangeUser(yMin, yMax);
       gr2_neg->GetYaxis()->SetRangeUser(yMin, yMax);
   
       c2->cd();
       gr2_pos->Draw("AP");
       gr2_neg->Draw("P same");
       c2->Update();  // Force the canvas to update and redraw the stats box
   
       // Add a custom title
       TPaveText* title = new TPaveText(0.1, 0.92, 0.9, 0.98, "brNDC");
       title->AddText("Angle Diff X-Z vs Track Momentum");
       title->SetFillColor(0);   // Transparent background
       title->SetTextAlign(22);  // Center-align
       title->SetTextSize(0.04);
       title->Draw();
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.7, 0.7, 0.9, 0.9);
       legend->AddEntry(gr2_pos, "Positive", "l");
       legend->AddEntry(gr2_neg, "Negative", "l");
       legend->SetTextSize(0.03);
       legend->SetBorderSize(1);
       legend->Draw();
   
       const TString tmp = baseDir + "figs/fastPrim_XZ_Angle_Diff_vs_Track_Momentum.pdf";
       c2->SaveAs(tmp);
   
       std::cout << "Angle diff vs track momentum saved to: " << tmp << std::endl;
   
       delete c2;
       delete gr2_pos;
       delete gr2_neg;
       delete legend;
       delete title;
     }
   
     // Configure the statistics box of histogram
     gStyle->SetOptStat("nemou");  // https://root.cern.ch/doc/master/classTPaveStats.html
   
     /// Plot YZ angle difference histogram
     {
       TCanvas* c = new TCanvas("c3", "YZ Angle Diff Histogram", 0, 0, 1600, 1200);
       c->SetFillColor(0);
       c->SetFrameFillColor(0);
       c->SetFrameBorderMode(0);
       c->SetFrameBorderSize(1);
       c->SetFrameFillColor(0);
   
       const int nBins  = 100;
       const float xMin = -0.4;  // [-3.2, 3.2] for gnn_iter
       const float xMax = 0.4;
       TH1F* h1         = new TH1F("Total", "YZ Angle Diff", nBins, xMin, xMax);
       TH1F* h1_pos     = new TH1F("Pos", "YZ Angle Diff (Positive Charge)", nBins, xMin, xMax);
       TH1F* h1_neg     = new TH1F("Neg", "YZ Angle Diff (Negative Charge)", nBins, xMin, xMax);
       h1->SetLineColor(kBlack);
       h1_pos->SetLineColor(kRed);
       h1_neg->SetLineColor(kBlue);
   
       for (unsigned int i = 0; i < angleDiffYZ.size(); i++) {
         h1->Fill(angleDiffYZ[i]);
         if (particleCharge[i] > 0) {
           h1_pos->Fill(angleDiffYZ[i]);
         }
         else {
           h1_neg->Fill(angleDiffYZ[i]);
         }
       }
   
       // log scale y axis
       c->SetLogy();
   
       c->cd();
       h1->Draw();
       h1_pos->Draw("same");
       h1_neg->Draw("same");
       c->Update();  // Force the canvas to update and redraw the stats box
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.1, 0.8, 0.2, 0.9);
       legend->AddEntry(h1, "Total", "l");
       legend->AddEntry(h1_pos, "Positive", "l");
       legend->AddEntry(h1_neg, "Negative", "l");
       legend->SetTextSize(0.02);
       legend->SetBorderSize(1);
       legend->Draw();
   
       h1->GetXaxis()->SetTitle("Angle Diff Y-Z (rad)");
       h1->GetYaxis()->SetTitle("Number of Entries");
   
       const TString tmp = baseDir + "figs/fastPrim_YZ_Angle_Diff_Histogram.pdf";
       c->SaveAs(tmp);
   
       std::cout << "Angle diff histogram saved to: " << tmp << std::endl;
   
       delete c;
       delete h1;
       delete h1_pos;
       delete h1_neg;
       delete legend;
     }
   
     /// Plot XZ angle difference histogram
     {
       TCanvas* c = new TCanvas("c4", "XZ Angle Diff Histogram", 0, 0, 1600, 1200);
       c->SetFillColor(0);
       c->SetFrameFillColor(0);
       c->SetFrameBorderMode(0);
       c->SetFrameBorderSize(1);
       c->SetFrameFillColor(0);
   
       const int nBins  = 100;
       const float xMin = -0.4f;  // [-3.2, 3.2] for gnn_iter
       const float xMax = 0.4f;
       TH1F* h2         = new TH1F("Total", "XZ Angle Diff", nBins, xMin, xMax);
       TH1F* h2_pos     = new TH1F("Pos", "XZ Angle Diff (Positive Charge)", nBins, xMin, xMax);
       TH1F* h2_neg     = new TH1F("Neg", "XZ Angle Diff (Negative Charge)", nBins, xMin, xMax);
       h2->SetLineColor(kBlack);
       h2_pos->SetLineColor(kRed);
       h2_neg->SetLineColor(kBlue);
   
       for (unsigned int i = 0; i < angleDiffXZ.size(); i++) {
         h2->Fill(angleDiffXZ[i]);
         if (particleCharge[i] > 0) {
           h2_pos->Fill(angleDiffXZ[i]);
         }
         else {
           h2_neg->Fill(angleDiffXZ[i]);
         }
       }
   
       // log scale y axis
       c->SetLogy();
   
       c->cd();
       h2->Draw();
       h2_pos->Draw("same");
       h2_neg->Draw("same");
       c->Update();  // Force the canvas to update and redraw the stats box
   
       // Create and draw a legend
       TLegend* legend = new TLegend(0.1, 0.8, 0.2, 0.9);
       legend->AddEntry(h2, "Total", "l");
       legend->AddEntry(h2_pos, "Positive", "l");
       legend->AddEntry(h2_neg, "Negative", "l");
       legend->SetTextSize(0.02);
       legend->SetBorderSize(1);
       legend->Draw();
   
       h2->GetXaxis()->SetTitle("Angle Diff X-Z (rad)");
       h2->GetYaxis()->SetTitle("Number of Entries");
   
       const TString tmp = baseDir + "figs/fastPrim_XZ_Angle_Diff_Histogram.pdf";
       c->SaveAs(tmp);
   
       std::cout << "Angle diff histogram saved to: " << tmp << std::endl;
   
       delete c;
       delete h2;
       delete h2_pos;
       delete h2_neg;
       delete legend;
     }
   
     /// back to default
     gStyle->SetOptStat(1111);
   }
   
   
   void L1AlgoDraw::SlopeDistMCLastIter()
   {
     std::cout << "L1AlgoDraw: HistoYZSlopeMCLastIter" << std::endl;
   
     std::vector<double> slopesRatioYZ;
     std::vector<double> slopesRatioXZ;
     std::vector<double> trackMom;
   
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
     for (const auto& mcTrk : mcData.GetTrackContainer()) {
       // draw reconstructable tracks only
       if (!mcTrk.IsReconstructable()) continue;
       /// skip tracks not found in GNN iteration
       bool found_gnn_iter = true;
       for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
         const auto& rTrk = L1.fvRecoTracks[irt];
         if (rTrk.fFoundInIteration != 3) {
           found_gnn_iter = false;
           break;
         }
       }
       if (!found_gnn_iter) continue;
   
       /// min 4 hits
       if (mcTrk.GetNofHits() < 4) continue;
       /// no jump tracks
       if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
   
       /// calculate slope ratio of successive edges from a track in YZ plane
       for (int i1 = 0; i1 < mcTrk.GetNofPoints() - 2; i1++) {
         auto& point1 = mcData.GetPoint(mcTrk.GetPointIndexes()[i1]);
         // get hit corresponding to points
         auto hitIndexes = point1.GetHitIndexes();
         if (hitIndexes.size() == 0) continue;
         auto ih        = hitIndexes[0];
         const auto& h1 = algo->GetInputData().GetHit(ih);
   
         auto& point2     = mcData.GetPoint(mcTrk.GetPointIndexes()[i1 + 1]);
         auto hitIndexes2 = point2.GetHitIndexes();
         if (hitIndexes2.size() == 0) continue;
         auto ih2       = hitIndexes2[0];
         const auto& h2 = algo->GetInputData().GetHit(ih2);
   
         auto& point3     = mcData.GetPoint(mcTrk.GetPointIndexes()[i1 + 2]);
         auto hitIndexes3 = point3.GetHitIndexes();
         if (hitIndexes3.size() == 0) continue;
         auto ih3       = hitIndexes3[0];
         const auto& h3 = algo->GetInputData().GetHit(ih3);
   
         // YZ slope
         double slope1YZ = (h2.Y() - h1.Y()) / (h2.Z() - h1.Z());
         double slope2YZ = (h3.Y() - h2.Y()) / (h3.Z() - h2.Z());
         slopesRatioYZ.push_back(slope1YZ / slope2YZ);
         // XZ slope
         double slope1XZ = (h2.X() - h1.X()) / (h2.Z() - h1.Z());
         double slope2XZ = (h3.X() - h2.X()) / (h3.Z() - h2.Z());
         slopesRatioXZ.push_back(slope1XZ / slope2XZ);
   
         trackMom.push_back(mcTrk.GetP());
       }  // points in track
     }  // tracks
     std::cout << "Number of YZ slopes: " << slopesRatioYZ.size() << std::endl;
     std::cout << "Number of XZ slopes: " << slopesRatioXZ.size() << std::endl;
   
     /// Find dependance of slope on track momentum.
     /// Plot YZ slopes
     {
       TCanvas* c1 = new TCanvas("c1", "Y-Z Slope Ratio vs Track Momentum", 0, 0, 800, 800);
       c1->SetGrid();
       c1->SetFillColor(0);
       c1->SetFrameFillColor(0);
       c1->SetFrameBorderMode(0);
       c1->SetFrameBorderSize(1);
       c1->SetFrameFillColor(0);
   
       TGraph* gr1 = new TGraph(slopesRatioYZ.size(), &(trackMom[0]), &(slopesRatioYZ[0]));
       gr1->SetMarkerStyle(20);
       gr1->SetMarkerSize(1.0);
       gr1->SetMarkerColor(kBlue);
       gr1->GetXaxis()->SetTitle("Track Momentum");
       gr1->GetYaxis()->SetTitle("Ratio Slope Y-Z");
       gr1->SetTitle("Ratio Slope Y-Z vs Track Momentum");
   
       c1->cd();
       gr1->Draw("AP");
   
       const TString tmp = "/u/otyagi/cbmroot/macro/run/L1CADraw/distributions/YZ_Slope_Ratio_vs_Track_Momentum.pdf";
       c1->SaveAs(tmp);
   
       std::cout << "Slope ratio vs track momentum saved to: " << tmp << std::endl;
   
       delete c1;
       delete gr1;
     }
   
     /// Plot XZ
     {
       TCanvas* c2 = new TCanvas("c2", "X-Z Slope Ratio vs Track Momentum", 0, 0, 800, 800);
       c2->SetGrid();
       c2->SetFillColor(0);
       c2->SetFrameFillColor(0);
       c2->SetFrameBorderMode(0);
       c2->SetFrameBorderSize(1);
       c2->SetFrameFillColor(0);
   
       TGraph* gr2 = new TGraph(slopesRatioXZ.size(), &(trackMom[0]), &(slopesRatioXZ[0]));
       gr2->SetMarkerStyle(20);
       gr2->SetMarkerSize(1.0);
       gr2->SetMarkerColor(kBlue);
       gr2->GetXaxis()->SetTitle("Track Momentum");
       gr2->GetYaxis()->SetTitle("Ratio Slope X-Z");
       gr2->SetTitle("Ratio Slope X-Z vs Track Momentum");
   
       c2->cd();
       gr2->Draw("AP");
   
       const TString tmp = "/u/otyagi/cbmroot/macro/run/L1CADraw/distributions/XZ_Slope_Ratio_vs_Track_Momentum.pdf";
       c2->SaveAs(tmp);
   
       std::cout << "Slope ratio vs track momentum saved to: " << tmp << std::endl;
   
       delete c2;
       delete gr2;
     }
   
     // Configure the statistics box
     gStyle->SetOptStat("emou");  // https://root.cern.ch/doc/master/classTPaveStats.html
   
     /// Plot YZ slopes
     {
       TCanvas* c1 = new TCanvas("c1", "Y-Z Slope Ratio", 0, 0, 800, 800);
       c1->SetGrid();
       c1->SetFillColor(0);
       c1->SetFrameFillColor(0);
       c1->SetFrameBorderMode(0);
       c1->SetFrameBorderSize(1);
       c1->SetFrameFillColor(0);
   
       TH1F* h1 = new TH1F("h1", "Y-Z Slope Ratio", 100, -5, 5);  // (bins, min, max)
       h1->SetFillColor(38);
       h1->SetFillStyle(3001);
       h1->SetLineColor(1);
       h1->SetLineWidth(1);
       h1->SetMarkerStyle(20);
       h1->SetMarkerSize(0.5);
       h1->SetMarkerColor(1);
       h1->GetXaxis()->SetTitle("Ratio Slope Y-Z");
       h1->GetYaxis()->SetTitle("Number of tracks");
   
       for (auto& sr : slopesRatioYZ) {
         h1->Fill(sr);
       }
   
       // Create a legend
       TLegend* legend = new TLegend(0.7, 0.8, 0.9, 0.9);
       // Add an entry to the legend with the number of entries
       legend->AddEntry(h1, "Number of entries", "f");
       // Draw the legend
       legend->Draw();
   
       c1->cd();
       h1->Draw();
   
       const TString tmp = "/u/otyagi/cbmroot/macro/run/L1CADraw/distributions/YZ_Slope_Ratio.pdf";
       c1->SaveAs(tmp);
   
       std::cout << "Slope ratio histogram saved to: " << tmp << std::endl;
   
       delete c1;
       delete h1;
       delete legend;
     }
   
     /// Plot XZ slopes
     {
       TCanvas* c2 = new TCanvas("c2", "X-Z Slope Ratio", 0, 0, 800, 800);
       c2->SetGrid();
       c2->SetFillColor(0);
       c2->SetFrameFillColor(0);
       c2->SetFrameBorderMode(0);
       c2->SetFrameBorderSize(1);
       c2->SetFrameFillColor(0);
   
       TH1F* h2 = new TH1F("h2", "X-Z Slope Ratio", 100, -15, 15);  // (bins, min, max)
       h2->SetFillColor(38);
       h2->SetFillStyle(3001);
       h2->SetLineColor(1);
       h2->SetLineWidth(1);
       h2->SetMarkerStyle(20);
       h2->SetMarkerSize(0.5);
       h2->SetMarkerColor(1);
       h2->GetXaxis()->SetTitle("Ratio Slope X-Z");
       h2->GetYaxis()->SetTitle("Number of tracks");
   
       for (auto& sr : slopesRatioXZ) {
         h2->Fill(sr);
       }
   
       // Create a legend
       TLegend* legend = new TLegend(0.7, 0.8, 0.9, 0.9);
       // Add an entry to the legend with the number of entries
       legend->AddEntry(h2, "Number of entries", "f");
       // Draw the legend
       legend->Draw();
   
       c2->cd();
       h2->Draw();
   
       const TString tmp = "/u/otyagi/cbmroot/macro/run/L1CADraw/distributions/XZ_Slope_Ratio.pdf";
       c2->SaveAs(tmp);
   
       std::cout << "Slope ratio histogram saved to: " << tmp << std::endl;
   
       delete c2;
       delete h2;
       delete legend;
     }
   
     /// back to default
     gStyle->SetOptStat(1111);
   }
   
   
   void L1AlgoDraw::DrawRecoTracks()
   {
     int NRecTracks = 0;
     int curRecoHit = 0;
   
     CbmL1& L1 = *CbmL1::Instance();
   
     cbm::algo::ca::Vector<ca::HitIndex_t>& recoHits = algo->fRecoHits;
     for (vector<Track>::iterator it = algo->fRecoTracks.begin(); it != algo->fRecoTracks.end(); ++it) {
       Track& T  = *it;
       int nHits = T.fNofHits;
   
       /// OT added: draw only tracks found in last iteration
       if (T.fFoundInIteration != 3) {
         curRecoHit += nHits;
         continue;
       }
   
       /// draw only true tracks
       {
         // const auto& MatchedRecoTrack = L1.fvRecoTracks[T.fCbmL1RecoTrackIndex];
         // if (MatchedRecoTrack.fIsGhost != 0) {  /// skip if not true track
         //   curRecoHit += nHits;
         //   continue;
         // }
       }
   
       vector<double> lx, ly, lz;
       vector<double> lx_turned, ly_turned, lz_turned;
   
       TPolyLine pline;
       pline.SetLineColor(kBlue);
       pline.SetLineWidth(2);
       if (fVerbose >= 4) {
         cout << "hits = ";
       }
       for (int iHit = 0; iHit < nHits; iHit++) {
         unsigned int ih = recoHits[curRecoHit++];
         if (fVerbose >= 4) {
           cout << ih << " ";
         }
   
         Point p = GetHitCoor(ih);  // CHANGE THIS
         lx.push_back(p.x);
         ly.push_back(p.y);
         lz.push_back(p.z);
   
         // print point coordinates
         // cout << "x = " << p.x << " y = " << p.y << " z = " << p.z << " station = " << vHits[ih].Station() << endl;
   
         TVector3 v3(p.x, p.y, p.z);
         v3.RotateX(TMath::Pi() / 5);
         v3.RotateY(TMath::Pi() / 20);
         v3.RotateZ(TMath::Pi() / 100);
         lx_turned.push_back(v3.x());
         ly_turned.push_back(v3.y());
         lz_turned.push_back(v3.z());
       }
       // cout << std::string(80, '-') << endl;
   
       if (fVerbose >= 4) {
         cout << endl;
       }
   
       if (1) {
         NRecTracks++;
   
         YZ->cd();
         pline.DrawPolyLine(lz.size(), &(lz[0]), &(ly[0]));
         XZ->cd();
         pline.DrawPolyLine(lz.size(), &(lz[0]), &(lx[0]));
         YX->cd();
         pline.DrawPolyLine(lx.size(), &(lx[0]), &(ly[0]));
         XYZ->cd();
         pline.DrawPolyLine(lz_turned.size(), &(lz_turned[0]), &(lx_turned[0]));
       }
     }
   
     cout << "L1CADraw: number of reconstructed tracks: " << NRecTracks << endl;
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   
     XYZ->cd();
     XYZ->Update();
   }
   
   
   void L1AlgoDraw::DrawGhosts()
   {
     CbmL1& L1 = *CbmL1::Instance();
   
     LOG(info) << "Number of reco tracks: " << L1.fvRecoTracks.size();
     int nGhosts = 0;
   
     for (vector<CbmL1Track>::iterator it = L1.fvRecoTracks.begin(); it != L1.fvRecoTracks.end(); ++it) {
       CbmL1Track& T = *it;
       int nHits     = T.Hits.size();
   
       /// if not found in last iteration skip
       if (T.fFoundInIteration != 3) {
         continue;
       }
   
       // draw only ghosts
       if (T.fIsGhost != 1) continue;
   
       vector<double> lx, ly, lz;
       vector<double> lx_turned, ly_turned, lz_turned;
       TPolyLine pline;
       pline.SetLineColor(kGreen);
       pline.SetLineWidth(2);
       pline.SetLineStyle(1);
   
       for (int iHit = 0; iHit < nHits; iHit++) {
         unsigned int ih = T.Hits[iHit];
   
         Point p = GetHitCoor(ih);  // CHANGE THIS
         lx.push_back(p.x);
         ly.push_back(p.y);
         lz.push_back(p.z);
   
         TVector3 v3(p.x, p.y, p.z);
         v3.RotateX(TMath::Pi() / 5);
         v3.RotateY(TMath::Pi() / 20);
         v3.RotateZ(TMath::Pi() / 100);
         lx_turned.push_back(v3.x());
         ly_turned.push_back(v3.y());
         lz_turned.push_back(v3.z());
       }
   
       TMarker marker;
       marker.SetMarkerColor(kGreen);
       marker.SetMarkerStyle(24);  // 26 - open triangle, 24 - open circle
       marker.SetMarkerSize(1.5 * HitSize);
   
       YZ->cd();
       pline.DrawPolyLine(lz.size(), &(lz[0]), &(ly[0]));
       for (int i = 0; i < int(lz.size()); i++) {
         marker.DrawMarker(lz[i], ly[i]);
       }
       XZ->cd();
       pline.DrawPolyLine(lz.size(), &(lz[0]), &(lx[0]));
       for (int i = 0; i < int(lz.size()); i++) {
         marker.DrawMarker(lz[i], lx[i]);
       }
       YX->cd();
       pline.DrawPolyLine(lx.size(), &(lx[0]), &(ly[0]));
       XYZ->cd();
       pline.DrawPolyLine(lz_turned.size(), &(lz_turned[0]), &(lx_turned[0]));
   
       nGhosts++;
     }
   
   
     cout << "L1CADraw: number of ghosts drawn: " << nGhosts << endl;
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
     XYZ->cd();
     XYZ->Update();
   }
   
   /// draw reco track from CBML1Track
   void L1AlgoDraw::DrawRecoTracks_CbmL1()
   {
     int NRecTracks = 0;
   
     CbmL1& L1 = *CbmL1::Instance();
   
     LOG(info) << "Number of reco tracks: " << L1.fvRecoTracks.size();
     int nGhosts = 0;
   
     for (vector<CbmL1Track>::iterator it = L1.fvRecoTracks.begin(); it != L1.fvRecoTracks.end(); ++it) {
       CbmL1Track& T = *it;
       int nHits     = T.Hits.size();
   
       /// if not found in last iteration skip
       // if (T.fFoundInIteration != 3) {
       //   continue;
       // }
   
       const auto& mcIndexes = T.GetMCTrackIndexes();
       if (mcIndexes.size() == 0) {
         nGhosts++;
         continue;
       }
   
       bool skip = true;
       for (auto iMT : mcIndexes) {
         const auto& mcTrk = L1.GetMCData().GetTrack(iMT);
         if (!mcTrk.IsReconstructable()) continue;
   
         /// skip tracks not found in GNN iteration
         bool found_gnn_iter = true;
         for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
           const auto& rTrk = L1.fvRecoTracks[irt];
           if (rTrk.fFoundInIteration != 3) {
             found_gnn_iter = false;
             break;
           }
         }
         if (!found_gnn_iter) continue;
   
         /// min 4 hits
         if (mcTrk.GetNofHits() < 4) continue;
         /// no jump tracks
         // if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
         // momentum > 0.2
         // if (mcTrk.GetP() < 0.2) continue;
   
         skip = false;
       }
       // if (skip) continue;
   
       /// draw only true tracks
       // if (T.fIsGhost != 0) {  /// skip if not true track
       //   continue;
       // }
   
       vector<double> lx, ly, lz;
       vector<double> lx_turned, ly_turned, lz_turned;
   
       TPolyLine pline;
       pline.SetLineColor(kBlue);
       pline.SetLineWidth(2);
       pline.SetLineStyle(1);
   
       for (int iHit = 0; iHit < nHits; iHit++) {
         unsigned int ih = T.Hits[iHit];
   
         Point p = GetHitCoor(ih);  // CHANGE THIS
         lx.push_back(p.x);
         ly.push_back(p.y);
         lz.push_back(p.z);
   
         TVector3 v3(p.x, p.y, p.z);
         v3.RotateX(TMath::Pi() / 5);
         v3.RotateY(TMath::Pi() / 20);
         v3.RotateZ(TMath::Pi() / 100);
         lx_turned.push_back(v3.x());
         ly_turned.push_back(v3.y());
         lz_turned.push_back(v3.z());
       }
   
       NRecTracks++;
       TMarker marker;
       marker.SetMarkerColor(kBlue);
       marker.SetMarkerStyle(24);  // 26 - open triangle, 24 - open circle
       marker.SetMarkerSize(1.5 * HitSize);
   
       YZ->cd();
       pline.DrawPolyLine(lz.size(), &(lz[0]), &(ly[0]));
       for (int i = 0; i < int(lz.size()); i++) {
         marker.DrawMarker(lz[i], ly[i]);
       }
       XZ->cd();
       pline.DrawPolyLine(lz.size(), &(lz[0]), &(lx[0]));
       for (int i = 0; i < int(lz.size()); i++) {
         marker.DrawMarker(lz[i], lx[i]);
       }
       YX->cd();
       pline.DrawPolyLine(lx.size(), &(lx[0]), &(ly[0]));
       XYZ->cd();
       pline.DrawPolyLine(lz_turned.size(), &(lz_turned[0]), &(lx_turned[0]));
     }
   
   
     cout << "L1CADraw: number of reconstructed tracks drawn: " << NRecTracks << endl;
     cout << "L1CADraw: number of ghosts: " << nGhosts << endl;
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   
     XYZ->cd();
     XYZ->Update();
   }
   
   
   void L1AlgoDraw::DrawTriplets(vector<ca::Triplet>& triplets, const ca::HitIndex_t* realIHit)
   {
     //   vector <ca::Triplet> triplets = algo->vTriplets;
     for (unsigned int iTrip = 0; iTrip < triplets.size(); iTrip++) {
       ca::Triplet& trip = triplets[iTrip];
   
       unsigned int iLHit = trip.GetLHit();
       iLHit              = realIHit[iLHit];
       unsigned int iMHit = trip.GetMHit();
       iMHit              = realIHit[iMHit];
       unsigned int iRHit = trip.GetRHit();
       iRHit              = realIHit[iRHit];
   
       DrawTriplet(iLHit, iMHit, iRHit);
     }
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   };
   
   void L1AlgoDraw::DrawTriplet(int il, int im, int ir)
   {
     TPolyLine pline;
     pline.SetLineColor(kBlack);
     TMarker marker;
     marker.SetMarkerColor(kBlack);
     marker.SetMarkerStyle(26);
     marker.SetMarkerSize(HitSize * 2);
   
     vector<double> lx, ly, lz;
   
     Point coor;
   
     coor = GetHitCoor(il);
     lx.push_back(coor.x);
     ly.push_back(coor.y);
     lz.push_back(coor.z);
   
     coor = GetHitCoor(im);
     lx.push_back(coor.x);
     ly.push_back(coor.y);
     lz.push_back(coor.z);
   
     coor = GetHitCoor(ir);
     lx.push_back(coor.x);
     ly.push_back(coor.y);
     lz.push_back(coor.z);
   
     const int nHits = 3;
     YZ->cd();
     pline.DrawPolyLine(nHits, &(lz[0]), &(ly[0]));
     marker.DrawMarker(lz[nHits - 1], ly[nHits - 1]);
     XZ->cd();
     pline.DrawPolyLine(nHits, &(lz[0]), &(lx[0]));
     marker.DrawMarker(lz[nHits - 1], lx[nHits - 1]);
     YX->cd();
     pline.DrawPolyLine(nHits, &(lx[0]), &(ly[0]));
     marker.DrawMarker(lx[nHits - 1], ly[nHits - 1]);
   }
   
   void L1AlgoDraw::DrawDoublets(vector<ca::HitIndex_t>* Doublets_hits,
                                 map<ca::HitIndex_t, ca::HitIndex_t>* Doublets_start, const int /*MaxArrSize*/,
                                 ca::HitIndex_t* StsRestHitsStartIndex, unsigned int* realIHit)
   {
     for (int iSta = 0; iSta < NStations - 1; iSta++) {
       const int firstHitOnSta                               = StsRestHitsStartIndex[iSta];
       const int firstHitOnNextSta                           = StsRestHitsStartIndex[iSta + 1];
       ca::HitIndex_t* staDoubletsHits                       = &(Doublets_hits[iSta][0]);
       map<ca::HitIndex_t, ca::HitIndex_t>& staDoubletsStart = Doublets_start[iSta];
   
       for (int iRestLHit = firstHitOnSta; iRestLHit < firstHitOnNextSta; iRestLHit++) {
         const int ilh       = iRestLHit - firstHitOnSta;
         const int iirhFirst = staDoubletsStart[ilh];
         const int iirhLast  = staDoubletsStart[ilh + 1] - 1;
   
         for (int iirh = iirhFirst; iirh <= iirhLast; iirh++) {
           const int iRestRHit = staDoubletsHits[iirh] + firstHitOnNextSta;
   
           const int iLHit = realIHit[iRestLHit];
           const int iRHit = realIHit[iRestRHit];
   
           DrawDoublet(iLHit, iRHit);
         }
       }
     }
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   };
   
   void L1AlgoDraw::DrawDoubletsOnSta(int iSta, ca::HitIndex_t* Doublets_hits, ca::HitIndex_t* Doublets_start,
                                      const int MaxArrSize, ca::HitIndex_t* StsRestHitsStartIndex, unsigned int* realIHit)
   {
     const int firstHitOnSta          = StsRestHitsStartIndex[iSta];
     const int firstHitOnNextSta      = StsRestHitsStartIndex[iSta + 1];
     ca::HitIndex_t* staDoubletsHits  = Doublets_hits + MaxArrSize * iSta;
     ca::HitIndex_t* staDoubletsStart = Doublets_start + MaxArrSize * iSta;
   
     for (int iRestLHit = firstHitOnSta; iRestLHit < firstHitOnNextSta; iRestLHit++) {
       const int ilh       = iRestLHit - firstHitOnSta;
       const int iirhFirst = staDoubletsStart[ilh];
       const int iirhLast  = staDoubletsStart[ilh + 1] - 1;
   
       for (int iirh = iirhFirst; iirh <= iirhLast; iirh++) {
         const int iRestRHit = staDoubletsHits[iirh] + firstHitOnNextSta;
   
         const int iLHit = realIHit[iRestLHit];
         const int iRHit = realIHit[iRestRHit];
   
         DrawDoublet(iLHit, iRHit);
       }
     }
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   };
   
   void L1AlgoDraw::DrawDoublet(int il, int ir)
   {
     TPolyLine pline;
     pline.SetLineColor(kBlue);
     TMarker marker;
     marker.SetMarkerColor(kBlue);
     marker.SetMarkerStyle(27);
     marker.SetMarkerSize(HitSize * 2);
   
     vector<double> lx, ly, lz;
   
     Point coor;
   
     coor = GetHitCoor(il);
     lx.push_back(coor.x);
     ly.push_back(coor.y);
     lz.push_back(coor.z);
   
     coor = GetHitCoor(ir);
     lx.push_back(coor.x);
     ly.push_back(coor.y);
     lz.push_back(coor.z);
   
     const int nHits = 2;
     YZ->cd();
     pline.DrawPolyLine(nHits, &(lz[0]), &(ly[0]));
     marker.DrawMarker(lz[nHits - 1], ly[nHits - 1]);
     XZ->cd();
     pline.DrawPolyLine(nHits, &(lz[0]), &(lx[0]));
     marker.DrawMarker(lz[nHits - 1], lx[nHits - 1]);
     YX->cd();
     pline.DrawPolyLine(nHits, &(lx[0]), &(ly[0]));
     marker.DrawMarker(lx[nHits - 1], ly[nHits - 1]);
   }
   
   
   void L1AlgoDraw::DrawInfo()
   {
     cout << " vHits.size = " << algo->GetInputData().GetNhits() << endl;
     cout << " vRecoHits.size = " << algo->fSliceRecoHits.size() << endl;
     cout << " vTracks.size = " << algo->fSliceRecoTracks.size() << endl;
   }
   
   void L1AlgoDraw::DrawTarget()
   {
   
     float x = algo->GetParameters().GetTargetPositionX()[0];
     float y = algo->GetParameters().GetTargetPositionY()[0];
     float z = algo->GetParameters().GetTargetPositionZ()[0];
     float x_t, z_t;
   
     TVector3 v3(x, y, z);
     v3.RotateX(TMath::Pi() / 5);
     v3.RotateY(TMath::Pi() / 20);
     v3.RotateZ(TMath::Pi() / 100);
     x_t = v3.x();
     z_t = v3.z();
   
     {
       YZ->cd();
   
       TMarker* marker = new TMarker(z, y, targetMStyle);
       marker->SetMarkerColor(kRed);
       marker->SetMarkerStyle(targetMStyle);
       marker->SetMarkerSize(HitSize);
       marker->Draw();
     }
   
     {
       XZ->cd();
   
       TMarker* marker = new TMarker(z, x, targetMStyle);
       marker->SetMarkerColor(kRed);
       marker->SetMarkerStyle(targetMStyle);
       marker->SetMarkerSize(HitSize);
       marker->Draw();
     }
   
     {
       YX->cd();
   
       TMarker* marker = new TMarker(x, y, targetMStyle);
       marker->SetMarkerColor(kRed);
       marker->SetMarkerStyle(targetMStyle);
       marker->SetMarkerSize(HitSize);
       marker->Draw();
     }
   
     {
       XYZ->cd();
   
       TMarker* marker = new TMarker(z_t, x_t, targetMStyle);
       marker->SetMarkerColor(kRed);
       marker->SetMarkerStyle(targetMStyle);
       marker->SetMarkerSize(HitSize);
       marker->Draw();
     }
   }
   
   void L1AlgoDraw::DrawInputHits()
   {
     int nhits = vHits.size();
     Double_t x_poly[nhits], y_poly[nhits], z_poly[nhits];
     Double_t x_poly_fake[nhits], y_poly_fake[nhits], z_poly_fake[nhits];
     Double_t x_poly_turned[nhits], z_poly_turned[nhits];
     Double_t x_poly_fake_turned[nhits], z_poly_fake_turned[nhits];
   
   
     for (int ista = NStations - 1; ista >= 0; ista--) {  //start downstream chambers
       Int_t n_poly      = 0;
       Int_t n_poly_fake = 0;
       for (int ih = HitsStartIndex[ista]; ih < HitsStopIndex[ista]; ih++) {
         ca::Hit& h      = vHits[ih];
         const auto& hQa = vHitsQa[ih];
         int iMC         = hQa.GetBestMcPointId();
   
         fscal x = h.X(), y = h.Y(), z = h.Z();
         fscal x_t, z_t;
   
         TVector3 v3(x, y, z);
         v3.RotateX(TMath::Pi() / 5);
         v3.RotateY(TMath::Pi() / 20);
         v3.RotateZ(TMath::Pi() / 100);
         x_t = v3.x();
         z_t = v3.z();
   
         if (iMC >= 0) {
           x_poly[n_poly]        = x;
           y_poly[n_poly]        = y;
           z_poly[n_poly]        = z;
           x_poly_turned[n_poly] = x_t;
           z_poly_turned[n_poly] = z_t;
           n_poly++;
         }
         else {
           x_poly_fake[n_poly_fake]        = x;
           y_poly_fake[n_poly_fake]        = y;
           z_poly_fake[n_poly_fake]        = z;
           x_poly_fake_turned[n_poly_fake] = x_t;
           z_poly_fake_turned[n_poly_fake] = z_t;
           n_poly_fake++;
         }
       }
   
       YZ->cd();
       TPolyMarker* pmyz = new TPolyMarker(n_poly, z_poly, y_poly);
       pmyz->SetMarkerColor(mcolor[ista]);
       pmyz->SetMarkerStyle(hitsMStyle);
       pmyz->SetMarkerSize(HitSize);
       pmyz->Draw();
   
       TPolyMarker* pmyz_fake = new TPolyMarker(n_poly_fake, z_poly_fake, y_poly_fake);
       pmyz_fake->SetMarkerColor(mcolorFake[ista]);
       pmyz_fake->SetMarkerStyle(fakesMStyle);
       pmyz_fake->SetMarkerSize(HitSize);
       pmyz_fake->Draw();
   
   
       XZ->cd();
       TPolyMarker* pmxz = new TPolyMarker(n_poly, z_poly, x_poly);
       pmxz->SetMarkerColor(mcolor[ista]);
       pmxz->SetMarkerStyle(hitsMStyle);
       pmxz->SetMarkerSize(HitSize);
       pmxz->Draw();
   
       TPolyMarker* pmxz_fake = new TPolyMarker(n_poly_fake, z_poly_fake, x_poly_fake);
       pmxz_fake->SetMarkerColor(mcolorFake[ista]);
       pmxz_fake->SetMarkerStyle(fakesMStyle);
       pmxz_fake->SetMarkerSize(HitSize);
       pmxz_fake->Draw();
   
   
       YX->cd();
       TPolyMarker* pmyx = new TPolyMarker(n_poly, x_poly, y_poly);
       pmyx->SetMarkerColor(mcolor[ista]);
       pmyx->SetMarkerStyle(hitsMStyle);
       pmyx->SetMarkerSize(HitSize);
       pmyx->Draw();
   
       TPolyMarker* pmyx_fake = new TPolyMarker(n_poly_fake, x_poly_fake, y_poly_fake);
       pmyx_fake->SetMarkerColor(mcolorFake[ista]);
       pmyx_fake->SetMarkerStyle(fakesMStyle);
       pmyx_fake->SetMarkerSize(HitSize);
       pmyx_fake->Draw();
   
   
       XYZ->cd();
       TPolyMarker* pmxyz = new TPolyMarker(n_poly, z_poly_turned, x_poly_turned);
       pmxyz->SetMarkerColor(mcolor[ista]);
       pmxyz->SetMarkerStyle(hitsMStyle);
       pmxyz->SetMarkerSize(HitSize);
       pmxyz->Draw();
   
       TPolyMarker* pmxyz_fake = new TPolyMarker(n_poly_fake, z_poly_fake_turned, x_poly_fake_turned);
       pmxyz_fake->SetMarkerColor(mcolorFake[ista]);
       pmxyz_fake->SetMarkerStyle(fakesMStyle);
       pmxyz_fake->SetMarkerSize(HitSize);
       pmxyz_fake->Draw();
     }
   
   }  // DrawInputHits
   
   /// draws hits in remainingHitsIds
   void L1AlgoDraw::DrawHitsInFile()
   {
     const auto& remainingHitsWindow = algo->remainingHitsIds;  // fWindowHits
     std::vector<int> remainingHits;                            // in fInputData
     std::vector<std::vector<float>> hits;
     for (auto ih : remainingHitsWindow) {
       remainingHits.push_back(algo->fWindowHits[ih].Id());
       std::vector<float> hit{algo->fWindowHits[ih].X(), algo->fWindowHits[ih].Y(), algo->fWindowHits[ih].Z(),
                              algo->fWindowHits[ih].Station()};
       hits.push_back(hit);
     }
     int nhits = hits.size();
     std::cout << "[L1AlgoDraw] Read " << nhits << std::endl;
   
     /// label hits to which track they belong
     std::vector<int> hitLabels(nhits, -1);
     std::vector<int> hitType(nhits, 0);  // 0 - ghost, 1 - true(non-reconstructable), 2 - true(reconstructable)
   
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
     for (int iTrack = 0; iTrack < mcData.GetTrackContainer().size(); iTrack++) {
       const auto& mcTrk = mcData.GetTrackContainer()[iTrack];
       for (int ip : mcTrk.GetPointIndexes()) {
         const auto& point     = mcData.GetPoint(ip);
         const auto& hitsTrack = point.GetHitIndexes();  // hit index in fInputData
         if (hitsTrack.size() == 0) continue;
         for (const auto& hitIndex : hitsTrack) {
           // find ih in remainingHits
           auto it = std::find(remainingHits.begin(), remainingHits.end(), hitIndex);
           if (it == remainingHits.end()) continue;
           int index        = std::distance(remainingHits.begin(), it);
           hitLabels[index] = iTrack;
           hitType[index]   = 1;
           if (!mcTrk.IsReconstructable()) continue;
           if (mcTrk.GetNofHits() < 4) continue;
           if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
           if (mcTrk.GetP() < 0.2) continue;
           /// skip tracks not found in GNN iteration
           bool found_gnn_iter = true;
           for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
             const auto& rTrk = L1.fvRecoTracks[irt];
             if (rTrk.fFoundInIteration != 3) {
               found_gnn_iter = false;
               break;
             }
           }
           if (!found_gnn_iter) continue;
           hitType[index] = 2;
         }
       }
     }
   
     std::vector<int> trackLabels;
     for (int iLabel = 0; iLabel < hitLabels.size(); iLabel++) {
       int label = hitLabels[iLabel];
       int type  = hitType[iLabel];
       if (label >= 0 && type == 2 && std::find(trackLabels.begin(), trackLabels.end(), label) == trackLabels.end()) {
         trackLabels.push_back(label);
       }
     }
     std::cout << "Number of reconstructable tracks: " << trackLabels.size() << std::endl;
   
     std::vector<std::vector<int>> tracks;  // index in hits
     for (const auto& label : trackLabels) {
       std::vector<int> track;
       for (int i = 0; i < hitLabels.size(); i++) {
         if (hitLabels[i] == label) {
           track.push_back(i);
         }
       }
       tracks.push_back(track);
     }
   
     Double_t x_poly[nhits], y_poly[nhits], z_poly[nhits];
     Double_t x_poly_turned[nhits], z_poly_turned[nhits];
     Int_t n_poly = 0;
     for (int ih = 0; ih < nhits; ih++) {
       const auto& hit = hits[ih];
       const float x   = hit[0];
       const float y   = hit[1];
       const float z   = hit[2];
       float x_t, z_t;
   
       TVector3 v3(x, y, z);
       v3.RotateX(TMath::Pi() / 5);
       v3.RotateY(TMath::Pi() / 20);
       v3.RotateZ(TMath::Pi() / 100);
       x_t = v3.x();
       z_t = v3.z();
   
       x_poly[n_poly]        = x;
       y_poly[n_poly]        = y;
       z_poly[n_poly]        = z;
       x_poly_turned[n_poly] = x_t;
       z_poly_turned[n_poly] = z_t;
       n_poly++;
     }
   
     constexpr int hitColour = 3;
   
     YZ->cd();
     TPolyMarker* pmyz = new TPolyMarker(n_poly, z_poly, y_poly);
     pmyz->SetMarkerColor(hitColour);
     pmyz->SetMarkerStyle(hitsMStyle);
     pmyz->SetMarkerSize(0.5 * HitSize);
     pmyz->Draw();
   
     XZ->cd();
     TPolyMarker* pmxz = new TPolyMarker(n_poly, z_poly, x_poly);
     pmxz->SetMarkerColor(hitColour);
     pmxz->SetMarkerStyle(hitsMStyle);
     pmxz->SetMarkerSize(0.5 * HitSize);
     pmxz->Draw();
   
     YX->cd();
     TPolyMarker* pmyx = new TPolyMarker(n_poly, x_poly, y_poly);
     pmyx->SetMarkerColor(hitColour);
     pmyx->SetMarkerStyle(hitsMStyle);
     pmyx->SetMarkerSize(0.5 * HitSize);
     pmyx->Draw();
   
     // TMarker m_xy;
     // int colorUnReco = 13;
     // int colorGhost  = 16;
     // std::vector<int> colors{2, 3, 4, 5, 6, 7, 8, 9, 30, 38, 41, 44, 46, 49};
     // // marker style
     // int trueStyle  = 20;
     // int ghostStyle = 24;
     // // marker size
     // float recoSize   = 0.8;
     // float unRecoSize = 0.5;
     // float ghostSize  = 0.3;
     // for (int ih = 0; ih < nhits; ih++) {
     //   int label  = hitLabels[ih];
     //   int type   = hitType[ih];
     //   int color  = colorGhost;
     //   int style  = ghostStyle;
     //   float size = ghostSize;
     //   if (type == 2) {  // reco
     //     color = colors[label % colors.size()];
     //     style = trueStyle;
     //     size  = recoSize;
     //   }
     //   else if (type == 1) {  // unreco
     //     color = colorUnReco;
     //     size  = unRecoSize;
     //   }
     //   m_xy.SetMarkerColor(color);
     //   m_xy.SetMarkerStyle(style);
     //   m_xy.SetMarkerSize(size);
     //   m_xy.DrawMarker(x_poly[ih], y_poly[ih]);
     // }
     // // Draw tracks
     // TPolyLine pline;
     // for (const auto& track : tracks) {
     //   int nHitsTrack = track.size();
     //   std::vector<double_t> x(nHitsTrack), y(nHitsTrack);
     //   for (int iHit = 0; iHit < nHitsTrack; iHit++) {
     //     const auto& hit1 = hits[track[iHit]];
     //     x[iHit]          = hit1[0];
     //     y[iHit]          = hit1[1];
     //   }
     //   pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
     //   pline.DrawPolyLine(nHitsTrack, &(x[0]), &(y[0]));
     // }
     YX->Update();
   
     XYZ->cd();
     TPolyMarker* pmxyz = new TPolyMarker(n_poly, z_poly_turned, x_poly_turned);
     pmxyz->SetMarkerColor(hitColour);
     pmxyz->SetMarkerStyle(hitsMStyle);
     pmxyz->SetMarkerSize(0.5 * HitSize);
     pmxyz->Draw();
   
   }  // DrawHitsInFile
   
   // draw tracks that are killing hits from unreco MC tracks
   void L1AlgoDraw::DrawKillingTracks()
   {
     LOG(info) << "L1AlgoDraw: DrawKillingTracks";
   
     TMarker marker;             // mark hit used in first iteration
     marker.SetMarkerColor(7);   // turquoise
     marker.SetMarkerStyle(20);  // closed circle
     marker.SetMarkerSize(2 * HitSize);
   
     int nTracksLeft = 0;
     std::vector<int> killingTracksIndexes;  // reco tracks from first iter that use up hits
   
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
     for (const auto& mcTrk : mcData.GetTrackContainer()) {
       // draw reconstructable tracks only
       if (!mcTrk.IsReconstructable()) continue;
       if (mcTrk.GetNofHits() < 4) continue;
       if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
       if (mcTrk.GetP() < 0.2) continue;
   
       /// skip tracks not found in GNN iteration
       // bool found_gnn_iter = true;
       // for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
       //   const auto& rTrk = L1.fvRecoTracks[irt];
       //   if (rTrk.fFoundInIteration != 3) {
       //     found_gnn_iter = false;
       //     break;
       //   }
       // }
       // if (!found_gnn_iter) continue;
   
       if (mcTrk.IsReconstructed()) continue;
       nTracksLeft++;
   
       /// draw all hits corresponding to points of MC tracks which have been used by tracks
       for (int ip : mcTrk.GetPointIndexes()) {
         const auto& point      = mcData.GetPoint(ip);
         const auto& hitIndexes = point.GetHitIndexes();  // in fInputData
         if (hitIndexes.size() == 0) continue;
         for (const auto ih : hitIndexes) {
           // find which reco track used strip of this hit
           for (vector<CbmL1Track>::iterator it = L1.fvRecoTracks.begin(); it != L1.fvRecoTracks.end(); ++it) {
             CbmL1Track& T = *it;
             for (const auto& hitIndex : T.GetHitIndexes()) {
               const auto& mcHit   = algo->GetInputData().GetHit(ih);
               const auto& recoHit = algo->GetInputData().GetHit(hitIndex);
               if (mcHit.FrontKey() == recoHit.FrontKey() || mcHit.BackKey() == recoHit.BackKey()) {
                 killingTracksIndexes.push_back(&T - &L1.fvRecoTracks[0]);
                 if (T.fFoundInIteration == 3)  // hit used by track found in second iteration. Lost competition
                 {
                   marker.SetMarkerColor(6);  // magenta
                 }
                 YZ->cd();
                 marker.DrawMarker(recoHit.Z(), recoHit.Y());
                 XZ->cd();
                 marker.DrawMarker(recoHit.Z(), recoHit.X());
                 YX->cd();
                 marker.DrawMarker(recoHit.X(), recoHit.Y());
               }
             }
           }  // reco tracks
         }  // hits for one mc point
       }  // points in track
     }  // tracks
   
     LOG(info) << "L1AlgoDraw: number of tracks left: " << nTracksLeft;
     LOG(info) << "L1AlgoDraw: number of killing tracks: " << killingTracksIndexes.size();
   
     // draw killed hits
     TPolyLine pline;
     pline.SetLineColor(7);  // turquoise
     pline.SetLineWidth(1.5);
     for (const auto& trackIndex : killingTracksIndexes) {
       const auto& T = L1.fvRecoTracks[trackIndex];
       if (T.fFoundInIteration == 3) {
         pline.SetLineColor(6);  // magenta
       }
       vector<double> lx, ly, lz;
       vector<double> lx_turned, ly_turned, lz_turned;
   
       for (const auto& ih : T.Hits) {
         Point p = GetHitCoor(ih);
         lx.push_back(p.x);
         ly.push_back(p.y);
         lz.push_back(p.z);
   
         TVector3 v3(p.x, p.y, p.z);
         v3.RotateX(TMath::Pi() / 5);
         v3.RotateY(TMath::Pi() / 20);
         v3.RotateZ(TMath::Pi() / 100);
         lx_turned.push_back(v3.x());
         ly_turned.push_back(v3.y());
         lz_turned.push_back(v3.z());
       }
   
       YZ->cd();
       pline.DrawPolyLine(lz.size(), &(lz[0]), &(ly[0]));
       XZ->cd();
       pline.DrawPolyLine(lz.size(), &(lz[0]), &(lx[0]));
       YX->cd();
       pline.DrawPolyLine(lx.size(), &(lx[0]), &(ly[0]));
       XYZ->cd();
       pline.DrawPolyLine(lz_turned.size(), &(lz_turned[0]), &(lx_turned[0]));
     }
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
     XYZ->cd();
     XYZ->Update();
   }
   
   
   void L1AlgoDraw::DrawAsk()
   {
     char symbol;
     if (ask) {
       std::cout << "ask>";
       do {
         std::cin.get(symbol);
         if (symbol == 'r') ask = false;
         if (symbol == 'q') exit(0);
       } while (symbol != '\n');
       std::cout << endl;
     }
   }
   
   void L1AlgoDraw::ClearView()
   {
     YZ->Clear();
     XZ->Clear();
     YX->Clear();
     XYZ->Clear();
   }
   
   L1AlgoDraw::Point L1AlgoDraw::GetHitCoor(int ih)
   {
     const ca::Hit& hit = algo->GetInputData().GetHit(ih);
     // ca::Hit& hit = vHits[ih]; //orig
     return Point(hit.X(), hit.Y(), hit.Z());
   }
   
   void L1AlgoDraw::SaveCanvas(TString name)
   {
     system("mkdir L1CADraw -p");
     chdir("L1CADraw");
     TString tmp = name;
     tmp += "YXView.pdf";
     YX->cd();
     //   YX->SaveAs("YXView.eps");
     YX->SaveAs(tmp);
   
     tmp = name;
     tmp += "XZView.pdf";
     XZ->cd();
     //   XZ->SaveAs("XZView.eps");
     XZ->SaveAs(tmp);
   
     tmp = name;
     tmp += "YZView.pdf";
     YZ->cd();
     //   YZ->SaveAs("YZView.eps");
     YZ->SaveAs(tmp);
   
     tmp = name;
     tmp += "XYZView.pdf";
     XYZ->cd();
     XYZ->SaveAs(tmp);
   
     std::cout << "Canvas saved in L1CADraw with name " << name << std::endl;
   
     chdir("..");
   }
   
   void L1AlgoDraw::DrawStations()
   {
     const auto& Stations = algo->GetParameters().GetStations();
   
     Vector<float> StaZPos{"L1AlgoDraw::StaZPos"};
     StaZPos.reserve(NStations);
     Vector<float> StaYPos{"L1AlgoDraw::StaYPos"};
     StaYPos.reserve(NStations);
     Vector<float> StaXPos{"L1AlgoDraw::StaXPos"};
     StaXPos.reserve(NStations);
   
     for (const auto& station : Stations) {
       StaZPos.push_back(station.GetZ()[0]);
       StaYPos.push_back(station.GetYmax()[0]);
       StaXPos.push_back(station.GetXmax()[0]);
     }
     LOG(info) << "Drawing " << NStations << " stations";
   
     // draws lines for stations
     // OT: DrawLine(x1,y1,x2,y2) draws line from point 1 to 2
     for (int ista = NStations - 1; ista >= 0; ista--) {
   
       TLine* line = new TLine();
       line->SetLineColor(StaColor);
   
       YZ->cd();
       line->DrawLine(StaZPos[ista], -StaYPos[ista], StaZPos[ista], StaYPos[ista]);
   
       XZ->cd();
       line->DrawLine(StaZPos[ista], -StaXPos[ista], StaZPos[ista], StaXPos[ista]);
   
       YX->cd();
       // Draw rectangle from -x to x and -y to y
       TBox* box = new TBox(-StaXPos[ista], -StaYPos[ista], StaXPos[ista], StaYPos[ista]);
       box->SetFillStyle(0);
       box->SetLineWidth(1);
       box->SetLineColor(StaColor);
       box->SetLineStyle(7);
       box->Draw();
   
       // // Draw beam pipe. Two circles with radii of beam pipe on first and last station
       // TEllipse *ellipseIn = new TEllipse(0.0, 0.0, rBeamPipeFirst);
       // TEllipse *ellipseOut = new TEllipse(0.0, 0.0, rBeamPipeLast);
       // ellipseIn->SetLineColor(StaColor); ellipseOut->SetLineColor(StaColor);
       // ellipseIn->SetFillStyle(0); ellipseOut->SetFillStyle(0);
       // ellipseIn->Draw(); ellipseOut->Draw();
     }
   
     // Write text about orientation
     TLatex latex;
     latex.SetTextFont(132);
     latex.SetTextAlign(12);
     latex.SetTextSize(0.035);
   
     YZ->cd();
     latex.DrawLatex(-45.0, 45.0, "YZ Side View");
     YZ->Draw();
     XZ->cd();
     latex.DrawLatex(-45.0, 45.0, "XZ Top View");
     XZ->Draw();
     YX->cd();
     latex.DrawLatex(-45.0, 45.0, "YX Front View");
     YX->Draw();
   }
   
   /// @brief: Read loss data from file and plot
   void L1AlgoDraw::plotEmbedLoss()
   {
     // read loss data from file into loss training
     std::string fDirName      = "/u/otyagi/cbmroot/NN/output/";
     std::string trainLossFile = fDirName + "EmbedLoss.txt";
     std::ifstream fin;
     fin.open(trainLossFile);
     if (!fin.is_open()) {
       std::cerr << "Error: could not open file " << trainLossFile << std::endl;
       return;
     }
   
     /// first line gives num of entries
     int nEntries;
     fin >> nEntries;
     std::vector<float> lossTraining;
     float loss;
     while (fin >> loss) {
       lossTraining.push_back(loss);
     }
     fin.close();
   
   
     TCanvas* c1 = new TCanvas("c1", "loss", 700, 500);
     c1->SetGrid();
     TGraph* gr = new TGraph(lossTraining.size());
     for (auto i = 0; i < lossTraining.size(); i++) {
       gr->SetPoint(i, i, lossTraining[i]);
     }
     gr->SetLineColor(kRed);
     // gr->SetLineStyle(2); // for dashed line
     gr->SetTitle("Training");
     gr->GetXaxis()->SetTitle("Epoch");
     gr->GetYaxis()->SetTitle("Loss");
     // gr->GetYaxis()->SetRangeUser(0.001, 0.012);
     gr->SetLineWidth(2);
     gr->SetMarkerStyle(20);  // Set the marker style to circle
     gr->SetMarkerSize(0.5);
     gr->Draw("ALP");  // Draw the graph with axis, line, markers
   
     c1->BuildLegend(0.7, 0.7, 0.9, 0.9);
     TString fSave = fDirName + "Embed_Loss.pdf";
     c1->SaveAs(fSave);
   
     std::cout << "Loss plot saved to " << fSave << std::endl;
   }
   
   /// @brief: Read loss data from file and plot
   void L1AlgoDraw::plotLoss(std::string& fName)
   {
     // read loss data from file into loss training
     std::string fDirName      = "/u/otyagi/cbmroot/NN/output/";
     std::string trainLossFile = fDirName + fName + "Loss.txt";
     std::string valLossFile   = fDirName + fName + "Loss.txt";
     std::ifstream fin;
     fin.open(trainLossFile);
     if (!fin.is_open()) {
       std::cerr << "Error: could not open file " << trainLossFile << std::endl;
       return;
     }
     /// read until end of file. Number of lines not known. each row is a loss value for a single epoch
     std::vector<float> lossTraining;
     float loss;
     while (fin >> loss) {
       lossTraining.push_back(loss);
     }
     fin.close();
   
     /// read validation loss data
     // fin.open(valLossFile);
     // if (!fin.is_open()) {
     //   std::cerr << "Error: could not open file " << valLossFile << std::endl;
     //   return;
     // }
     // std::vector<float> lossValidation;
     // while (fin >> loss) {
     //   lossValidation.push_back(loss);
     // }
     // fin.close();
   
     TCanvas* c1 = new TCanvas("c1", "loss", 700, 500);
     c1->SetGrid();
     TGraph* gr = new TGraph(lossTraining.size());
     for (auto i = 0; i < lossTraining.size(); i++) {
       gr->SetPoint(i, i, lossTraining[i]);
     }
     gr->SetLineColor(kRed);
     // gr->SetLineStyle(2); // for dashed line
     gr->SetTitle("Training");
     gr->GetXaxis()->SetTitle("Epoch");
     gr->GetYaxis()->SetTitle("Loss");
     // gr->GetYaxis()->SetRangeUser(0.001, 0.012);
     gr->SetLineWidth(2);
     gr->SetMarkerStyle(20);  // Set the marker style to circle
     gr->SetMarkerSize(0.5);
     gr->Draw("ALP");  // Draw the graph with axis, line, markers
   
     ///@todo: Validation
     // TGraph* gr2 = new TGraph(lossValidation.size());
     // for (auto i = 0; i < lossValidation.size(); i++) {
     //   gr2->SetPoint(i, i, lossValidation[i]);
     // }
     // gr2->SetTitle("Validation");
     // gr2->SetLineColor(kBlue);
     // gr2->SetLineWidth(2);
     // gr2->SetMarkerStyle(20);
     // gr2->SetMarkerSize(0.5);
     // gr2->Draw("LP same");
   
     c1->BuildLegend(0.7, 0.7, 0.9, 0.9);
     TString fSave = fDirName + fName + "_Loss.pdf";
     c1->SaveAs(fSave);
   
     std::cout << "Loss plot saved to " << fSave << std::endl;
   }
   
   /// @brief: Read accuracy data from file and plot
   void L1AlgoDraw::plotAccuracy(std::string& fName)
   {
     // read loss data from file into loss training
     std::string fDirName = "/u/otyagi/cbmroot/NN/output/";
     std::string file     = fDirName + fName + "Accuracy.txt";
     std::ifstream fin;
     fin.open(file);
     if (!fin.is_open()) {
       std::cerr << "Error: could not open file " << file << std::endl;
       return;
     }
   
     float accuracy;
     std::vector<float> accTraining;
     while (fin >> accuracy) {
       accTraining.push_back(accuracy);
     }
     fin.close();
   
   
     TCanvas* c1 = new TCanvas("c1", "Accuracy", 700, 500);
     c1->SetGrid();
     TGraph* gr = new TGraph(accTraining.size());
     for (auto i = 0; i < accTraining.size(); i++) {
       gr->SetPoint(i, i, accTraining[i]);
     }
     gr->SetLineColor(kRed);
     gr->SetTitle("Training");
     gr->GetXaxis()->SetTitle("Epoch");
     gr->GetYaxis()->SetTitle("Accuracy");
     gr->GetYaxis()->SetRangeUser(0.5, 1.1);
     gr->SetLineWidth(2);
     gr->SetMarkerStyle(20);  // Set the marker style to circle
     gr->SetMarkerSize(0.5);
     gr->Draw("ALP");  // Draw the graph with axis, line, markers
   
     c1->BuildLegend(0.8, 0.8, 0.9, 0.9);
     TString fSave = fDirName + fName + "_Accuracy.pdf";
     c1->SaveAs(fSave);
   
     std::cout << "Accuracy plot saved to " << fSave << std::endl;
   }
   
   
   void L1AlgoDraw::drawClassfierScoreDistribution(std::string& fName)
   {
     // read edge order data from file into edgeOrder
     std::string fDirName       = "/u/otyagi/cbmroot/NN/output/";
     std::string fScoreFileName = fDirName + fName + "Score.txt";
     std::ifstream fin;
     fin.open(fScoreFileName);
     if (!fin.is_open()) {
       std::cerr << "Error: could not open file " << fScoreFileName << std::endl;
       return;
     }
   
     // read until the end of the file, each row is a score for a single label
     // row format: label score    . label - True=0/Fake=1
     std::vector<float> trueLabelScores;
     std::vector<float> fakeLabelScores;
     float score;
     int label;
     while (fin >> label >> score) {
       if (label == 0) {
         trueLabelScores.push_back(score);
       }
       else {
         fakeLabelScores.push_back(score);
       }
     }
     fin.close();
   
     std::cout << "L1AlgoDraw: True: " << trueLabelScores.size() << " Fake: " << fakeLabelScores.size() << std::endl;
   
     int nBins    = 100;
     int maxScore = 1;
     TCanvas* c1  = new TCanvas("c1", "Histo", -1, 0, 500, 500);
     TH1F* histo  = new TH1F("Legend", "Classifier score distribution", nBins, 0, maxScore);
     // create two histograms together for true and fake edge scores in blue and red
     for (auto value : trueLabelScores) {
       histo->Fill(value);
     }
     histo->SetStats(0);
     histo->SetLineColor(kBlue);
     histo->Draw();
   
     TH1F* histo2 = new TH1F("Legend", "Fake Label classifier score distribution", nBins, 0, maxScore);
     for (auto value : fakeLabelScores) {
       histo2->Fill(value);
     }
     histo2->SetStats(0);
     histo2->SetLineColor(kRed);
     histo2->Draw("same");
   
   
     TString tmp = fDirName + fName + "_ScoreDistribution.pdf";
     c1->SaveAs(tmp);
   
     std::cout << "Edge score distribution saved to " << tmp << std::endl;
   }
   
   
   void L1AlgoDraw::drawEdgeOrderHisto()
   {
   
     // read edge order data from file into edgeOrder
     std::string fedgeOrderFile = "/u/otyagi/cbmroot/NN/output/embed_EdgeOrder.txt";
     std::ifstream fin;
     fin.open(fedgeOrderFile);
     if (!fin.is_open()) {
       std::cerr << "Error: could not open file " << fedgeOrderFile << std::endl;
       return;
     }
     int nEntries;
     fin >> nEntries;
     std::vector<int> edgeOrder(nEntries);
     for (int i = 0; i < nEntries; i++) {
       fin >> edgeOrder[i];
     }
     fin.close();
   
     gStyle->SetOptStat("nemruo");
     const int nBins    = 100;
     const int maxOrder = 500;
     TCanvas* c1        = new TCanvas("c1", "Histo", -1, 0, 500, 500);
   
     TH1F* histo = new TH1F("Legend", "kNN order distribution", nBins, 0, maxOrder);
     for (auto value : edgeOrder) {
       histo->Fill(value);
     }
     histo->SetLineColor(kBlue);
   
     // Set y-axis range
     histo->SetMinimum(0.5);
     // histo->SetMaximum(2000);
   
     c1->SetLogy();
     histo->Draw();
   
     TString tmp = "/u/otyagi/cbmroot/NN/output/test_kNN_Order_Histo.pdf";
     c1->SaveAs(tmp);
   
     delete c1;
     delete histo;
     gStyle->SetOptStat("nemr");  // reset to default
   
     std::cout << "Edge order histogram saved to " << tmp << std::endl;
   }
   
   /// @brief: Draw the R-z projection graph for all reconstructible MC tracks
   void L1AlgoDraw::DrawRzProjection()
   {
     TCanvas* c1 = new TCanvas("c1", "Rz projection", 700, 500);
     std::vector<std::vector<double>> lrs, lzs;
     std::vector<bool> isReconstructed;
   
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
     for (const auto& mcTrk : mcData.GetTrackContainer()) {
       // draw reconstructable tracks only
       if (!mcTrk.IsReconstructable()) continue;
   
       isReconstructed.push_back(mcTrk.IsReconstructed());
   
       double par[3];
   
       std::vector<double> lr, lz;
       /// start track from its vertex
       // lx.push_back(par[0]);
       // ly.push_back(par[1]);
       // lz.push_back(par[5]);
   
       for (int ip : mcTrk.GetPointIndexes()) {
         auto& point = mcData.GetPoint(ip);
         par[0]      = point.GetX();
         par[1]      = point.GetY();
         par[2]      = point.GetZ();
         double dist = sqrt(par[0] * par[0] + par[1] * par[1]);
         lr.push_back(dist);
         lz.push_back(par[2]);
       }
   
       lrs.push_back(lr);
       lzs.push_back(lz);
   
     }  // over MC tracks
   
     const int nTracks = lrs.size();
     TMultiGraph* mg   = new TMultiGraph();
     /// add target
     TGraph* target = new TGraph(1);
     target->SetPoint(0, -44.0, 0);
     target->SetMarkerStyle(29);
     target->SetMarkerColor(6);  // magenta
     target->SetMarkerSize(2);
     mg->Add(target);
   
     /// add tracks
     TGraph* g[nTracks];
     for (int i = 0; i < nTracks; i++) {
       g[i] = new TGraph(lzs[i].size(), &lzs[i][0], &lrs[i][0]);
       if (isReconstructed[i]) {
         g[i]->SetLineColor(kBlue);
       }
       else {
         g[i]->SetLineColor(kRed);
       }
       g[i]->SetMarkerStyle(4);
       g[i]->SetMarkerSize(0.5);
       mg->Add(g[i]);
     }
     mg->Draw("ALP");
     mg->SetTitle("R-z Projection");
     mg->GetXaxis()->SetTitle("z [cm]");
     mg->GetXaxis()->CenterTitle();
     mg->GetYaxis()->SetTitle("r [cm]");
     mg->GetYaxis()->CenterTitle();
     mg->GetYaxis()->SetRangeUser(0, 50);
     mg->GetXaxis()->SetRangeUser(-45, 65);
   
   
     chdir("L1CADraw");
     TString tmp = "eventRZProj.pdf";
     c1->SaveAs(tmp);
   }
   
   ///@brief: calc curvature for triplets in reconstructable mc tracks and plot histogram of curvatures
   void L1AlgoDraw::DrawCurvatureDistribution()
   {
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
   
     std::vector<float> curvatures;
     for (const auto& mcTrk : mcData.GetTrackContainer()) {
       if (!mcTrk.IsReconstructable()) continue;  // only reconstructable tracks
   
       /// calculate curvature of triplets on consecutive stations
       for (int i = 0; i < mcTrk.GetNofPoints() - 2; i++) {
         const auto& point1 = mcData.GetPoint(mcTrk.GetPointIndexes()[i]);
         const auto& point2 = mcData.GetPoint(mcTrk.GetPointIndexes()[i + 1]);
         const auto& point3 = mcData.GetPoint(mcTrk.GetPointIndexes()[i + 2]);
         /// check if stations are consecutive
         if (point1.GetStationId() + 1 != point2.GetStationId() || point2.GetStationId() + 1 != point3.GetStationId()) {
           continue;
         }
         curvatures.push_back(calcCurvature(point1.GetX(), point1.GetY(), point1.GetZ(), point2.GetX(), point2.GetY(),
                                            point2.GetZ(), point3.GetX(), point3.GetY(), point3.GetZ()));
       }
     }
   
     std::cout << "Number of curvatures: " << curvatures.size() << std::endl;
     std::cout << "Min curvature: " << *std::min_element(curvatures.begin(), curvatures.end()) << std::endl;
     float maxCurvature = *std::max_element(curvatures.begin(), curvatures.end());
     std::cout << "Max curvature: " << maxCurvature << std::endl;
   
     /// plot histogram of curvatures
     TCanvas* c1 = new TCanvas("c1", "Curvature", 700, 500);
     TH1F* histo = new TH1F("Legend", "Triplet curvature distribution", 100, 0, maxCurvature);
     for (const auto value : curvatures) {
       histo->Fill(value);
     }
     histo->SetStats(0);
     histo->SetLineColor(kBlue);
     histo->Draw();
   
     // save histogram
     chdir("L1CADraw");
     TString tmp = "CurvatureDistribution.pdf";
     c1->SaveAs(tmp);
   
     std::cout << "Curvature distribution saved to " << tmp << std::endl;
   }
   
   /// calculates curvature of three hits
   float L1AlgoDraw::calcCurvature(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3,
                                   float z3)
   {
     float k = 0.0f;
   
     // calc distance between points
     float d12 = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1) + (z2 - z1) * (z2 - z1));
     float d23 = sqrt((x3 - x2) * (x3 - x2) + (y3 - y2) * (y3 - y2) + (z3 - z2) * (z3 - z2));
     float d31 = sqrt((x1 - x3) * (x1 - x3) + (y1 - y3) * (y1 - y3) + (z1 - z3) * (z1 - z3));
   
     // check if triplet is collinear.
     if ((d12 + d23) == d31) {
       k = 0;
       return k;
     }
   
     // calc area of triangle (Heron's formula)
     float s    = (d12 + d23 + d31) / 2.0f;
     float area = sqrt(s * (s - d12) * (s - d23) * (s - d31));
   
     // calc curvature
     k = (4.0f * area) / (d12 * d23 * d31);
   
     return k;
   }
   
   
   void L1AlgoDraw::DrawClones()
   {
     int nLongPrim = 0;
   
     int NCloneTracks = 0;
     CbmL1& L1        = *CbmL1::Instance();
     TPolyLine pline;
   
     const auto& mcData = L1.GetMCData();
     for (const auto& mcTrk : mcData.GetTrackContainer()) {
       //draw reconstructable tracks only
       if (!mcTrk.IsReconstructable()) continue;
       if (mcTrk.GetP() < 0.1) continue;
   
       // select only long primary tracks
   
       int nStaHit       = mcTrk.GetTotNofStationsWithHit();
       int nStaPoint     = mcTrk.GetTotNofStationsWithPoint();
       int nMaxPointsSta = mcTrk.GetMaxNofPointsOnStation();
       int nMaxHitsSta   = mcTrk.GetMaxNofHitsOnStation();
       if (mcTrk.IsAdditional()) continue;
       if (!mcTrk.IsPrimary()) continue;
       if (mcTrk.GetP() < 1) continue;
       if (nStaHit != 12) continue;
       nLongPrim++;
       if (nLongPrim > 3) break;
   
   
       pline.SetLineColor(kRed);
       int npoints = mcTrk.GetNofPoints();
       if (npoints < 1) continue;
   
       double par[3];
   
       bool ok = true;
   
       std::cout << "Num clones: " << mcTrk.GetNofClones() << std::endl;
       std::cout << "Points in track: " << npoints << std::endl;
       std::cout << "Hits in track: " << mcTrk.GetNofHits() << std::endl;
       std::cout << "Total number of stations with hit: " << nStaHit << std::endl;
       std::cout << "Max points on station: " << nMaxPointsSta << std::endl;
       std::cout << "Max hits on station: " << nMaxHitsSta << std::endl;
   
       for (auto ireco : mcTrk.GetRecoTrackIndexes()) {
         vector<double> lx, ly, lz;
         const auto& recoTrack = L1.fvRecoTracks[ireco];
         for (unsigned int i = 0; i < recoTrack.Hits.size(); i++) {
           const ca::Hit& h = algo->GetInputData().GetHit(recoTrack.Hits[i]);
           par[0]           = h.X();
           par[1]           = h.Y();
           par[2]           = h.Z();
           std::cout << "x = " << par[0] << " y = " << par[1] << " z = " << par[2] << std::endl;
           lx.push_back(par[0]);
           ly.push_back(par[1]);
           lz.push_back(par[2]);
         }
   
         if (ok) {
           NCloneTracks++;
           pline.SetLineColor(NCloneTracks % 10 + 1);
           YZ->cd();
           pline.DrawPolyLine(lx.size(), &(lz[0]), &(ly[0]));
           XZ->cd();
           pline.DrawPolyLine(lx.size(), &(lz[0]), &(lx[0]));
           YX->cd();
           pline.DrawPolyLine(lx.size(), &(lx[0]), &(ly[0]));
         }
         std::cout << "End Track " << std::string(30, '-') << std::endl;
       }
       std::cout << "End Clones " << std::string(80, '=') << std::endl;
     }
     cout << "L1CADraw: number of clone tracks drawn: " << NCloneTracks << endl;
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   }
   
   void L1AlgoDraw::DrawTouchTracks() {}
   
   void L1AlgoDraw::drawSavedHitsByTrackType()
   {
   
     const std::string fName = "/u/otyagi/cbmroot/macro/run/hitsByTrackType.txt";
     // read until the end of the file. Each line is: track type, no of hits
     // track type: 1 - true, 0 - ghost
     std::ifstream fin;
     fin.open(fName);
     if (!fin.is_open()) {
       std::cerr << "Error: could not open file " << fName << std::endl;
       return;
     }
     std::vector<int> trueTrackHits;
     std::vector<int> ghostTrackHits;
     int edgeType;
     int nHits;
     while (fin >> edgeType >> nHits) {
       if (edgeType == 0) {
         ghostTrackHits.push_back(nHits);
       }
       else {
         trueTrackHits.push_back(nHits);
       }
     }
     fin.close();
     std::cout << "Read " << trueTrackHits.size() << " true tracks and " << ghostTrackHits.size() << " ghost tracks \n";
   
   
     /// plot histogram. no of hits x axis, no of tracks y axis. Two tracks: true and ghost
     TCanvas* c1      = new TCanvas("c1", "Num Hits by Track Type", 700, 500);
     TH1F* histoGhost = new TH1F("Legend", "Ghost", 12, 0, 12);
     TH1F* histoTrue  = new TH1F("Legend", "True", 12, 0, 12);
   
     histoTrue->SetStats(0);
     histoTrue->SetLineColor(kBlue);
     histoGhost->SetStats(0);
     histoGhost->SetLineColor(kBlack);
   
     for (const auto value : trueTrackHits) {
       histoTrue->Fill(value);
     }
     for (const auto value : ghostTrackHits) {
       histoGhost->Fill(value);
     }
   
     histoGhost->Draw();
     histoTrue->Draw("same");
   
     // save histogram
     chdir("L1CADraw");
     TString tmp = "Num_Hits_by_TrackType.pdf";
     c1->SaveAs(tmp);
   
     std::cout << "Saved to " << tmp << std::endl;
   }
   
   void L1AlgoDraw::drawFalseNegativeEdges()
   {
     const std::string filename = "/u/otyagi/cbmroot/NN/output/falseNegativeEdges.txt";
     std::ifstream file(filename);
     if (!file.is_open()) {
       std::cerr << "Error: could not open file " << filename << std::endl;
       return;
     }
     // read until the end of the file. Each line is edge with: x1 y1 z1 x2 y2 z2
     std::vector<std::vector<float>> edges;
     std::string line;
     while (std::getline(file, line)) {
       std::istringstream iss(line);
       std::vector<float> edge(6);
       iss >> edge[0] >> edge[1] >> edge[2] >> edge[3] >> edge[4] >> edge[5];
       edges.push_back(edge);
     }
     file.close();
     int nEdges = edges.size();
     std::cout << "[L1AlgoDraw]Read " << nEdges << " false negative edges from file: " << filename << std::endl;
   
     /// draw edges on canvas
     TPolyLine pline;
     pline.SetLineColor(kBlue);
     TMarker marker;
     marker.SetMarkerColor(kBlue);
     marker.SetMarkerStyle(8);
     marker.SetMarkerSize(HitSize * 1);
   
     int nDrawn      = 0;
     const int nHits = 2;
     for (int i = 0; i < nEdges; i++) {
       if (i % 500 != 0) continue;
       std::vector<float>& edge = edges[i];
       vector<double> lx, ly, lz;
   
       lx.push_back(edge[0]);
       lx.push_back(edge[3]);
       ly.push_back(edge[1]);
       ly.push_back(edge[4]);
       lz.push_back(edge[2]);
       lz.push_back(edge[5]);
   
       YZ->cd();
       pline.DrawPolyLine(nHits, &(lz[0]), &(ly[0]));
       XZ->cd();
       pline.DrawPolyLine(nHits, &(lz[0]), &(lx[0]));
       YX->cd();
       pline.DrawPolyLine(nHits, &(lx[0]), &(ly[0]));
   
       nDrawn++;
     }
     std::cout << "[L1AlgoDraw] Drawn " << nDrawn << " false negative edges." << '\n';
   }
   
   void L1AlgoDraw::drawDaughterPairs()
   {
     // draw MC tracks where both daughters are reconstructable
     int NRegMCTracks   = 0;
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
   
     TPolyLine pline;
   
     std::vector<int> trackIndexes;
     for (int iTrack = 0; iTrack < mcData.GetTrackContainer().size(); iTrack++) {
       const auto& mcTrk = mcData.GetTrackContainer()[iTrack];
       // draw reconstructable tracks only
       if (!mcTrk.IsReconstructable()) continue;
       /// min 4 hits
       if (mcTrk.GetNofHits() < 4) continue;
       /// no jump tracks
       if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
       // must be secondary
       if (mcTrk.IsPrimary()) continue;
   
       // skip if already in trackIndexes
       if (std::find(trackIndexes.begin(), trackIndexes.end(), iTrack) != trackIndexes.end()) continue;
   
       // find if daughter is reconstructable
       const int motherID                = mcTrk.GetMotherId();
       bool otherDaughterReconstructable = false;
       int otherDaughterIndex            = -1;
       for (int jTrack = 0; jTrack < mcData.GetTrackContainer().size(); jTrack++) {
         if (iTrack == jTrack) continue;
         const auto& mcTrk2 = mcData.GetTrackContainer()[jTrack];
         if (mcTrk2.GetMotherId() != motherID) continue;
         if (!mcTrk2.IsReconstructable()) continue;
         if (mcTrk2.GetNofHits() < 4) continue;
         if (mcTrk2.GetNofConsStationsWithHit() != mcTrk2.GetTotNofStationsWithHit()) continue;
         if (mcTrk2.IsPrimary()) continue;
   
         otherDaughterReconstructable = true;
         otherDaughterIndex           = jTrack;
         break;
       }
       if (!otherDaughterReconstructable) continue;
   
       // if iTrack already in trackIndexes, skip
       if (std::find(trackIndexes.begin(), trackIndexes.end(), iTrack) == trackIndexes.end()) {
         trackIndexes.push_back(iTrack);
         trackIndexes.push_back(otherDaughterIndex);
       }
   
     }  // over MC tracks
   
     std::cout << "L1CADraw: number of daughter pair reconstructable tracks: " << trackIndexes.size() / 2 << std::endl;
   
     // line styles
     std::vector<int> trackColors{1, 2, 3, 4, 6, 7};
     std::vector<int> trackStyles{1, 3, 6};
     std::vector<int> trackThickness{1, 2, 3};
   
     for (int iTrack = 0; iTrack < trackIndexes.size(); iTrack++) {
   
       const auto& mcTrk = mcData.GetTrackContainer()[trackIndexes[iTrack]];
   
       int color = trackColors[(iTrack / 2) % trackColors.size()];
       pline.SetLineColor(color);
       int style = trackStyles[(iTrack / (2 * trackColors.size())) % trackStyles.size()];
       pline.SetLineStyle(style);
       int thickness = trackThickness[(iTrack / (2 * trackColors.size() * trackStyles.size())) % trackThickness.size()];
       pline.SetLineWidth(thickness);
       // std::cout << "Drawing track " << iTrack << " with color " << color << " style " << style << " thickness "
       //           << thickness << " with hits " << mcTrk.GetNofHits() << std::endl;
   
       double par[6];
       par[0] = mcTrk.GetStartX();
       par[1] = mcTrk.GetStartY();
       par[2] = mcTrk.GetTx();
       par[3] = mcTrk.GetTy();
       par[4] = mcTrk.GetCharge() / mcTrk.GetP();
       par[5] = mcTrk.GetStartZ();
   
       vector<double> lx, ly, lz;
       vector<int> matchedWithHit;  // 1 - MC point is matched with hit, 0 - not matched
       vector<int> hitUsedEarlier;  // 1 - hit was used earlier, 0 - not used
       // start track from its vertex
       lx.push_back(par[0]);
       ly.push_back(par[1]);
       lz.push_back(par[5]);
   
       bool ok = true;
   
       for (int ip : mcTrk.GetPointIndexes()) {
         auto& point = mcData.GetPoint(ip);
   
         int isMatched = point.GetHitIndexes().size() > 0 ? 1 : 0;
         matchedWithHit.push_back(isMatched);
   
         par[0] = point.GetX();
         par[1] = point.GetY();
         par[2] = point.GetTx();
         par[3] = point.GetTy();
         par[4] = point.GetQp();
         par[5] = point.GetZ();
   
         lx.push_back(par[0]);
         ly.push_back(par[1]);
         lz.push_back(par[5]);
       }
   
       if (ok) {
         NRegMCTracks++;
         // TMarker marker;
         // marker.SetMarkerColor(kBlack);
         // marker.SetMarkerStyle(24);  // 26 - open triangle, 24 - open circle
         // marker.SetMarkerSize(HitSize);
   
         YZ->cd();
         pline.DrawPolyLine(lx.size(), &(lz[0]), &(ly[0]));
         // for (int i = 0; i < int(lx.size()); i++) {
         //   if (matchedWithHit[i] == 1) {
         //     marker.SetMarkerColor(kBlack);
         //     // if (hitUsedEarlier[i] == 1) {
         //     //   markerUsed.DrawMarker(lz[i], ly[i]);
         //     // }
         //   }
         //   else {  // no match of point with hit
         //     marker.SetMarkerColor(kRed);
         //   }
         //   marker.DrawMarker(lz[i], ly[i]);
         // }
         XZ->cd();
         pline.DrawPolyLine(lx.size(), &(lz[0]), &(lx[0]));
         // for (int i = 0; i < int(lx.size()); i++) {
         //   if (matchedWithHit[i] == 1) {
         //     marker.SetMarkerColor(kBlack);
         //     // if (hitUsedEarlier[i] == 1) {
         //     //   markerUsed.DrawMarker(lz[i], lx[i]);
         //     // }
         //   }
         //   else {
         //     marker.SetMarkerColor(kRed);
         //   }
         //   marker.DrawMarker(lz[i], lx[i]);
         // }
         YX->cd();
         pline.DrawPolyLine(lx.size(), &(lx[0]), &(ly[0]));
         // for (int i = 0; i < int(lx.size()); i++) {
         //   if (matchedWithHit[i] == 1) {
         //     marker.SetMarkerColor(kBlack);
         //   }
         //   else {
         //     marker.SetMarkerColor(kRed);
         //   }
         //   // marker.DrawMarker(lx[i], ly[i]);
         // }
       }
   
     }  // over MC tracks
   
     cout << "L1CADraw: number of daughter pair reconstructable tracks drawn: " << NRegMCTracks << endl;
   
     YZ->cd();
     YZ->Update();
     XZ->cd();
     XZ->Update();
     YX->cd();
     YX->Update();
   }
   
   void L1AlgoDraw::drawSavedTrainingEdges()
   {
     const std::string filename = "/u/otyagi/cbmroot/NN/data/secondary_edges_true_ev_500_800.txt";
     std::ifstream file(filename);
     if (!file.is_open()) {
       std::cerr << "Error: could not open file " << filename << std::endl;
       return;
     }
     // read until the end of the file. Each line is edge with: evNum x1 y1 z1 sta1 x2 y2 z2 sta2
     std::vector<std::vector<float>> edges;
     std::string line;
     float dummy;
     while (std::getline(file, line)) {
       std::istringstream iss(line);
       std::vector<float> edge(6);
       iss >> dummy >> edge[0] >> edge[1] >> edge[2] >> dummy >> edge[3] >> edge[4] >> edge[5] >> dummy;
       edges.push_back(edge);
     }
     file.close();
     int nEdges = edges.size();
     std::cout << "[L1AlgoDraw]Read " << nEdges << " training edges from file: " << filename << std::endl;
   
     /// draw edges on canvas
     TPolyLine pline;
     pline.SetLineColor(kRed);
   
     int nDrawn      = 0;
     const int nHits = 2;
     for (int i = 0; i < nEdges; i++) {
       if (i % 1000 != 0) continue;
       std::vector<float>& edge = edges[i];
       vector<double> lx, ly, lz;
   
       lx.push_back(edge[0]);
       lx.push_back(edge[3]);
       ly.push_back(edge[1]);
       ly.push_back(edge[4]);
       lz.push_back(edge[2]);
       lz.push_back(edge[5]);
   
       YZ->cd();
       pline.DrawPolyLine(nHits, &(lz[0]), &(ly[0]));
       XZ->cd();
       pline.DrawPolyLine(nHits, &(lz[0]), &(lx[0]));
       YX->cd();
       pline.DrawPolyLine(nHits, &(lx[0]), &(ly[0]));
   
       nDrawn++;
     }
     std::cout << "[L1AlgoDraw] Drawn " << nDrawn << " training edges." << '\n';
   }
   
   void L1AlgoDraw::DrawEmbedding()
   {
     // get hits from remainingHitsIds
     const auto& remainingHitsWindow = algo->remainingHitsIds;  // fWindowHits
     std::vector<int> remainingHits;                            // in fInputData
     std::vector<std::vector<float>> hits;
     for (auto ih : remainingHitsWindow) {
       remainingHits.push_back(algo->fWindowHits[ih].Id());
       std::vector<float> hit;
       hit.push_back(algo->fWindowHits[ih].X());
       hit.push_back(algo->fWindowHits[ih].Y());
       hit.push_back(algo->fWindowHits[ih].Z());
       hits.push_back(hit);
     }
     int nhits = hits.size();
     std::cout << "[L1AlgoDraw] Read " << nhits << std::endl;
   
     /// label hits to which track they belong
     std::vector<int> hitLabels(nhits, -1);
     std::vector<int> hitType(nhits, 0);  // 0 - ghost, 1 - true(non-reconstructable), 2 - true(reconstructable)
   
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
     for (int iTrack = 0; iTrack < mcData.GetTrackContainer().size(); iTrack++) {
       const auto& mcTrk = mcData.GetTrackContainer()[iTrack];
       for (int ip : mcTrk.GetPointIndexes()) {
         const auto& point     = mcData.GetPoint(ip);
         const auto& hitsTrack = point.GetHitIndexes();  // hit index in fInputData
         if (hitsTrack.size() == 0) continue;
         for (const auto& hitIndex : hitsTrack) {
           // find ih in remainingHits
           auto it = std::find(remainingHits.begin(), remainingHits.end(), hitIndex);
           if (it == remainingHits.end()) continue;
           int index        = std::distance(remainingHits.begin(), it);
           hitLabels[index] = iTrack;
           hitType[index]   = 1;
           if (!mcTrk.IsReconstructable()) continue;
           if (mcTrk.GetNofHits() < 4) continue;
           if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
           if (mcTrk.GetP() < 0.1) continue;
           /// skip tracks not found in GNN iteration
           bool found_gnn_iter = true;
           for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
             const auto& rTrk = L1.fvRecoTracks[irt];
             if (rTrk.fFoundInIteration != 3) {
               found_gnn_iter = false;
               break;
             }
           }
           if (!found_gnn_iter) continue;
           hitType[index] = 2;
         }
       }
     }
   
     std::vector<int> trackLabels;
     for (int iLabel = 0; iLabel < hitLabels.size(); iLabel++) {
       int label = hitLabels[iLabel];
       int type  = hitType[iLabel];
       if (label >= 0 && type == 2 && std::find(trackLabels.begin(), trackLabels.end(), label) == trackLabels.end()) {
         trackLabels.push_back(label);
       }
     }
     std::cout << "Number of reconstructable tracks: " << trackLabels.size() << std::endl;
   
     std::vector<std::vector<int>> tracks;  // index in hits
     for (const auto& label : trackLabels) {
       std::vector<int> track;
       for (int i = 0; i < hitLabels.size(); i++) {
         if (hitLabels[i] == label) {
           track.push_back(i);
         }
       }
       tracks.push_back(track);
     }
     int totalHits = 0;
     for (const auto& track : tracks) {
       totalHits += track.size();
     }
     std::cout << "Total number of hits in all tracks: " << totalHits << std::endl;
     int nHitsType2 = std::count(hitType.begin(), hitType.end(), 2);
     std::cout << "Number of hits with hitType = 2: " << nHitsType2 << std::endl;
     int nHitsType1 = std::count(hitType.begin(), hitType.end(), 1);
     std::cout << "Number of hits with hitType = 1: " << nHitsType1 << std::endl;
     int nHitsType0 = std::count(hitType.begin(), hitType.end(), 0);
     std::cout << "Number of hits with hitType = 0: " << nHitsType0 << std::endl;
   
     // print num of non-negative labels in hitLabels
     int nNonNegLabels = std::count_if(hitLabels.begin(), hitLabels.end(), [](int i) { return i >= 0; });
     std::cout << "Number of hits matched with tracks: " << nNonNegLabels << std::endl;
   
   
     // 2. embed hits with embedNet
     std::vector<std::vector<float>> embedCoord;
     embedCoord.reserve(10000);
     std::vector<std::vector<float>> NNinput;
     NNinput.reserve(10000);
     std::vector<std::vector<std::vector<float>>> NNinputFinal;
     std::vector<int> EmbNetTopology_ = {3, 16, 16, 4};
     EmbedNet EmbNet_                 = EmbedNet(EmbNetTopology_);
     std::string srcDir               = "/u/otyagi/cbmroot/NN/";
     std::string fNameModel           = "models/embed";  // saved models in: "models/embed"
     std::string fNameWeights         = srcDir + fNameModel + "Weights.txt";
     std::string fNameBiases          = srcDir + fNameModel + "Biases.txt";
     EmbNet_.loadModel(fNameWeights, fNameBiases);
   
     for (const auto& hit : hits) {
       std::vector<fscal> input;
       input.push_back(hit[0]);
       input.push_back(hit[1]);
       input.push_back(hit[2] + 44.0f);  // shift z to positive
       NNinput.push_back(input);
     }
     NNinputFinal.push_back(NNinput);
     EmbNet_.run(NNinputFinal);
     EmbNet_.getEmbeddedCoords(embedCoord, 0);  // only works for event one.
     std::cout << "EmbedCoord size: " << embedCoord.size() << std::endl;
   
     // draw hits in 1-2 projection
     nhits = embedCoord.size();
     float e1_poly[nhits], e2_poly[nhits], e3_poly[nhits], e4_poly[nhits];
     int n_poly = 0;
     for (int ih = 0; ih < nhits; ih++) {
       const auto& hit = embedCoord[ih];
       const float e1  = hit[0];
       const float e2  = hit[1];
       const float e3  = hit[2];
       const float e4  = hit[3];
   
       e1_poly[n_poly] = e1;
       e2_poly[n_poly] = e2;
       e3_poly[n_poly] = e3;
       e4_poly[n_poly] = e4;
       n_poly++;
     }
   
     // find min and max for e1, e2, e3, e4
     float min_e1 = *std::min_element(e1_poly, e1_poly + n_poly);
     float max_e1 = *std::max_element(e1_poly, e1_poly + n_poly);
     float min_e2 = *std::min_element(e2_poly, e2_poly + n_poly);
     float max_e2 = *std::max_element(e2_poly, e2_poly + n_poly);
     float min_e3 = *std::min_element(e3_poly, e3_poly + n_poly);
     float max_e3 = *std::max_element(e3_poly, e3_poly + n_poly);
     float min_e4 = *std::min_element(e4_poly, e4_poly + n_poly);
     float max_e4 = *std::max_element(e4_poly, e4_poly + n_poly);
   
     // print min and max
     std::cout << "min e1: " << min_e1 << " max e1: " << max_e1 << std::endl;
     std::cout << "min e2: " << min_e2 << " max e2: " << max_e2 << std::endl;
     std::cout << "min e3: " << min_e3 << " max e3: " << max_e3 << std::endl;
     std::cout << "min e4: " << min_e4 << " max e4: " << max_e4 << std::endl;
   
     // 12
     TCanvas* c_12 = new TCanvas("c1", "Proj_12", 700, 500);
     TCanvas* c_13 = new TCanvas("c2", "Proj_13", 700, 500);
     TCanvas* c_14 = new TCanvas("c3", "Proj_14", 700, 500);
     TCanvas* c_23 = new TCanvas("c4", "Proj_23", 700, 500);
     TCanvas* c_24 = new TCanvas("c5", "Proj_24", 700, 500);
     TCanvas* c_34 = new TCanvas("c6", "Proj_34", 700, 500);
   
     const float text_scale = 0.9;
     TLatex latex;
     latex.SetTextFont(132);
     latex.SetTextAlign(12);
     latex.SetTextSize(0.035);
   
     const float scale_border = 1.1;
     {
       c_12->Range(scale_border * min_e1, scale_border * min_e2, scale_border * max_e2,
                   scale_border * max_e2);  // set coords frame in canvas. bottom left and top right
       c_12->Draw();
       c_12->Update();
   
       // 13
       c_13->Range(scale_border * min_e1, scale_border * min_e3, scale_border * max_e1,
                   scale_border * max_e3);  // set coords frame in canvas. bottom left and top right
       c_13->Draw();
       c_13->Update();
   
       // 14
       c_14->Range(scale_border * min_e1, scale_border * min_e4, scale_border * max_e1,
                   scale_border * max_e4);  // set coords frame in canvas. bottom left and top right
       c_14->Draw();
       c_14->Update();
   
       // 23
       c_23->Range(scale_border * min_e2, scale_border * min_e3, scale_border * max_e2,
                   scale_border * max_e3);  // set coords frame in canvas. bottom left and top right
       c_23->Draw();
       c_23->Update();
   
       // 24
       c_24->Range(scale_border * min_e2, scale_border * min_e4, scale_border * max_e2,
                   scale_border * max_e4);  // set coords frame in canvas. bottom left and top right
       c_24->Draw();
       c_24->Update();
   
       // 34
       c_34->Range(scale_border * min_e3, scale_border * min_e4, scale_border * max_e3,
                   scale_border * max_e4);  // set coords frame in canvas. bottom left and top right
       c_34->Draw();
       c_34->Update();
     }
   
     int colorUnReco = 13;
     int colorGhost  = 16;
     std::vector<int> colors{2, 3, 4, 5, 6, 7, 8, 9, 30, 38, 41, 44, 46, 49};
     int trueStyle  = 20;
     int ghostStyle = 24;
     // marker size
     float recoSize   = 0.8;
     float unRecoSize = 0.5;
     float ghostSize  = 0.3;
   
     TPolyLine pline;
     std::vector<TMarker*> markers;
   
     c_12->cd();
     latex.DrawLatex(text_scale * min_e1, text_scale * max_e2, "e1-e2");
     // Draw hits
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e1 = hit[0];
       const double e2 = hit[1];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       // default - ghost
       int color  = colorGhost;
       int style  = ghostStyle;
       float size = ghostSize;
   
       TMarker* marker = new TMarker(e1, e2, ghostStyle);
       if (type == 2) {  // reco
         color = colors[label % colors.size()];
         style = trueStyle;
         size  = recoSize;
       }
       else if (type == 1) {  // unreco
         color = colorUnReco;
         size  = unRecoSize;
       }
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);  // Keep markers alive
     }
     // Draw tracks
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e1_track(nHitsTrack), e2_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e1_track[iHit]   = hit1[0];
         e2_track[iHit]   = hit1[1];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e1_track[0]), &(e2_track[0]));
     }
     c_12->Update();
     markers.clear();
   
     c_13->cd();
     latex.DrawLatex(text_scale * min_e1, text_scale * max_e3, "e1-e3");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e1 = hit[0];
       const double e3 = hit[2];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colorGhost;
       int style  = ghostStyle;
       float size = ghostSize;
   
       TMarker* marker = new TMarker(e1, e3, ghostStyle);
       if (type == 2) {
         color = colors[label % colors.size()];
         style = trueStyle;
         size  = recoSize;
       }
       else if (type == 1) {
         color = colorUnReco;
         size  = unRecoSize;
       }
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e1_track(nHitsTrack), e3_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e1_track[iHit]   = hit1[0];
         e3_track[iHit]   = hit1[2];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e1_track[0]), &(e3_track[0]));
     }
     c_13->Update();
     markers.clear();
   
     c_14->cd();
     latex.DrawLatex(text_scale * min_e1, text_scale * max_e4, "e1-e4");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e1 = hit[0];
       const double e4 = hit[3];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colorGhost;
       int style  = ghostStyle;
       float size = ghostSize;
   
       TMarker* marker = new TMarker(e1, e4, ghostStyle);
       if (type == 2) {
         color = colors[label % colors.size()];
         style = trueStyle;
         size  = recoSize;
       }
       else if (type == 1) {
         color = colorUnReco;
         size  = unRecoSize;
       }
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e1_track(nHitsTrack), e4_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e1_track[iHit]   = hit1[0];
         e4_track[iHit]   = hit1[3];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e1_track[0]), &(e4_track[0]));
     }
     c_14->Update();
     markers.clear();
   
     c_23->cd();
     latex.DrawLatex(text_scale * min_e2, text_scale * max_e3, "e2-e3");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e2 = hit[1];
       const double e3 = hit[2];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colorGhost;
       int style  = ghostStyle;
       float size = ghostSize;
   
       TMarker* marker = new TMarker(e2, e3, ghostStyle);
       if (type == 2) {
         color = colors[label % colors.size()];
         style = trueStyle;
         size  = recoSize;
       }
       else if (type == 1) {
         color = colorUnReco;
         size  = unRecoSize;
       }
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e2_track(nHitsTrack), e3_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e2_track[iHit]   = hit1[1];
         e3_track[iHit]   = hit1[2];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e2_track[0]), &(e3_track[0]));
     }
     c_23->Update();
     markers.clear();
   
     c_24->cd();
     latex.DrawLatex(text_scale * min_e2, text_scale * max_e4, "e2-e4");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e2 = hit[1];
       const double e4 = hit[3];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colorGhost;
       int style  = ghostStyle;
       float size = ghostSize;
   
       TMarker* marker = new TMarker(e2, e4, ghostStyle);
       if (type == 2) {
         color = colors[label % colors.size()];
         style = trueStyle;
         size  = recoSize;
       }
       else if (type == 1) {
         color = colorUnReco;
         size  = unRecoSize;
       }
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e2_track(nHitsTrack), e4_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e2_track[iHit]   = hit1[1];
         e4_track[iHit]   = hit1[3];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e2_track[0]), &(e4_track[0]));
     }
     c_24->Update();
     markers.clear();
   
     c_34->cd();
     latex.DrawLatex(text_scale * min_e3, text_scale * max_e4, "e3-e4");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e3 = hit[2];
       const double e4 = hit[3];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colorGhost;
       int style  = ghostStyle;
       float size = ghostSize;
   
       TMarker* marker = new TMarker(e3, e4, ghostStyle);
       if (type == 2) {
         color = colors[label % colors.size()];
         style = trueStyle;
         size  = recoSize;
       }
       else if (type == 1) {
         color = colorUnReco;
         size  = unRecoSize;
       }
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e3_track(nHitsTrack), e4_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e3_track[iHit]   = hit1[2];
         e4_track[iHit]   = hit1[3];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e3_track[0]), &(e4_track[0]));
     }
     c_34->Update();
     markers.clear();
   
   
     chdir("L1CADraw/embed");
     TString tmp = "proj_12.pdf";  // projection to 1-2 plane
     c_12->SaveAs(tmp);
     tmp = "proj_13.pdf";  // projection to 1-3 plane
     c_13->SaveAs(tmp);
   
     tmp = "proj_14.pdf";  // projection to 1-4 plane
     c_14->SaveAs(tmp);
   
     tmp = "proj_23.pdf";  // projection to 2-3 plane
     c_23->SaveAs(tmp);
   
     tmp = "proj_24.pdf";  // projection to 2-4 plane
     c_24->SaveAs(tmp);
   
     tmp = "proj_34.pdf";  // projection to 3-4 plane
     c_34->SaveAs(tmp);
   
     std::cout << "Embedded projections saved in L1CADraw/embed" << std::endl;
   
     chdir("../..");
   }
   
   
   void L1AlgoDraw::DrawEmbeddingStation()
   {
     // get hits from remainingHitsIds
     const auto& remainingHitsWindow = algo->remainingHitsIds;  // fWindowHits
     std::vector<int> remainingHits;                            // in fInputData
     std::vector<std::vector<float>> hits;
     for (auto ih : remainingHitsWindow) {
       remainingHits.push_back(algo->fWindowHits[ih].Id());
       std::vector<float> hit;
       hit.push_back(algo->fWindowHits[ih].X());
       hit.push_back(algo->fWindowHits[ih].Y());
       hit.push_back(algo->fWindowHits[ih].Z());
       hit.push_back(algo->fWindowHits[ih].Station());
       hits.push_back(hit);
     }
     int nhits = hits.size();
     std::cout << "[L1AlgoDraw] Read " << nhits << std::endl;
   
     /// label hits to which track they belong
     std::vector<int> hitLabels(nhits, -1);
     std::vector<int> hitType(nhits, 0);  // 0 - ghost, 1 - true(non-reconstructable), 2 - true(reconstructable)
   
     CbmL1& L1          = *CbmL1::Instance();
     const auto& mcData = L1.GetMCData();
     for (int iTrack = 0; iTrack < mcData.GetTrackContainer().size(); iTrack++) {
       const auto& mcTrk = mcData.GetTrackContainer()[iTrack];
       for (int ip : mcTrk.GetPointIndexes()) {
         const auto& point     = mcData.GetPoint(ip);
         const auto& hitsTrack = point.GetHitIndexes();  // hit index in fInputData
         if (hitsTrack.size() == 0) continue;
         for (const auto& hitIndex : hitsTrack) {
           // find ih in remainingHits
           auto it = std::find(remainingHits.begin(), remainingHits.end(), hitIndex);
           if (it == remainingHits.end()) continue;
           int index        = std::distance(remainingHits.begin(), it);
           hitLabels[index] = iTrack;
           hitType[index]   = 1;
           if (!mcTrk.IsReconstructable()) continue;
           if (mcTrk.GetNofHits() < 4) continue;
           if (mcTrk.GetNofConsStationsWithHit() != mcTrk.GetTotNofStationsWithHit()) continue;
           if (mcTrk.GetP() < 0.1) continue;
           /// skip tracks not found in GNN iteration
           bool found_gnn_iter = true;
           for (unsigned int irt : mcTrk.GetRecoTrackIndexes()) {
             const auto& rTrk = L1.fvRecoTracks[irt];
             if (rTrk.fFoundInIteration != 3) {
               found_gnn_iter = false;
               break;
             }
           }
           if (!found_gnn_iter) continue;
           hitType[index] = 2;
         }
       }
     }
   
     std::vector<int> trackLabels;
     for (int iLabel = 0; iLabel < hitLabels.size(); iLabel++) {
       int label = hitLabels[iLabel];
       int type  = hitType[iLabel];
       if (label >= 0 && type == 2 && std::find(trackLabels.begin(), trackLabels.end(), label) == trackLabels.end()) {
         trackLabels.push_back(label);
       }
     }
     std::cout << "Number of reconstructable tracks: " << trackLabels.size() << std::endl;
   
     std::vector<std::vector<int>> tracks;  // index in hits
     for (const auto& label : trackLabels) {
       std::vector<int> track;
       for (int i = 0; i < hitLabels.size(); i++) {
         if (hitLabels[i] == label) {
           track.push_back(i);
         }
       }
       tracks.push_back(track);
     }
     int totalHits = 0;
     for (const auto& track : tracks) {
       totalHits += track.size();
     }
     std::cout << "Total number of hits in all tracks: " << totalHits << std::endl;
     int nHitsType2 = std::count(hitType.begin(), hitType.end(), 2);
     std::cout << "Number of hits with hitType = 2: " << nHitsType2 << std::endl;
     int nHitsType1 = std::count(hitType.begin(), hitType.end(), 1);
     std::cout << "Number of hits with hitType = 1: " << nHitsType1 << std::endl;
     int nHitsType0 = std::count(hitType.begin(), hitType.end(), 0);
     std::cout << "Number of hits with hitType = 0: " << nHitsType0 << std::endl;
   
     // print num of non-negative labels in hitLabels
     int nNonNegLabels = std::count_if(hitLabels.begin(), hitLabels.end(), [](int i) { return i >= 0; });
     std::cout << "Number of hits matched with tracks: " << nNonNegLabels << std::endl;
   
   
     // 2. embed hits with embedNet
     std::vector<std::vector<float>> embedCoord;
     embedCoord.reserve(10000);
     std::vector<std::vector<float>> NNinput;
     NNinput.reserve(10000);
     std::vector<std::vector<std::vector<float>>> NNinputFinal;
     std::vector<int> EmbNetTopology_ = {3, 16, 16, 4};
     EmbedNet EmbNet_                 = EmbedNet(EmbNetTopology_);
     std::string srcDir               = "/u/otyagi/cbmroot/NN/";
     std::string fNameModel           = "models/embed";  // saved models in: "models/embed"
     std::string fNameWeights         = srcDir + fNameModel + "Weights.txt";
     std::string fNameBiases          = srcDir + fNameModel + "Biases.txt";
     EmbNet_.loadModel(fNameWeights, fNameBiases);
   
     for (const auto& hit : hits) {
       std::vector<fscal> input;
       input.push_back(hit[0]);
       input.push_back(hit[1]);
       input.push_back(hit[2] + 44.0f);  // shift z to positive
       NNinput.push_back(input);
     }
     NNinputFinal.push_back(NNinput);
     EmbNet_.run(NNinputFinal);
     EmbNet_.getEmbeddedCoords(embedCoord, 0);  // only works for event one.
     std::cout << "EmbedCoord size: " << embedCoord.size() << std::endl;
   
     // draw hits in 1-2 projection
     nhits = embedCoord.size();
     float e1_poly[nhits], e2_poly[nhits], e3_poly[nhits], e4_poly[nhits];
     int n_poly = 0;
     for (int ih = 0; ih < nhits; ih++) {
       const auto& hit = embedCoord[ih];
       const float e1  = hit[0];
       const float e2  = hit[1];
       const float e3  = hit[2];
       const float e4  = hit[3];
   
       e1_poly[n_poly] = e1;
       e2_poly[n_poly] = e2;
       e3_poly[n_poly] = e3;
       e4_poly[n_poly] = e4;
       n_poly++;
     }
   
     // find min and max for e1, e2, e3, e4
     float min_e1 = *std::min_element(e1_poly, e1_poly + n_poly);
     float max_e1 = *std::max_element(e1_poly, e1_poly + n_poly);
     float min_e2 = *std::min_element(e2_poly, e2_poly + n_poly);
     float max_e2 = *std::max_element(e2_poly, e2_poly + n_poly);
     float min_e3 = *std::min_element(e3_poly, e3_poly + n_poly);
     float max_e3 = *std::max_element(e3_poly, e3_poly + n_poly);
     float min_e4 = *std::min_element(e4_poly, e4_poly + n_poly);
     float max_e4 = *std::max_element(e4_poly, e4_poly + n_poly);
   
     // print min and max
     std::cout << "min e1: " << min_e1 << " max e1: " << max_e1 << std::endl;
     std::cout << "min e2: " << min_e2 << " max e2: " << max_e2 << std::endl;
     std::cout << "min e3: " << min_e3 << " max e3: " << max_e3 << std::endl;
     std::cout << "min e4: " << min_e4 << " max e4: " << max_e4 << std::endl;
   
     // 12
     TCanvas* c_12 = new TCanvas("c1", "Proj_12", 700, 500);
     TCanvas* c_13 = new TCanvas("c2", "Proj_13", 700, 500);
     TCanvas* c_14 = new TCanvas("c3", "Proj_14", 700, 500);
     TCanvas* c_23 = new TCanvas("c4", "Proj_23", 700, 500);
     TCanvas* c_24 = new TCanvas("c5", "Proj_24", 700, 500);
     TCanvas* c_34 = new TCanvas("c6", "Proj_34", 700, 500);
   
     const float text_scale = 0.9;
     TLatex latex;
     latex.SetTextFont(132);
     latex.SetTextAlign(12);
     latex.SetTextSize(0.035);
   
     const float scale_border = 1.1;
     {
       c_12->Range(scale_border * min_e1, scale_border * min_e2, scale_border * max_e2,
                   scale_border * max_e2);  // set coords frame in canvas. bottom left and top right
       c_12->Draw();
       c_12->Update();
   
       // 13
       c_13->Range(scale_border * min_e1, scale_border * min_e3, scale_border * max_e1,
                   scale_border * max_e3);  // set coords frame in canvas. bottom left and top right
       c_13->Draw();
       c_13->Update();
   
       // 14
       c_14->Range(scale_border * min_e1, scale_border * min_e4, scale_border * max_e1,
                   scale_border * max_e4);  // set coords frame in canvas. bottom left and top right
       c_14->Draw();
       c_14->Update();
   
       // 23
       c_23->Range(scale_border * min_e2, scale_border * min_e3, scale_border * max_e2,
                   scale_border * max_e3);  // set coords frame in canvas. bottom left and top right
       c_23->Draw();
       c_23->Update();
   
       // 24
       c_24->Range(scale_border * min_e2, scale_border * min_e4, scale_border * max_e2,
                   scale_border * max_e4);  // set coords frame in canvas. bottom left and top right
       c_24->Draw();
       c_24->Update();
   
       // 34
       c_34->Range(scale_border * min_e3, scale_border * min_e4, scale_border * max_e3,
                   scale_border * max_e4);  // set coords frame in canvas. bottom left and top right
       c_34->Draw();
       c_34->Update();
     }
   
     int colorUnReco = 13;
     int colorGhost  = 16;
     std::vector<int> colors{2, 3, 4, 5, 6, 7, 8, 9, 30, 38, 41, 44, 46, 49};
     int trueStyle  = 20;
     int ghostStyle = 24;
     // marker size
     float recoSize   = 0.8;
     float unRecoSize = 0.5;
     float ghostSize  = 0.3;
   
     TPolyLine pline;
     std::vector<TMarker*> markers;
   
     c_12->cd();
     latex.DrawLatex(text_scale * min_e1, text_scale * max_e2, "e1-e2");
     // Draw hits
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e1 = hit[0];
       const double e2 = hit[1];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       // default - ghost
       int color  = colors[(int) hits[ih][3] % colors.size()];
       int style  = ghostStyle;
       float size = 0.5;
   
       TMarker* marker = new TMarker(e1, e2, ghostStyle);
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);  // Keep markers alive
     }
     // Draw tracks
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e1_track(nHitsTrack), e2_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e1_track[iHit]   = hit1[0];
         e2_track[iHit]   = hit1[1];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e1_track[0]), &(e2_track[0]));
     }
     c_12->Update();
     markers.clear();
   
     c_13->cd();
     latex.DrawLatex(text_scale * min_e1, text_scale * max_e3, "e1-e3");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e1 = hit[0];
       const double e3 = hit[2];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colors[(int) hits[ih][3] % colors.size()];
       int style  = ghostStyle;
       float size = 0.5;
   
       TMarker* marker = new TMarker(e1, e3, ghostStyle);
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e1_track(nHitsTrack), e3_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e1_track[iHit]   = hit1[0];
         e3_track[iHit]   = hit1[2];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e1_track[0]), &(e3_track[0]));
     }
     c_13->Update();
     markers.clear();
   
     c_14->cd();
     latex.DrawLatex(text_scale * min_e1, text_scale * max_e4, "e1-e4");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e1 = hit[0];
       const double e4 = hit[3];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colors[(int) hits[ih][3] % colors.size()];
       int style  = ghostStyle;
       float size = 0.5;
   
       TMarker* marker = new TMarker(e1, e4, ghostStyle);
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e1_track(nHitsTrack), e4_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e1_track[iHit]   = hit1[0];
         e4_track[iHit]   = hit1[3];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e1_track[0]), &(e4_track[0]));
     }
     c_14->Update();
     markers.clear();
   
     c_23->cd();
     latex.DrawLatex(text_scale * min_e2, text_scale * max_e3, "e2-e3");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e2 = hit[1];
       const double e3 = hit[2];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colors[(int) hits[ih][3] % colors.size()];
       int style  = ghostStyle;
       float size = 0.5;
   
       TMarker* marker = new TMarker(e2, e3, ghostStyle);
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e2_track(nHitsTrack), e3_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e2_track[iHit]   = hit1[1];
         e3_track[iHit]   = hit1[2];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e2_track[0]), &(e3_track[0]));
     }
     c_23->Update();
     markers.clear();
   
     c_24->cd();
     latex.DrawLatex(text_scale * min_e2, text_scale * max_e4, "e2-e4");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e2 = hit[1];
       const double e4 = hit[3];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colors[(int) hits[ih][3] % colors.size()];
       int style  = ghostStyle;
       float size = 0.5;
   
       TMarker* marker = new TMarker(e2, e4, ghostStyle);
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e2_track(nHitsTrack), e4_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e2_track[iHit]   = hit1[1];
         e4_track[iHit]   = hit1[3];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e2_track[0]), &(e4_track[0]));
     }
     c_24->Update();
     markers.clear();
   
     c_34->cd();
     latex.DrawLatex(text_scale * min_e3, text_scale * max_e4, "e3-e4");
     for (int ih = 0; ih < (int) embedCoord.size(); ih++) {
       const auto& hit = embedCoord[ih];
       const double e3 = hit[2];
       const double e4 = hit[3];
       const int label = hitLabels[ih];
       const int type  = hitType[ih];
   
       int color  = colors[(int) hits[ih][3] % colors.size()];
       int style  = ghostStyle;
       float size = 0.5;
   
       TMarker* marker = new TMarker(e3, e4, ghostStyle);
   
       marker->SetMarkerColor(color);
       marker->SetMarkerSize(size);
       marker->SetMarkerStyle(style);
       marker->Draw();
       markers.push_back(marker);
     }
     for (const auto& track : tracks) {
       int nHitsTrack = track.size();
       std::vector<double_t> e3_track(nHitsTrack), e4_track(nHitsTrack);
       for (int iHit = 0; iHit < nHitsTrack; iHit++) {
         const auto& hit1 = embedCoord[track[iHit]];
         e3_track[iHit]   = hit1[2];
         e4_track[iHit]   = hit1[3];
       }
       pline.SetLineColor(colors[trackLabels[&track - &tracks[0]] % colors.size()]);
       pline.DrawPolyLine(nHitsTrack, &(e3_track[0]), &(e4_track[0]));
     }
     c_34->Update();
     markers.clear();
   
     chdir("L1CADraw/embed");
     TString tmp = "Stat_proj_12.pdf";  // projection to 1-2 plane
     c_12->SaveAs(tmp);
     tmp = "Stat_proj_13.pdf";  // projection to 1-3 plane
     c_13->SaveAs(tmp);
   
     tmp = "Stat_proj_14.pdf";  // projection to 1-4 plane
     c_14->SaveAs(tmp);
   
     tmp = "Stat_proj_23.pdf";  // projection to 2-3 plane
     c_23->SaveAs(tmp);
   
     tmp = "Stat_proj_24.pdf";  // projection to 2-4 plane
     c_24->SaveAs(tmp);
   
     tmp = "Stat_proj_34.pdf";  // projection to 3-4 plane
     c_34->SaveAs(tmp);
   
     std::cout << "Embedded station projections saved in L1CADraw/embed" << std::endl;
   
     chdir("../..");
   }