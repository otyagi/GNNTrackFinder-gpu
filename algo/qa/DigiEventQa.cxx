/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], P.-A. Loizeau */

#include "DigiEventQa.h"

#include "Histogram.h"

#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using cbm::algo::qa::H1D;
using std::string;
using std::vector;

namespace cbm::algo::evbuild
{

  // ---   Execution   --------------------------------------------------------
  DigiEventQaData DigiEventQa::operator()(const vector<DigiEvent>& events) const
  {
    // --- Instantiate return object
    DigiEventQaData result;
    for (const auto& entry : fConfig.fData) {
      ECbmModuleId subsystem = entry.first;
      auto& detConfig        = fConfig.fData.at(subsystem);
      result.fDigiTimeHistos[subsystem] =
        &(result.fHistContainer.fvH1.emplace_front(DigiEventQaConfig::GetDigiTimeHistoName(subsystem), "",
                                                   detConfig.fNumBins, detConfig.fMinValue, detConfig.fMaxValue));
    }

    // --- Event loop. Fill histograms.
    for (const auto& event : events) {                  //
      for (auto& subsystem : result.fDigiTimeHistos) {  //
        QaDigiTimeInEvent(event, subsystem.first, subsystem.second);
      }
    }
    result.fNumEvents = events.size();

    return result;
  }
  // --------------------------------------------------------------------------


  // ---  QA: digi time within event   ----------------------------------------
  void DigiEventQa::QaDigiTimeInEvent(const DigiEvent& event, ECbmModuleId system, H1D* histo) const
  {
    switch (system) {

      case ECbmModuleId::kBmon: FillDeltaT<CbmBmonDigi>(event.fBmon, event.fTime, histo); break;
      case ECbmModuleId::kSts: FillDeltaT<CbmStsDigi>(event.fSts, event.fTime, histo); break;
      case ECbmModuleId::kMuch: FillDeltaT<CbmMuchDigi>(event.fMuch, event.fTime, histo); break;
      case ECbmModuleId::kRich: FillDeltaT<CbmRichDigi>(event.fRich, event.fTime, histo); break;
      case ECbmModuleId::kTrd: FillDeltaT<CbmTrdDigi>(event.fTrd, event.fTime, histo); break;
      case ECbmModuleId::kTrd2d: FillDeltaT<CbmTrdDigi>(event.fTrd2d, event.fTime, histo); break;
      case ECbmModuleId::kTof: FillDeltaT<CbmTofDigi>(event.fTof, event.fTime, histo); break;
      case ECbmModuleId::kFsd: FillDeltaT<CbmFsdDigi>(event.fFsd, event.fTime, histo); break;
      case ECbmModuleId::kPsd: FillDeltaT<CbmPsdDigi>(event.fPsd, event.fTime, histo); break;
      default: throw std::runtime_error("DigiEventQa: Invalid system Id " + ::ToString(system));
    }
  }
  // --------------------------------------------------------------------------


  // -----   Info to string   -------------------------------------------------
  std::string DigiEventQa::ToString() const
  {
    std::stringstream out;
    out << "--- Using DigiEventQa with parameters:";
    for (const auto& entry : fConfig.fData) {
      out << "\n   " << std::left << std::setw(5) << ::ToString(entry.first) << ": ";
      out << "bins" << std::right << std::setw(5) << entry.second.fNumBins;
      out << "  range [" << std::right << std::setw(5) << entry.second.fMinValue;
      out << ", " << std::right << std::setw(5) << entry.second.fMaxValue << "] ns";
    }
    return out.str();
  }
  // --------------------------------------------------------------------------


}  // namespace cbm::algo::evbuild
