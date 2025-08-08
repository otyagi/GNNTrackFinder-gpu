#include "MLPMath.h"


float MLPMath::CalculateRandomWeight(int lSize, int rSize)
{
  // source: https://pytorch.org/docs/stable/generated/torch.nn.Linear.html
  float k = sqrt(1.f / float(lSize * rSize));
  std::random_device rand_dev;
  std::mt19937 generator(rand_dev());
  std::uniform_real_distribution<float> distr(-k, k);
  return distr(generator);
}


std::vector<std::vector<float>> MLPMath::addMatrix2D(std::vector<std::vector<float>>& A,
                                                     std::vector<std::vector<float>>& B)
{

  int nArows    = (int) A.size();
  int nAcolumns = (int) A[0].size();
  int nBrows    = (int) B.size();
  int nBcolumns = (int) B[0].size();

  if (nArows != nBrows || nAcolumns != nBcolumns) {
    std::cerr << "Matrix indexes incompatible for addition. A shape is (" << nArows << ", " << nAcolumns
              << ") while B shape is (" << nBrows << ", " << nBcolumns << ")." << std::endl;
    exit(EXIT_FAILURE);
  }
  else {
    // initialize matrix C to all zero values
    std::vector<std::vector<float>> C;
    C.resize(nArows);
    for (int i = 0; i < nArows; i++) {
      C[i].resize(nAcolumns);
    }


    for (int i = 0; i < nArows; i++) {
      for (int j = 0; j < nAcolumns; j++) {
        C[i][j] = A[i][j] + B[i][j];
      }
    }
    return C;
  }
}

std::vector<float> MLPMath::MatMul2D1D(std::vector<std::vector<float>>& A, std::vector<float>& B)
{

  int nArows    = A.size();
  int nAcolumns = A[0].size();
  int nBrows    = B.size();

  // initialize matrix C to all zero values
  std::vector<float> C(nArows);

  // check if A and B are commensurate
  if (nAcolumns == nBrows) {

    for (int i = 0; i < nArows; i++) {
      float tmp                = 0.0f;
      std::vector<float>& temp = A[i];
      for (int k = 0; k < nAcolumns; k++) {
        tmp += temp[k] * B[k];
      }
      C[i] = tmp;
    }
    return C;
  }
  else {
    std::cerr << "Matrix indexes incompatible for multiplication. A columns =" << nAcolumns
              << ", but B rows =" << nBrows << std::endl;
    exit(EXIT_FAILURE);
  }
}

std::vector<float> MLPMath::MatMul1D1D(std::vector<float>& A, std::vector<float>& B)
{

  // initialize matrix C to all zero values
  std::vector<float> result(A.size());

  // check if A and B are commensurate and multiply
  if (A.size() == B.size()) {

    for (std::size_t i = 0; i < A.size(); i++) {
      result[i] = A[i] * B[i];
    }
    return result;
  }
  else {
    std::cerr << "Matrix indexes incompatible for multiplication. A size =" << (int) A.size()
              << "B size =" << (int) B.size() << std::endl;
    exit(EXIT_FAILURE);
  }
}


std::vector<float> MLPMath::applySoftmax(std::vector<float>& input)
{

  int inputSize = (int) input.size();
  std::vector<float> output(inputSize);

  float maxValue = input[0];
  for (int i = 1; i < inputSize; i++) {
    if (maxValue < input[i]) {
      maxValue = input[i];
    }
  }

  float expSum = 0.0f;
  std::vector<float> expValue(inputSize);

  for (int i = 0; i < inputSize; i++) {
    float dummy = exp(input[i] - maxValue);
    expValue[i] = dummy;
    expSum += dummy;
  }

  for (int i = 0; i < inputSize; i++) {
    output[i] = expValue[i] / expSum;
  }

  return output;
}


std::vector<std::vector<float>> MLPMath::Transpose2D(std::vector<std::vector<float>>& A)
{

  std::size_t nArows    = A.size();
  std::size_t nAcolumns = A[0].size();

  std::vector<std::vector<float>> result(nAcolumns);
  for (std::size_t i = 0; i < nAcolumns; i++) {
    result[i].resize(nArows);
  }


  for (std::size_t i = 0; i < nAcolumns; i++) {
    for (std::size_t j = 0; j < nArows; j++) {
      result[i][j] = A[j][i];
    }
  }

  return result;
}


std::vector<float> MLPMath::sigmoid(std::vector<float>& input)
{

  std::vector<float> result((int) input.size());

  for (int i = 0; i < (int) input.size(); i++) {
    if (input[i] > 0.f) {
      result[i] = 1.0f / (1.0f + exp(-input[i]));
    }
    else {
      result[i] = exp(input[i]) / (1.0f + exp(input[i]));
    }
  }

  return result;
}

float MLPMath::sigmoid(float& input)
{

  float result;

  if (input > 0.f) {
    result = 1.0f / (1.0f + exp(-input));
  }
  else {
    result = exp(input) / (1.0f + exp(input));
  }

  return result;
}

std::vector<float> MLPMath::affineTransform(std::vector<std::vector<float>>& weight, std::vector<float>& input,
                                            std::vector<float>& bias)
{

  std::vector<float> result((int) bias.size());

  std::vector<float> term1 = MLPMath::MatMul2D1D(weight, input);

  for (std::size_t i = 0; i < bias.size(); i++) {
    result[i] = term1[i] + bias[i];
  }

  return result;
}


std::vector<float> MLPMath::addVector(const std::vector<float>& A, const std::vector<float>& B)
{

  std::vector<float> result(A.size());

  for (std::size_t i = 0; i < A.size(); i++) {
    result[i] = A[i] + B[i];
  }

  return result;
}

std::vector<float> MLPMath::applyTanH(std::vector<float>& rawHidden)
{

  std::vector<float> result(rawHidden.size());
  float twoexp = 0.0f;

  for (std::size_t i = 0; i < rawHidden.size(); i++) {
    float tmp = rawHidden[i];
    if (tmp > 20.f) {  // to handle numerical overflow
      result[i] = 1.0f;
    }
    else {
      twoexp    = exp(2 * tmp);
      result[i] = (twoexp - 1.0f) / (twoexp + 1.0f);
    }
  }

  return result;
}
