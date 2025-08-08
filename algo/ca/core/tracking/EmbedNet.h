#pragma once  // include this header only once per compilation unit

#include "AlgoFairloggerCompat.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <math.h>

typedef std::vector<std::vector<float>> Matrix2D;
typedef std::vector<std::vector<int>> IntMatrix;  // template this
typedef std::vector<std::vector<unsigned int>> UIntMatrix;

class EmbedNet {
 public:
  EmbedNet(const std::vector<int>& MLPTopology);

  EmbedNet()
  {  // default empty constructor
  }

  /// initialize the shapes of all containers
  void Initialize();

  void feedForward(int iEvent);

  void feedForwardValid();

  void backPropagation(int iEvent);

  void resetGradients();

  void backPropEmbedLoss(std::vector<float>& outputDelta, int eventIndex, int hitIndex);

  void calcEmbedLoss(int iEvent);

  void calcWeightGradient(Matrix2D& weightGradient, std::vector<float>& rDelta, int eventIndex, int hitIndex,
                          int layerIndex);

  std::vector<float> calclFeatureGradient(Matrix2D& weight, std::vector<float>& rDelta);

  void parameterUpdate();

  void startTraining(int numEpochs);

  /// Shuffles at the start of every training epoch
  void analyzeEpoch(int epochIndex);

  void analyzeBatch(int iEvent, int epochIndex);

  void startTesting(int numTesting);

  std::vector<float> applyActivation(std::vector<float>& input);

  void backPropActivation(int layerIndex, int activationType, int eventIndex, int hitIndex);

  void SGD1D(std::vector<float>& oldWeights, std::vector<float>& gradient);

  void SGD2D(std::vector<std::vector<float>>& oldWeights, std::vector<std::vector<float>>& gradient);

  void resetStatistics();

  void printStatistics();

  void updateStatistics();

  void loadBatch(int batchIndex);

  void loadBatchData(int batchIndex);

  void loadTrainDataEmbedding();

  void loadValidationDataEmbedding();

  void run(const std::vector<Matrix2D>& inputCoords);

  void saveModel(std::string& fNameWeights, std::string& fNameBiases);

  void loadModel(std::string& fNameWeights, std::string& fNameBiases);

  std::vector<std::vector<Matrix2D>>& getActivations() { return activations_; }

  std::vector<IntMatrix>& getMCInfo() { return EpochMCInfo_; }

  float getLossTraining() const { return loss_; }

  float getLossValidation() const { return lossValidation_; }

  void getEmbeddedCoords(std::vector<std::vector<float>>& embeddedCoords, int iEvent) const
  {
    embeddedCoords.clear();
    std::size_t nHits = activations_[iEvent].size();
    embeddedCoords.resize(nHits);
    int nLayers = topology_.size() - 1;

    // coords are in activations[hitIndex][nLayers]
    for (std::size_t iHit = 0; iHit < nHits; iHit++) {
      embeddedCoords[iHit].resize(activations_[iEvent][iHit][nLayers].size());
      std::copy(activations_[iEvent][iHit][nLayers].begin(), activations_[iEvent][iHit][nLayers].end(),
                embeddedCoords[iHit].begin());
    }
  }

  float getMargin() const { return margin_; }

  void setPathTrainData(const std::vector<std::string>& path) { filePathTrainData_ = path; }

  void setPathValidationData(const std::vector<std::string>& path) { filePathValidationData_ = path; }

  int getHitsEvent(int eventNum) const { return numHitsEvents_[eventNum]; }

  void setMaxNStations(int maxNStations) { maxNStations_ = maxNStations; }

  void writeLossToFile();

  void printTopology() const;

  void setTrackType(const int trackType) { trackType_ = trackType; }

  ~EmbedNet();

 private:
  /// embedding hyper-parameters
  const float margin_       = 1e-3f;   // def: 1e-3. dSq upto which NN pushes out fake edges in embedding space
  float SGDLearningRate_    = 1e0f;    // 1e0f with margin 1e-2 very good for tanH

  const float posLossWeight_ = 1.0f;  // def: 1. Scaling to genuine edges loss

  int trackType_ = -1; // 0: fastPrim, 3: secondary
  // selection ratio of random fake edges in loss calculation. Adjust for 1:1 genuine:fake ratio
  // For training with all data - 0.003
  // For fastPrim - 0.01, for secondary - 0.025
  float fakeEdgeSelectProb_ = 0.025f;  

  // paths to data
  std::vector<std::string> filePathTrainData_;
  std::vector<std::string> filePathValidationData_;

  // Geometry
  int maxNStations_ = 0;

  /// store edges
  std::vector<std::pair<int, int>> genuineEdges_;  // elements are (subBatchIndex hit1, subBatchIndex hit2)
  std::vector<std::pair<int, int>> fakeEdges_;
  std::vector<int> genuineEdgesEvent_, fakeEdgesEvent_;  // eventNum for each edge


  bool inTesting_     = false;
  bool inValidation_  = false;
  bool inTraining_    = false;  // @todo pack into struct called status
  bool doValidation_  = false;
  int activationType_ = 0;

  float loss_ = 0.0f, lossValidation_ = 0.0f;
  std::vector<float> lossTrainHist_, lossGenHist_, lossTrainFakeHist_;
  std::vector<float> lossValidationHist_, lossValidationGenHist_, lossValidationFakeHist_;


  std::vector<int> topology_;
  int numLayers_ = 0;
  int nBatches_  = 0;
  int batchSize_ = 0, batchSizeValidation_ = 0;
  int numTraining_   = 0;
  int numTesting_    = 0;
  int numValidation_ = 0;
  int nEpochs_       = 0;
  std::vector<int> numHitsEvents_;  // number of hits in each event for track type

  std::vector<Matrix2D> weights_;
  std::vector<Matrix2D> weightGradients_;
  Matrix2D biases_;
  Matrix2D biasGradients_;

  // storing training
  std::vector<Matrix2D> batchInputs_;   // [eventNum][hitIndex][(x,y,z,sta)]
  std::vector<IntMatrix> batchMCInfo_;  // [eventNum][hitIndex][station_id, mc_id]
  std::vector<Matrix2D> EpochInputs_;   // [eventNum][hitIndex][x,y,z,sta]
  std::vector<IntMatrix> EpochMCInfo_;  // [eventNum][hitIndex][station_id, mc_id]
  // access index [subBatchIndex][hit][layerIndex]
  std::vector<std::vector<Matrix2D>> featureGradients_;
  std::vector<std::vector<Matrix2D>> rawOutputs_;   // store raw output, [event][hit][0] is input
  std::vector<std::vector<Matrix2D>> activations_;  // input is [eventNum][hit][0]; output is [eventNum][hit][last]


  //storing validation
  std::vector<Matrix2D> batchInputsValid_;
  std::vector<IntMatrix> batchMCInfoValid_;  // (sta,mcID)
  std::vector<Matrix2D> EpochInputsValid_;
  std::vector<IntMatrix> EpochMCInfoValid_;
  std::vector<std::vector<Matrix2D>> rawOutputsValid_;
  std::vector<std::vector<Matrix2D>> activationsValid_;
};
