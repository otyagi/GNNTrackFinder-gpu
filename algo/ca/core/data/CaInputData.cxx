/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaInputData.cxx
/// \brief  Structure for input data to the L1 tracking algorithm (implementation)
/// \since  08.08.2022
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CaInputData.h"

using cbm::algo::ca::InputData;

// ---------------------------------------------------------------------------------------------------------------------
//
InputData::InputData() {}

// ---------------------------------------------------------------------------------------------------------------------
//
InputData::InputData(const InputData& other)
  : fvHits(other.fvHits)
  , fvStreamStartIndices(other.fvStreamStartIndices)
  , fvStreamStopIndices(other.fvStreamStopIndices)
  , fNhitKeys(other.fNhitKeys)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
InputData::InputData(InputData&& other) noexcept { this->Swap(other); }

// ---------------------------------------------------------------------------------------------------------------------
//
InputData& InputData::operator=(const InputData& other)
{
  if (this != &other) {
    InputData(other).Swap(*this);
  }
  return *this;
}

// ---------------------------------------------------------------------------------------------------------------------
//
InputData& InputData::operator=(InputData&& other) noexcept
{
  if (this != &other) {
    InputData tmp(std::move(other));
    this->Swap(tmp);
  }
  return *this;
}
