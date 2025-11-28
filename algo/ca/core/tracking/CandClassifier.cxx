#include "CandClassifier.h"
#include "MLPMath.h"
#include "MLPutil.h"

typedef std::vector<std::vector<float>> Matrix;


CandClassifier::CandClassifier(const std::vector<int>& MLPTopology)
{

  topology_       = MLPTopology;
  numLayers_      = (int) topology_.size() - 1;
  activationType_ = 0;  // use 0 for tanh activation

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

void CandClassifier::startTraining(int numTraining, int batchSize, int numEpochs)
{
  numEpochs_   = numEpochs;
  batchSize_   = batchSize;
  numTraining_ = numTraining;
  nBatches_    = numTraining / batchSize;  // this must be an integer

  // to do : move this to an initialization function. same code repeated in testing and
  // initialize storage
  activations_.resize(batchSize_);       // stores values at neurons after activation function has been applied
  rawOutputs_.resize(batchSize_);        // stores neuron values before activation
  featureGradients_.resize(batchSize_);  // store gradients at neurons. Used in backpropagation
  for (int i = 0; i < batchSize_; i++) {
    // Note : Indexing of the above quantities is shifted by one from that of weights/biases
    // i.e. rawOutputs[0] is the input to the neural network, and
    // activations[numLayers] is the prediction of the network
    activations_[i].resize(numLayers_ + 1);
    rawOutputs_[i].resize(numLayers_ + 1);
    featureGradients_[i].resize(numLayers_ + 1);
    for (int j = 0; j < (numLayers_ + 1); j++) {
      activations_[i][j].resize(topology_[j]);
      rawOutputs_[i][j].resize(topology_[j]);
      featureGradients_[i][j].resize(topology_[j]);
    }
  }

  loadTrainDataCandClassifier();

  for (int epochIndex = 0; epochIndex < numEpochs; epochIndex++) {
    if (epochIndex == (numEpochs_ - 1)) {
      saveScoreDist_ = true;
    }
    analyzeEpoch();

    saveScoreDist_ = false;

    if (epochIndex == 0) {
      std::cout << "Epoch: " << epochIndex + 1 << " " << std::flush;
    }
    else {
      std::cout << epochIndex + 1 << " " << std::flush;
    }
  }
  std::cout << '\n';
  printStatistics();

  // write data to file. Plotted later in L1AlgoDraw
  if (savePerFiles_) {
    writeLossToFile(lossFName);
    writeAccuracyToFile(accFName);
    writeScoreDistToFile(scoreFName);
  }

  // do validation?
}

void CandClassifier::analyzeEpoch()
{
  // shuffle data. Dont need to shuffle during testing
  if (!inTesting_) {
    MLPutil::shuffleData(EpochInputs_, EpochClasses_);
  }

  for (int batchIndex = 0; batchIndex < nBatches_; batchIndex++) {
    loadBatch(batchIndex);
    analyzeBatch();
  }

  updateStatistics();  // store loss and accuracy info

  resetStatistics();  // reset counters used to calc accuracy
}


void CandClassifier::loadBatch(int batchIndex)
{

  // clear previous batch data
  batchClasses_.clear();
  batchInputs_.clear();

  loadBatchData(batchIndex);

  // temp fix for batchSize = 1,
  //rawOutputs_[0] = batchInputs_[0]; // temp fix for batch size = 1
  //targetClass_ = batchClasses_[0];
}


void CandClassifier::analyzeBatch()
{
  feedForward();

  analyzeOutput();

  calculateLoss();

  if (!inTesting_) {

    backPropagation();
    parameterUpdate();

    // reset gradients and feature Gradients
    resetGradients();
  }
}

void CandClassifier::startTesting(const int numTesting)
{
  inTesting_  = true;
  numTesting_ = numTesting;
  batchSize_  = 1;
  nBatches_   = numTesting_ / batchSize_;

  // to do : move this to an initialization function. same code repeated in testing and
  // initialize storage
  activations_.resize(batchSize_);       // stores values at neurons after activation function has been applied
  rawOutputs_.resize(batchSize_);        // stores neuron values before activation
  featureGradients_.resize(batchSize_);  // store gradients at neurons. Used in backpropagation
  for (int i = 0; i < batchSize_; i++) {
    // Note : Indexing of the above quantities is shifted by one from that of weights/biases
    // i.e. rawOutputs[0] is the input to the neural network, and
    // activations[numLayers] is the prediction of the network
    activations_[i].resize(numLayers_ + 1);
    rawOutputs_[i].resize(numLayers_ + 1);
    featureGradients_[i].resize(numLayers_ + 1);
    for (int j = 0; j < (numLayers_ + 1); j++) {
      activations_[i][j].resize(topology_[j]);
      rawOutputs_[i][j].resize(topology_[j]);
      featureGradients_[i][j].resize(topology_[j]);
    }
  }

  std::cout << "UNTESTED!!!!!!!" << std::endl;

  analyzeEpoch();

  std::cout << std::endl;
  printStatistics();

  printConfusionMatrix(0);  // only one epoch in testing
}


void CandClassifier::run(const Matrix& allEdges, std::vector<int>& trueEdgesIndex, std::vector<float>& trueEdgesScore)
{

  inTesting_  = true;
  batchSize_  = 1;
  numTesting_ = (int) allEdges.size();
  nBatches_   = numTesting_ / batchSize_;
  trueEdgesIndex.clear();

  // initialize neural net storage
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

  // load data
  EpochInputs_.clear();
  EpochInputs_.resize(numTesting_);
  for (std::size_t i = 0; i < EpochInputs_.size(); i++) {
    EpochInputs_[i].resize(topology_[0]);
    for (int j = 0; j < topology_[0]; j++) {
      EpochInputs_[i][j] = allEdges[i][j];
    }
  }

  int nTrueEdges = 0, nFakeEdges = 0;
  for (int batchIndex = 0; batchIndex < nBatches_; batchIndex++) {
    batchInputs_.clear();
    batchInputs_.resize(batchSize_);
    for (int subBatchIndex = 0; subBatchIndex < batchSize_; subBatchIndex++) {
      batchInputs_[subBatchIndex].resize(topology_[0]);
      batchInputs_[subBatchIndex] = EpochInputs_[batchIndex * batchSize_ + subBatchIndex];
    }

    feedForward();

    for (int subBatchIndex = 0; subBatchIndex < batchSize_; subBatchIndex++) {
      if (getClassification(subBatchIndex) == 0) {
        trueEdgesIndex.push_back((batchIndex * batchSize_ + subBatchIndex));
        trueEdgesScore.push_back(activations_[subBatchIndex][numLayers_][0]);
        nTrueEdges++;
      }
      else {
        nFakeEdges++;
      }
    }  // sub Batch loop
  }  // batch loop

  std::cout << "True Edges = " << nTrueEdges << ", False Edges = " << nFakeEdges << std::endl;

  std::cout << "Run End. " << std::endl;
}


CandClassifier::~CandClassifier() {}


void CandClassifier::analyzeOutput()
{

  for (int subBatchIndex = 0; subBatchIndex < batchSize_; subBatchIndex++) {

    // add here counters to measure true positive, true negative, false positive, false negative
    if ((batchClasses_[subBatchIndex] == 0) && (getClassification(subBatchIndex) == 0)) {  // true positive
      nCorrectClassifiedEpoch_++;
      truePos_++;
    }
    else if ((batchClasses_[subBatchIndex] == 1) && (getClassification(subBatchIndex) == 1)) {  // true negative
      nCorrectClassifiedEpoch_++;
      trueNeg_++;
    }
    else if ((batchClasses_[subBatchIndex] == 0) && (getClassification(subBatchIndex) == 1)) {  // false negative
      nFalseClassifiedEpoch_++;
      falseNeg_++;
    }
    else if ((batchClasses_[subBatchIndex] == 1) && (getClassification(subBatchIndex) == 0)) {  // false positive
      nFalseClassifiedEpoch_++;
      falsePos_++;
    }
  }


  /// write to score distribution file
  if (saveScoreDist_) {
    for (int i = 0; i < batchSize_; i++) {
      scoreDist_.push_back(std::make_pair(batchClasses_[i], activations_[i][numLayers_][0]));
    }
  }
}


void CandClassifier::feedForward()
{

  for (int subBatchIndex = 0; subBatchIndex < batchSize_; subBatchIndex++) {  // loop over batches

    rawOutputs_[subBatchIndex][0] = batchInputs_[subBatchIndex];       // set input layer from data
    for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {  // propagate through all layers

      Matrix& weight           = weights_[layerIndex];
      std::vector<float>& bias = biases_[layerIndex];

      // affine transform
      if (layerIndex == 0) {  // first layer input is from data
        rawOutputs_[subBatchIndex][layerIndex + 1] =
          MLPMath::affineTransform(weight, rawOutputs_[subBatchIndex][0], bias);
      }
      else {
        rawOutputs_[subBatchIndex][layerIndex + 1] =
          MLPMath::affineTransform(weight, activations_[subBatchIndex][layerIndex], bias);
      }

      // apply activation function on the rawOutputs
      if (layerIndex == (numLayers_ - 1)) {  // sigmoid on last layer
        activations_[subBatchIndex][numLayers_][0] = MLPMath::sigmoid(rawOutputs_[subBatchIndex][numLayers_][0]);
      }
      else {
        activations_[subBatchIndex][layerIndex + 1] = applyActivation(rawOutputs_[subBatchIndex][layerIndex + 1]);
      }
    }
  }
}


void CandClassifier::backPropagation()
{
  const float lambda = 1.0f;

  for (int subBatchIndex = 0; subBatchIndex < batchSize_; subBatchIndex++) {
    //calc output delta for sigmoid + loss combined
    const float output         = activations_[subBatchIndex][numLayers_][0];
    const float oneMinusOutput = 1.0f - output;
    float outputDelta          = output;
    if (batchClasses_[subBatchIndex] == 1) {  /// fake edge
                                              //   outputDelta -= 1.0f;
      outputDelta = std::pow(oneMinusOutput, focalGamma_ - 1) * (focalGamma_ * log(output) - (oneMinusOutput / output))
                    * output * oneMinusOutput;
      outputDelta *= lambda;  // weight factor for loss function
    }
    else {  //// true edge
      outputDelta = std::pow(output, focalGamma_ - 1) * ((output / oneMinusOutput) - focalGamma_ * log(oneMinusOutput))
                    * output * oneMinusOutput;
    }
    // set last layer feature Gradient
    featureGradients_[subBatchIndex][numLayers_] = std::vector<float>{outputDelta};

    // propagate back through layers
    for (int layerIndex = (numLayers_ - 1); layerIndex >= 0; layerIndex--) {
      Matrix& weight         = weights_[layerIndex];
      Matrix weightT         = MLPMath::Transpose2D(weight);
      Matrix& weightGradient = weightGradients_[layerIndex];

      // backprop through activation
      // Note : backprop through last layer already handled in output delta calc.
      if (layerIndex != (numLayers_ - 1)) {
        backPropActivation(layerIndex, activationType_, subBatchIndex);  // updates featureGradients_
      }

      // fetch gradients of the layer on the right. This will already have gradients filled
      std::vector<float>& rDelta = featureGradients_[subBatchIndex][layerIndex + 1];
      // calc. weight gradient for current layer
      calcWeightGradient(weightGradient, rDelta, subBatchIndex, layerIndex);
      // calc. bias gradient for current layer
      biasGradients_[layerIndex] = MLPMath::addVector(biasGradients_[layerIndex], rDelta);
      // calc. feature Gradients for current layer. Will be used as rDelta in next iteration
      featureGradients_[subBatchIndex][layerIndex] = calclFeatureGradient(weightT, rDelta);
    }
  }
}


void CandClassifier::calcWeightGradient(Matrix& weightGradient, std::vector<float>& rDelta, int subBatchIndex,
                                        int layerIndex)
{

  std::vector<float>& lactivation = activations_[subBatchIndex][layerIndex];
  if (layerIndex == 0) {  // there is no activation on first layer
    lactivation = rawOutputs_[subBatchIndex][layerIndex];
  }

  for (std::size_t i = 0; i < weightGradient.size(); i++) {
    for (std::size_t j = 0; j < weightGradient[0].size(); j++) {
      weightGradient[i][j] += rDelta[i] * lactivation[j];
    }
  }
}


std::vector<float> CandClassifier::calclFeatureGradient(Matrix& weightT, std::vector<float>& rDelta)
{

  std::vector<float> lFeatureGradient = MLPMath::MatMul2D1D(weightT, rDelta);

  return lFeatureGradient;
}

std::vector<float> CandClassifier::applyActivation(std::vector<float>& input)
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

void CandClassifier::backPropActivation(int layerIndex, int activationType, int subBatchIndex)
{

  std::vector<float>& rDelta     = featureGradients_[subBatchIndex][layerIndex + 1];
  std::vector<float>& activation = activations_[subBatchIndex][layerIndex + 1];
  std::vector<float>& rawOutput  = rawOutputs_[subBatchIndex][layerIndex + 1];

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

/// returns prediction of network. score is 0 for true triplet and 1 for fake.
int CandClassifier::getClassification(int subBatchIndex)
{
  float threshold = inTesting_ ? thresholdTest_ : thresholdTrain_;

  // we use sigmoid activation on last layer which means activations[numLayers] gives probabilities
  float score = activations_[subBatchIndex][numLayers_][0];
  if (score < threshold)
    return 0;  // true edge
  else
    return 1;
}


void CandClassifier::calculateLoss()
{
  float batch_loss = 0.0f;
  float output     = 0.0f;
  int targetClass  = -1;
  for (int subBatchIndex = 0; subBatchIndex < batchSize_; subBatchIndex++) {
    targetClass = batchClasses_[subBatchIndex];
    output      = activations_[subBatchIndex][numLayers_][0];
    if (targetClass == 1) {                                               /// fake edge
      batch_loss += -std::pow(1.0f - output, focalGamma_) * log(output);  // sigmoid focal loss
      //   loss_ += -log(output); // binary cross entropy
    }
    else {  /// true edge
      batch_loss += -std::pow(output, focalGamma_) * log(1.0f - output);
      //   loss_ += -log(1.0f - output);
    }

    //std::cout << output << std::endl;
    if (isinf(batch_loss)) std::cout << "Warning: Loss is inf" << std::endl;
    if (isinf(batch_loss)) loss_ = 100.f;
    if (isnan(batch_loss)) {
      std::cout << "Warning: Loss is nan" << std::endl;
      std::cout << "output = " << output << std::endl;
      // std::cout << "log(output) = " << log(output) << std::endl;
      // std::cout << "log(1.0f - output) = " << log(1.0f - output) << std::endl;
      // std::cout << "pow(output, focalGamma_) = " << std::pow(output, focalGamma_) << std::endl;
      // std::cout << "pow(1.0f - output, focalGamma_) = " << std::pow(1.0f - output, focalGamma_) << std::endl;
    }
  }
  if (!isnan(batch_loss)) loss_ += batch_loss / (float) batchSize_;
}


void CandClassifier::parameterUpdate()
{
// #define DEBUG_WEIGHTS
#ifdef DEBUG_WEIGHTS
  /// print weight gradient on last layer
  std::cout << "Weight Gradient: " << std::endl;
  for (int i = 0; i < topology_[numLayers_ - 1]; i++) {
    for (int j = 0; j < topology_[numLayers_]; j++) {
      std::cout << weightGradients_[numLayers_ - 1][j][i] << " ";
    }
    if (i == 16) break;
  }
  std::cout << std::endl;

  //// print old weight on last layer
  std::cout << "Weights: " << std::endl;
  for (int i = 0; i < topology_[numLayers_ - 1]; i++) {
    for (int j = 0; j < topology_[numLayers_]; j++) {
      std::cout << weights_[numLayers_ - 1][j][i] << " ";
    }
    if (i == 16) break;
  }
  std::cout << '\n';

  std::cout << std::string(50, '-') << '\n';
#endif

  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {

    // update weight
    Matrix& oldWeight      = weights_[layerIndex];
    Matrix& weightGradient = weightGradients_[layerIndex];
    SGD2D(oldWeight, weightGradient);

    // update bias
    std::vector<float>& oldbias      = biases_[layerIndex];
    std::vector<float>& biasGradient = biasGradients_[layerIndex];
    SGD1D(oldbias, biasGradient);
  }
}

/**
@brief: loads data into EpochInputs_ and [true(1)/fake(0)] into EpochMCInfo_
**/
void CandClassifier::loadTrainDataCandClassifier()
{
  MLPutil::loadTrainDataCandClassifier(numTraining_, EpochInputs_, EpochClasses_, useTestingData_);
  numTraining_ = (int) EpochInputs_.size();
  if (numTraining_ == 0) {
    std::cout << "Error: No training data found. Exiting." << std::endl;
    return;
  }
  nBatches_ = numTraining_ / batchSize_;
  LOG(info) << "Training data Candidate Classifier loaded.";
}


void CandClassifier::loadBatchData(int batchIndex)
{

  batchInputs_.resize(batchSize_);
  for (int i = 0; i < batchSize_; i++) {
    batchInputs_[i].resize(topology_[0]);
  }
  batchClasses_.resize(batchSize_);

  // load examples into batch
  for (int i = 0; i < batchSize_; i++) {
    batchInputs_[i]  = EpochInputs_[batchIndex * batchSize_ + i];
    batchClasses_[i] = EpochClasses_[batchIndex * batchSize_ + i];
  }
}

void CandClassifier::updateStatistics()
{

  int trainingSize   = nCorrectClassifiedEpoch_ + nFalseClassifiedEpoch_;  // numTraining_
  float accThisEpoch = (float) nCorrectClassifiedEpoch_ / (float) trainingSize;

  if (!inTesting_) {  // training
    accuracyTrainingEpoch_.push_back(accThisEpoch);
    truePosTrainingEpoch_.push_back(truePos_);
    trueNegTrainingEpoch_.push_back(trueNeg_);
    falsePosTrainingEpoch_.push_back(falsePos_);
    falseNegTrainingEpoch_.push_back(falseNeg_);

    // loss per training example this epoch
    float loss = getLoss() / (float) trainingSize;
    lossTrainingEpoch_.push_back(loss);
  }
  else {  // testing
    accuracyValidationEpoch_.push_back(accThisEpoch);
    truePosValidationEpoch_.push_back(truePos_);
    trueNegValidationEpoch_.push_back(trueNeg_);
    falsePosValidationEpoch_.push_back(falsePos_);
    falseNegValidationEpoch_.push_back(falseNeg_);

    // loss per validation example this epoch
    float loss = getLoss() / (float) trainingSize;
    lossValidationEpoch_.push_back(loss);
  }


  // std::cout << "Correct = " << nCorrectClassifiedEpoch_ << " , Wrong =  " << nFalseClassifiedEpoch_ << std::endl;
}

void CandClassifier::printStatistics()
{

  std::cout.precision(2);

  int printFreq = 1;
  if (numEpochs_ > 10) printFreq = numEpochs_ / 10;

  if (!inTesting_) {
    for (int iEp = 0; iEp < numEpochs_; iEp += printFreq) {
      std::cout << "Epoch " << iEp + 1 << ":  Training Acc.= " << accuracyTrainingEpoch_[iEp] * 100;
      std::cout << " , Loss = " << lossTrainingEpoch_[iEp] << std::endl;
      printConfusionMatrix(iEp);
      if (((numEpochs_ - iEp) <= printFreq) && (numEpochs_ > 10)) {
        std::cout << "Epoch " << numEpochs_ << ":  Training Acc.= " << accuracyTrainingEpoch_[numEpochs_ - 1] * 100;
        std::cout << " , Loss = " << lossTrainingEpoch_[numEpochs_ - 1] << std::endl;
        printConfusionMatrix(numEpochs_ - 1);
      }
    }
  }
  else {
    std::cout << "Testing Acc.= " << accuracyValidationEpoch_[0] * 100;
    std::cout << " , Loss = " << lossValidationEpoch_[0] << std::endl;
  }

  //std::cout << "Correct = " << nCorrectClassifiedEpoch_ << " .Wrong =  " << nFalseClassifiedEpoch_ << std::endl;
}

void CandClassifier::printConfusionMatrix(int epochIndex)
{
  std::cout << std::setprecision(2);

  int truePos, trueNeg, falsePos, falseNeg;
  if (inTesting_) {
    if (epochIndex >= (int) truePosValidationEpoch_.size()) {
      std::cout << "Error:: printConfusionMatrix: epochIndex out of range." << std::endl;
      return;
    }
    truePos  = truePosValidationEpoch_[epochIndex];
    trueNeg  = trueNegValidationEpoch_[epochIndex];
    falsePos = falsePosValidationEpoch_[epochIndex];
    falseNeg = falseNegValidationEpoch_[epochIndex];
  }
  else {
    if (epochIndex >= (int) truePosTrainingEpoch_.size()) {
      std::cout << "Error:: printConfusionMatrix: epochIndex out of range." << std::endl;
      return;
    }
    truePos  = truePosTrainingEpoch_[epochIndex];
    trueNeg  = trueNegTrainingEpoch_[epochIndex];
    falsePos = falsePosTrainingEpoch_[epochIndex];
    falseNeg = falseNegTrainingEpoch_[epochIndex];
  }

  std::cout << "Confusion Matrix at epoch " << epochIndex + 1 << " : " << std::endl;
  // format : https://en.wikipedia.org/wiki/Confusion_matrix
  // print confusion matrix in table format with true values on y axis and predicted values on x axis
  std::cout << "True\\Predicted\t1\t\t0" << std::endl;
  std::cout << std::fixed << "1\t\t" << truePos << "\t\t" << falseNeg << std::endl;
  std::cout << std::fixed << "0\t\t" << falsePos << "\t\t" << trueNeg << std::endl;


  std::cout << "Accuracy = " << (float) (truePos + trueNeg) / (float) (truePos + trueNeg + falsePos + falseNeg)
            << std::endl;
  std::cout << "Precision = " << (float) (truePos) / (float) (truePos + falsePos) << std::endl;
  std::cout << "Recall = " << (float) (truePos) / (float) (truePos + falseNeg) << std::endl;
}

void CandClassifier::resetStatistics()
{

  nCorrectClassifiedEpoch_ = 0;
  nFalseClassifiedEpoch_   = 0;
  truePos_                 = 0;
  trueNeg_                 = 0;
  falsePos_                = 0;
  falseNeg_                = 0;
  loss_                    = 0.f;
}

void CandClassifier::SGD1D(std::vector<float>& oldWeights, std::vector<float>& gradient)
{
  for (std::size_t i = 0; i < oldWeights.size(); i++) {
    // do not update if gradient is nan
    if (isnan(gradient[i])) {
      continue;
    }
    oldWeights[i] -= SGDLearningRate_ * gradient[i];
  }
}

void CandClassifier::SGD2D(std::vector<std::vector<float>>& oldWeights, std::vector<std::vector<float>>& gradient)
{
  for (std::size_t i = 0; i < oldWeights.size(); i++) {
    for (std::size_t j = 0; j < oldWeights[0].size(); j++) {
      // do not update if gradient is nan
      if (isnan(gradient[i][j])) {
        std::cout << "Warning: SGD2D: gradient is nan \n";
        continue;
      }
      oldWeights[i][j] -= SGDLearningRate_ * gradient[i][j];
    }
  }
}

void CandClassifier::resetGradients()
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
        featureGradients_[i][j][k] = 0.0f;
      }
    }
  }
}

