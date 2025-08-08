#include "EmbedNet.h"

#include "MLPMath.h"
#include "MLPutil.h"

#include <random>

// #define DEBUG_WEIGHTS

typedef std::vector<std::vector<float>> Matrix2D;

EmbedNet::EmbedNet(const std::vector<int>& topology)
{

  topology_  = topology;
  numLayers_ = (int) topology_.size() - 1;

  // There are numLayers of weights and biases
  // weights[0] is the weight matrix from the input layer to the first hidden layer

  // Initialize weights and their gradients using topology
  weights_.resize(numLayers_);
  weightGradients_.resize(numLayers_);
  for (int i = 0; i < numLayers_; i++) {
    weights_[i].resize(topology_[i + 1]);
    weightGradients_[i].resize(topology_[i + 1]);
    for (int j = 0; j < topology_[i + 1]; j++) {
      weights_[i][j].resize(topology_[i]);
      weightGradients_[i][j].resize(topology_[i]);
      for (int k = 0; k < topology_[i]; k++) {
        weights_[i][j][k]         = MLPMath::CalculateRandomWeight(topology_[i], topology_[i + 1]);
        weightGradients_[i][j][k] = 0.0f;
      }
    }
  }

  // Initialize Biases and their gradients
  biases_.resize(numLayers_);
  biasGradients_.resize(numLayers_);
  for (int i = 0; i < numLayers_; i++) {
    biases_[i].resize(topology_[i + 1]);
    biasGradients_[i].resize(topology_[i + 1]);
    for (int j = 0; j < topology_[i + 1]; j++) {
      biases_[i][j]        = 0.f;
      biasGradients_[i][j] = 0.f;
    }
  }
}

// path to training data needs to be set before calling this
void EmbedNet::Initialize()
{
  //@todo reserve

  std::vector<Matrix2D> activationEvent;
  std::vector<Matrix2D> rawOutputEvent;
  std::vector<Matrix2D> featureGradientEvent;
  for (int iBatch = 0; iBatch < nBatches_; iBatch++) {
    int nHitsEvent = getHitsEvent(iBatch);
    activationEvent.resize(nHitsEvent);
    rawOutputEvent.resize(nHitsEvent);
    featureGradientEvent.resize(nHitsEvent);
    for (int iHit = 0; iHit < nHitsEvent; iHit++) {
      // Note : Indexing of the above quantities is shifted by one from that of weights/biases
      // i.e. rawOutputs[0] is the input to the neural network, and
      // activations[numLayers] is the prediction of the network
      activationEvent[iHit].resize(numLayers_ + 1);
      rawOutputEvent[iHit].resize(numLayers_ + 1);
      featureGradientEvent[iHit].resize(numLayers_ + 1);
      for (int iLayer = 0; iLayer < (numLayers_ + 1); iLayer++) {
        activationEvent[iHit][iLayer].resize(topology_[iLayer]);
        rawOutputEvent[iHit][iLayer].resize(topology_[iLayer]);
        featureGradientEvent[iHit][iLayer].resize(topology_[iLayer]);
      }
    }

    activations_.push_back(activationEvent);
    rawOutputs_.push_back(rawOutputEvent);
    featureGradients_.push_back(featureGradientEvent);
  }

  if (doValidation_) {
    inTraining_   = false;
    inValidation_ = true;
    std::vector<Matrix2D> activationEventValid;
    std::vector<Matrix2D> rawOutputEventValid;
    for (int iBatch = 0; iBatch < batchSizeValidation_; iBatch++) {
      int nHitsEvent = getHitsEvent(iBatch);
      activationEventValid.resize(nHitsEvent);
      rawOutputEventValid.resize(nHitsEvent);
      for (int iHit = 0; iHit < nHitsEvent; iHit++) {
        activationEventValid[iHit].resize(numLayers_ + 1);
        rawOutputEventValid[iHit].resize(numLayers_ + 1);
        for (int iLayer = 0; iLayer < (numLayers_ + 1); iLayer++) {
          activationEventValid[iHit][iLayer].resize(topology_[iLayer]);
          rawOutputEventValid[iHit][iLayer].resize(topology_[iLayer]);
        }
      }

      activationsValid_.push_back(activationEventValid);
      rawOutputsValid_.push_back(rawOutputEventValid);
    }
    inValidation_ = false;
    inTraining_   = true;
  }
}

