/* Copyright (C) 2010-2012 Frankfurt Institute for Advanced Studies, Goethe-Universität Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Igor Kulakov [committer] */

   #ifndef L1AlgoDraw_h
   #define L1AlgoDraw_h 1
   
   #include "CaHit.h"
   #include "CaStation.h"
   #include "CaTriplet.h"
   #include "CbmL1Hit.h"
   
   #include <TString.h>
   
   #include <map>
   #include <vector>
   
   namespace
   {
     using namespace cbm::algo;
   }
   
   class TCanvas;
   
   namespace cbm::algo::ca
   {
     class Framework;
   }
   
   class L1AlgoDraw {
     struct Point {
       double x{0}, y{0}, z{0};
       Point() = default;
       Point(double _x, double _y, double _z) : x(_x), y(_y), z(_z){};
     };
   
    public:
     L1AlgoDraw();
   
     void InitL1Draw(ca::Framework* algo_);
   
     void DrawMCTracks();
     void DrawRecoTracks();
   
     void DrawTriplets(std::vector<ca::Triplet>& triplets, const ca::HitIndex_t* realIHit);
     void DrawDoublets(std::vector<ca::HitIndex_t>* Doublets_hits,
                       std::map<ca::HitIndex_t, ca::HitIndex_t>* Doublets_start, const int MaxArrSize,
                       ca::HitIndex_t* HitsStartIndex, unsigned int* realIHit);
     void DrawDoubletsOnSta(int iSta, ca::HitIndex_t* Doublets_hits, ca::HitIndex_t* Doublets_start, const int MaxArrSize,
                            ca::HitIndex_t* StsRestHitsStartIndex, unsigned int* realIHit);
   
     void DrawTarget();
     void DrawInputHits();  // draw all hits, which TF have gotten
     void DrawRestHits(ca::HitIndex_t* StsRestHitsStartIndex, ca::HitIndex_t* StsRestHitsStopIndex,
                       unsigned int* realIHit);  // draw only hits which leave on current iteration.
   
     void DrawInfo();
     void ClearView();
     void SaveCanvas(TString name);
     void DrawAsk();
   
     // OT added
     void DrawStations();
     void plotLoss(std::string& fName);
     void plotEmbedLoss();
     void plotAccuracy(std::string& fName);
     void drawClassfierScoreDistribution(std::string& fName);
     void drawEdgeOrderHisto();  // for results of embedding network
     void DrawRzProjection();
     void DrawClones();
     void DrawTouchTracks();
     void DrawCurvatureDistribution();
     float calcCurvature(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3);
     void DrawHitsInFile();
     void drawFalseNegativeEdges();
     void drawSavedTrainingEdges();
     void drawSavedHitsByTrackType();
     void DrawRecoTracks_CbmL1();
     void DrawUsedHits();
     void SlopeDistMCLastIter(); // not useful. check before use
     void drawAngleDiffDistMC();
     void drawOverlapTripletAngleDiffDistMC();
     void drawDaughterPairs();
     void DrawEmbedding();
     void DrawEmbeddingStation();
     void DrawKillingTracks(); 
     void DrawGhosts();
   
    private:
     L1AlgoDraw(const L1AlgoDraw&);
     L1AlgoDraw& operator=(const L1AlgoDraw&);
   
     Point GetHitCoor(int ih);
   
     void DrawTriplet(int il, int im, int ir);
     void DrawDoublet(int il, int ir);
   
     ca::Framework* algo{nullptr};
   
     std::vector<CbmL1HitDebugInfo> vHitsQa{};
     std::vector<ca::Hit> vHits{};
     int HitsStartIndex[20]{0};
     int HitsStopIndex[20]{0};
   
     int NStations{0};
     ca::Station<ca::fvec> vStations[20]{};
   
     // int mcolor[10]{5, 7, 3, 8, 6, 2, 4, 1, 9, 14};  // color for hits on i-station
     int mcolor[12]{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};                  // color for hits on i-station
     int mcolorFake[12]{14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14};  // fake hit color
     int StaColor{17};                                                    // color for stations (grey)
     int hitsMStyle{20};                                                  // style for hits (filled circle)
     int fakesMStyle{24};                                                 // style for fakes (open circle)
     int targetMStyle{29};                                                // style for target (star)
   
     double HitSize{0.25};  // size of hits
   
     int fVerbose{0};
     TCanvas* YZ{nullptr};
     TCanvas* YX{nullptr};
     TCanvas* XZ{nullptr};
     TCanvas* XYZ{nullptr};
     bool ask{true};
   };
   
   
   #endif
   