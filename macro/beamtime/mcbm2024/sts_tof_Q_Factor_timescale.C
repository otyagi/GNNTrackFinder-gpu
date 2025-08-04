/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */


std::vector<double> GenerateLogBinArray(uint32_t uNbDecadesLog, uint32_t uNbStepsDecade, uint32_t uNbSubStepsInStep,
                                        uint32_t& uNbBinsLog, int32_t iStartExp = 0, bool bAddZeroStart = false)
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

double_t ExtractMean(TH1* pHistoIn, double_t dThreshold = 1.0)
{
  // Q-Factor = Max Bin Content / Mean Content of all bin in range
  // => Tend toward 1 if bins are more identical
  if (pHistoIn->Integral()) {
    uint32_t uNbBins           = pHistoIn->GetNbinsX();
    uint32_t uNbNonEmpty       = 0;
    double_t dIntegralNonEmpty = 0.0;
    for (uint32_t uBin = 1; uBin < uNbBins; ++uBin) {
      double_t dBinCount = pHistoIn->GetBinContent(uBin);
      if (dThreshold <= dBinCount) {
        uNbNonEmpty++;
        dIntegralNonEmpty += dBinCount;
      }
    }
    return (dIntegralNonEmpty / uNbNonEmpty);
  }
  else {
    return 0.0;
  }
}
double_t ExtractQFactor(TH1* pHistoIn, double_t dThreshold = 1.0)
{
  // Q-Factor = Max Bin Content / Mean Content of all bin in range
  // => Tend toward 1 if bins are more identical
  if (pHistoIn->Integral()) {
    return (pHistoIn->GetBinContent(pHistoIn->GetMaximumBin())) / ExtractMean(pHistoIn, dThreshold);
  }
  else {
    return 0.0;
  }
}

/** @brief Macro for check of Hades-like Q-Factor based on bmon digis, against variations of bin size and integration
 *         length
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 ** @param dTsSizeNs      Size of one Ts in ns = Total length of one time disribution histogram
 **/
