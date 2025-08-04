/* Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Florian Uhlig [committer], Andreas Redelbach */

#include "CbmCheckTiming.h"

#include "CbmDigiManager.h"
#include "CbmFlesHistosTools.h"
#include "CbmMuchBeamTimeDigi.h"
#include "CbmPsdDigi.h"
#include "CbmRichDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"
#include "CbmTrdDigi.h"

#include "FairRootManager.h"
#include "FairRunOnline.h"
#include <Logger.h>

#include "TClonesArray.h"
#include "TF1.h"
#include "TH1.h"
#include "TH2.h"
#include "THttpServer.h"
#include <TFile.h>

#include <iomanip>
#include <iostream>
using std::fixed;
using std::setprecision;

// ---- Default constructor -------------------------------------------
CbmCheckTiming::CbmCheckTiming() : FairTask("CbmCheckTiming") {}

// ---- Destructor ----------------------------------------------------
CbmCheckTiming::~CbmCheckTiming() {}

// ----  Initialisation  ----------------------------------------------
void CbmCheckTiming::SetParContainers()
{
  // Load all necessary parameter containers from the runtime data base
  /*
  FairRunAna* ana = FairRunAna::Instance();
  FairRuntimeDb* rtdb=ana->GetRuntimeDb();

  <CbmCheckTimingDataMember> = (<ClassPointer>*)
    (rtdb->getContainer("<ContainerName>"));
  */
}

// ---- Init ----------------------------------------------------------
InitStatus CbmCheckTiming::Init()
{

  // Get a handle from the IO manager
  FairRootManager* ioman = FairRootManager::Instance();
  fDigiMan               = CbmDigiManager::Instance();
  fDigiMan->UseMuchBeamTimeDigi();
  fDigiMan->Init();

  // Get a pointer to the previous already existing data level
  fBmonDigiVec = ioman->InitObjectAs<std::vector<CbmTofDigi> const*>("BmonDigi");
  if (!fBmonDigiVec) {
    fBmonDigiArr = dynamic_cast<TClonesArray*>(ioman->GetObject("BmonDigi"));
    if (!fBmonDigiArr) { LOG(fatal) << "No TClonesArray with Bmon digis found."; }
  }

  if (!fDigiMan->IsPresent(ECbmModuleId::kSts)) { LOG(info) << "No STS digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kMuch)) { LOG(info) << "No MUCH digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kTrd)) { LOG(info) << "No TRD digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kTof)) { LOG(info) << "No TOF digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kRich)) { LOG(info) << "No RICH digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kPsd)) { LOG(info) << "No PSD digis found."; }

  if (fCheckInterSystemOffset) CreateHistos();

  return kSUCCESS;
}

Int_t CbmCheckTiming::CalcNrBins(Int_t offsetRange)
{

  if (offsetRange < 1001) { fBinWidth = 5; }
  else if (offsetRange < 10001) {
    fBinWidth = 10;
  }
  else if (offsetRange < 100001) {
    fBinWidth = 100;
  }
  else {
    fBinWidth = 100;
  }

  if ((static_cast<Double_t>(offsetRange) / 6.25) == (offsetRange / 6.25)) return (offsetRange / 6.25 * 2);

  return (offsetRange / fBinWidth * 2);
}

