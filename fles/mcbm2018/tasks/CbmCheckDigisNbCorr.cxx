/* Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmCheckDigisNbCorr.h"

#include "CbmDigiManager.h"
#include "CbmMuchBeamTimeDigi.h"
#include "CbmMuchDigi.h"
#include "CbmRichDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"

#include "TimesliceMetaData.h"

#include "FairRootManager.h"
#include "FairRunOnline.h"
#include <Logger.h>

#include "TClonesArray.h"
#include "TH1.h"
#include "TH2.h"
#include "THttpServer.h"
#include "TProfile.h"
#include <TDirectory.h>
#include <TFile.h>

#include <iomanip>
using std::fixed;
using std::setprecision;

// ---- Default constructor -------------------------------------------
CbmCheckDigisNbCorr::CbmCheckDigisNbCorr()
  : FairTask("CbmCheckDigisNbCorr")
  , fuMinTotPulserBmon(90)
  , fuMaxTotPulserBmon(100)

{
}

// ---- Destructor ----------------------------------------------------
CbmCheckDigisNbCorr::~CbmCheckDigisNbCorr() {}

// ----  Initialisation  ----------------------------------------------
void CbmCheckDigisNbCorr::SetParContainers()
{
  // Load all necessary parameter containers from the runtime data base
  /*
  FairRunAna* ana = FairRunAna::Instance();
  FairRuntimeDb* rtdb=ana->GetRuntimeDb();

  <CbmCheckDigisNbCorrDataMember> = (<ClassPointer>*)
    (rtdb->getContainer("<ContainerName>"));
  */
}

// ---- Init ----------------------------------------------------------
InitStatus CbmCheckDigisNbCorr::Init()
{

  // Get a handle from the IO manager
  FairRootManager* ioman = FairRootManager::Instance();

  // Get a pointer to the previous already existing data level
  fTsMetaData = static_cast<TClonesArray*>(ioman->GetObject("TimesliceMetaData"));
  if (!fTsMetaData) { LOG(info) << "No TClonesArray with TS meta data found."; }
  // DigiManager
  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();

  // Get a pointer to the previous already existing data level
  fBmonDigiVec = ioman->InitObjectAs<std::vector<CbmTofDigi> const*>("BmonDigi");
  if (!fBmonDigiVec) {
    fBmonDigiArr = dynamic_cast<TClonesArray*>(ioman->GetObject("BmonDigi"));
    if (!fBmonDigiArr) { LOG(fatal) << "No TClonesArray with Bmon digis found."; }
  }

  if (!fDigiMan->IsPresent(ECbmModuleId::kSts)) { LOG(info) << "No TClonesArray with STS digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kMuch)) { LOG(info) << "No TClonesArray with MUCH digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kTof)) { LOG(info) << "No TClonesArray with TOF digis found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kRich)) { LOG(info) << "No TClonesArray with RICH digis found."; }

  CreateHistos();

  return kSUCCESS;
}

void CbmCheckDigisNbCorr::CalcNrBins() { fiBinNb = fdTsLengthNs / fdBinWidthNs; }