// set pathTrainData already needs to called
void EmbedNet::startTraining(int numEpochs)
{

  LOG(info) << "Training started.";

  batchSize_           = 1;
  nBatches_            = filePathTrainData_.size();  // num of events
  batchSizeValidation_ = 1;                          // @todo: get rid of this
  doValidation_        = false;                      // make input argument
  inValidation_        = false;
  inTraining_          = true;
  nEpochs_             = numEpochs;

  // load all data used in training
  loadTrainDataEmbedding();
  if (doValidation_) loadValidationDataEmbedding();

  Initialize();

  for (int epochIndex = 0; epochIndex < numEpochs; epochIndex++) {
    analyzeEpoch(epochIndex);
    if (doValidation_) {
      inValidation_ = true;
      inTraining_   = false;
      analyzeEpoch(epochIndex);
      inValidation_ = false;
      inTraining_   = true;
    }
    std::cout << std::string(50, '=') << '\n';
  }
  std::cout << '\n';

  writeLossToFile();

  MLPutil::kNNHisto(activations_, EpochMCInfo_, numLayers_);

  // printStatistics();
}

void EmbedNet::analyzeEpoch(int epochIndex)
{

  if (inTraining_) {  // shuffle event order for training
    MLPutil::shuffleData(EpochInputs_, EpochMCInfo_);
  }

  // std::cout << "Batch: ";
  for (int batchIndex = 0; batchIndex < nBatches_; batchIndex++) {
    loadBatch(batchIndex);
    analyzeBatch(batchIndex, epochIndex);
    // std::cout << batchIndex << " " << std::flush;
  }
  // std::cout << '\n' << std::flush;

  updateStatistics();  // store loss and accuracy info

  resetStatistics();  // reset counters used to calc accuracy
}

void EmbedNet::loadBatch(int batchIndex)
{

  // clear previous batch data
  if (inTraining_) {
    batchMCInfo_.clear();
    batchInputs_.clear();
  }
  else if (inValidation_) {
    batchMCInfoValid_.clear();
    batchInputsValid_.clear();
  }

  loadBatchData(batchIndex);
}


void EmbedNet::analyzeBatch(int batchIndex, int epochIndex)
{
  if (inTraining_) {
    feedForward(batchIndex);
  }
  else if (inValidation_) {
    feedForwardValid();  // Find a better way to do this.
  }

  calcEmbedLoss(batchIndex);

  /// print loss of the batch. average over loss of all batches in an epoch
  if (batchIndex == (nBatches_ - 1)) {
    float avgLoss     = 0.0f;
    float avgLossGen  = 0.0f;
    float avgLossFake = 0.0f;
    /// Use lossTrainHistory_ to calc average loss of last nBatches_
    for (int iBatch = 0; iBatch < nBatches_; iBatch++) {
      avgLoss += lossTrainHist_[epochIndex * nBatches_ + iBatch];
      avgLossGen += lossGenHist_[epochIndex * nBatches_ + iBatch];
      avgLossFake += lossTrainFakeHist_[epochIndex * nBatches_ + iBatch];
    }
    avgLoss /= nBatches_;
    avgLossGen /= nBatches_;
    avgLossFake /= nBatches_;
    std::cout << "Epoch: " << epochIndex + 1 << ", Loss: " << avgLoss << ", lGenuine: " << avgLossGen
              << ", lFake: " << avgLossFake << '\n';
  }

  if (inTraining_) {
    backPropagation(batchIndex);
    parameterUpdate();
    resetGradients();
  }
}


void EmbedNet::startTesting(int numTesting)
{

  inTesting_    = true;
  inTraining_   = false;
  inValidation_ = false;
  doValidation_ = false;
  batchSize_    = 1;
  numTesting_   = numTesting;
  nBatches_     = numTesting_ / batchSize_;

  // initialize storage
  activations_.resize(nBatches_);
  rawOutputs_.resize(nBatches_);
  featureGradients_.resize(nBatches_);  // store gradients of neurons. used in backprop
  for (std::size_t i = 0; i < 1; i++) {
    activations_[i].resize(numLayers_ + 1);
    rawOutputs_[i].resize(numLayers_ + 1);
    featureGradients_[i].resize(numLayers_ + 1);
    for (int j = 0; j < (numLayers_ + 1); j++) {
      activations_[i][j].resize(topology_[j]);
      rawOutputs_[i][j].resize(topology_[j]);
      featureGradients_[i][j].resize(topology_[j]);
    }
  }

  loadTrainDataEmbedding();

  analyzeEpoch(0);

  printStatistics();
}