void CandClassifier::writeLossToFile(const std::string& fName)
{
  std::ofstream fout;
  fout.open(fName, std::ofstream::trunc);  // trunc removes previous contents of file

  for (std::size_t i = 0; i < lossTrainingEpoch_.size(); i++) {
    fout << lossTrainingEpoch_[i] << std::endl;
  }
  fout.close();
  std::cout << "Loss data written to file: " << fName << '\n';
}

void CandClassifier::writeAccuracyToFile(const std::string& fName)
{
  std::ofstream fout;
  fout.open(fName, std::ofstream::trunc);  // trunc removes previous contents of file

  for (std::size_t i = 0; i < accuracyTrainingEpoch_.size(); i++) {
    fout << accuracyTrainingEpoch_[i] << std::endl;
  }
  fout.close();
  std::cout << "Accuracy data written to file: " << fName << '\n';
}

void CandClassifier::writeScoreDistToFile(const std::string& fName)
{
  std::ofstream fout;
  fout.open(fName, std::ofstream::trunc);  // trunc removes previous contents of file

  for (std::size_t i = 0; i < scoreDist_.size(); i++) {
    fout << scoreDist_[i].first << " " << scoreDist_[i].second << std::endl;
  }
  fout.close();
  std::cout << "Score distribution data written to file: " << fName << '\n';
}

