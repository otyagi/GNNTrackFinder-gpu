/* Copyright (C) 2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer], Florian Uhlig */

#include "CbmFlesHistosTools.h"

#include <cmath>
//#include <vector>

std::vector<double> GenerateLogBinArray(uint32_t uNbDecadesLog, uint32_t uNbStepsDecade, uint32_t uNbSubStepsInStep,
                                        uint32_t& uNbBinsLog, int32_t iStartExp, bool bAddZeroStart)
{
  /// Logarithmic bining for self time comparison
  /// Number of log bins =
  ///      9 for the sub-unit decade
  ///    + 9 for each unit of each decade * 10 for the subdecade range
  ///    + 1 for the closing bin top edge
  uNbBinsLog = uNbStepsDecade + uNbStepsDecade * uNbSubStepsInStep * uNbDecadesLog;

  /// Need uNbBinsLog + 1 values as we need to provide the end of last bin
  uint32_t uArrayLength = uNbBinsLog + 1;
  double dBinsLog[uArrayLength];
  /// First fill sub-unit decade
  for (uint32_t uSubU = 0; uSubU < uNbStepsDecade; uSubU++) {
    dBinsLog[uSubU] = std::pow(10, iStartExp - 1) * (1 + uSubU);
  }

  /// Then fill the main decades
  double dSubstepSize = 1.0 / uNbSubStepsInStep;
  for (uint32_t uDecade = 0; uDecade < uNbDecadesLog; uDecade++) {
    double dBase        = std::pow(10, iStartExp + static_cast<int32_t>(uDecade));
    uint32_t uDecadeIdx = uNbStepsDecade + uDecade * uNbStepsDecade * uNbSubStepsInStep;
    for (uint32_t uStep = 0; uStep < uNbStepsDecade; uStep++) {
      uint32_t uStepIdx = uDecadeIdx + uStep * uNbSubStepsInStep;
      for (uint32_t uSubStep = 0; uSubStep < uNbSubStepsInStep; uSubStep++) {
        dBinsLog[uStepIdx + uSubStep] = dBase * (1 + uStep) + dBase * dSubstepSize * uSubStep;
      }  // for( uint32_t uSubStep = 0; uSubStep < uNbSubStepsInStep; uSubStep++ )
    }    // for( uint32_t uStep = 0; uStep < uNbStepsDecade; uStep++ )
  }      // for( uint32_t uDecade = 0; uDecade < uNbDecadesLog; uDecade ++)
  dBinsLog[uNbBinsLog] = std::pow(10, iStartExp + uNbDecadesLog);

  /// use vector instead
  std::vector<double> dBinsLogVect;

  ///    + 1 optional if bin [ 0; Min [ should be added
  if (bAddZeroStart) {
    uNbBinsLog++;
    dBinsLogVect.push_back(0);
  }

  for (uint32_t i = 0; i < uArrayLength; ++i) {
    dBinsLogVect.push_back(dBinsLog[i]);
  }  // for( uint32_t i = 0; i < uArrayLength; ++i )

  return dBinsLogVect;
}