void sts_tof_Q_Factor_timescale(TString inputFileName, uint32_t uRunId, size_t numTimeslices = 0, double_t dStartTs = 0,
                                double_t dStopTs = 0, double_t dTsSizeNs = 128000000.0)
{

  gROOT->cd();
  TFile* file = new TFile(inputFileName, "READ");
  TTree* tree = (TTree*) (file->Get("cbmsim"));

  std::vector<CbmBmonDigi>* vDigisBmon = new std::vector<CbmBmonDigi>();
  tree->SetBranchAddress("BmonDigi", &vDigisBmon);

  std::vector<CbmStsDigi>* vDigisSts = new std::vector<CbmStsDigi>();
  tree->SetBranchAddress("StsDigi", &vDigisSts);

  std::vector<CbmTofDigi>* vDigisTof = new std::vector<CbmTofDigi>();
  tree->SetBranchAddress("TofDigi", &vDigisTof);

  uint32_t nentries = tree->GetEntries();
  cout << "Entries: " << nentries << endl;
  nentries = (numTimeslices && numTimeslices < nentries ? numTimeslices : nentries);

  if (0 < dStopTs && dStopTs < nentries) {
    nentries = dStopTs + 1;
  }
  numTimeslices = nentries - dStartTs;

  gROOT->cd();

  uint32_t uMinNbBins = 10;
  uint32_t uMaxNbBins = 300000;
  /// Hint: keep fractions of TS size and under 100 us
  std::vector<double_t> vdBinSizesNs = {40, 100, 1.28e3, 10.24e3};  // 14 values
  /// Hint: keep fractions of TS size + multiples of bin size and above 10 us
  std::vector<double_t> vdIntegrationNs = {2.56e6};  // 13 values
  /// Dimension: same as BinSizes vector!!
  std::vector<double_t> vdQfactorHistMax = {2000., 400., 40., 20.};

  std::vector<std::vector<uint32_t>> vuNbBinsHisto(vdIntegrationNs.size(),
                                                   std::vector<uint32_t>(vdBinSizesNs.size(), 0));
  std::vector<uint32_t> vuNbHistoCyclesPerTS(vdIntegrationNs.size(), 0);
  std::vector<std::vector<TH1*>> vHistoOld(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vHistoNew(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vQvalOld(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vQvalNew(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vMeanOld(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));
  std::vector<std::vector<TH1*>> vMeanNew(vdIntegrationNs.size(), std::vector<TH1*>(vdBinSizesNs.size(), nullptr));

  std::vector<TH1*> vBinCountDistributionOld(vdBinSizesNs.size(), nullptr);
  std::vector<TH1*> vBinCountDistributionNew(vdBinSizesNs.size(), nullptr);

  uint16_t uNbPlots = 0;
  for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
    /// Pre-check values before in spreadsheet to make sure integer !!!!
    vuNbHistoCyclesPerTS[uHistSz] = dTsSizeNs / vdIntegrationNs[uHistSz];

    for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
      /// Pre-check values before in spreadsheet to make sure integer !!!!
      vuNbBinsHisto[uHistSz][uBinSz] = vdIntegrationNs[uHistSz] / vdBinSizesNs[uBinSz];
      if (uMinNbBins <= vuNbBinsHisto[uHistSz][uBinSz] /*&& vuNbBinsHisto[uHistSz][uBinSz] <= uMaxNbBins*/) {
        vHistoOld[uHistSz][uBinSz] =
          new TH1D(Form("binHist_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Counts per %5.0f ns bin in cycle of range %9.0f ns, Old Bmon; Time in Cycle [ns]; Digis []",
                        vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbBinsHisto[uHistSz][uBinSz], 0.0, vdIntegrationNs[uHistSz]);
        vHistoNew[uHistSz][uBinSz] =
          new TH1D(Form("binHist_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Counts per %5.0f ns bin in cycle of range %9.0f ns, New Bmon; Time in Cycle [ns]; Digis []",
                        vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbBinsHisto[uHistSz][uBinSz], 0.0, vdIntegrationNs[uHistSz]);

        double_t dBinOffset = 1.0 / (2.0 * vuNbHistoCyclesPerTS[uHistSz]);
        vQvalOld[uHistSz][uBinSz] =
          new TH1D(Form("evoQFactor_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, Old Bmon; Time in Run [TS]; Q Factor []",
                        uRunId, vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);
        vQvalNew[uHistSz][uBinSz] =
          new TH1D(Form("evoQFactor_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, New Bmon; Time in Run [TS]; Q Factor []",
                        uRunId, vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);

        vMeanOld[uHistSz][uBinSz] =
          new TH1D(Form("evoMean_Old_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, Old Bmon; Time in Run [TS]; Mean []", uRunId,
                        vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);
        vMeanNew[uHistSz][uBinSz] =
          new TH1D(Form("evoMean_New_%9.0f_%5.0f", vdIntegrationNs[uHistSz], vdBinSizesNs[uBinSz]),
                   Form("Q Factor, run %4d, %5.0f ns bin, %9.0f ns range, New Bmon; Time in Run [TS]; Mean []", uRunId,
                        vdBinSizesNs[uBinSz], vdIntegrationNs[uHistSz]),  //
                   vuNbHistoCyclesPerTS[uHistSz] * numTimeslices, 0.0 - dBinOffset, numTimeslices - dBinOffset);

        uNbPlots++;
      }
    }
  }

  for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
    vBinCountDistributionOld[uBinSz] =
      new TH1D(Form("binCntDist_Old_%5.0f", vdBinSizesNs[uBinSz]),
               Form("Counts per %5.0f ns bin, Old Bmon; Digis []; Bins []", vdBinSizesNs[uBinSz]),  //
               10000, -0.5, 9999.5);
    vBinCountDistributionNew[uBinSz] =
      new TH1D(Form("binCntDist_New_%5.0f", vdBinSizesNs[uBinSz]),
               Form("Counts per %5.0f ns bin, New Bmon; Digis []; Bins []", vdBinSizesNs[uBinSz]),  //
               10000, -0.5, 9999.5);
  }

  std::vector<uint32_t> vuIdxHistoCycleinTS(vdIntegrationNs.size(), 0);
  uint32_t uTotalCountOld = 0;
  uint32_t uTotalCountNew = 0;

  /// Loop on timeslices
  for (Int_t iEntry = dStartTs; iEntry < nentries; iEntry++) {
    tree->GetEntry(iEntry);
    uint32_t nDigisBmon = vDigisBmon->size();
    uint32_t nDigisSts  = vDigisSts->size();
    uint32_t nDigisTof  = vDigisTof->size();


    if (nDigisBmon < 500) {
      std::cout << "TS " << iEntry << ", BMON Digis: " << nDigisBmon << " => spill break, skipping this TS"
                << std::endl;
      continue;
    }

    /*
    std::vector<CbmBmonDigi> vDigisOld;
    std::vector<CbmBmonDigi> vDigisNew;
    vDigisOld.reserve(nDigisBmon);
    vDigisNew.reserve(nDigisBmon);
    for (auto& digi : *vDigisBmon) {
      if (1 == CbmTofAddress::GetChannelSide(digi.GetAddress())) {
        if (CbmTofAddress::GetChannelId(digi.GetAddress()) < 4) {
          vDigisNew.push_back(digi);
        }
        else {
          LOG(fatal) << "Bad sCVD channel: " << CbmTofAddress::GetChannelId(digi.GetAddress());
        }
      }
      else {
        vDigisOld.push_back(digi);
      }
    }
    std::cout << "TS " << iEntry << ", BMON Digis: " << nDigisBmon << " => " << vDigisOld.size() << " Old + "
              << vDigisNew.size() << " sCVD" << std::endl;
    uTotalCountOld += vDigisOld.size();
    uTotalCountNew += vDigisNew.size();
    */
    std::cout << "TS " << iEntry << ", BMON Digis: " << nDigisBmon << " & " << nDigisSts << " Sts"
              << " & " << nDigisTof << " TOF" << std::endl;

    vuIdxHistoCycleinTS.assign(vdIntegrationNs.size(), 0);
    uint32_t uDigiIdx = 0;
    for (auto itOld = vDigisTof->begin(); itOld != vDigisTof->end(); ++itOld) {
      double_t dTime = (*itOld).GetTime();
      if (dTime < 0) {
        std::cout << Form("TS %5d TOF Digi %6u %8.0f out of TS down!!!!!", iEntry, uDigiIdx, dTime) << std::endl;
        uDigiIdx++;
        continue;
      }
      if (dTsSizeNs * 1.01 < dTime) {
        std::cout << Form("TS %5d TOF Digi %6u %8.0f out of TS up [%5.3f]!!!!!", iEntry, uDigiIdx, dTime,
                          dTime / dTsSizeNs)
                  << std::endl;
        uDigiIdx++;
        continue;
      }
      if (0 == uDigiIdx % 1000000) {
        std::cout << Form("TS %5d TOF Digi %8u / %8u", iEntry, uDigiIdx, nDigisTof) << std::endl;
      }

      for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
        uint32_t uCurrentCycle = std::floor(dTime / vdIntegrationNs[uHistSz]);
        if (vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle) {
          for (; vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle; ++vuIdxHistoCycleinTS[uHistSz]) {
            double_t dTsFractional =
              (vdIntegrationNs[uHistSz] * vuIdxHistoCycleinTS[uHistSz]) / dTsSizeNs + iEntry - dStartTs;
            for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
              if (nullptr != vQvalOld[uHistSz][uBinSz]) {
                /*
                std::cout << Form("Extracting TS %5d Digi %8u / %8u hist sz %2d bin sz %2u cycle %u / %u time %9.0f ns",
                  iEntry, uDigiIdx, nDigisTof, uHistSz, uBinSz, vuIdxHistoCycleinTS[uHistSz], uCurrentCycle, dTime)
                  << std::endl;
                */

                double_t dQFactor = ExtractQFactor(vHistoOld[uHistSz][uBinSz], 8);
                vQvalOld[uHistSz][uBinSz]->Fill(dTsFractional, dQFactor);
                vMeanOld[uHistSz][uBinSz]->Fill(dTsFractional, ExtractMean(vHistoOld[uHistSz][uBinSz], 8));
                /*
                if (1280 == vdBinSizesNs[uBinSz]) {
                  std::cout << Form("%9f %9.0f %9.0f %9d %9.0f",
                                dTsFractional, vdIntegrationNs[uHistSz], vHistoOld[uHistSz][uBinSz]->Integral(),
                                vHistoOld[uHistSz][uBinSz]->GetNbinsX(),
                                vHistoOld[uHistSz][uBinSz]->GetBinContent(vHistoOld[uHistSz][uBinSz]->GetMaximumBin()))
                    << std::endl;
                }
                */
                for (uint32_t uBin = 1; uBin <= vHistoOld[uHistSz][uBinSz]->GetNbinsX(); ++uBin) {
                  vBinCountDistributionOld[uBinSz]->Fill(vHistoOld[uHistSz][uBinSz]->GetBinContent(uBin));
                }

                // if (0.0 < dQFactor) {
                vHistoOld[uHistSz][uBinSz]->Reset();
                // }
              }
            }
          }
        }

        double_t dTimeInCycle = std::fmod(dTime, vdIntegrationNs[uHistSz]);
        for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
          if (nullptr != vQvalOld[uHistSz][uBinSz]) {
            vHistoOld[uHistSz][uBinSz]->Fill(dTimeInCycle);
          }
        }
      }
      uDigiIdx++;
    }
    /// FIXME: last "slice" is lost or properly take?
    vuIdxHistoCycleinTS.assign(vdIntegrationNs.size(), 0);
    uDigiIdx = 0;
    for (auto itNew = vDigisSts->begin(); itNew != vDigisSts->end(); ++itNew) {
      double_t dTime = (*itNew).GetTime();
      if (dTime < 0) {
        std::cout << Form("TS %5d Sts Digi %6u %8.0f!!!!!", iEntry, uDigiIdx, dTime) << std::endl;
        continue;
      }
      if (dTsSizeNs * 1.01 < dTime) {
        std::cout << Form("TS %5d Sts Digi %6u %8.0f out of TS up [%5.3f]!!!!!", iEntry, uDigiIdx, dTime,
                          dTime / dTsSizeNs)
                  << std::endl;
        uDigiIdx++;
        continue;
      }
      if (0 == uDigiIdx % 1000000) {
        std::cout << Form("TS %5d Sts Digi %8u / %8u", iEntry, uDigiIdx, nDigisTof) << std::endl;
      }
      for (uint32_t uHistSz = 0; uHistSz < vdIntegrationNs.size(); ++uHistSz) {
        uint32_t uCurrentCycle = std::floor(dTime / vdIntegrationNs[uHistSz]);
        if (vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle) {
          for (; vuIdxHistoCycleinTS[uHistSz] < uCurrentCycle; ++vuIdxHistoCycleinTS[uHistSz]) {
            double_t dTsFractional =
              (vdIntegrationNs[uHistSz] * vuIdxHistoCycleinTS[uHistSz]) / dTsSizeNs + iEntry - dStartTs;
            for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
              if (nullptr != vQvalNew[uHistSz][uBinSz]) {
                double_t dQFactor = ExtractQFactor(vHistoNew[uHistSz][uBinSz]);
                vQvalNew[uHistSz][uBinSz]->Fill(dTsFractional, dQFactor);
                vMeanNew[uHistSz][uBinSz]->Fill(dTsFractional, ExtractMean(vHistoNew[uHistSz][uBinSz]));

                for (uint32_t uBin = 1; uBin <= vHistoNew[uHistSz][uBinSz]->GetNbinsX(); ++uBin) {
                  vBinCountDistributionNew[uBinSz]->Fill(vHistoNew[uHistSz][uBinSz]->GetBinContent(uBin));
                }

                if (0.0 < dQFactor) {
                  vHistoNew[uHistSz][uBinSz]->Reset();
                }
              }
            }
          }
        }

        double_t dTimeInCycle = std::fmod(dTime, vdIntegrationNs[uHistSz]);
        for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
          if (nullptr != vQvalNew[uHistSz][uBinSz]) {
            vHistoNew[uHistSz][uBinSz]->Fill(dTimeInCycle);
          }
        }
      }
      uDigiIdx++;
    }
    /// FIXME: last "slice" is lost or properly take?
    std::cout << Form("Donw with TS %5d Digi %8u / %8u", iEntry, uDigiIdx, nDigisTof) << std::endl;
  }

  uint16_t nPadX = std::ceil(std::sqrt(uNbPlots));
  uint16_t nPadY = std::ceil(1.0 * uNbPlots / nPadX);
  std::cout << Form("Nb plots %3d Nb pads X %2d Y %2d", uNbPlots, nPadX, nPadY) << std::endl;

  uint32_t uNbBinsLog                = 0;
  std::vector<double> dBinsLogVector = GenerateLogBinArray(4, 9, 1, uNbBinsLog, 2);
  double* dBinsLog                   = dBinsLogVector.data();

  TH1* hMeanVsBinSzOld = new TH1D(
    Form("MeanVsBinSzOld_%04u", uRunId),
    Form("Fit of the Mean vs Bin Size, Old, run %04u; Bin Size [ns]; Mean [Digis/bin]", uRunId), uNbBinsLog, dBinsLog);
  TH1* hMeanVsBinSzNew = new TH1D(
    Form("MeanVsBinSzNew_%04u", uRunId),
    Form("Fit of the Mean vs Bin Size, New, run %04u; Bin Size [ns]; Mean [Digis/bin]", uRunId), uNbBinsLog, dBinsLog);
  TH1* hQfactorVsBinSzOld =
    new TH1D(Form("hQfactorVsBinSzOld_%04u", uRunId),
             Form("Fit of Q-Factor vs Bin Size, run %04u; Bin Size [ns]; Q-Factor []", uRunId), uNbBinsLog, dBinsLog);
  TH1* hQfactorVsBinSzNew =
    new TH1D(Form("hQfactorVsBinSzNew_%04u", uRunId),
             Form("Fit of Q-Factor vs Bin Size, run %04u; Bin Size [ns]; Q-Factor []", uRunId), uNbBinsLog, dBinsLog);

  TCanvas* tempOld = new TCanvas("tempOld", "temp Old");
  TCanvas* tempNew = new TCanvas("tempNew", "temp New");
  tempOld->Divide(5, 4);
  tempNew->Divide(5, 4);
  uint32_t uPadIdx = 1;
  for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
    /// Fit Mean and Q-Factor
    if (0 < vMeanOld[0][uBinSz]->GetEntries()) {
      tempOld->cd(uPadIdx);
      auto fCstMeanOld = new TF1("fCstMeanOld", "[0]", 0, numTimeslices);
      vMeanOld[0][uBinSz]->Fit(fCstMeanOld, "", "", 10, 50);
      hMeanVsBinSzOld->Fill(vdBinSizesNs[uBinSz], fCstMeanOld->GetParameter(0));

      TCanvas temp("tempcanv", "temp");
      temp.cd();
      auto fCstQfactorOld = new TF1("fCstQfactorOld", "[0]", 0, numTimeslices);
      vQvalOld[0][uBinSz]->Fit(fCstQfactorOld, "", "", 10, 50);
      hQfactorVsBinSzOld->Fill(vdBinSizesNs[uBinSz], fCstQfactorOld->GetParameter(0));
      delete fCstMeanOld;
      delete fCstQfactorOld;
    }

    if (0 < vMeanNew[0][uBinSz]->GetEntries()) {
      tempNew->cd(uPadIdx);
      auto fCstMeanNew = new TF1("fCstMeanNew", "[0]", 0, numTimeslices);
      vMeanNew[0][uBinSz]->Fit(fCstMeanNew, "", "", 10, 50);
      hMeanVsBinSzNew->Fill(vdBinSizesNs[uBinSz], fCstMeanNew->GetParameter(0));

      TCanvas temp("tempcanv", "temp");
      temp.cd();
      auto fCstQfactorNew = new TF1("fCstQfactorNew", "[0]", 0, numTimeslices);
      vQvalNew[0][uBinSz]->Fit(fCstQfactorNew, "", "", 10, 50);
      hQfactorVsBinSzNew->Fill(vdBinSizesNs[uBinSz], fCstQfactorNew->GetParameter(0));
      delete fCstMeanNew;
      delete fCstQfactorNew;
    }
    uPadIdx++;
  }

  TCanvas* test = new TCanvas("test", "test");
  test->Divide(2, 2);

  test->cd(1);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  hMeanVsBinSzOld->Draw("hist");

  test->cd(2);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  hMeanVsBinSzNew->Draw("hist");

  test->cd(3);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  hQfactorVsBinSzOld->Draw("hist");

  test->cd(4);
  gPad->SetGridx();
  gPad->SetGridy();
  gPad->SetLogx();
  gPad->SetLogy();
  hQfactorVsBinSzNew->Draw("hist");

  TCanvas* cBinCntDistOld = new TCanvas("cBinCntDistOld", "BinCntDist Old");
  TCanvas* cBinCntDistNew = new TCanvas("cBinCntDistNew", "BinCntDist New");
  cBinCntDistOld->Divide(5, 4);
  cBinCntDistNew->Divide(5, 4);
  uPadIdx = 1;
  for (uint32_t uBinSz = 0; uBinSz < vdBinSizesNs.size(); ++uBinSz) {
    cBinCntDistOld->cd(uPadIdx);
    gPad->SetGridx();
    gPad->SetGridy();
    gPad->SetLogy();
    vBinCountDistributionOld[uBinSz]->Draw("hist");

    cBinCntDistNew->cd(uPadIdx);
    gPad->SetGridx();
    gPad->SetGridy();
    gPad->SetLogy();
    vBinCountDistributionNew[uBinSz]->Draw("hist");

    uPadIdx++;
  }

  TFile* outFile =
    new TFile(Form("data/sts_tof_q_factor_timescale_%04u_%03luTs.root", uRunId, numTimeslices), "RECREATE");
  outFile->cd();
  test->Write();
  hMeanVsBinSzOld->Write();
  hMeanVsBinSzNew->Write();
  hQfactorVsBinSzOld->Write();
  hQfactorVsBinSzNew->Write();
  gROOT->cd();

  outFile->Close();

  outFile = new TFile(Form("data/sts_tof_bin_counts_vs_bin_size_%04u_%03luTs_%03.0f_%03.0f.root", uRunId, numTimeslices,
                           dStartTs, dStopTs),
                      "RECREATE");
  outFile->cd();
  test->Write();
  cBinCntDistOld->Write();
  cBinCntDistNew->Write();
  gROOT->cd();

  outFile->Close();

  std::cout << "Beam particles in spill: " << uTotalCountOld << " (Old) VS " << uTotalCountNew << " (New)" << std::endl;
  double_t dAvgCountSecOld = (1.0 * uTotalCountOld) / (numTimeslices * 0.128);
  double_t dAvgCountSecNew = (1.0 * uTotalCountNew) / (numTimeslices * 0.128);
  std::cout << "Avg Beam particles/s: " << dAvgCountSecOld << " (Old) VS " << dAvgCountSecNew << " (New)" << std::endl;
}