void EmbedNet::run(const std::vector<Matrix2D>& inputCoords)
{

  inTesting_ = true;
  batchSize_ = (int) inputCoords.size();  // numEvents = should be 1
  nBatches_  = 1;

  // initialize storage
  std::vector<Matrix2D> activationEvent;
  std::vector<Matrix2D> rawOutputEvent;
  for (int iBatch = 0; iBatch < batchSize_; iBatch++) {
    int nHitsEvent = inputCoords[iBatch].size();
    activationEvent.resize(nHitsEvent);
    rawOutputEvent.resize(nHitsEvent);
    for (int iHit = 0; iHit < nHitsEvent; iHit++) {
      activationEvent[iHit].resize(numLayers_ + 1);
      rawOutputEvent[iHit].resize(numLayers_ + 1);
      for (int iLayer = 0; iLayer < (numLayers_ + 1); iLayer++) {
        activationEvent[iHit][iLayer].resize(topology_[iLayer]);
        rawOutputEvent[iHit][iLayer].resize(topology_[iLayer]);
      }
    }
    activations_.push_back(activationEvent);
    rawOutputs_.push_back(rawOutputEvent);
  }

  for (int batchIndex = 0; batchIndex < nBatches_; batchIndex++) {
    batchInputs_.clear();
    batchInputs_.resize(batchSize_);
    for (int iEvent = 0; iEvent < batchSize_; iEvent++) {
      int nHitsEvent = inputCoords[iEvent].size();
      batchInputs_[iEvent].resize(nHitsEvent);
      for (int iHit = 0; iHit < nHitsEvent; iHit++) {
        batchInputs_[iEvent][iHit].resize(topology_[0]);
        batchInputs_[iEvent][iHit] = inputCoords[iEvent][iHit];
      }
    }
    feedForward(batchIndex);
  }  // batch loop

  // std::cout << "Run End." << std::endl;
}


EmbedNet::~EmbedNet() {}

/// batchIndex is iEvent
void EmbedNet::feedForward(int iEvent)
{

  int nHitsEvent = 0;
  if (!inTesting_) {  // replace with ? based on inTesting_
    nHitsEvent = getHitsEvent(iEvent);
  }
  else {
    nHitsEvent = (int) batchInputs_[0].size();
  }
  for (int iHit = 0; iHit < nHitsEvent; iHit++) {
    rawOutputs_[iEvent][iHit][0] = batchInputs_[0][iHit];              // set input layer from data
    for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {  // propagate through all layers
      Matrix2D& weight         = weights_[layerIndex];
      std::vector<float>& bias = biases_[layerIndex];

      // affine transform
      if (layerIndex == 0) {  // first layer input is from data
        rawOutputs_[iEvent][iHit][layerIndex + 1] =
          MLPMath::affineTransform(weight, rawOutputs_[iEvent][iHit][0], bias);
      }
      else {
        rawOutputs_[iEvent][iHit][layerIndex + 1] =
          MLPMath::affineTransform(weight, activations_[iEvent][iHit][layerIndex], bias);
      }

      // apply activation function on the rawOutputs
      activations_[iEvent][iHit][layerIndex + 1] = applyActivation(rawOutputs_[iEvent][iHit][layerIndex + 1]);

    }  // layer
  }  // hit
}

void EmbedNet::feedForwardValid()
{
  for (int iEvent = 0; iEvent < batchSizeValidation_; iEvent++) {
    int nHitsEvent = getHitsEvent(iEvent);
    for (int iHit = 0; iHit < nHitsEvent; iHit++) {
      rawOutputsValid_[iEvent][iHit][0] = batchInputsValid_[iEvent][iHit];  // set input layer from data
      for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {     // propagate through all layers
        Matrix2D& weight         = weights_[layerIndex];
        std::vector<float>& bias = biases_[layerIndex];

        // affine transform
        if (layerIndex == 0) {  // first layer input is from data
          rawOutputsValid_[iEvent][iHit][layerIndex + 1] =
            MLPMath::affineTransform(weight, rawOutputsValid_[iEvent][iHit][0], bias);
        }
        else {
          rawOutputsValid_[iEvent][iHit][layerIndex + 1] =
            MLPMath::affineTransform(weight, activationsValid_[iEvent][iHit][layerIndex], bias);
        }

        // apply activation function on the rawOutputs
        if (layerIndex == (numLayers_ - 1)) {  // tanH on last layer.
          activationsValid_[iEvent][iHit][numLayers_] = MLPMath::applyTanH(rawOutputsValid_[iEvent][iHit][numLayers_]);
        }
        else {
          activationsValid_[iEvent][iHit][layerIndex + 1] =
            applyActivation(rawOutputsValid_[iEvent][iHit][layerIndex + 1]);
        }
      }  // layer
    }  // hit
  }  // event
}


