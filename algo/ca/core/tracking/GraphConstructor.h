/// \file GraphConstructor.h
/// \brief Graph constructor for GNN tracker
/// \author O. Tyagi

#pragma once  // include this header only once per compilation unit

#include "CaWindowData.h"
#include "KfSimd.h"
// #include "CaField.h"
// #include "CaFramework.h"
// #include "CaGridEntry.h"
// #include "CaStation.h"
// #include "CaTrackParam.h"
// #include "CaTriplet.h"
// #include "CaVector.h"
#include "CaTrackFitter.h"
#include "EmbedNet.h"
#include "MLPutil.h"

namespace cbm::algo::ca
{
  class alignas(kf::VcMemAlign) GraphConstructor {
   public:
    /// Constructor
    GraphConstructor(const ca::InputData& input, WindowData& wData, TrackFitter& fTrackFitter);

    /// Destructor
    ~GraphConstructor() = default;

    /// -- FUNCTIONS
    void FindFastPrim(const int mode);

    void FindSlowPrimJump(const int mode);

    void FindAllSecJump(const int mode);

    void CreateTracksTriplets(const int mode, const int GNNIteration);

    void SaveAllEdgesAsTracks();

    void SaveAllTripletsAsTracks();

    void FitTriplets(const int GNNiteration);

    void PrepareFinalTracks();

    void CreateMetricLearningDoublets(const int iter);
    
    void CreateMetricLearningDoubletsJump(const int iter);

    /// -- VARIABLES

    std::vector<std::vector<unsigned int>> doublets[20];  // [sta][lhit][mhit]
    /// lhit is index in vGrid(no. of hits on station). use fAlgo.vGrid[sta].GetEntries()[lhit].GetObjectId() to get index in fWindowsHits
    /// doublets[sta][lhit][mhit] = index in frWData

    std::vector<std::vector<std::pair<int, int>>> edges;  // [sta][ihitl, ihitm] index in frWData.Hit

    std::vector<std::vector<int>> triplets_;            // [ihitl, ihitm, ihitr] index in frWData.Hit
    std::vector<float> tripletScores_;                  // triplet score
    std::vector<std::vector<float>> tripletFitParams_;  // [chi2, qp, Cqp, Tx, C22, Ty, C33]

    std::vector<std::vector<int>> tracks;                            // indexes in frWData.Hit
    std::vector<std::pair<std::vector<int>, float>> trackAndScores;  // [trackIndex, trackScore]

   private:
    const ca::InputData& frInput;
    WindowData& frWData;
    TrackFitter& frTrackFitter;

    const int NStations = 12;  // set in constructor

    const int maxNeighOrderPrim_        = 20;  // def - 20
    const int maxNeighOrderAllPrim_     = 25;  // def - 25
    const int maxNeighOrderAllPrimJump_ = 10;  // def - 10

    const int maxNeighOrderSec_     = 25;  // def - 25
    const int maxNeighOrderSecJump_ = 10;  // def - 10 decent

    // Candidate classifier parameters
    const bool useCandClassifier_        = false;
    const float CandClassifierThreshold_ = 0.5f;
  };
}  // namespace cbm::algo::ca