/// store weights and biases in a file. Removes previous contents of file
void CandClassifier::saveModel(std::string& fNameWeights, std::string& fNameBiases)
{

  std::ofstream fout;

  // write weight to file
  fout.open(fNameWeights, std::ofstream::trunc);  // trunc removes previous contents of file

  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {
    Matrix& weight = weights_[layerIndex];
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

  std::cout << "Candidate Classifier Model saved." << std::endl;
}

/// loads weights and biases from file into the network
void CandClassifier::loadModel(std::string& fNameWeights, std::string& fNameBiases)
{

  std::ifstream fin;

  // requires the weight and bias matrices to be initialized correctly(correct topology)
  // TO DO: change such that the network is initialized from the file

  // load weight
  fin.open(fNameWeights);
  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {
    Matrix& weight = weights_[layerIndex];
    for (std::size_t i = 0; i < weight.size(); i++) {
      for (std::size_t j = 0; j < weight[0].size(); j++) {
        fin >> weight[i][j];
      }
    }
  }
  fin.close();

  // load bias
  fin.open(fNameBiases);
  for (int layerIndex = 0; layerIndex < numLayers_; layerIndex++) {
    std::vector<float>& bias = biases_[layerIndex];
    for (std::size_t i = 0; i < bias.size(); i++) {
      fin >> bias[i];
    }
  }
  fin.close();

  std::cout << "Candidate Classifier Model loaded. \n";
}


void CandClassifier::saveFalseNegativeEdges()
{
  /// save false negative edges coordinates to file
  std::string fName = "/u/otyagi/cbmroot/NN/output/falseNegativeCandidates.txt";
  std::ofstream file(fName);
  if (!file) std::cerr << "Could not open file :" << fName << std::endl;

  int nFalseNegatives = 0;
  for (int batchIndex = 0; batchIndex < nBatches_; batchIndex++) {
    loadBatch(batchIndex);
    feedForward();
    for (int subBatchIndex = 0; subBatchIndex < batchSize_; subBatchIndex++) {
      if ((batchClasses_[subBatchIndex] == 0) && (getClassification(subBatchIndex) == 1)) {  // false negative
        // change to loop and print all inputs
        const float par1 = batchInputs_[subBatchIndex][0];  // chi2
        file << '\n';
        nFalseNegatives++;
      }
    }
  }
  file.close();
  std::cout << nFalseNegatives << " False negative candidates written to file:" << fName << '\n';
}