void EmbedNet::backPropagation(int iEvent)
{
  int nHitsEvent = 0;
  nHitsEvent     = getHitsEvent(iEvent);
  for (int iHit = 0; iHit < nHitsEvent; iHit++) {
    // loss delta
    std::vector<float> lossDelta(topology_[numLayers_], 0);
    backPropEmbedLoss(lossDelta, iEvent, iHit);
    featureGradients_[iEvent][iHit][numLayers_] = lossDelta;

    for (int layerIndex = (numLayers_ - 1); layerIndex >= 0; layerIndex--) {  // backwards
      Matrix2D& weight = weights_[layerIndex];
      Matrix2D weightT = MLPMath::Transpose2D(weight);
      /// @todo dont do this separately absorb into calc by flipping index. Write new func matvecTrans()
      Matrix2D& weightGradient = weightGradients_[layerIndex];

      backPropActivation(layerIndex, activationType_, iEvent, iHit);  // updates featureGradients_

      // fetch gradients of the layer on the right. This will already have gradients filled
      std::vector<float>& rDelta = featureGradients_[iEvent][iHit][layerIndex + 1];
      // calc. weight gradient for current layer
      calcWeightGradient(weightGradient, rDelta, iEvent, iHit, layerIndex);
      // calc. bias gradient for current layer
      biasGradients_[layerIndex] = MLPMath::addVector(biasGradients_[layerIndex], rDelta);
      // calc. feature Gradients for current layer. Will be used as rDelta in next iteration
      featureGradients_[iEvent][iHit][layerIndex] = calclFeatureGradient(weightT, rDelta);
    }  //layer
  }  //hit
}


void EmbedNet::calcWeightGradient(Matrix2D& weightGradient, std::vector<float>& rDelta, int eventIndex, int hitIndex,
                                  int layerIndex)
{

  std::vector<float>& lactivation = activations_[eventIndex][hitIndex][layerIndex];
  if (layerIndex == 0) {  // there is no activation on first layer
    lactivation = rawOutputs_[eventIndex][hitIndex][layerIndex];
  }

  for (std::size_t i = 0; i < weightGradient.size(); i++) {
    for (std::size_t j = 0; j < weightGradient[0].size(); j++) {
      weightGradient[i][j] += rDelta[i] * lactivation[j];
    }
  }
}

std::vector<float> EmbedNet::calclFeatureGradient(Matrix2D& weightT, std::vector<float>& rDelta)
{

  std::vector<float> lFeatureGradient = MLPMath::MatMul2D1D(weightT, rDelta);

  return lFeatureGradient;
}

std::vector<float> EmbedNet::applyActivation(std::vector<float>& input)
{

  std::vector<float> output(input.size());
  switch (activationType_) {
    case 0:  // TanH
      output = MLPMath::applyTanH(input);
      return output;
      break;
    case 1:  // ReLU
      for (std::size_t i = 0; i < input.size(); i++) {
        if (input[i] > 0)
          output[i] = input[i];
        else
          output[i] = 0.0f;
      }
      return output;
      break;
    case 2:  // sigmoid
      // check this
      return MLPMath::sigmoid(input);
    default: return output; break;
  }
}

void EmbedNet::backPropActivation(int layerIndex, int activationType, int eventIndex, int hitIndex)
{

  std::vector<float>& rDelta     = featureGradients_[eventIndex][hitIndex][layerIndex + 1];
  std::vector<float>& activation = activations_[eventIndex][hitIndex][layerIndex + 1];
  std::vector<float>& rawOutput  = rawOutputs_[eventIndex][hitIndex][layerIndex + 1];

  switch (activationType) {
    case 0:  // TanH
      for (std::size_t i = 0; i < rDelta.size(); i++) {
        rDelta[i] = (1.0f - activation[i] * activation[i]) * rDelta[i];
      }
      break;
    case 1:  // ReLU
      for (std::size_t i = 0; i < rDelta.size(); i++) {
        if (rawOutput[i] < 0.0f) rDelta[i] = 0.0f;
      }
      break;
    case 2:  // sigmoid
      for (std::size_t i = 0; i < rDelta.size(); i++) {
        rDelta[i] = activation[i] * (1.0f - activation[i]);
      }
      break;
    default: break;
  }
}


