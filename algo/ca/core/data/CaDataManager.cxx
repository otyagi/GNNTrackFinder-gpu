/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaDataManager.h
/// \brief  Input-output data manager for L1 tracking algorithm
/// \since  08.08.2022
/// \author S.Zharko <s.zharko@gsi.de>

#include "CaDataManager.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include <fstream>

using cbm::algo::ca::DataManager;
using cbm::algo::ca::InputData;

// ---------------------------------------------------------------------------------------------------------------------
//
InputData&& DataManager::TakeInputData()
{
  // Init the input data
  InitData();

  // Check the input data
  // TODO: Provide assertion
  // if (CheckInputData<constants::control::InputDataQaLevel>()) {
  //   pAlgo->ReceiveInputData(std::move(fInputData));
  //
  // }

  return std::move(fInputData);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void DataManager::ReadInputData(const std::string& fileName)
{
  // Reset input data object
  ResetInputData();
  LOG(info) << "L1: Input data will be read from file \"" << fileName << "\"";

  // Open input binary file
  std::ifstream ifs(fileName, std::ios::binary);
  if (!ifs) {
    LOG(fatal) << "L1: input data reader: data file \"" << fileName << "\" was not found";
  }

  // Get InputData object
  try {
    boost::archive::binary_iarchive ia(ifs);
    ia >> fInputData;
  }
  catch (const std::exception&) {
    LOG(fatal) << "L1: input data reader: data file \"" << fileName << "\" has incorrect data format or was corrupted";
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void DataManager::ResetInputData(HitIndex_t nHits) noexcept
{
  InputData tmp;
  fInputData.Swap(tmp);
  fLastStreamId = -1;
  fInputData.fvStreamStartIndices.reserve(2000);  // TODO: What are these numbers? Please, put them into constants.h
  fInputData.fvStreamStopIndices.reserve(2000);
  fInputData.fvHits.reserve(nHits);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void DataManager::InitData()
{
  // set the end pointers to data streams
  // TODO: SZh 14.08.2023: Move the max allowed number of streams to the constants.h

  int nStreams = fInputData.fvStreamStartIndices.size();
  if (!nStreams) {  // No data streams provided
    fInputData.fvStreamStartIndices.push_back(0);
    fInputData.fvStreamStopIndices.push_back(fInputData.fvHits.size());
  }
  else {
    if (nStreams > 3000) {
      LOG(warning) << "ca::DataManager: unexpected order of input data: too many data streams!!! ";
      fInputData.fvStreamStartIndices.shrink(3000);
    }
    fInputData.fvStreamStopIndices.reset(nStreams);
    for (int i = 0; i < nStreams - 1; i++) {
      fInputData.fvStreamStopIndices[i] = fInputData.fvStreamStartIndices[i + 1];
    }
    fInputData.fvStreamStopIndices[nStreams - 1] = fInputData.fvHits.size();
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void DataManager::WriteInputData(const std::string& fileName) const
{
  // Check current data object for consistency
  if constexpr (constants::control::InputDataQaLevel > 0) {
    if (!CheckInputData<constants::control::InputDataQaLevel>()) {
      LOG(error) << "ca::DataManager: input data writer: attempt to write invalid input data object to file \""
                 << fileName << "\"";
      return;
    }
  }

  // Open output binary file
  std::ofstream ofs(fileName, std::ios::binary);
  if (!ofs) {
    LOG(error) << "ca::DataManager: input data writer: failed opening file \"" << fileName
               << " for writing input data\"";
    return;
  }

  // Serialize InputData object and write
  boost::archive::binary_oarchive oa(ofs);
  oa << fInputData;
}
