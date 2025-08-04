/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: P.-A. Loizeau [committer] */

#include "RecoGeneralQa.h"

#include <cmath>

using cbm::algo::qa::Data;
using cbm::algo::qa::H1D;

namespace cbm::algo::qa
{
  RecoGeneralQa::RecoGeneralQa(const uint64_t& runStartTimeNs, std::shared_ptr<HistogramSender> pSender)
    : fRunStartTimeNs(runStartTimeNs)
    , fpSender(pSender)
  {
  }

  // ---   Execution   --------------------------------------------------------
  void RecoGeneralQa::operator()(const fles::Timeslice& ts)
  {
    if (!fpSender.get()) {
      return;
    }

    if (fInitNotDone) {
      double_t dBegAxisX  = 0.0;
      double_t dSizeTsSec = 0.128;  /// FIXME: Avoid default value in case first component has only a single MS
      if (ts.num_components() != 0 && ts.num_microslices(0) > 1) {
        dSizeTsSec = ((ts.descriptor(0, 1).idx - ts.descriptor(0, 0).idx) * ts.num_core_microslices()) * 1.0e-9;
      }
      auto cName = "processed_ts";
      auto cTitl = "Statistics of TS processed online";
      auto canv  = CanvasConfig(cName, cTitl, 2, 1);
      {
        auto pad              = PadConfig();
        auto name             = "timeslices_count_evo";
        auto titl             = "Number of processed TS vs time in run; time in run [s]; Nb TS []";
        int32_t nbBins        = std::ceil(7200.0 / (dSizeTsSec * kNbTsPerBinCount));  // a bit more than 2h range
        double_t dEndAxisX    = nbBins * dSizeTsSec * kNbTsPerBinCount;
        fphTimeslicesCountEvo = fQaData.MakeObj<H1D>(name, titl, nbBins, dBegAxisX, dEndAxisX);
        pad.RegisterHistogram(fphTimeslicesCountEvo, "hist");
        canv.AddPadConfig(pad);
      }
      {
        auto pad                 = PadConfig();
        auto name                = "timeslices_fraction_evo";
        auto titl                = "Fraction of TS processed vs time in run; time in run [s]; Processed TS []";
        int32_t nbBins           = std::ceil(7200.0 / (dSizeTsSec * kNbTsPerBinFrac));  // a bit more than 2h range
        double_t dEndAxisX       = nbBins * dSizeTsSec * kNbTsPerBinFrac;
        fphTimeslicesFractionEco = fQaData.MakeObj<H1D>(name, titl, nbBins, dBegAxisX, dEndAxisX);
        pad.RegisterHistogram(fphTimeslicesFractionEco, "hist");
        canv.AddPadConfig(pad);
      }
      fQaData.AddCanvasConfig(canv);

      fQaData.Init(fpSender);
      fInitNotDone = false;
    }

    fphTimeslicesCountEvo->Fill((ts.start_time() - fRunStartTimeNs) * 1e-9);
    fphTimeslicesFractionEco->Fill((ts.start_time() - fRunStartTimeNs) * 1e-9, 1.0 / kNbTsPerBinFrac);

    fQaData.Send(fpSender);  // Send and reset!
  }

}  // namespace cbm::algo::qa