void EmbedNet::calcEmbedLoss(int iEvent)
{
  // fill embedded coordinates and corresponding MC info(sta, MCID)
  Matrix2D embedCoord;
  IntMatrix hitMCInfo;
  int nHitsEvent = 0;
  if (inTraining_) {
    nHitsEvent = getHitsEvent(iEvent);
    embedCoord.resize(nHitsEvent);
    hitMCInfo.resize(nHitsEvent);
    for (int iHit = 0; iHit < nHitsEvent; iHit++) {
      embedCoord[iHit].resize(topology_[numLayers_]);
      embedCoord[iHit] = activations_[iEvent][iHit][numLayers_];
      hitMCInfo[iHit].resize(2);
      hitMCInfo[iHit] = batchMCInfo_[0][iHit];
    }  //hit
  }
  // else if (inValidation_) {
  //   embedCoord.resize(batchSizeValidation_);
  //   hitMCInfo.resize(batchSizeValidation_);
  //   for (int iEvent = 0; iEvent < batchSizeValidation_; iEvent++) {
  //     nHitsEvent = getHitsEvent(iEvent);
  //     for (int iHit = 0; iHit < nHitsEvent; iHit++) {
  //       embedCoord[iEvent][iHit].resize(topology_[numLayers_]);
  //       embedCoord[iEvent][iHit] = activationsValid_[iEvent][iHit][numLayers_];
  //       hitMCInfo[iEvent][iHit].resize(2);
  //       hitMCInfo[iEvent][iHit] = batchMCInfoValid_[iEvent][iHit];
  //     }  //hit
  //   }    //event
  // }

  /// go to hits on adjacent stations and calc distance sq between them
  /// if hits share MCID, then they are genuine edges else fake edges.
  float lGenuine = 0.0f, lFake = 0.0f;     // loss for genuine and fake edges
  std::vector<float> dSqGenuine, dSqFake;  // distance sq for genuine and fake edges
  int nGen = 0, nFake = 0;                 // number of genuine and fake edges
  float dist = 0;
  genuineEdges_.clear();
  fakeEdges_.clear();  // todo: put in more efficient place
  genuineEdgesEvent_.clear();
  fakeEdgesEvent_.clear();
  std::random_device rd;
  std::mt19937 g(rd());
  nHitsEvent = getHitsEvent(iEvent);
  for (int hit1 = 0; hit1 < nHitsEvent; hit1++) {
    for (int hit2 = 0; hit2 < nHitsEvent; hit2++) {                  // unsorted
      if (hitMCInfo[hit2][0] != (hitMCInfo[hit1][0] + 1)) continue;  // adjacent stations

      if (hitMCInfo[hit1][1] == hitMCInfo[hit2][1]) {  // MC ID same -> genuine edge
        dist = MLPutil::hitDistanceSq(embedCoord[hit1], embedCoord[hit2]);
        dSqGenuine.push_back(dist);
        nGen++;
        genuineEdges_.push_back(std::make_pair(hit1, hit2));
        genuineEdgesEvent_.push_back(iEvent);
      }
      else {  // MC ID different -> fake edge
        // generate number between 0 and 1. if < fakeEdgeSelectProb_ then add to fake edges
        float r = (float) g() / (float) g.max();
        if (r > fakeEdgeSelectProb_) continue;
        dist = MLPutil::hitDistanceSq(embedCoord[hit1], embedCoord[hit2]);
        dSqFake.push_back(dist);
        nFake++;
        fakeEdges_.push_back(std::make_pair(hit1, hit2));
        fakeEdgesEvent_.push_back(iEvent);
      }
    }
  }

  // std::cout << "Event " << iEvent << ": nGen=" << nGen << ", nFake=" << nFake << '\n';

  /// calc L_genuine
  for (int i = 0; i < nGen; i++) {
    lGenuine += dSqGenuine[i];
  }
  lGenuine /= (float) nGen;

  /// calc L_fake
  for (int i = 0; i < nFake; i++) {
    lFake += std::max(0.0f, margin_ - dSqFake[i]);
  }
  lFake /= (float) nFake;

  if (inTraining_) {
    loss_ = posLossWeight_ * lGenuine + lFake;
  }
  else if (inValidation_) {
    lossValidation_ = posLossWeight_ * lGenuine + lFake;
  }

  if (inTraining_) {
    // std::cout << "training Loss = " << loss_ << ". lGenuine=" << lGenuine << ", lFake=" << lFake << '\n';
  }
  else if (inValidation_) {
    std::cout << "Validation Loss = " << lossValidation_ << ". lGenuine=" << lGenuine << ", lFake=" << lFake << '\n';
  }

  if (inTraining_) {  // updated in
    lossTrainHist_.push_back(loss_);
    lossGenHist_.push_back(lGenuine);
    lossTrainFakeHist_.push_back(lFake);
  }
  else if (inValidation_) {
    lossValidationHist_.push_back(lossValidation_);
    lossValidationGenHist_.push_back(lGenuine);
    lossValidationFakeHist_.push_back(lFake);
  }
}


