/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

/** @brief Macro for check of BMON digis bunching in direct vector from legacy or online unpacking
 ** @param input          Name of input file
 ** @param nTimeSlices    Number of time-slices to process
 **/
void check_bmon_digis_bunches(TString inputFileName, uint32_t uRunId, size_t numTimeslices = 0,
                              double_t dBunchCutOff = 40.0, double_t dBunchRange = 500.0, double_t dBunchStep = 5.0,
                              bool bNbDigisZ = true)
{
  gROOT->cd();
  TFile* file = new TFile(inputFileName, "READ");
  TTree* tree = (TTree*) (file->Get("cbmsim"));

  std::vector<CbmBmonDigi>* vDigisBmon = new std::vector<CbmBmonDigi>();
  tree->SetBranchAddress("BmonDigi", &vDigisBmon);

  uint32_t nentries = tree->GetEntries();
  cout << "Entries: " << nentries << endl;
  nentries = (numTimeslices && numTimeslices < nentries ? numTimeslices : nentries);

  gROOT->cd();

  int32_t iNbBinsRange = std::round(dBunchRange / dBunchStep) + 1;

  TString sName  = "histRangeMulOld";
  TString sTitle = Form("Digis multiplicity vs range old BMON, run %u; Range to first digi [ns]; Digis mul []", uRunId);
  TH2* fHistRangeMulOld = new TH2I(sName, sTitle, iNbBinsRange, 0.0, dBunchRange + dBunchStep, 300, 1.0, 300.0);

  sName  = "histRangeMulNew";
  sTitle = Form("Digis multiplicity vs range new BMON, run %u; Range to first digi [ns]; Digis mul []", uRunId);
  TH2* fHistRangeMulNew = new TH2I(sName, sTitle, iNbBinsRange, 0.0, dBunchRange + dBunchStep, 300, 0.0, 300.0);

  sName  = "histBunchMulOld";
  sTitle = Form("Bunch multiplicity vs duration old BMON, cutoff %.2f ns, run %u; Bunch range [ns]; Bunch mul []; %s",
                dBunchCutOff, uRunId, bNbDigisZ ? " Digis in bunches []" : " Bunches []");
  TH2* fHistBunchMulOld = new TH2I(sName, sTitle, iNbBinsRange, 0.0, dBunchRange + dBunchStep, 300, 1.0, 300.0);

  sName  = "histBunchMulNew";
  sTitle = Form("Bunch multiplicity vs duration old BMON, cutoff %.2f ns, run %u; Bunch range [ns]; Bunch mul []; %s",
                dBunchCutOff, uRunId, bNbDigisZ ? " Digis in bunches []" : " Bunches []");
  TH2* fHistBunchMulNew = new TH2I(sName, sTitle, iNbBinsRange, 0.0, dBunchRange + dBunchStep, 300, 0.0, 300.0);

  for (Int_t iEntry = 0; iEntry < nentries; iEntry++) {
    //if (iEntry % 10 == 0 ) std::cout << "Entry " << iEntry << " / " << nentries << std::endl;

    tree->GetEntry(iEntry);
    uint32_t nDigisBmon = vDigisBmon->size();

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
    std::cout << "TS " << iEntry << ", BMON  Digis: " << nDigisBmon << " => " << vDigisOld.size() << " Old + "
              << vDigisNew.size() << " sCVD" << std::endl;

    for (auto itOld = vDigisOld.begin(); itOld != vDigisOld.end(); ++itOld) {
      auto nextDigiOld = itOld;
      ++nextDigiOld;
      uint32_t uRangeMul = 1;
      while (nextDigiOld != vDigisOld.end()) {
        double_t dDt = (*nextDigiOld).GetTime() - (*itOld).GetTime();
        if (dDt < dBunchRange) {
          uRangeMul++;
          fHistRangeMulOld->Fill(dDt, uRangeMul);
          ++nextDigiOld;
        }
        else {
          break;
        }
      }
      if (1 == uRangeMul) {
        fHistRangeMulOld->Fill(dBunchRange, uRangeMul);
      }
    }
    for (auto itNew = vDigisNew.begin(); itNew != vDigisNew.end(); ++itNew) {
      auto nextDigiNew = itNew;
      ++nextDigiNew;
      uint32_t uRangeMul = 1;
      while (nextDigiNew != vDigisNew.end()) {
        double_t dDt = (*nextDigiNew).GetTime() - (*itNew).GetTime();
        if (dDt < dBunchRange) {
          uRangeMul++;
          fHistRangeMulNew->Fill(dDt, uRangeMul);
          ++nextDigiNew;
        }
        else {
          break;
        }
      }
      if (1 == uRangeMul) {
        fHistRangeMulNew->Fill(dBunchRange, uRangeMul);
      }
    }

    for (auto itOld = vDigisOld.begin(); itOld != vDigisOld.end();) {
      auto currDigiOld = itOld;
      auto nextDigiOld = itOld;
      ++nextDigiOld;
      uint32_t uBunchMul = 1;
      while (nextDigiOld != vDigisOld.end()) {
        double_t dDt      = (*nextDigiOld).GetTime() - (*itOld).GetTime();
        double_t dDtDigis = (*nextDigiOld).GetTime() - (*currDigiOld).GetTime();
        if (dDt < dBunchRange && dDtDigis < dBunchCutOff) {
          uBunchMul++;
          currDigiOld = nextDigiOld;
          ++nextDigiOld;
        }
        else {
          /// End of bunch, fill intervall between first and last digi in bunch + multiplicity
          double_t dDtBunch = (*currDigiOld).GetTime() - (*itOld).GetTime();
          // For bNbDigisZ = true, Z = Nb Digis in bunches, else Z = Nb bunches
          fHistBunchMulOld->Fill(dDtBunch, uBunchMul, bNbDigisZ ? uBunchMul : 1);
          break;
        }
      }
      /// Go to first digi out of bunch
      itOld = nextDigiOld;
      if (1 == uBunchMul) {
        fHistBunchMulOld->Fill(0.0, 1);
      }
    }
    for (auto itNew = vDigisNew.begin(); itNew != vDigisNew.end();) {
      auto currDigiNew = itNew;
      auto nextDigiNew = itNew;
      ++nextDigiNew;
      uint32_t uBunchMul = 1;
      while (nextDigiNew != vDigisNew.end()) {
        double_t dDt      = (*nextDigiNew).GetTime() - (*itNew).GetTime();
        double_t dDtDigis = (*nextDigiNew).GetTime() - (*currDigiNew).GetTime();
        if (dDt < dBunchRange && dDtDigis < dBunchCutOff) {
          uBunchMul++;
          currDigiNew = nextDigiNew;
          ++nextDigiNew;
        }
        else {
          /// End of bunch, fill intervall between first and last digi in bunch + multiplicity
          double_t dDtBunch = (*currDigiNew).GetTime() - (*itNew).GetTime();
          // For bNbDigisZ = true, Z = Nb Digis in bunches, else Z = Nb bunches
          fHistBunchMulNew->Fill(dDtBunch, uBunchMul, bNbDigisZ ? uBunchMul : 1);
          break;
        }
      }
      /// Go to first digi out of bunch
      itNew = nextDigiNew;
      if (1 == uBunchMul) {
        fHistBunchMulNew->Fill(0.0, 1);
      }
    }
  }

  TCanvas* fCanvBunches = new TCanvas("canvBunches", "BMON Digis Bunch multiplicity at various short ranges");
  fCanvBunches->Divide(2, 2);

  fCanvBunches->cd(1);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistRangeMulOld->Draw("Colz");

  fCanvBunches->cd(2);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistRangeMulNew->Draw("Colz");

  fCanvBunches->cd(3);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistBunchMulOld->Draw("Colz");

  fCanvBunches->cd(4);
  gPad->SetLogz();
  gPad->SetGridx();
  gPad->SetGridy();
  fHistBunchMulNew->Draw("Colz");
}
