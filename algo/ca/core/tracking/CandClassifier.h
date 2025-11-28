#pragma once  // include this header only once per compilation unit

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <math.h>
#include <iomanip>


typedef std::vector<std::vector<float>> Matrix;
typedef std::vector<std::vector<int>> IntMatrix;
typedef std::vector<std::vector<unsigned int>> UIntMatrix;

class CandClassifier {
 public:
  CandClassifier(const std::vector<int>& MLPTopology);

  CandClassifier()
  {  // default empty constructor
  }

  void analyzeOutput();

  int getClassification(int subBatchIndex);

  void feedForward();

  void backPropagation();

  void resetGradients();

  void calculateLoss();

  void calcWeightGradient(Matrix& weightGradient, std::vector<float>& rDelta, int subBatchIndex, int layerIndex);

  std::vector<float> calclFeatureGradient(Matrix& weight, std::vector<float>& rDelta);

  void parameterUpdate();

  void startTraining(int numTraining, int batchSize, int numEpochs);

  void analyzeEpoch();

  void analyzeBatch();

  void startTesting(const int numTesting);

  std::vector<float> applyActivation(std::vector<float>& input);

  void backPropActivation(int layerIndex, int activationType, int subBatchIndex);

  void SGD1D(std::vector<float>& oldWeights, std::vector<float>& gradient);

  void SGD2D(std::vector<std::vector<float>>& oldWeights, std::vector<std::vector<float>>& gradient);

  void resetStatistics();

  void printStatistics();

  void updateStatistics();

  void loadBatch(int batchIndex);

  void loadBatchData(int batchIndex);

  void loadTrainDataCandClassifier();

  void run(const Matrix& allCands, std::vector<int>& trueCandsIndex, std::vector<float>& trueCandsScore);

  void printConfusionMatrix(int epochIndex);

  void saveModel(std::string& fNameWeights, std::string& fNameBiases);

  void loadModel(std::string& fNameWeights, std::string& fNameBiases);

  float getLoss() { return loss_; }

  void setPathTrainData(const std::vector<std::string>& path) { filePathTrainData_ = path; }

  void writeLossToFile(const std::string& fName);

  void writeAccuracyToFile(const std::string& fName);

  void writeScoreDistToFile(const std::string& fName);

  void saveFalseNegativeEdges();

  void savePerformanceFiles(bool savePerFiles) { savePerFiles_ = savePerFiles; }

  void setTrainThreshold(const float threshold) { thresholdTrain_ = threshold; }

  void setTestThreshold(const float threshold) { thresholdTest_ = threshold; }


  void useTestingData()
  {
    useTestingData_  = true;
    SGDLearningRate_ = 0.0f;  // dont want to update weights
  }

  ~CandClassifier();

 private:
  float thresholdTrain_ = 0.5f;  // threshold for classification. score<threshold -> classified as 0(true candidate)
  float thresholdTest_  = 0.5f;  // 0.2 for model_6

  float lambdaCand_ = -1.0f;  // weight parameter in loss function to account for imbalance in true/fake edges in data

  bool inTesting_      = false;
  bool useTestingData_ = false;  // use testing data for training
  bool saveScoreDist_  = false;  // true in last epoch to save score distribution
  std::vector<std::pair<int, float>> scoreDist_;
  int activationType_ = 0;

  float loss_       = 0.0f;
  float focalGamma_ = 1.0f;  // default is 2. for cross entropy set to 1

  /// Learning rate for SGD
  float SGDLearningRate_ = 1e-3;  // def - 1e-3


  std::vector<int> topology_;
  int numLayers_     = -1;
  int nBatches_      = 0;
  int batchSize_     = 0;
  int numTraining_   = 0;
  int numTesting_    = 0;
  int numValidation_ = 0;
  int numEpochs_     = 0;

  std::vector<Matrix> weights_;
  std::vector<Matrix> weightGradients_;
  Matrix biases_;
  Matrix biasGradients_;

  Matrix batchInputs_;
  std::vector<int> batchClasses_;
  Matrix EpochInputs_;
  std::vector<int> EpochClasses_;

  // storing
  std::vector<Matrix> featureGradients_;
  // store raw output, rawOutput[subBatchIndex][0] is input
  std::vector<Matrix> rawOutputs_;
  // store activation (after applying non-linearity)
  std::vector<Matrix>
    activations_;  // input is activations[subBatchIndex][0] and output is activations[subBatchIndex][last]

  // storing results
  std::vector<float> accuracyTrainingEpoch_;
  std::vector<int> truePosTrainingEpoch_;
  std::vector<int> trueNegTrainingEpoch_;
  std::vector<int> falsePosTrainingEpoch_;
  std::vector<int> falseNegTrainingEpoch_;
  std::vector<float> lossTrainingEpoch_;

  std::vector<float> accuracyValidationEpoch_;
  std::vector<int> truePosValidationEpoch_;
  std::vector<int> trueNegValidationEpoch_;
  std::vector<int> falsePosValidationEpoch_;
  std::vector<int> falseNegValidationEpoch_;
  std::vector<float> lossValidationEpoch_;

  // Counter for statistics:
  int nCorrectClassifiedEpoch_ = 0;
  int nFalseClassifiedEpoch_   = 0;
  int truePos_                 = 0;
  int trueNeg_                 = 0;
  int falsePos_                = 0;
  int falseNeg_                = 0;

  // path to training data
  std::vector<std::string> filePathTrainData_;

  // path to output files
  bool savePerFiles_           = false;  // save loss and accuracy to file
  const std::string sDir       = "/u/otyagi/cbmroot/NN/output/CandClassifier/";
  const std::string lossFName  = sDir + "Train_CandClassifierLoss.txt";
  const std::string accFName   = sDir + "Train_CandClassifierAccuracy.txt";
  const std::string scoreFName = sDir + "Train_CandClassifierScore.txt";
};