/// @brief: calculates derivative of loss wrt one hit's embedding coordinates
void EmbedNet::backPropEmbedLoss(std::vector<float>& lossDelta, int eventIndex, int hitIndex)
{

  float dist = 0.0f;
  std::vector<float> deltaGen;
  deltaGen.resize(topology_[numLayers_], 0.0f);
  std::vector<float> deltaFake;
  deltaFake.resize(topology_[numLayers_], 0.0f);

  for (std::size_t iGen = 0; iGen < genuineEdges_.size(); iGen++) {
    if (genuineEdgesEvent_[iGen] != eventIndex) continue;
    float factor = 0.0f;
    if (genuineEdges_[iGen].first == hitIndex) {
      factor = 1.0f;
    }
    else if (genuineEdges_[iGen].second == hitIndex) {
      factor = -1.0f;
    }
    else {
      continue;
    }
    std::vector<float>& embedCoord1 = activations_[eventIndex][genuineEdges_[iGen].first][numLayers_];
    std::vector<float>& embedCoord2 = activations_[eventIndex][genuineEdges_[iGen].second][numLayers_];
    for (int oIndex = 0; oIndex < topology_[numLayers_]; oIndex++) {
      deltaGen[oIndex] = deltaGen[oIndex] + factor * (embedCoord1[oIndex] - embedCoord2[oIndex]);
    }
  }

  /// fake edges
  for (std::size_t iFake = 0; iFake < fakeEdges_.size(); iFake++) {
    if (fakeEdgesEvent_[iFake] != eventIndex) continue;
    if (fakeEdges_[iFake].first == hitIndex) {
      std::vector<float>& embedCoord1 = activations_[eventIndex][fakeEdges_[iFake].first][numLayers_];
      std::vector<float>& embedCoord2 = activations_[eventIndex][fakeEdges_[iFake].second][numLayers_];
      dist                            = MLPutil::hitDistanceSq(embedCoord1, embedCoord2);
      if (dist < margin_) {
        for (int oIndex = 0; oIndex < topology_[numLayers_]; oIndex++) {
          deltaFake[oIndex] -= (embedCoord1[oIndex] - embedCoord2[oIndex]);
        }
      }
    }
    else if (fakeEdges_[iFake].second == hitIndex) {
      std::vector<float>& embedCoord1 = activations_[eventIndex][fakeEdges_[iFake].first][numLayers_];
      std::vector<float>& embedCoord2 = activations_[eventIndex][fakeEdges_[iFake].second][numLayers_];
      dist                            = MLPutil::hitDistanceSq(embedCoord1, embedCoord2);
      if (dist < margin_) {
        for (int oIndex = 0; oIndex < topology_[numLayers_]; oIndex++) {
          deltaFake[oIndex] += (embedCoord1[oIndex] - embedCoord2[oIndex]);
        }
      }
    }
  }

  for (int oIndex = 0; oIndex < topology_[numLayers_]; oIndex++) {
    lossDelta[oIndex] =
      posLossWeight_ * deltaGen[oIndex] / (float) genuineEdges_.size() + deltaFake[oIndex] / (float) fakeEdges_.size();
  }

  /// print loss delta
  // for (int i = 0; i < topology_[numLayers_]; i++){
  //     std::cout<<lossDelta[i]<<" ";
  // }
  // std::cout<<std::endl;
}


void EmbedNet::parameterUpdate()
{
#ifdef DEBUG_WEIGHTS
  /// print weight gradient on last layer
  std::cout << "Weight Gradient: " << std::endl;
  for (int i = 0; i < topology_[numLayers_ - 1]; i++) {
    for (int j = 0; j < topology_[numLayers_]; j++) {
      std::cout << weightGradients_[numLayers_ - 1][j][i] << " ";
    }
    std::cout << std::endl;
    if (i == 10) break;
  }

  //// print old weight on last layer
  std::cout << "Weights: " << std::string(50, '-') << std::endl;
  for (int i = 0; i < topology_[numLayers_ - 1]; i++) {
    for (int j = 0; j < topology_[numLayers_]; j++) {
      std::cout << weights_[numLayers_ - 1][j][i] << " ";
    }
    std::cout << std::endl;
    if (i == 10) break;
  }
#endif

  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {

    // update weight
    Matrix2D& oldWeight      = weights_[layerIndex];
    Matrix2D& weightGradient = weightGradients_[layerIndex];
    SGD2D(oldWeight, weightGradient);

    // update bias
    std::vector<float>& oldbias      = biases_[layerIndex];
    std::vector<float>& biasGradient = biasGradients_[layerIndex];
    SGD1D(oldbias, biasGradient);
  }
}

/**
@brief: loads [x,y,z,sta] data from file into EpochInputs_ and [sta, MC ID] into EpochMCInfo_
**/
void EmbedNet::loadTrainDataEmbedding()
{
  if (trackType_ == -1) std::cout << "Track type not set. Set type before running network." << std::endl;

  MLPutil::loadDataEmbed(filePathTrainData_, EpochInputs_, EpochMCInfo_, trackType_, numHitsEvents_);

  LOG(info) << "Training data loaded.";
}

