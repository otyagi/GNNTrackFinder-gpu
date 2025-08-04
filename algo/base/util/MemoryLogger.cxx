/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#include "MemoryLogger.h"

#include "AlgoFairloggerCompat.h"
#include "System.h"

using namespace cbm::algo;

template<typename T>
T MemoryLogger::BytesToMB(T bytes) const
{
  return bytes / (1024 * 1024);
}

void MemoryLogger::Log()
{
  size_t currentRSS = GetCurrentRSS();
  size_t peakRSS    = GetPeakRSS();

  ptrdiff_t deltaRSS = currentRSS - mLastRSS;
  float deltaPercent = 100.0f * deltaRSS / currentRSS;

  L_(debug) << "Current memory usage: " << BytesToMB(currentRSS) << "MB (delta  " << BytesToMB(deltaRSS) << "MB / "
            << deltaPercent << "%)"
            << ", peak: " << BytesToMB(peakRSS) << "MB";

  mLastRSS = currentRSS;
}