void CbmCheckDigisNbCorr::CreateHistos()
{
  /// Resize storage array
  CalcNrBins();
  fvuNbDigisPerBinBmon.resize(fiBinNb, 0);
  fvuNbDigisPerBinSts.resize(fiBinNb, 0);
  fvuNbDigisPerBinMuch.resize(fiBinNb, 0);
  fvuNbDigisPerBinTof.resize(fiBinNb, 0);
  fvuNbDigisPerBinRich.resize(fiBinNb, 0);

  fvuNbDigisPerBinStsDpb.resize(kuMaxNbStsDpbs);
  for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb)
    fvuNbDigisPerBinStsDpb[uStsDpb].resize(fiBinNb, 0);

  /// 2D correlations between systems
  // Bmon vs. TST
  fBmonStsCorr = new TH2F("fBmonStsCorr",
                          Form("Bmon - STS digis Nb correlation per %.0f ns time interval; Nb "
                               "Bmon Digis []; Nb STS Digis []; Counts",
                               fdBinWidthNs),
                          1000, 0, 1000, 1000, 0, 1000);
  // Bmon vs. MUCH
  fBmonMuchCorr = new TH2F("fBmonMuchCorr",
                           Form("Bmon - MUCH digis Nb correlation per %.0f ns time interval; "
                                "Nb Bmon Digis []; Nb MUCH Digis []; Counts",
                                fdBinWidthNs),
                           1000, 0, 1000, 1000, 0, 1000);
  // Bmon vs. TOF
  fBmonTofCorr = new TH2F("fBmonTofCorr",
                          Form("Bmon - TOF digis Nb correlation per %.0f ns time interval; Nb "
                               "Bmon Digis []; Nb TOF Digis []; Counts",
                               fdBinWidthNs),
                          1000, 0, 1000, 1000, 0, 1000);
  // Bmon vs. RICH
  fBmonRichCorr = new TH2F("fBmonRichCorr",
                           Form("Bmon - RICH digis Nb correlation per %.0f ns time interval; "
                                "Nb Bmon Digis []; Nb RICH Digis []; Counts",
                                fdBinWidthNs),
                           1000, 0, 1000, 1000, 0, 1000);

  // STS vs. MUCH
  fStsMuchCorr = new TH2F("fStsMuchCorr",
                          Form("STS - MUCH digis Nb correlation per %.0f ns time interval; "
                               "Nb STS Digis []; Nb STS Digis []; Counts",
                               fdBinWidthNs),
                          1000, 0, 1000, 1000, 0, 1000);
  // STS vs. TOF
  fStsTofCorr = new TH2F("fStsTofCorr",
                         Form("STS - TOF digis Nb correlation per %.0f ns time interval; "
                              "Nb STS Digis []; Nb TOF Digis []; Counts",
                              fdBinWidthNs),
                         1000, 0, 1000, 1000, 0, 1000);
  // STS vs. RICH
  fStsRichCorr = new TH2F("fStsRichCorr",
                          Form("STS - RICH digis Nb correlation per %.0f ns time interval; "
                               "Nb STS Digis []; Nb RICH Digis []; Counts",
                               fdBinWidthNs),
                          1000, 0, 1000, 1000, 0, 1000);

  // MUCH vs. TOF
  fMuchTofCorr = new TH2F("fMuchTofCorr",
                          Form("MUCH - TOF digis Nb correlation per %.0f ns time interval; "
                               "Nb MUCH Digis []; Nb TOF Digis []; Counts",
                               fdBinWidthNs),
                          1000, 0, 1000, 1000, 0, 1000);
  // MUCH vs. RICH
  fMuchRichCorr = new TH2F("fMuchRichCorr",
                           Form("MUCH - RICH digis Nb correlation per %.0f ns time interval; "
                                "Nb MUCH Digis []; Nb RICH Digis []; Counts",
                                fdBinWidthNs),
                           1000, 0, 1000, 1000, 0, 1000);

  // TOF vs. RICH
  fTofRichCorr = new TH2F("fTofRichCorr",
                          Form("TOF - RICH digis Nb correlation per %.0f ns time interval; "
                               "Nb TOF Digis []; Nb RICH Digis []; Counts",
                               fdBinWidthNs),
                          1000, 0, 1000, 1000, 0, 1000);

  /// Profile correlations between systems
  // Bmon vs. TST
  fBmonStsCorrProf = new TProfile("fBmonStsCorrProf",
                                  Form("Bmon - STS digis Nb correlation per %.0f ns time "
                                       "interval; Nb Bmon Digis []; Nb STS Digis []",
                                       fdBinWidthNs),
                                  1000, 0, 1000);
  // Bmon vs. MUCH
  fBmonMuchCorrProf = new TProfile("fBmonMuchCorrProf",
                                   Form("Bmon - MUCH digis Nb correlation per %.0f ns time "
                                        "interval; Nb Bmon Digis []; Nb MUCH Digis []",
                                        fdBinWidthNs),
                                   1000, 0, 1000);
  // Bmon vs. TOF
  fBmonTofCorrProf = new TProfile("fBmonTofCorrProf",
                                  Form("Bmon - TOF digis Nb correlation per %.0f ns time "
                                       "interval; Nb Bmon Digis []; Nb TOF Digis []",
                                       fdBinWidthNs),
                                  1000, 0, 1000);
  // Bmon vs. RICH
  fBmonRichCorrProf = new TProfile("fBmonRichCorrProf",
                                   Form("Bmon - RICH digis Nb correlation per %.0f ns time "
                                        "interval; Nb Bmon Digis []; Nb RICH Digis []",
                                        fdBinWidthNs),
                                   1000, 0, 1000);

  // STS vs. MUCH
  fStsMuchCorrProf = new TProfile("fStsMuchCorrProf",
                                  Form("STS - MUCH digis Nb correlation per %.0f ns time "
                                       "interval; Nb STS Digis []; Nb STS Digis []",
                                       fdBinWidthNs),
                                  1000, 0, 1000);
  // STS vs. TOF
  fStsTofCorrProf = new TProfile("fStsTofCorrProf",
                                 Form("STS - TOF digis Nb correlation per %.0f ns time "
                                      "interval; Nb STS Digis []; Nb TOF Digis []",
                                      fdBinWidthNs),
                                 1000, 0, 1000);
  // STS vs. RICH
  fStsRichCorrProf = new TProfile("fStsRichCorrProf",
                                  Form("STS - RICH digis Nb correlation per %.0f ns time "
                                       "interval; Nb STS Digis []; Nb RICH Digis []",
                                       fdBinWidthNs),
                                  1000, 0, 1000);

  // MUCH vs. TOF
  fMuchTofCorrProf = new TProfile("fMuchTofCorrProf",
                                  Form("MUCH - TOF digis Nb correlation per %.0f ns time "
                                       "interval; Nb MUCH Digis []; Nb TOF Digis []",
                                       fdBinWidthNs),
                                  1000, 0, 1000);
  // MUCH vs. RICH
  fMuchRichCorrProf = new TProfile("fMuchRichCorrProf",
                                   Form("MUCH - RICH digis Nb correlation per %.0f ns time "
                                        "interval; Nb MUCH Digis []; Nb RICH Digis []",
                                        fdBinWidthNs),
                                   1000, 0, 1000);

  // TOF vs. RICH
  fTofRichCorrProf = new TProfile("fTofRichCorrProf",
                                  Form("TOF - RICH digis Nb correlation per %.0f ns time "
                                       "interval; Nb TOF Digis []; Nb RICH Digis []",
                                       fdBinWidthNs),
                                  1000, 0, 1000);

  for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb) {
    fBmonStsDpbCorr[uStsDpb] = new TH2F(Form("fBmonStsDpbCorr%02u", uStsDpb),
                                        Form("Bmon - STS digis Nb correlation per %.0f ns time interval, "
                                             "DPB %02u; Nb Bmon Digis []; Nb STS Digis []; Counts",
                                             fdBinWidthNs, uStsDpb),
                                        1000, 0, 1000, 1000, 0, 1000);
    fStsMuchDpbCorr[uStsDpb] = new TH2F(Form("fStsMuchDpbCorr%02u", uStsDpb),
                                        Form("STS - MUCH digis Nb correlation per %.0f ns time interval, DPB "
                                             "%02u; Nb STS Digis []; Nb STS Digis []; Counts",
                                             fdBinWidthNs, uStsDpb),
                                        1000, 0, 1000, 1000, 0, 1000);
    fStsTofDpbCorr[uStsDpb]  = new TH2F(Form("fStsTofDpbCorr%02u", uStsDpb),
                                       Form("STS - TOF digis Nb correlation per %.0f ns time interval, "
                                            "DPB %02u; Nb STS Digis []; Nb TOF Digis []; Counts",
                                            fdBinWidthNs, uStsDpb),
                                       1000, 0, 1000, 1000, 0, 1000);
    fStsRichDpbCorr[uStsDpb] = new TH2F(Form("fStsRichDpbCorr%02u", uStsDpb),
                                        Form("STS - RICH digis Nb correlation per %.0f ns time interval, DPB "
                                             "%02u; Nb STS Digis []; Nb RICH Digis []; Counts",
                                             fdBinWidthNs, uStsDpb),
                                        1000, 0, 1000, 1000, 0, 1000);

    fBmonStsDpbCorrProf[uStsDpb] = new TProfile(Form("fBmonStsDpbCorrProf%02u", uStsDpb),
                                                Form("Bmon - STS digis Nb correlation per %.0f ns time "
                                                     "interval, DPB %02u; Nb Bmon Digis []; Nb STS Digis []",
                                                     fdBinWidthNs, uStsDpb),
                                                1000, 0, 1000);
    fStsMuchDpbCorrProf[uStsDpb] = new TProfile(Form("fStsMuchDpbCorrProf%02u", uStsDpb),
                                                Form("STS - MUCH digis Nb correlation per %.0f ns time "
                                                     "interval, DPB %02u; Nb STS Digis []; Nb STS Digis []",
                                                     fdBinWidthNs, uStsDpb),
                                                1000, 0, 1000);
    fStsTofDpbCorrProf[uStsDpb]  = new TProfile(Form("fStsTofDpbCorrProf%02u", uStsDpb),
                                               Form("STS - TOF digis Nb correlation per %.0f ns time "
                                                    "interval, DPB %02u; Nb STS Digis []; Nb TOF Digis []",
                                                    fdBinWidthNs, uStsDpb),
                                               1000, 0, 1000);
    fStsRichDpbCorrProf[uStsDpb] = new TProfile(Form("fStsRichDpbCorrProf%02u", uStsDpb),
                                                Form("STS - RICH digis Nb correlation per %.0f ns time "
                                                     "interval, DPB %02u; Nb STS Digis []; Nb RICH Digis []",
                                                     fdBinWidthNs, uStsDpb),
                                                1000, 0, 1000);
  }  // for( UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb )

  /// Register the histos in the HTTP server
  FairRunOnline* run = FairRunOnline::Instance();
  if (run) {
    THttpServer* server = run->GetHttpServer();
    if (nullptr != server) {
      /// 2D correlations between systems
      server->Register("CheckDigisNbCorr", fBmonStsCorr);
      server->Register("CheckDigisNbCorr", fBmonMuchCorr);
      server->Register("CheckDigisNbCorr", fBmonTofCorr);
      server->Register("CheckDigisNbCorr", fBmonRichCorr);

      server->Register("CheckDigisNbCorr", fStsMuchCorr);
      server->Register("CheckDigisNbCorr", fStsTofCorr);
      server->Register("CheckDigisNbCorr", fStsRichCorr);

      server->Register("CheckDigisNbCorr", fMuchTofCorr);
      server->Register("CheckDigisNbCorr", fMuchRichCorr);

      server->Register("CheckDigisNbCorr", fTofRichCorr);

      /// Profile correlations between systems
      server->Register("CheckDigisNbCorr", fBmonStsCorrProf);
      server->Register("CheckDigisNbCorr", fBmonMuchCorrProf);
      server->Register("CheckDigisNbCorr", fBmonTofCorrProf);
      server->Register("CheckDigisNbCorr", fBmonRichCorrProf);

      server->Register("CheckDigisNbCorr", fStsMuchCorrProf);
      server->Register("CheckDigisNbCorr", fStsTofCorrProf);
      server->Register("CheckDigisNbCorr", fStsRichCorrProf);

      server->Register("CheckDigisNbCorr", fMuchTofCorrProf);
      server->Register("CheckDigisNbCorr", fMuchRichCorrProf);

      server->Register("CheckDigisNbCorr", fTofRichCorrProf);

      for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb) {
        server->Register("CheckDigisNbCorr", fBmonStsDpbCorr[uStsDpb]);
        server->Register("CheckDigisNbCorr", fStsMuchDpbCorr[uStsDpb]);
        server->Register("CheckDigisNbCorr", fStsTofDpbCorr[uStsDpb]);
        server->Register("CheckDigisNbCorr", fStsRichDpbCorr[uStsDpb]);

        server->Register("CheckDigisNbCorr", fBmonStsDpbCorrProf[uStsDpb]);
        server->Register("CheckDigisNbCorr", fStsMuchDpbCorrProf[uStsDpb]);
        server->Register("CheckDigisNbCorr", fStsTofDpbCorrProf[uStsDpb]);
        server->Register("CheckDigisNbCorr", fStsRichDpbCorrProf[uStsDpb]);
      }  // for( UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb )
    }    // if( nullptr != server )
  }      // if (run)
}
// ---- ReInit  -------------------------------------------------------
InitStatus CbmCheckDigisNbCorr::ReInit() { return kSUCCESS; }