void CbmCheckTiming::CreateHistos()
{
  /// Logarithmic bining for self time comparison
  uint32_t iNbBinsLog = 0;
  /// Parameters are NbDecadesLog, NbStepsDecade, NbSubStepsInStep
  std::vector<double> dBinsLogVector = GenerateLogBinArray(9, 9, 1, iNbBinsLog);
  double* dBinsLog                   = dBinsLogVector.data();
  //  double * dBinsLog = GenerateLogBinArray( 9, 9, 1, iNbBinsLog );

  Int_t nrOfBinsSts = CalcNrBins(fStsOffsetRange);
  // Bmon vs. Sts
  fBmonStsDiff =
    new TH1D("fBmonStsDiff", "Sts-Bmon;time diff [ns];Counts", nrOfBinsSts, -fStsOffsetRange, fStsOffsetRange);

  fBmonStsDiffCharge = new TH2F("fBmonStsDiffCharge", "Sts-Bmon;time diff [ns]; Charge [a.u]; Counts", nrOfBinsSts,
                                -fStsOffsetRange, fStsOffsetRange, 256, 0, 256);

  fBmonStsDiffEvo = new TH2F("fBmonStsDiffEvo", "Sts-Bmon;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsSts,
                             -fStsOffsetRange, fStsOffsetRange);


  Int_t nrOfBinsMuch = CalcNrBins(fMuchOffsetRange);
  // Bmon vs. Much
  fBmonMuchDiff =
    new TH1D("fBmonMuchDiff", "Much-Bmon;time diff [ns];Counts", nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);

  fBmonMuchDiffCharge = new TH2F("fBmonMuchDiffCharge", "Much-Bmon;time diff [ns]; Charge [a.u]; ;Counts", nrOfBinsMuch,
                                 -fMuchOffsetRange, fMuchOffsetRange, 256, 0, 256);

  fBmonMuchDiffEvo = new TH2F("fBmonMuchDiffEvo", "Much-Bmon;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsMuch,
                              -fMuchOffsetRange, fMuchOffsetRange);

  Int_t nrOfBinsTrd = CalcNrBins(fTrdOffsetRange);
  // To vs. Trd
  fBmonTrdDiff =
    new TH1D("fBmonTrdDiff", "Trd-Bmon;time diff [ns];Counts", nrOfBinsTrd, -fTrdOffsetRange, fTrdOffsetRange);

  fBmonTrdDiffCharge = new TH2F("fBmonTrdDiffCharge", "Trd-Bmon;time diff [ns]; Charge [a.u]; ;Counts", nrOfBinsTrd,
                                -fTrdOffsetRange, fTrdOffsetRange, 256, 0, 256);

  fBmonTrdDiffEvo = new TH2F("fBmonTrdDiffEvo", "Trd-Bmon;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsTrd,
                             -fTrdOffsetRange, fTrdOffsetRange);

  Int_t nrOfBinsTof = CalcNrBins(fTofOffsetRange);
  // To vs. Tof
  fBmonTofDiff =
    new TH1D("fBmonTofDiff", "Tof-Bmon;time diff [ns];Counts", nrOfBinsTof, -fTofOffsetRange, fTofOffsetRange);

  fBmonTofDiffCharge = new TH2F("fBmonTofDiffCharge", "Tof-Bmon;time diff [ns]; Charge [a.u]; ;Counts", nrOfBinsTof,
                                -fTofOffsetRange, fTofOffsetRange, 256, 0, 256);

  fBmonTofDiffEvo = new TH2F("fBmonTofDiffEvo", "Tof-Bmon;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsTof,
                             -fTofOffsetRange, fTofOffsetRange);


  Int_t nrOfBinsRich = CalcNrBins(fRichOffsetRange);
  // To vs. Rich
  fBmonRichDiff =
    new TH1D("fBmonRichDiff", "Rich-Bmon;time diff [ns];Counts", nrOfBinsRich, -fRichOffsetRange, fRichOffsetRange);

  fBmonRichDiffCharge = new TH2F("fBmonRichDiffCharge", "Rich-Bmon;time diff [ns]; Charge [a.u]; ;Counts", nrOfBinsRich,
                                 -fRichOffsetRange, fRichOffsetRange, 256, 0, 256);

  fBmonRichDiffEvo = new TH2F("fBmonRichDiffEvo", "Rich-Bmon;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsRich,
                              -fRichOffsetRange, fRichOffsetRange);

  Int_t nrOfBinsPsd = CalcNrBins(fPsdOffsetRange);
  // To vs. Psd
  fBmonPsdDiff =
    new TH1D("fBmonPsdDiff", "Psd-Bmon;time diff [ns];Counts", nrOfBinsPsd, -fPsdOffsetRange, fPsdOffsetRange);

  fBmonPsdDiffCharge = new TH2F("fBmonPsdDiffCharge", "Psd-Bmon;time diff [ns]; Charge [a.u]; ;Counts", nrOfBinsPsd,
                                -fPsdOffsetRange, fPsdOffsetRange, 7000, 0, 70000);

  fBmonPsdDiffEvo = new TH2F("fBmonPsdDiffEvo", "Psd-Bmon;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsPsd,
                             -fPsdOffsetRange, fPsdOffsetRange);

  // Bmon vs. Sts
  fBmonStsDiffEvoLong = new TH2F("fBmonStsDiffEvoLong", "Sts-Bmon;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsSts, -fStsOffsetRange, fStsOffsetRange);
  // Bmon vs. Much
  fBmonMuchDiffEvoLong = new TH2F("fBmonMuchDiffEvoLong", "Much-Bmon;TS; time diff [ns];Counts", 1800, 0, 180000,
                                  nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);
  // To vs. Trd
  fBmonTrdDiffEvoLong = new TH2F("fBmonTrdDiffEvoLong", "Trd-Bmon;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsTrd, -fTrdOffsetRange, fTrdOffsetRange);
  // To vs. Tof
  fBmonTofDiffEvoLong = new TH2F("fBmonTofDiffEvoLong", "Tof-Bmon;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsTof, -fTofOffsetRange, fTofOffsetRange);
  // To vs. Rich
  fBmonRichDiffEvoLong = new TH2F("fBmonRichDiffEvoLong", "Rich-Bmon;TS; time diff [ns];Counts", 1800, 0, 180000,
                                  nrOfBinsRich, -fRichOffsetRange, fRichOffsetRange);

  // To vs. Psd
  fBmonPsdDiffEvoLong = new TH2F("fBmonPsdDiffEvoLong", "Psd-Bmon;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsPsd, -fPsdOffsetRange, fPsdOffsetRange);

  // Bmon vs. STS for the different DPBs
  fBmonStsDpbDiff = new TH2F("fBmonStsDpbDiff", "Much-Bmon;DPB; time diff [ns];Counts", 2, -0.5, 1.5, nrOfBinsSts,
                             -fStsOffsetRange, fStsOffsetRange);

  for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb) {
    fBmonStsDpbDiffEvo[uStsDpb] =
      new TH2F(Form("fBmonStsDpbDiffEvo%02u", uStsDpb), Form("Sts-Bmon DPB %02u;TS; time diff [ns];Counts", uStsDpb),
               1800, 0, 180000, nrOfBinsSts, -fStsOffsetRange, fStsOffsetRange);
    fStsDpbCntsEvo[uStsDpb] =
      new TH1F(Form("fStsDpbCntsEvo%02u", uStsDpb), Form("Time STS DPB %02u;TS; Hit Counts", uStsDpb), 1800, 0, 180000);
  }  // for( UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb )

  // Bmon vs. Much for the different DPBs/AFCK
  fBmonMuchRocDiff = new TH2F("fBmonMuchRocDiff", "Much-Bmon;AFCK; time diff [ns];Counts", kuMaxNbMuchDpbs, -0.5,
                              kuMaxNbMuchDpbs - 0.5, nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);

  // Bmon vs. Much for the different ASICs
  fBmonMuchAsicDiff = new TH2F("fBmonMuchAsicDiff", "Much-Bmon;ASIC; time diff [ns];Counts", kuMaxNbMuchAsics, -0.5,
                               kuMaxNbMuchAsics - 0.5, nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);

  for (UInt_t uMuchAsic = 0; uMuchAsic < kuMaxNbMuchAsics; ++uMuchAsic)
    fBmonMuchAsicDiffEvo[uMuchAsic] = new TH2F(Form("fBmonMuchAsicDiffEvo%02u", uMuchAsic),
                                               Form("Much-Bmon ASIC %02u;TS; time diff [ns];Counts", uMuchAsic), 1800,
                                               0, 180000, nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);

  // Bmon vs. Bmon
  fBmonBmonDiff = new TH1F("fBmonBmonDiff", "Bmon-Bmon_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // sts vs. Sts
  fStsStsDiff = new TH1F("fStsStsDiff", "Sts-Sts_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Much vs. Much
  fMuchMuchDiff = new TH1F("fMuchMuchDiff", "Much-Much_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Trd vs. Trd
  fTrdTrdDiff = new TH1F("fTrdTrdDiff", "Trd-Trd_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Tof vs. Tof
  fTofTofDiff = new TH1F("fTofTofDiff", "Tof-Tof_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Rich vs. Rich
  fRichRichDiff = new TH1F("fRichRichDiff", "Rich-Rich_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Psd vs. Psd
  fPsdPsdDiff = new TH1F("fPsdPsdDiff", "Psd-Psd_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);

  fBmonAddress = new TH1F("fBmonAddress", "Bmon address;address;Counts", 1000000, 0, 1000000.);

  fBmonChannel = new TH1F("fBmonChannel", "Bmon channel;channel nr;Counts", 100, -0.5, 99.5);

  fSelBmonStsDiff = new TH1F("fSelBmonStsDiff", "Sts-Bmon if Bmon in coinc with TOF;time diff [ns];Counts", nrOfBinsSts,
                             -fStsOffsetRange, fStsOffsetRange);
  fSelBmonMuchDiff = new TH1F("fSelBmonMuchDiff", "Much-Bmon if Bmon in coinc with TOF;time diff [ns];Counts",
                              nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);
  fSelBmonTrdDiff = new TH1F("fSelBmonTrdDiff", "Trd-Bmon if Bmon in coinc with TOF;time diff [ns];Counts", nrOfBinsTrd,
                             -fTrdOffsetRange, fTrdOffsetRange);
  fSelBmonTofDiff = new TH1F("fSelBmonTofDiff", "Tof-Bmon if Bmon in coinc with TOF;time diff [ns];Counts", nrOfBinsTof,
                             -fTofOffsetRange, fTofOffsetRange);
  fSelBmonRichDiff = new TH1F("fSelBmonRichDiff", "Rich-Bmon if Bmon in coinc with TOF;time diff [ns];Counts",
                              nrOfBinsRich, -fRichOffsetRange, fRichOffsetRange);
  fSelBmonPsdDiff = new TH1F("fSelBmonPsdDiff", "Psd-Bmon if Bmon in coinc with TOF;time diff [ns];Counts", nrOfBinsPsd,
                             -fPsdOffsetRange, fPsdOffsetRange);

  /// Cleanup array of log bins
  //  delete dBinsLog;

  /// Register the histos in the HTTP server
  FairRunOnline* run = FairRunOnline::Instance();
  if (run) {
    THttpServer* server = run->GetHttpServer();
    if (nullptr != server) {
      server->Register("/CheckTiming", fBmonStsDiff);
      server->Register("/CheckTiming", fBmonMuchDiff);
      server->Register("/CheckTiming", fBmonTrdDiff);
      server->Register("/CheckTiming", fBmonTofDiff);
      server->Register("/CheckTiming", fBmonRichDiff);
      server->Register("/CheckTiming", fBmonPsdDiff);
      server->Register("/CheckTiming", fBmonStsDiffCharge);
      server->Register("/CheckTiming", fBmonMuchDiffCharge);
      server->Register("/CheckTiming", fBmonTrdDiffCharge);
      server->Register("/CheckTiming", fBmonTofDiffCharge);
      server->Register("/CheckTiming", fBmonRichDiffCharge);
      server->Register("/CheckTiming", fBmonPsdDiffCharge);
      server->Register("/CheckTiming", fBmonStsDiffEvo);
      server->Register("/CheckTiming", fBmonMuchDiffEvo);
      server->Register("/CheckTiming", fBmonTrdDiffEvo);
      server->Register("/CheckTiming", fBmonTofDiffEvo);
      server->Register("/CheckTiming", fBmonRichDiffEvo);
      server->Register("/CheckTiming", fBmonPsdDiffEvo);
      server->Register("/CheckTiming", fBmonStsDiffEvoLong);
      server->Register("/CheckTiming", fBmonMuchDiffEvoLong);
      server->Register("/CheckTiming", fBmonTrdDiffEvoLong);
      server->Register("/CheckTiming", fBmonTofDiffEvoLong);
      server->Register("/CheckTiming", fBmonRichDiffEvoLong);
      server->Register("/CheckTiming", fBmonPsdDiffEvoLong);
      server->Register("/CheckTiming", fBmonBmonDiff);
      server->Register("/CheckTiming", fStsStsDiff);
      server->Register("/CheckTiming", fMuchMuchDiff);
      server->Register("/CheckTiming", fTrdTrdDiff);
      server->Register("/CheckTiming", fTofTofDiff);
      server->Register("/CheckTiming", fRichRichDiff);
      server->Register("/CheckTiming", fPsdPsdDiff);

      server->Register("/CheckTiming", fBmonStsDpbDiff);
      for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb)
        server->Register("/CheckTiming/sts", fBmonStsDpbDiffEvo[uStsDpb]);

      server->Register("/CheckTiming", fBmonMuchRocDiff);
      server->Register("/CheckTiming", fBmonMuchAsicDiff);
      for (UInt_t uMuchAsic = 0; uMuchAsic < kuMaxNbMuchAsics; ++uMuchAsic)
        server->Register("/CheckTiming/much", fBmonMuchAsicDiffEvo[uMuchAsic]);

      server->Register("/CheckTiming", fSelBmonStsDiff);
      server->Register("/CheckTiming", fSelBmonMuchDiff);
      server->Register("/CheckTiming", fSelBmonTrdDiff);
      server->Register("/CheckTiming", fSelBmonTofDiff);
      server->Register("/CheckTiming", fSelBmonRichDiff);
      server->Register("/CheckTiming", fSelBmonPsdDiff);
    }
  }
}
// ---- ReInit  -------------------------------------------------------
InitStatus CbmCheckTiming::ReInit() { return kSUCCESS; }

// ---- Exec ----------------------------------------------------------
void CbmCheckTiming::Exec(Option_t* /*option*/)
{
  LOG(debug) << "executing TS " << fNrTs;

  if (fCheckTimeOrdering) CheckTimeOrder();
  if (fCheckInterSystemOffset) CheckInterSystemOffset();
  if ((0 < fNrTs) && (fNrTsForFit > 0) && (0 == fNrTs % fNrTsForFit)) {
    LOG(info) << "Fitting peaks for number of TS = " << fNrTs;
    FitPeaks();
  }
  if (0 < fNrTs && 0 == fNrTs % 2000) WriteHistos();
  fNrTs++;
}

void CbmCheckTiming::CheckInterSystemOffset()
{
  LOG(debug) << "Begin";
  Int_t nrBmonDigis = 0;
  if (fBmonDigiVec) nrBmonDigis = fBmonDigiVec->size();
  else if (fBmonDigiArr)
    nrBmonDigis = fBmonDigiArr->GetEntriesFast();
  LOG(debug) << "BmonDigis: " << nrBmonDigis;

  Int_t nrStsDigis {0};
  if (fDigiMan->IsPresent(ECbmModuleId::kSts)) {
    nrStsDigis = fDigiMan->GetNofDigis(ECbmModuleId::kSts);
    LOG(debug) << "StsDigis: " << nrStsDigis;
  }

  Int_t nrMuchDigis {0};
  if (fDigiMan->IsPresent(ECbmModuleId::kMuch)) {
    nrMuchDigis = fDigiMan->GetNofDigis(ECbmModuleId::kMuch);
    LOG(debug) << "MuchDigis: " << nrMuchDigis;
  }

  Int_t nrTrdDigis {0};
  if (fDigiMan->IsPresent(ECbmModuleId::kTrd)) {
    nrTrdDigis = fDigiMan->GetNofDigis(ECbmModuleId::kTrd);
    LOG(debug) << "TrdDigis: " << nrTrdDigis;
  }

  Int_t nrTofDigis {0};
  if (fDigiMan->IsPresent(ECbmModuleId::kTof)) {
    nrTofDigis = fDigiMan->GetNofDigis(ECbmModuleId::kTof);
    LOG(debug) << "TofDigis: " << nrTofDigis;
  }

  Int_t nrRichDigis {0};
  if (fDigiMan->IsPresent(ECbmModuleId::kRich)) {
    nrRichDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
    LOG(debug) << "RichDigis: " << nrRichDigis;
  }

  Int_t nrPsdDigis {0};
  if (fDigiMan->IsPresent(ECbmModuleId::kPsd)) {
    nrPsdDigis = fDigiMan->GetNofDigis(ECbmModuleId::kPsd);
    LOG(debug) << "PsdDigis: " << nrPsdDigis;
  }

  //  if (nrBmonDigis < 100000) {
  if (nrBmonDigis < 1000000) {
    /// Re-initialize array references
    fPrevBmonFirstDigiSts  = 0.;
    fPrevBmonFirstDigiMuch = 0.;
    fPrevBmonFirstDigiTrd  = 0.;
    fPrevBmonFirstDigiTof  = 0.;
    fPrevBmonFirstDigiRich = 0.;
    fPrevBmonFirstDigiPsd  = 0.;

    fvdTimeSelectedBmon.clear();

    for (Int_t iBmon = 0; iBmon < nrBmonDigis; ++iBmon) {

      if (iBmon % 1000 == 0) LOG(debug) << "Executing entry " << iBmon;

      const CbmTofDigi* BmonDigi = nullptr;
      if (fBmonDigiVec) BmonDigi = &(fBmonDigiVec->at(iBmon));
      else if (fBmonDigiArr)
        BmonDigi = dynamic_cast<const CbmTofDigi*>(fBmonDigiArr->At(iBmon));
      assert(BmonDigi);

      /// Skip pulser events.
      if (fuMinTotPulserBmon < BmonDigi->GetCharge() && BmonDigi->GetCharge() < fuMaxTotPulserBmon) continue;

      Double_t T0Time   = BmonDigi->GetTime();
      Int_t BmonAddress = BmonDigi->GetAddress();
      fBmonAddress->Fill(BmonAddress);

      fBmonChannel->Fill(BmonDigi->GetChannel());

      if (nrStsDigis > 0 && nrStsDigis < 1000000)
        fPrevBmonFirstDigiSts = FillSystemOffsetHistos<CbmStsDigi>(
          fBmonStsDiff, fBmonStsDiffCharge, fBmonStsDiffEvo, fBmonStsDiffEvoLong, fBmonStsDpbDiff, T0Time,
          fStsOffsetRange, fPrevBmonFirstDigiSts, kTRUE, kFALSE, kFALSE, kFALSE);
      /// Templating to CbmMuchBeamTimeDigi fails with a bad any_cast due to hardcoded Digi type in
      /// CbmDigiManager!
      if (nrMuchDigis > 0 && nrMuchDigis < 1000000)
        fPrevBmonFirstDigiMuch = FillSystemOffsetHistos<CbmMuchBeamTimeDigi>(
          fBmonMuchDiff, fBmonMuchDiffCharge, fBmonMuchDiffEvo, fBmonMuchDiffEvoLong, fBmonMuchRocDiff, T0Time,
          fMuchOffsetRange, fPrevBmonFirstDigiMuch, kFALSE, kTRUE, kFALSE, kFALSE);
      if (nrTrdDigis > 0 && nrTrdDigis < 1000000)
        fPrevBmonFirstDigiTrd = FillSystemOffsetHistos<CbmTrdDigi>(
          fBmonTrdDiff, fBmonTrdDiffCharge, fBmonTrdDiffEvo, fBmonTrdDiffEvoLong, nullptr, T0Time, fTrdOffsetRange,
          fPrevBmonFirstDigiTrd, kFALSE, kFALSE, kFALSE, kFALSE);
      fuNbTofDigiInSync = 0;
      if (nrTofDigis > 0 && nrTofDigis < 1000000)
        fPrevBmonFirstDigiTof = FillSystemOffsetHistos<CbmTofDigi>(
          fBmonTofDiff, fBmonTofDiffCharge, fBmonTofDiffEvo, fBmonTofDiffEvoLong, nullptr, T0Time, fTofOffsetRange,
          fPrevBmonFirstDigiTof, kFALSE, kFALSE, kTRUE, kFALSE);
      if (nrRichDigis > 0 && nrRichDigis < 1000000)
        fPrevBmonFirstDigiRich = FillSystemOffsetHistos<CbmRichDigi>(
          fBmonRichDiff, fBmonRichDiffCharge, fBmonRichDiffEvo, fBmonRichDiffEvoLong, nullptr, T0Time, fRichOffsetRange,
          fPrevBmonFirstDigiRich, kFALSE, kFALSE, kFALSE, kFALSE);
      if (nrPsdDigis > 0 && nrPsdDigis < 1000000)
        fPrevBmonFirstDigiPsd = FillSystemOffsetHistos<CbmPsdDigi>(
          fBmonPsdDiff, fBmonPsdDiffCharge, fBmonPsdDiffEvo, fBmonPsdDiffEvoLong, nullptr, T0Time, fPsdOffsetRange,
          fPrevBmonFirstDigiPsd, kFALSE, kFALSE, kFALSE, kTRUE);

      if (fuNbTofDigisSel <= fuNbTofDigiInSync) fvdTimeSelectedBmon.push_back(T0Time);
    }

    /// Re-initialize array references
    fPrevBmonFirstDigiSts  = 0.;
    fPrevBmonFirstDigiMuch = 0.;
    fPrevBmonFirstDigiTrd  = 0.;
    fPrevBmonFirstDigiTof  = 0.;
    fPrevBmonFirstDigiRich = 0.;
    fPrevBmonFirstDigiPsd  = 0.;
    for (UInt_t uIdxSelBmon = 0; uIdxSelBmon < fvdTimeSelectedBmon.size(); ++uIdxSelBmon) {
      if (nrStsDigis > 0 && nrStsDigis < 1000000)
        fPrevBmonFirstDigiSts =
          FillHistosSelBmon<CbmStsDigi>(fSelBmonStsDiff, fvdTimeSelectedBmon[uIdxSelBmon], fStsOffsetRange,
                                        fPrevBmonFirstDigiSts, kTRUE, kFALSE, kFALSE, kFALSE);
      /// Templating to CbmMuchBeamTimeDigi fails with a bad any_cast due to hardcoded Digi type in
      /// CbmDigiManager!
      if (nrMuchDigis > 0 && nrMuchDigis < 1000000)
        fPrevBmonFirstDigiMuch =
          FillHistosSelBmon<CbmMuchBeamTimeDigi>(fSelBmonMuchDiff, fvdTimeSelectedBmon[uIdxSelBmon], fMuchOffsetRange,
                                                 fPrevBmonFirstDigiMuch, kFALSE, kTRUE, kFALSE, kFALSE);
      if (nrTrdDigis > 0 && nrTrdDigis < 1000000)
        fPrevBmonFirstDigiTrd =
          FillHistosSelBmon<CbmTrdDigi>(fSelBmonTrdDiff, fvdTimeSelectedBmon[uIdxSelBmon], fTrdOffsetRange,
                                        fPrevBmonFirstDigiTrd, kFALSE, kFALSE, kFALSE, kFALSE);
      if (nrTofDigis > 0 && nrTofDigis < 1000000)
        fPrevBmonFirstDigiTof =
          FillHistosSelBmon<CbmTofDigi>(fSelBmonTofDiff, fvdTimeSelectedBmon[uIdxSelBmon], fTofOffsetRange,
                                        fPrevBmonFirstDigiTof, kFALSE, kFALSE, kTRUE, kFALSE);
      if (nrRichDigis > 0 && nrRichDigis < 1000000)
        fPrevBmonFirstDigiRich =
          FillHistosSelBmon<CbmRichDigi>(fSelBmonRichDiff, fvdTimeSelectedBmon[uIdxSelBmon], fRichOffsetRange,
                                         fPrevBmonFirstDigiRich, kFALSE, kFALSE, kFALSE, kFALSE);
      if (nrPsdDigis > 0 && nrPsdDigis < 1000000)
        fPrevBmonFirstDigiPsd =
          FillHistosSelBmon<CbmPsdDigi>(fSelBmonPsdDiff, fvdTimeSelectedBmon[uIdxSelBmon], fPsdOffsetRange,
                                        fPrevBmonFirstDigiPsd, kFALSE, kFALSE, kFALSE, kTRUE);
    }  //  for( UInt_t uIdxSelBmon = 0; uIdxSelBmon < fvdTimeSelectedBmon.size(); ++uIdxSelBmon )
  }
}

template<class Digi>
Int_t CbmCheckTiming::FillSystemOffsetHistos(TH1* histo, TH2* histoCharge, TH2* histoEvo, TH2* histoEvoLong,
                                             TH2* histoAFCK, const Double_t T0Time, const Int_t offsetRange,
                                             Int_t iStartDigi, Bool_t bSts, Bool_t bMuch, Bool_t bTof, Bool_t /*bPsd*/)
{
  Int_t nrDigis         = fDigiMan->GetNofDigis(Digi::GetSystem());
  Int_t iFirstDigiInWin = iStartDigi;

  for (Int_t i = iStartDigi; i < nrDigis; ++i) {

    const Digi* digi = fDigiMan->Get<Digi>(i);

    /// Ignore pulser events in TOF
    if (kTRUE == bTof)
      if (fuMinTotPulserTof < digi->GetCharge() && digi->GetCharge() < fuMaxTotPulserTof) continue;
    /*
    /// Keep only hits with higher probability to be signal
    if( kTRUE == bMuch )
      if( digi->GetCharge() < 2 )
        continue;

    /// Ignore even STS channels as they have ~10-100 x more noise
    if( kTRUE == bSts )
      if( ( digi->GetCharge() < 7 || 31 == digi->GetCharge() ) ||
          ( 0 == ( digi->GetChannel() % 2 ) ) )
        continue;

    /// Take selected PSD channel
    if( kTRUE == bPsd )
      if( digi->GetAddress() != (0<<10)+8 )
         continue;
*/

    //    Double_t diffTime = T0Time - digi->GetTime();
    Double_t diffTime = digi->GetTime() - T0Time;  // Use Bmon as reference Time

    if (diffTime < -offsetRange) {
      ++iFirstDigiInWin;                // Update Index of first digi in Win to next digi
      continue;                         // not yet in interesting range
    }                                   // if (diffTime > offsetRange)
    if (diffTime > offsetRange) break;  // already past interesting range
    histo->Fill(diffTime);
    histoCharge->Fill(diffTime, digi->GetCharge());
    histoEvo->Fill(fNrTs, diffTime);
    histoEvoLong->Fill(fNrTs, diffTime);

    /// STS DPB mapping: ladder 1 is in DPB 1
    if (bSts && histoAFCK) {
      UInt_t uDPB = (0 < (digi->GetAddress() & 0x00000400));
      histoAFCK->Fill(uDPB, diffTime);
      if (uDPB < kuMaxNbStsDpbs) fBmonStsDpbDiffEvo[uDPB]->Fill(fNrTs, diffTime);
    }  // if (bSts && histoAFCK)

    /// MUCH DPB mapping
    if (bMuch && histoAFCK) {
      const CbmMuchBeamTimeDigi* muchDigi = nullptr;
      try {
        muchDigi = boost::any_cast<const CbmMuchBeamTimeDigi*>(digi);
      }
      catch (...) {
        LOG(fatal) << "Failed boost any_cast in CbmCheckTiming::FillSystemOffsetHistos "
                      "for a digi of type "
                   << Digi::GetClassName();
      }  // try/catch
      assert(muchDigi);
      UInt_t afck = muchDigi->GetRocId();
      UInt_t asic = muchDigi->GetNxId();
      histoAFCK->Fill(afck, diffTime);
      fBmonMuchAsicDiff->Fill(asic, diffTime);
      if (asic < kuMaxNbMuchAsics) fBmonMuchAsicDiffEvo[asic]->Fill(fNrTs, diffTime);
    }  // if (bMuch && histoAFCK)

    /// TOF coincidence counting
    if (kTRUE == bTof && -200 < diffTime && diffTime < 200) fuNbTofDigiInSync++;
  }

  return iFirstDigiInWin;
}

template<class Digi>
Int_t CbmCheckTiming::FillHistosSelBmon(TH1* histo, const Double_t T0Time, const Int_t offsetRange, Int_t iStartDigi,
                                        Bool_t /*bSts*/, Bool_t /*bMuch*/, Bool_t bTof, Bool_t /*bPsd*/)
{
  Int_t nrDigis         = fDigiMan->GetNofDigis(Digi::GetSystem());
  Int_t iFirstDigiInWin = iStartDigi;

  for (Int_t i = iStartDigi; i < nrDigis; ++i) {

    const Digi* digi = fDigiMan->Get<Digi>(i);

    /// Ignore pulser events in TOF
    if (kTRUE == bTof)
      if (fuMinTotPulserTof < digi->GetCharge() && digi->GetCharge() < fuMaxTotPulserTof) continue;
    /*
    /// Keep only hits with higher probability to be signal
    if( kTRUE == bMuch )
      if( digi->GetCharge() < 2 )
        continue;

    /// Ignore even STS channels as they have ~10-100 x more noise
    if( kTRUE == bSts )
      if( ( digi->GetCharge() < 7 || 31 == digi->GetCharge() ) ||
          ( 0 == ( digi->GetChannel() % 2 ) ) )
        continue;

    /// Take selected PSD channel
    if( kTRUE == bPsd )
      if( digi->GetAddress() != (0<<10)+8 )
         continue;
*/

    //    Double_t diffTime = T0Time - digi->GetTime();
    Double_t diffTime = digi->GetTime() - T0Time;  // Use Bmon as reference Time

    if (diffTime < -offsetRange) {
      ++iFirstDigiInWin;                // Update Index of first digi in Win to next digi
      continue;                         // not yet in interesting range
    }                                   // if (diffTime > offsetRange)
    if (diffTime > offsetRange) break;  // already past interesting range
    histo->Fill(diffTime);
  }

  return iFirstDigiInWin;
}

void CbmCheckTiming::CheckTimeOrder()
{
  if (fBmonDigiVec || fBmonDigiArr) {
    Int_t nrBmonDigis = 0;
    if (fBmonDigiVec) nrBmonDigis = fBmonDigiVec->size();
    else if (fBmonDigiArr)
      nrBmonDigis = fBmonDigiArr->GetEntriesFast();
    fNrOfBmonDigis += nrBmonDigis;
    fNrOfBmonErrors += CheckIfSortedBmon(fBmonBmonDiff, fPrevTimeBmon, "Bmon");
  }
  if (fDigiMan->IsPresent(ECbmModuleId::kSts)) {
    Int_t nrStsDigis = fDigiMan->GetNofDigis(ECbmModuleId::kSts);
    fNrOfStsDigis += nrStsDigis;
    fNrOfStsErrors += CheckIfSorted<CbmStsDigi>(fStsStsDiff, fPrevTimeSts, "Sts");

    for (Int_t i = 0; i < nrStsDigis; ++i) {
      const CbmStsDigi* Digi = fDigiMan->Get<CbmStsDigi>(i);
      UInt_t uDPB            = (0 < (Digi->GetAddress() & 0x00000400));
      if (uDPB < kuMaxNbStsDpbs) fStsDpbCntsEvo[uDPB]->Fill(fNrTs);
    }
  }
  if (fDigiMan->IsPresent(ECbmModuleId::kMuch)) {
    Int_t nrMuchDigis = fDigiMan->GetNofDigis(ECbmModuleId::kMuch);
    fNrOfMuchDigis += nrMuchDigis;
    fNrOfMuchErrors += CheckIfSorted<CbmMuchBeamTimeDigi>(fMuchMuchDiff, fPrevTimeMuch, "Much");
  }
  if (fDigiMan->IsPresent(ECbmModuleId::kTrd)) {
    Int_t nrTrdDigis = fDigiMan->GetNofDigis(ECbmModuleId::kTrd);
    fNrOfTrdDigis += nrTrdDigis;
    fNrOfTrdErrors += CheckIfSorted<CbmTrdDigi>(fTrdTrdDiff, fPrevTimeTrd, "Trd");
  }
  if (fDigiMan->IsPresent(ECbmModuleId::kTof)) {
    Int_t nrTofDigis = fDigiMan->GetNofDigis(ECbmModuleId::kTof);
    fNrOfTofDigis += nrTofDigis;
    fNrOfTofErrors += CheckIfSorted<CbmTofDigi>(fTofTofDiff, fPrevTimeTof, "Tof");
  }
  if (fDigiMan->IsPresent(ECbmModuleId::kRich)) {
    Int_t nrRichDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
    fNrOfRichDigis += nrRichDigis;
    fNrOfRichErrors += CheckIfSorted<CbmRichDigi>(fRichRichDiff, fPrevTimeRich, "Rich");
  }
  if (fDigiMan->IsPresent(ECbmModuleId::kPsd)) {
    Int_t nrPsdDigis = fDigiMan->GetNofDigis(ECbmModuleId::kPsd);
    fNrOfPsdDigis += nrPsdDigis;
    fNrOfPsdErrors += CheckIfSorted<CbmPsdDigi>(fPsdPsdDiff, fPrevTimePsd, "Psd");
  }
}

template<class Digi>
Int_t CbmCheckTiming::CheckIfSorted(TH1* histo, Double_t& prevTime, TString detector)
{
  Int_t nrOfErrors = 0;
  Int_t nrDigis    = fDigiMan->GetNofDigis(Digi::GetSystem());

  for (Int_t i = 0; i < nrDigis; ++i) {

    const Digi* digi = fDigiMan->Get<Digi>(i);

    Double_t diffTime = digi->GetTime() - prevTime;
    histo->Fill(diffTime);

    if (diffTime < 0.) {
      LOG(info) << fixed << setprecision(15) << diffTime << "ns";
      LOG(info) << "Previous " << detector << " digi (" << fixed << setprecision(15) << prevTime * 1.e-9
                << ") has a larger time than the current one (" << digi->GetTime() * 1.e-9 << ") for digi " << i
                << " of ts " << fNrTs;
      nrOfErrors++;
    }

    prevTime = digi->GetTime();
  }

  return nrOfErrors;
}

Int_t CbmCheckTiming::CheckIfSortedBmon(TH1* histo, Double_t& prevTime, TString detector)
{

  // Extra implementation since Bmon is not included in CbmDigiManager
  Int_t nrOfErrors = 0;

  Int_t nrDigis = 0;
  if (fBmonDigiVec) nrDigis = fBmonDigiVec->size();
  else if (fBmonDigiArr)
    nrDigis = fBmonDigiArr->GetEntriesFast();

  for (Int_t i = 0; i < nrDigis; ++i) {

    const CbmTofDigi* digi = nullptr;
    if (fBmonDigiVec) digi = &(fBmonDigiVec->at(i));
    else if (fBmonDigiArr)
      digi = dynamic_cast<const CbmTofDigi*>(fBmonDigiArr->At(i));
    if (!digi) {
      nrOfErrors++;
      continue;
    }

    Double_t diffTime = digi->GetTime() - prevTime;
    histo->Fill(diffTime);

    if (diffTime < 0.) {
      LOG(info) << fixed << setprecision(15) << diffTime << "ns";
      LOG(info) << "Previous " << detector << " digi (" << fixed << setprecision(15) << prevTime * 1.e-9
                << ") has a larger time than the current one (" << digi->GetTime() * 1.e-9 << ") for digi " << i
                << " of ts " << fNrTs;
      nrOfErrors++;
    }

    prevTime = digi->GetTime();
  }

  return nrOfErrors;
}


// ---- Finish --------------------------------------------------------
void CbmCheckTiming::Finish()
{
  if (fCheckTimeOrdering) {
    LOG(info) << "Total number of Bmon out of order digis: " << fNrOfBmonErrors;
    LOG(info) << "Total number of Bmon digis: " << fNrOfBmonDigis;
    LOG(info) << "Total number of Sts out of order digis: " << fNrOfStsErrors;
    LOG(info) << "Total number of Sts digis: " << fNrOfStsDigis;
    LOG(info) << "Total number of Much out of order digis: " << fNrOfMuchErrors;
    LOG(info) << "Total number of Much digis: " << fNrOfMuchDigis;
    LOG(info) << "Total number of Trd out of order digis: " << fNrOfTrdErrors;
    LOG(info) << "Total number of Trd digis: " << fNrOfTrdDigis;
    LOG(info) << "Total number of Tof out of order digis: " << fNrOfTofErrors;
    LOG(info) << "Total number of Tof digis: " << fNrOfTofDigis;
    LOG(info) << "Total number of Rich out of order digis: " << fNrOfRichErrors;
    LOG(info) << "Total number of Rich digis: " << fNrOfRichDigis;
    LOG(info) << "Total number of Psd out of order digis: " << fNrOfPsdErrors;
    LOG(info) << "Total number of Psd digis: " << fNrOfPsdDigis;
  }
  FitPeaks();
  WriteHistos();

  LOG(info) << Form("Checked %6d Timeslices", fNrTs);
}


void CbmCheckTiming::FitPeaks()
{
  // Peak positions for differences wrt Bmon
  trd_peak_pos  = fBmonTrdDiff->GetMaximumBin() * fBmonTrdDiff->GetBinWidth(1) + fBmonTrdDiff->GetXaxis()->GetXmin();
  sts_peak_pos  = fBmonStsDiff->GetMaximumBin() * fBmonStsDiff->GetBinWidth(1) + fBmonStsDiff->GetXaxis()->GetXmin();
  much_peak_pos = fBmonMuchDiff->GetMaximumBin() * fBmonMuchDiff->GetBinWidth(1) + fBmonMuchDiff->GetXaxis()->GetXmin();
  tof_peak_pos  = fBmonTofDiff->GetMaximumBin() * fBmonTofDiff->GetBinWidth(1) + fBmonTofDiff->GetXaxis()->GetXmin();
  rich_peak_pos = fBmonRichDiff->GetMaximumBin() * fBmonRichDiff->GetBinWidth(1) + fBmonRichDiff->GetXaxis()->GetXmin();
  psd_peak_pos  = fBmonPsdDiff->GetMaximumBin() * fBmonPsdDiff->GetBinWidth(1) + fBmonPsdDiff->GetXaxis()->GetXmin();

  // Peak positions for differences wrt Bmon if coincidence with TOF
  trd_coin_peak_pos =
    fSelBmonTrdDiff->GetMaximumBin() * fSelBmonTrdDiff->GetBinWidth(1) + fSelBmonTrdDiff->GetXaxis()->GetXmin();
  sts_coin_peak_pos =
    fSelBmonStsDiff->GetMaximumBin() * fSelBmonStsDiff->GetBinWidth(1) + fSelBmonStsDiff->GetXaxis()->GetXmin();
  much_coin_peak_pos =
    fSelBmonMuchDiff->GetMaximumBin() * fSelBmonMuchDiff->GetBinWidth(1) + fSelBmonMuchDiff->GetXaxis()->GetXmin();
  //tof_coin_peak_pos = fBmonTofDiff->GetMaximumBin()*fBmonTofDiff->GetBinWidth(1)+fBmonTofDiff->GetXaxis()->GetXmin();
  rich_coin_peak_pos =
    fSelBmonRichDiff->GetMaximumBin() * fSelBmonRichDiff->GetBinWidth(1) + fSelBmonRichDiff->GetXaxis()->GetXmin();
  psd_coin_peak_pos =
    fSelBmonPsdDiff->GetMaximumBin() * fSelBmonPsdDiff->GetBinWidth(1) + fSelBmonPsdDiff->GetXaxis()->GetXmin();

  LOG(info) << "STS entries = " << fBmonStsDiff->GetEntries();
  LOG(info) << "STS-Bmon entries if Bmon in coincidence with TOF = " << fSelBmonStsDiff->GetEntries();
  LOG(info) << "MUCH entries = " << fBmonMuchDiff->GetEntries();
  LOG(info) << "MUCH-Bmon entries if Bmon in coincidence with TOF = " << fSelBmonMuchDiff->GetEntries();
  LOG(info) << "TRD entries = " << fBmonTrdDiff->GetEntries();
  LOG(info) << "TRD-Bmon entries if Bmon in coincidence with TOF = " << fSelBmonTrdDiff->GetEntries();
  LOG(info) << "TOF entries = " << fBmonTofDiff->GetEntries();
  LOG(info) << "RICH entries = " << fBmonRichDiff->GetEntries();
  LOG(info) << "RICH-Bmon entries if Bmon in coincidence with TOF = " << fSelBmonRichDiff->GetEntries();
  LOG(info) << "PSD entries = " << fBmonPsdDiff->GetEntries();
  LOG(info) << "PSD-Bmon entries if Bmon in coincidence with TOF = " << fSelBmonPsdDiff->GetEntries();
  LOG(info) << "STS peak position [ns] = " << sts_peak_pos;
  LOG(info) << "STS peak position [ns] if Bmon in coincidence with TOF = " << sts_coin_peak_pos;
  LOG(info) << "MUCH peak position [ns] = " << much_peak_pos;
  LOG(info) << "MUCH peak position [ns] if Bmon in coincidence with TOF = " << much_coin_peak_pos;
  LOG(info) << "TRD peak position [ns] = " << trd_peak_pos;
  LOG(info) << "TRD peak position [ns] if Bmon in coincidence with TOF = " << trd_coin_peak_pos;
  LOG(info) << "TOF peak position [ns] = " << tof_peak_pos;
  LOG(info) << "RICH peak position [ns] = " << rich_peak_pos;
  LOG(info) << "RICH peak position [ns] if Bmon in coincidence with TOF = " << rich_coin_peak_pos;
  LOG(info) << "PSD peak position [ns] = " << psd_peak_pos;
  LOG(info) << "PSD peak position [ns] if Bmon in coincidence with TOF = " << psd_coin_peak_pos;

  //Average height of bins...
  trd_average  = fBmonTrdDiff->Integral() / (fBmonTrdDiff->GetNbinsX());
  sts_average  = fBmonStsDiff->Integral() / (fBmonStsDiff->GetNbinsX());
  much_average = fBmonMuchDiff->Integral() / (fBmonMuchDiff->GetNbinsX());
  tof_average  = fBmonTofDiff->Integral() / (fBmonTofDiff->GetNbinsX());
  rich_average = fBmonRichDiff->Integral() / (fBmonRichDiff->GetNbinsX());
  psd_average  = fBmonPsdDiff->Integral() / (fBmonPsdDiff->GetNbinsX());

  //TRD
  if (trd_average > 0) {
    TF1* gs_trd =
      new TF1("gs_trd", "gaus(0)+pol0(3)", trd_peak_pos - 2 * fTrdPeakWidthNs, trd_peak_pos + 2 * fTrdPeakWidthNs);
    gs_trd->SetParameters(0.7 * trd_average, trd_peak_pos, fTrdPeakWidthNs, trd_average);
    fBmonTrdDiff->Fit("gs_trd", "R");
    TF1* fitresult_trd = fBmonTrdDiff->GetFunction("gs_trd");
    LOG(info) << "TRD parameters from Gauss fit = " << fitresult_trd->GetParameter(0) << ",  "
              << fitresult_trd->GetParameter(1) << ",  " << fitresult_trd->GetParameter(2);
    LOG(info) << "TRD signal/background (p0/p3) = "
              << (fitresult_trd->GetParameter(0)) / (fitresult_trd->GetParameter(3));
  }

  //STS
  if (sts_average > 0) {
    TF1* gs_sts =
      new TF1("gs_sts", "gaus(0)+pol0(3)", sts_peak_pos - 2 * fStsPeakWidthNs, sts_peak_pos + 2 * fStsPeakWidthNs);
    gs_sts->SetParameters(sts_average, sts_peak_pos, fStsPeakWidthNs, sts_average);
    fBmonStsDiff->Fit("gs_sts", "R");
    TF1* fitresult_sts = fBmonStsDiff->GetFunction("gs_sts");
    LOG(info) << "STS parameters from Gauss fit = " << fitresult_sts->GetParameter(0) << ",  "
              << fitresult_sts->GetParameter(1) << ",  " << fitresult_sts->GetParameter(2);
    LOG(info) << "STS signal/background (p0/p3) = "
              << (fitresult_sts->GetParameter(0)) / (fitresult_sts->GetParameter(3));
  }

  //MUCH
  if (much_average > 0) {
    TF1* gs_much =
      new TF1("gs_much", "gaus(0)+pol0(3)", much_peak_pos - 2 * fMuchPeakWidthNs, much_peak_pos + 2 * fMuchPeakWidthNs);
    gs_much->SetParameters(much_average, much_peak_pos, fMuchPeakWidthNs, much_average);
    fBmonMuchDiff->Fit("gs_much", "R");
    TF1* fitresult_much = fBmonMuchDiff->GetFunction("gs_much");
    LOG(info) << "MUCH parameters from Gauss fit = " << fitresult_much->GetParameter(0) << ",  "
              << fitresult_much->GetParameter(1) << ",  " << fitresult_much->GetParameter(2);
    LOG(info) << "MUCH signal/background (p0/p3) = "
              << (fitresult_much->GetParameter(0)) / (fitresult_much->GetParameter(3));
  }

  //TOF
  if (tof_average > 0) {
    TF1* gs_tof =
      new TF1("gs_tof", "gaus(0)+pol0(3)", tof_peak_pos - 2 * fTofPeakWidthNs, tof_peak_pos + 2 * fTofPeakWidthNs);
    gs_tof->SetParameters(tof_average, tof_peak_pos, fTofPeakWidthNs, tof_average);
    fBmonTofDiff->Fit("gs_tof", "R");
    TF1* fitresult_tof = fBmonTofDiff->GetFunction("gs_tof");
    LOG(info) << "TOF parameters from Gauss fit = " << fitresult_tof->GetParameter(0) << ",  "
              << fitresult_tof->GetParameter(1) << ",  " << fitresult_tof->GetParameter(2);
    LOG(info) << "TOF signal/background (p0/p3) = "
              << (fitresult_tof->GetParameter(0)) / (fitresult_tof->GetParameter(3));
  }

  //RICH
  if (rich_average > 0) {
    TF1* gs_rich =
      new TF1("gs_rich", "gaus(0)+pol0(3)", rich_peak_pos - 2 * fRichPeakWidthNs, rich_peak_pos + 2 * fRichPeakWidthNs);
    gs_rich->SetParameters(0.5 * rich_average, rich_peak_pos, fRichPeakWidthNs, rich_average);
    fBmonRichDiff->Fit("gs_rich", "R");
    TF1* fitresult_rich = fBmonRichDiff->GetFunction("gs_rich");
    LOG(info) << "RICH parameters from Gauss fit = " << fitresult_rich->GetParameter(0) << ",  "
              << fitresult_rich->GetParameter(1) << ",  " << fitresult_rich->GetParameter(2);
    LOG(info) << "RICH signal/background (p0/p3) = "
              << (fitresult_rich->GetParameter(0)) / (fitresult_rich->GetParameter(3));
  }

  //PSD
  if (psd_average > 0) {
    TF1* gs_psd =
      new TF1("gs_psd", "gaus(0)+pol0(3)", psd_peak_pos - 2 * fPsdPeakWidthNs, psd_peak_pos + 2 * fPsdPeakWidthNs);
    gs_psd->SetParameters(psd_average, psd_peak_pos, fPsdPeakWidthNs, psd_average);
    fBmonPsdDiff->Fit("gs_psd", "R");
    TF1* fitresult_psd = fBmonPsdDiff->GetFunction("gs_psd");
    LOG(info) << "PSD parameters from Gauss fit = " << fitresult_psd->GetParameter(0) << ",  "
              << fitresult_psd->GetParameter(1) << ",  " << fitresult_psd->GetParameter(2);
    LOG(info) << "PSD signal/background (p0/p3) = "
              << (fitresult_psd->GetParameter(0)) / (fitresult_psd->GetParameter(3));
  }
}

void CbmCheckTiming::WriteHistos()
{
  TFile* old     = gFile;
  TFile* outfile = TFile::Open(fOutFileName, "RECREATE");

  fBmonStsDiff->Write();
  fBmonMuchDiff->Write();
  fBmonTrdDiff->Write();
  fBmonTofDiff->Write();
  fBmonRichDiff->Write();
  fBmonPsdDiff->Write();

  fBmonStsDiffCharge->Write();
  fBmonMuchDiffCharge->Write();
  fBmonTrdDiffCharge->Write();
  fBmonTofDiffCharge->Write();
  fBmonRichDiffCharge->Write();
  fBmonPsdDiffCharge->Write();

  fBmonStsDiffEvo->Write();
  fBmonMuchDiffEvo->Write();
  fBmonTrdDiffEvo->Write();
  fBmonTofDiffEvo->Write();
  fBmonRichDiffEvo->Write();
  fBmonPsdDiffEvo->Write();

  fBmonStsDiffEvoLong->Write();
  fBmonMuchDiffEvoLong->Write();
  fBmonTrdDiffEvoLong->Write();
  fBmonTofDiffEvoLong->Write();
  fBmonRichDiffEvoLong->Write();
  fBmonPsdDiffEvoLong->Write();

  fBmonBmonDiff->Write();
  fStsStsDiff->Write();
  fMuchMuchDiff->Write();
  fTrdTrdDiff->Write();
  fTofTofDiff->Write();
  fRichRichDiff->Write();
  fPsdPsdDiff->Write();

  fBmonAddress->Write();
  fBmonChannel->Write();

  fBmonStsDpbDiff->Write();
  for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb) {
    fBmonStsDpbDiffEvo[uStsDpb]->Write();
    fStsDpbCntsEvo[uStsDpb]->Write();
  }

  fBmonMuchRocDiff->Write();
  fBmonMuchAsicDiff->Write();
  for (UInt_t uMuchAsic = 0; uMuchAsic < kuMaxNbMuchAsics; ++uMuchAsic)
    fBmonMuchAsicDiffEvo[uMuchAsic]->Write();

  fSelBmonStsDiff->Write();
  fSelBmonMuchDiff->Write();
  fSelBmonTrdDiff->Write();
  fSelBmonTofDiff->Write();
  fSelBmonRichDiff->Write();
  fSelBmonPsdDiff->Write();

  outfile->Close();
  delete outfile;

  gFile = old;
}

ClassImp(CbmCheckTiming)