///@todo: update to match loadTraining data
void EmbedNet::loadValidationDataEmbedding()
{

  // std::string filePath;
  // std::string sPath   = "data/embed/";
  // const int startTime = 5300, endTime = 5400;
  // std::string sTimeInt = "_T_" + std::to_string(startTime) + "_" + std::to_string(endTime) + ".dat";
  // filePath             = sPath + "embed_train_data" + sTimeInt;

  // static std::ifstream file;
  // file.open(filePath.data());
  // if (!file) std::cerr << "Could not read file: " << filePath << std::endl;
  // file.close();

  // // prepare storage
  // int numExamples = batchSizeValidation_;  // read this from the first line in the file
  // EpochInputsValid_.resize(numExamples);
  // EpochMCInfoValid_.resize(numExamples);
  // for (std::size_t i = 0; i < EpochInputsValid_.size(); i++) {
  //   EpochInputsValid_[i].resize(topology_[0]);
  //   EpochMCInfoValid_[i].resize(2);
  // }
  // // MLPutil::loadDataEmbed(filePath, EpochInputsValid_, EpochMCInfoValid_);

  // std::cout << "Validation data loaded." << std::endl;
}

void EmbedNet::loadBatchData(int eventId)
{
  /// set sizes
  if (inTraining_) {
    batchInputs_.resize(batchSize_);
    batchMCInfo_.resize(batchSize_);
    int nHitsEvent = getHitsEvent(eventId);
    batchInputs_[0].resize(nHitsEvent);
    batchMCInfo_[0].resize(nHitsEvent);
    for (int iHit = 0; iHit < nHitsEvent; iHit++) {
      batchInputs_[0][iHit].resize(topology_[0]);
      batchMCInfo_[0][iHit].resize(2);
    }
  }
  else if (inValidation_) {
    batchInputsValid_.resize(batchSizeValidation_);
    batchMCInfoValid_.resize(batchSizeValidation_);
    for (int iEvent = 0; iEvent < batchSizeValidation_; iEvent++) {
      int nHitsEvent = getHitsEvent(iEvent);
      batchInputsValid_[iEvent].resize(nHitsEvent);
      batchMCInfoValid_[iEvent].resize(nHitsEvent);
      for (int iHit = 0; iHit < nHitsEvent; iHit++) {
        batchInputsValid_[iEvent][iHit].resize(topology_[0]);
        batchMCInfoValid_[iEvent][iHit].resize(2);
      }
    }
  }

  // load examples into batch
  if (inTraining_) {
    for (int i = 0; i < batchSize_; i++) {
      batchInputs_[i] = EpochInputs_[eventId * batchSize_ + i];
      batchMCInfo_[i] = EpochMCInfo_[eventId * batchSize_ + i];
    }
  }
  else if (inValidation_) {
    for (int i = 0; i < batchSizeValidation_; i++) {
      batchInputsValid_[i] = EpochInputsValid_[i];
      batchMCInfoValid_[i] = EpochMCInfoValid_[i];
    }
  }
}

void EmbedNet::updateStatistics()
{

  // if (inTraining_) {
  //     lossTrainHist_.push_back( getLossTraining() );
  // }
  // if (inValidation_) {
  //     lossValidationHist_.push_back( getLossValidation() );
  // }
}


/// @todo rewrite for embedding
void EmbedNet::printStatistics()
{

  // std::cout.precision(2);

  // int numEpochs = (int)accuracyTrainingEpoch_.size();

  // int printFreq = 1;
  // if (numEpochs > 20) printFreq = numEpochs/10;

  // if (!inTesting_) {
  //     for (int iEp = 0; iEp < numEpochs; iEp += printFreq) {
  //         std::cout<< "Epoch "<< iEp+1 << ":  Loss = "<< lossTrainingEpoch_[iEp] << std::endl;
  //         if ( (numEpochs-iEp) <= printFreq) {
  //             std::cout<< "Epoch "<< numEpochs << ":  Loss = "<< lossTrainingEpoch_[numEpochs-1] << std::endl;
  //         }
  //     }
  // }
  // else {
  //     std::cout<< "Validation Loss = "<< lossValidationEpoch_[0] << std::endl;
  // }
}

void EmbedNet::resetStatistics()
{

  loss_ = 0.f;
  if (doValidation_) lossValidation_ = 0.f;
}

void EmbedNet::SGD1D(std::vector<float>& oldWeights, std::vector<float>& gradient)
{

  for (std::size_t i = 0; i < oldWeights.size(); i++) {
    oldWeights[i] -= SGDLearningRate_ * gradient[i];
  }
}