// ---- Exec ----------------------------------------------------------
void CbmCheckDigisNbCorr::Exec(Option_t* /*option*/)
{
  /// Initialize the counters for each bin
  for (Int_t uBin = 0; uBin < fiBinNb; ++uBin) {
    fvuNbDigisPerBinBmon[uBin] = 0;
    fvuNbDigisPerBinSts[uBin]  = 0;
    fvuNbDigisPerBinMuch[uBin] = 0;
    fvuNbDigisPerBinTof[uBin]  = 0;
    fvuNbDigisPerBinRich[uBin] = 0;
    for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb)
      fvuNbDigisPerBinStsDpb[uStsDpb][uBin] = 0;
  }  // for( UInt_t uBin = 0; uBin < fiBinNb; ++uBin )

  LOG(debug) << "executing TS " << fNrTs;
  Double_t dTsStart = fNrTs * fdTsLengthNs + 20393267200. - fdTsLengthNs;
  if (1 == fTsMetaData->GetEntriesFast())
    dTsStart = static_cast<TimesliceMetaData*>(fTsMetaData->At(0))->GetStartTime();

  LOG(debug) << "Begin";
  Int_t nrBmonDigis = -1;
  if (fBmonDigiVec) nrBmonDigis = fBmonDigiVec->size();
  else if (fBmonDigiArr)
    nrBmonDigis = fBmonDigiArr->GetEntriesFast();
  Int_t nrStsDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kSts);
  Int_t nrMuchDigis = fDigiMan->GetNofDigis(ECbmModuleId::kMuch);
  Int_t nrTofDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kTof);
  Int_t nrRichDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);

  LOG(debug) << "BmonDigis: " << nrBmonDigis;
  LOG(debug) << "StsDigis: " << nrStsDigis;
  LOG(debug) << "MuchDigis: " << nrMuchDigis;
  LOG(debug) << "TofDigis: " << nrTofDigis;
  LOG(debug) << "RichDigis: " << nrRichDigis;

  /// Loop on digis for each detector and counts digis in proper bin
  /// Bmon
  for (Int_t iDigi = 0; iDigi < nrBmonDigis; ++iDigi) {
    const CbmTofDigi* pDigi = nullptr;
    if (fBmonDigiVec) pDigi = &(fBmonDigiVec->at(iDigi));
    else if (fBmonDigiArr)
      pDigi = dynamic_cast<const CbmTofDigi*>(fBmonDigiArr->At(iDigi));
    assert(pDigi);

    /// Ignore pulser hits in Bmon
    if (fuMinTotPulserBmon < pDigi->GetCharge() && pDigi->GetCharge() < fuMaxTotPulserBmon) continue;

    Double_t dTime = pDigi->GetTime() - dTsStart;
    /// Jump hits with time before start of TS after offseting
    if (dTime < 0) continue;
    /// Stop on first hit with time after end of TS after offseting
    if (fdTsLengthNs <= dTime) break;

    /// Increase count in corresponding bin
    UInt_t uBin = dTime / fdBinWidthNs;
    fvuNbDigisPerBinBmon[uBin]++;
  }  // for( Int_t iDigi = 0; iDigi < nrBmonDigis; ++iDigi )

  /// STS
  for (Int_t iDigi = 0; iDigi < nrStsDigis; ++iDigi) {
    const CbmStsDigi* pDigi = fDigiMan->Get<CbmStsDigi>(iDigi);

    Double_t dTime = pDigi->GetTime() - dTsStart - fdStsOffset;
    /// Jump hits with time before start of TS after offseting
    if (dTime < 0) continue;
    /// Stop on first hit with time after end of TS after offseting
    if (fdTsLengthNs <= dTime) break;

    /// Increase count in corresponding bin
    UInt_t uBin = dTime / fdBinWidthNs;
    fvuNbDigisPerBinSts[uBin]++;

    UInt_t uDPB = (0 < (pDigi->GetAddress() & 0x00000400));
    fvuNbDigisPerBinStsDpb[uDPB][uBin]++;
  }  // for( Int_t iDigi = 0; iDigi < nrStsDigis; ++iDigi )

  /// MUCH
  for (Int_t iDigi = 0; iDigi < nrMuchDigis; ++iDigi) {
    const CbmMuchDigi* pDigi = fDigiMan->Get<CbmMuchDigi>(iDigi);

    Double_t dTime = pDigi->GetTime() - dTsStart - fdMuchOffset;
    /// Jump hits with time before start of TS after offseting
    if (dTime < 0) continue;
    /// Stop on first hit with time after end of TS after offseting
    if (fdTsLengthNs <= dTime) break;

    /// Increase count in corresponding bin
    UInt_t uBin = dTime / fdBinWidthNs;
    fvuNbDigisPerBinMuch[uBin]++;
  }  // for( Int_t iDigi = 0; iDigi < nrMuchDigis; ++iDigi )

  /// TOF
  for (Int_t iDigi = 0; iDigi < nrTofDigis; ++iDigi) {
    const CbmTofDigi* pDigi = fDigiMan->Get<CbmTofDigi>(iDigi);

    /// Ignore pulser hits in TOF
    if (92 < pDigi->GetCharge() && pDigi->GetCharge() < 96) continue;

    Double_t dTime = pDigi->GetTime() - dTsStart - fdTofOffset;
    /// Jump hits with time before start of TS after offseting
    if (dTime < 0) continue;
    /// Stop on first hit with time after end of TS after offseting
    if (fdTsLengthNs <= dTime) break;

    /// Increase count in corresponding bin
    UInt_t uBin = dTime / fdBinWidthNs;
    fvuNbDigisPerBinTof[uBin]++;
  }  // for( Int_t iDigi = 0; iDigi < nrTofDigis; ++iDigi )

  /// RICH
  for (Int_t iDigi = 0; iDigi < nrRichDigis; ++iDigi) {
    const CbmRichDigi* pDigi = fDigiMan->Get<CbmRichDigi>(iDigi);

    Double_t dTime = pDigi->GetTime() - dTsStart - fdRichOffset;
    /// Jump hits with time before start of TS after offseting
    if (dTime < 0) continue;
    /// Stop on first hit with time after end of TS after offseting
    if (fdTsLengthNs <= dTime) break;

    /// Increase count in corresponding bin
    UInt_t uBin = dTime / fdBinWidthNs;
    fvuNbDigisPerBinRich[uBin]++;
  }  // for( Int_t iDigi = 0; iDigi < nrRichDigis; ++iDigi )

  /// Fill the histograms for each bin
  for (Int_t uBin = 0; uBin < fiBinNb; ++uBin) {
    /// 2D & Profiles
    if (0 < fvuNbDigisPerBinBmon[uBin] || 0 < fvuNbDigisPerBinSts[uBin]) {
      fBmonStsCorr->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinSts[uBin]);
      fBmonStsCorrProf->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinSts[uBin]);
    }  // if( 0 < fvuNbDigisPerBinBmon[   uBin ] || 0 < fvuNbDigisPerBinSts[  uBin ] )
    if (0 < fvuNbDigisPerBinBmon[uBin] || 0 < fvuNbDigisPerBinMuch[uBin]) {
      fBmonMuchCorr->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinMuch[uBin]);
      fBmonMuchCorrProf->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinMuch[uBin]);
    }  // if( 0 < fvuNbDigisPerBinBmon[   uBin ] || 0 < fvuNbDigisPerBinMuch[  uBin ] )
    if (0 < fvuNbDigisPerBinBmon[uBin] || 0 < fvuNbDigisPerBinTof[uBin]) {
      fBmonTofCorr->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinTof[uBin]);
      fBmonTofCorrProf->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinTof[uBin]);
    }  // if( 0 < fvuNbDigisPerBinBmon[   uBin ] || 0 < fvuNbDigisPerBinTof[  uBin ] )
    if (0 < fvuNbDigisPerBinBmon[uBin] || 0 < fvuNbDigisPerBinRich[uBin]) {
      fBmonRichCorr->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinRich[uBin]);
      fBmonRichCorrProf->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinRich[uBin]);
    }  // if( 0 < fvuNbDigisPerBinBmon[   uBin ] || 0 < fvuNbDigisPerBinRich[  uBin ] )

    if (0 < fvuNbDigisPerBinSts[uBin] || 0 < fvuNbDigisPerBinMuch[uBin]) {
      fStsMuchCorr->Fill(fvuNbDigisPerBinSts[uBin], fvuNbDigisPerBinMuch[uBin]);
      fStsMuchCorrProf->Fill(fvuNbDigisPerBinSts[uBin], fvuNbDigisPerBinMuch[uBin]);
    }  // if( 0 < fvuNbDigisPerBinSts[   uBin ] || 0 < fvuNbDigisPerBinMuch[  uBin ] )
    if (0 < fvuNbDigisPerBinSts[uBin] || 0 < fvuNbDigisPerBinTof[uBin]) {
      fStsTofCorr->Fill(fvuNbDigisPerBinSts[uBin], fvuNbDigisPerBinTof[uBin]);
      fStsTofCorrProf->Fill(fvuNbDigisPerBinSts[uBin], fvuNbDigisPerBinTof[uBin]);
    }  // if( 0 < fvuNbDigisPerBinSts[   uBin ] || 0 < fvuNbDigisPerBinTof[  uBin ] )
    if (0 < fvuNbDigisPerBinSts[uBin] || 0 < fvuNbDigisPerBinRich[uBin]) {
      fStsRichCorr->Fill(fvuNbDigisPerBinSts[uBin], fvuNbDigisPerBinRich[uBin]);
      fStsRichCorrProf->Fill(fvuNbDigisPerBinSts[uBin], fvuNbDigisPerBinRich[uBin]);
    }  // if( 0 < fvuNbDigisPerBinSts[   uBin ] || 0 < fvuNbDigisPerBinRich[  uBin ] )

    if (0 < fvuNbDigisPerBinMuch[uBin] || 0 < fvuNbDigisPerBinTof[uBin]) {
      fMuchTofCorr->Fill(fvuNbDigisPerBinMuch[uBin], fvuNbDigisPerBinTof[uBin]);
      fMuchTofCorrProf->Fill(fvuNbDigisPerBinMuch[uBin], fvuNbDigisPerBinTof[uBin]);
    }  // if( 0 < fvuNbDigisPerBinMuch[   uBin ] || 0 < fvuNbDigisPerBinTof[  uBin ] )
    if (0 < fvuNbDigisPerBinMuch[uBin] || 0 < fvuNbDigisPerBinRich[uBin]) {
      fMuchRichCorr->Fill(fvuNbDigisPerBinMuch[uBin], fvuNbDigisPerBinRich[uBin]);
      fMuchRichCorrProf->Fill(fvuNbDigisPerBinMuch[uBin], fvuNbDigisPerBinRich[uBin]);
    }  // if( 0 < fvuNbDigisPerBinMuch[   uBin ] || 0 < fvuNbDigisPerBinRich[  uBin ] )

    if (0 < fvuNbDigisPerBinTof[uBin] || 0 < fvuNbDigisPerBinRich[uBin]) {
      fTofRichCorr->Fill(fvuNbDigisPerBinTof[uBin], fvuNbDigisPerBinRich[uBin]);
      fTofRichCorrProf->Fill(fvuNbDigisPerBinTof[uBin], fvuNbDigisPerBinRich[uBin]);
    }  // if( 0 < fvuNbDigisPerBinTof[   uBin ] || 0 < fvuNbDigisPerBinRich[  uBin ] )

    for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb) {
      if (0 < fvuNbDigisPerBinBmon[uBin] || 0 < fvuNbDigisPerBinStsDpb[uStsDpb][uBin]) {
        fBmonStsDpbCorr[uStsDpb]->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinStsDpb[uStsDpb][uBin]);
        fBmonStsDpbCorrProf[uStsDpb]->Fill(fvuNbDigisPerBinBmon[uBin], fvuNbDigisPerBinStsDpb[uStsDpb][uBin]);
      }  // if( 0 < fvuNbDigisPerBinBmon[   uBin ] || 0 < fvuNbDigisPerBinStsDpb[uStsDpb][  uBin ] )

      if (0 < fvuNbDigisPerBinStsDpb[uStsDpb][uBin] || 0 < fvuNbDigisPerBinMuch[uBin]) {
        fStsMuchDpbCorr[uStsDpb]->Fill(fvuNbDigisPerBinStsDpb[uStsDpb][uBin], fvuNbDigisPerBinMuch[uBin]);
        fStsMuchDpbCorrProf[uStsDpb]->Fill(fvuNbDigisPerBinStsDpb[uStsDpb][uBin], fvuNbDigisPerBinMuch[uBin]);
      }  // if( 0 < fvuNbDigisPerBinStsDpb[uStsDpb][   uBin ] || 0 < fvuNbDigisPerBinMuch[  uBin ] )
      if (0 < fvuNbDigisPerBinStsDpb[uStsDpb][uBin] || 0 < fvuNbDigisPerBinTof[uBin]) {
        fStsTofDpbCorr[uStsDpb]->Fill(fvuNbDigisPerBinStsDpb[uStsDpb][uBin], fvuNbDigisPerBinTof[uBin]);
        fStsTofDpbCorrProf[uStsDpb]->Fill(fvuNbDigisPerBinStsDpb[uStsDpb][uBin], fvuNbDigisPerBinTof[uBin]);
      }  // if( 0 < fvuNbDigisPerBinStsDpb[uStsDpb][   uBin ] || 0 < fvuNbDigisPerBinTof[  uBin ] )
      if (0 < fvuNbDigisPerBinStsDpb[uStsDpb][uBin] || 0 < fvuNbDigisPerBinRich[uBin]) {
        fStsRichDpbCorr[uStsDpb]->Fill(fvuNbDigisPerBinStsDpb[uStsDpb][uBin], fvuNbDigisPerBinRich[uBin]);
        fStsRichDpbCorrProf[uStsDpb]->Fill(fvuNbDigisPerBinStsDpb[uStsDpb][uBin], fvuNbDigisPerBinRich[uBin]);
      }  // if( 0 < fvuNbDigisPerBinStsDpb[uStsDpb][   uBin ] || 0 < fvuNbDigisPerBinRich[  uBin ] )
    }    // for( UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb )
  }      // for( UInt_t uBin = 0; uBin < fiBinNb; ++uBin )

  fNrTs++;
}