void EmbedNet::SGD2D(std::vector<std::vector<float>>& oldWeights, std::vector<std::vector<float>>& gradient)
{

  for (std::size_t i = 0; i < oldWeights.size(); i++) {
    for (std::size_t j = 0; j < oldWeights[0].size(); j++) {
      oldWeights[i][j] -= SGDLearningRate_ * gradient[i][j];
    }
  }
}

void EmbedNet::resetGradients()
{

  // reset weight gradients to zero
  for (std::size_t i = 0; i < weightGradients_.size(); i++) {
    for (std::size_t j = 0; j < weightGradients_[i].size(); j++) {
      for (std::size_t k = 0; k < weightGradients_[i][j].size(); k++) {
        weightGradients_[i][j][k] = 0.0f;
      }
    }
  }

  // reset bias gradients to zero
  for (std::size_t i = 0; i < biasGradients_.size(); i++) {
    for (std::size_t j = 0; j < biasGradients_[i].size(); j++) {
      biasGradients_[i][j] = 0.0f;
    }
  }

  // reset feature Gradients
  for (std::size_t i = 0; i < featureGradients_.size(); i++) {
    for (std::size_t j = 0; j < featureGradients_[i].size(); j++) {
      for (std::size_t k = 0; k < featureGradients_[i][j].size(); k++) {
        for (std::size_t l = 0; l < featureGradients_[i][j][k].size(); l++) {
          featureGradients_[i][j][k][l] = 0.0f;
        }
      }
    }
  }
}

/// store weights and biases in a file
// @todo: 1.  store network parameters in a separate file so training can be reproduced
void EmbedNet::saveModel(std::string& fNameWeights, std::string& fNameBiases)
{

  std::ofstream fout;

  // write weight to file
  fout.open(fNameWeights, std::ofstream::trunc);  // trunc removes previous contents of file

  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {
    Matrix2D& weight = weights_[layerIndex];
    for (std::size_t i = 0; i < weight.size(); i++) {
      for (std::size_t j = 0; j < weight[0].size(); j++) {
        fout << weight[i][j] << std::endl;
      }
    }
  }
  fout.close();

  // write bias to file
  fout.open(fNameBiases, std::ofstream::trunc);
  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {
    std::vector<float>& bias = biases_[layerIndex];
    for (std::size_t i = 0; i < bias.size(); i++) {
      fout << bias[i] << std::endl;
    }
  }
  fout.close();

  std::cout << "EmbedNet Model saved." << std::endl;
}

// loads weights and biases from file into the network
void EmbedNet::loadModel(std::string& fNameWeights, std::string& fNameBiases)
{

  std::ifstream fin;

  // requires the weight and bias matrices to be initialized correctly(correct topology)
  // TO DO: change such that the network is initialized from the file

  // load weight
  fin.open(fNameWeights);
  if (!fin.is_open()) {
    std::cerr << "Error: Could not open weight file " << fNameWeights << std::endl;
    return;
  }
  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {
    Matrix2D& weight = weights_[layerIndex];
    for (std::size_t i = 0; i < weight.size(); i++) {
      for (std::size_t j = 0; j < weight[0].size(); j++) {
        fin >> weight[i][j];
      }
    }
  }
  fin.close();

  // load bias
  fin.open(fNameBiases);
  if (!fin.is_open()) {
    std::cerr << "Error: Could not open bias file " << fNameBiases << std::endl;
    return;
  }
  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {
    std::vector<float>& bias = biases_[layerIndex];
    for (std::size_t i = 0; i < bias.size(); i++) {
      fin >> bias[i];
    }
  }
  fin.close();

  // std::cout << "EmbedNet Model loaded." << std::endl;
}

void EmbedNet::printTopology() const
{
  std::cout << "Number of layers: " << numLayers_ << std::endl;
  std::cout << "Topology: ";
  for (int i = 0; i < (int) topology_.size(); i++) {
    std::cout << topology_[i] << ", ";
  }
  std::cout << std::endl;
}


void EmbedNet::writeLossToFile()
{
  std::string fName = "/u/otyagi/cbmroot/NN/output/EmbedLoss.txt";
  std::ofstream fout;
  fout.open(fName, std::ofstream::trunc);

  fout << nEpochs_ << std::endl;
  for (std::size_t iEpoch = 0; iEpoch < nEpochs_; iEpoch++) {
    /// write to file average of loss over all batches in an epoch
    float avgLoss = 0.0f;
    for (std::size_t j = 0; j < nBatches_; j++) {
      avgLoss += lossTrainHist_[iEpoch * nBatches_ + j];
    }
    avgLoss /= nBatches_;

    fout << avgLoss << std::endl;
  }
  fout.close();

  std::cout << "Loss written to file: " << fName << std::endl;
}