// ---- Finish --------------------------------------------------------
void CbmCheckDigisNbCorr::Finish() { WriteHistos(); }

void CbmCheckDigisNbCorr::WriteHistos()
{
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* outfile = TFile::Open(fOutFileName, "RECREATE");


  /// 2D
  fBmonStsCorr->Write();
  fBmonMuchCorr->Write();
  fBmonTofCorr->Write();
  fBmonRichCorr->Write();
  fStsMuchCorr->Write();
  fStsTofCorr->Write();
  fStsRichCorr->Write();
  fMuchTofCorr->Write();
  fMuchRichCorr->Write();
  fTofRichCorr->Write();

  /// Profiles
  fBmonStsCorrProf->Write();
  fBmonMuchCorrProf->Write();
  fBmonTofCorrProf->Write();
  fBmonRichCorrProf->Write();
  fStsMuchCorrProf->Write();
  fStsTofCorrProf->Write();
  fStsRichCorrProf->Write();
  fMuchTofCorrProf->Write();
  fMuchRichCorrProf->Write();
  fTofRichCorrProf->Write();

  for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb) {
    fBmonStsDpbCorr[uStsDpb]->Write();
    fStsMuchDpbCorr[uStsDpb]->Write();
    fStsTofDpbCorr[uStsDpb]->Write();
    fStsRichDpbCorr[uStsDpb]->Write();

    fBmonStsDpbCorrProf[uStsDpb]->Write();
    fStsMuchDpbCorrProf[uStsDpb]->Write();
    fStsTofDpbCorrProf[uStsDpb]->Write();
    fStsRichDpbCorrProf[uStsDpb]->Write();
  }  // for( UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb )

  outfile->Close();
  delete outfile;

  gFile      = oldFile;
  gDirectory = oldDir;
}

ClassImp(CbmCheckDigisNbCorr)
