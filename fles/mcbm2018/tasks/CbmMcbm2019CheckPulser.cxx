/* Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmMcbm2019CheckPulser.h"

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
#include "TH1.h"
#include "TH2.h"
#include "THttpServer.h"
#include "TProfile.h"
#include <TDirectory.h>
#include <TFile.h>
#include <type_traits>

#include <iomanip>
#include <iostream>
using std::fixed;
using std::setprecision;

// ---- Default constructor -------------------------------------------
CbmMcbm2019CheckPulser::CbmMcbm2019CheckPulser() : FairTask("CbmMcbm2019CheckPulser") {}

// ---- Destructor ----------------------------------------------------
CbmMcbm2019CheckPulser::~CbmMcbm2019CheckPulser() {}

// ----  Initialisation  ----------------------------------------------
void CbmMcbm2019CheckPulser::SetParContainers()
{
  // Load all necessary parameter containers from the runtime data base
  /*
  FairRunAna* ana = FairRunAna::Instance();
  FairRuntimeDb* rtdb=ana->GetRuntimeDb();

  <CbmMcbm2019CheckPulserDataMember> = (<ClassPointer>*)
    (rtdb->getContainer("<ContainerName>"));
  */
}

// ---- Init ----------------------------------------------------------
InitStatus CbmMcbm2019CheckPulser::Init()
{

  // Get a handle from the IO manager
  FairRootManager* ioman = FairRootManager::Instance();

  // Digi manager
  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->UseMuchBeamTimeDigi();
  fDigiMan->Init();

  // Bmon is not included in DigiManager; have to take care here
  // Try to find a vector branch for the digi
  fBmonDigiVector = ioman->InitObjectAs<std::vector<CbmTofDigi> const*>("BmonDigi");
  if (!fBmonDigiVector) {
    LOG(info) << "No Bmon digi vector found; trying TClonesArray";
    if (std::is_convertible<TObject*, CbmTofDigi*>::value) {
      fBmonDigiArray = dynamic_cast<TClonesArray*>(ioman->GetObject("BmonDigi"));
      if (!fBmonDigiArray) LOG(info) << "No Bmon digi input found.";
    }  //? CbmTofDigi derives from TObject
  }    //? No vector for Bmon digis

  if (!fDigiMan->IsPresent(ECbmModuleId::kSts)) { LOG(info) << "No STS digi input found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kMuch)) { LOG(info) << "No MUCH digi input found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kTrd)) {
    LOG(info) << "No TRD digi input found.";
  }  // if ( ! fDigiMan->IsPresent(ECbmModuleId::kTrd) )
  else {
  }  // else of if ( ! fDigiMan->IsPresent(ECbmModuleId::kTrd) )

  if (!fDigiMan->IsPresent(ECbmModuleId::kTof)) { LOG(info) << "No TOF digi input found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kRich)) { LOG(info) << "No RICH digi input found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kPsd)) { LOG(info) << "No PSD digi input found."; }

  /// Access the TS metadata to know TS start tim
  fTimeSliceMetaDataArray = dynamic_cast<TClonesArray*>(ioman->GetObject("TimesliceMetaData"));
  if (!fTimeSliceMetaDataArray) LOG(fatal) << "No TS metadata input found";

  CreateHistos();

  return kSUCCESS;
}

Int_t CbmMcbm2019CheckPulser::CalcNrBins(Int_t offsetRange)
{

  if (offsetRange < 251) {
    Double_t dClocks = offsetRange;
    dClocks /= 6.25;
    return (dClocks * 112 * 2);  /// Bmon/TOF FTS bining
  }
  else if (offsetRange < 501) {
    fBinWidth = 1;
  }
  else if (offsetRange < 1001) {
    fBinWidth = 5;
  }
  else if (offsetRange < 10001) {
    fBinWidth = 10;
  }
  else if (offsetRange < 100001) {
    fBinWidth = 100;
  }
  else {
    fBinWidth = 100;
  }

  return (offsetRange / fBinWidth * 2);
}

void CbmMcbm2019CheckPulser::CreateHistos()
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
    new TH1F("fBmonStsDiff", "Bmon-Sts;time diff [ns];Counts", nrOfBinsSts, -fStsOffsetRange, fStsOffsetRange);

  fBmonStsDiffEvo = new TH2F("fBmonStsDiffEvo", "Bmon-Sts;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsSts,
                             -fStsOffsetRange, fStsOffsetRange);


  Int_t nrOfBinsMuch = CalcNrBins(fMuchOffsetRange);
  // Bmon vs. Much
  fBmonMuchDiff =
    new TH1F("fBmonMuchDiff", "Bmon-Much;time diff [ns];Counts", nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);

  fBmonMuchDiffEvo = new TH2F("fBmonMuchDiffEvo", "Bmon-Much;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsMuch,
                              -fMuchOffsetRange, fMuchOffsetRange);


  Int_t nrOfBinsTrd = CalcNrBins(fTrdOffsetRange);
  // To vs. Trd
  fBmonTrdDiff =
    new TH1F("fBmonTrdDiff", "Bmon-Trd;time diff [ns];Counts", nrOfBinsTrd, -fTrdOffsetRange, fTrdOffsetRange);

  fBmonTrdDiffEvo = new TH2F("fBmonTrdDiffEvo", "Bmon-Trd;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsTrd,
                             -fTrdOffsetRange, fTrdOffsetRange);


  Int_t nrOfBinsTof = CalcNrBins(fTofOffsetRange);
  // To vs. Tof
  fBmonTofDiff =
    new TH1F("fBmonTofDiff", "Bmon-Tof;time diff [ns];Counts", nrOfBinsTof, -fTofOffsetRange, fTofOffsetRange);

  fBmonTofDiffEvo = new TH2F("fBmonTofDiffEvo", "Bmon-Tof;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsTof,
                             -fTofOffsetRange, fTofOffsetRange);


  Int_t nrOfBinsRich = CalcNrBins(fRichOffsetRange);
  // To vs. Rich
  fBmonRichDiff =
    new TH1F("fBmonRichDiff", "Bmon-Rich;time diff [ns];Counts", nrOfBinsRich, -fRichOffsetRange, fRichOffsetRange);

  fBmonRichDiffEvo = new TH2F("fBmonRichDiffEvo", "Bmon-Rich;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsRich,
                              -fRichOffsetRange, fRichOffsetRange);

  Int_t nrOfBinsPsd = CalcNrBins(fPsdOffsetRange);
  // To vs. Psd
  fBmonPsdDiff =
    new TH1F("fBmonPsdDiff", "Bmon-Psd;time diff [ns];Counts", nrOfBinsPsd, -fPsdOffsetRange, fPsdOffsetRange);

  fBmonPsdDiffEvo = new TH2F("fBmonPsdDiffEvo", "Bmon-Psd;TS; time diff [ns];Counts", 1000, 0, 10000, nrOfBinsPsd,
                             -fPsdOffsetRange, fPsdOffsetRange);

  fBmonPsdDiffCharge = new TH2F("fBmonPsdDiffCharge", "Bmon-Psd;time diff [ns]; Charge [a.u]; ;Counts", nrOfBinsPsd,
                                -fPsdOffsetRange, fPsdOffsetRange, 7000, 0, 70000);


  // Bmon vs. Sts
  fBmonStsDiffEvoLong = new TH2F("fBmonStsDiffEvoLong", "Bmon-Sts;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsSts, -fStsOffsetRange, fStsOffsetRange);
  // Bmon vs. Much
  fBmonMuchDiffEvoLong = new TH2F("fBmonMuchDiffEvoLong", "Bmon-Much;TS; time diff [ns];Counts", 1800, 0, 180000,
                                  nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);
  // To vs. Trd
  fBmonTrdDiffEvoLong = new TH2F("fBmonTrdDiffEvoLong", "Bmon-Trd;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsTrd, -fTrdOffsetRange, fTrdOffsetRange);
  // To vs. Tof
  fBmonTofDiffEvoLong = new TH2F("fBmonTofDiffEvoLong", "Bmon-Tof;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsTof, -fTofOffsetRange, fTofOffsetRange);
  // To vs. Rich
  fBmonRichDiffEvoLong = new TH2F("fBmonRichDiffEvoLong", "Bmon-Rich;TS; time diff [ns];Counts", 1800, 0, 180000,
                                  nrOfBinsRich, -fRichOffsetRange, fRichOffsetRange);

  // To vs. Psd
  fBmonPsdDiffEvoLong = new TH2F("fBmonPsdDiffEvoLong", "Bmon-Psd;TS; time diff [ns];Counts", 1800, 0, 180000,
                                 nrOfBinsPsd, -fPsdOffsetRange, fPsdOffsetRange);


  // Bmon vs. Sts
  fBmonStsMeanEvo = new TProfile("fBmonStsMeanEvo", "Bmon-Sts; time in run [s]; Mean time diff [ns]", 4320, 0, 4320);
  // Bmon vs. Much
  fBmonMuchMeanEvo = new TProfile("fBmonMuchMeanEvo", "Bmon-Much; time in run [s]; Mean time diff [ns]", 4320, 0, 4320);
  // To vs. Tof
  fBmonTrdMeanEvo = new TProfile("fBmonTrdMeanEvo", "Bmon-Trd; time in run [s]; Mean time diff [ns]", 4320, 0, 4320);
  // To vs. Tof
  fBmonTofMeanEvo = new TProfile("fBmonTofMeanEvo", "Bmon-Tof; time in run [s]; Mean time diff [ns]", 4320, 0, 4320);
  // To vs. Rich
  fBmonRichMeanEvo = new TProfile("fBmonRichMeanEvo", "Bmon-Rich; time in run [s]; Mean time diff [ns]", 4320, 0, 4320);
  // To vs. Psd
  fBmonPsdMeanEvo = new TProfile("fBmonPsdMeanEvo", "Bmon-Psd; time in run [s]; Mean time diff [ns]", 4320, 0, 4320);
  //       4320, 0, 259200);


  // Bmon vs. STS for the different DPBs
  fBmonStsDpbDiff = new TH2F("fBmonStsDpbDiff", "Bmon-Much;DPB; time diff [ns];Counts", 2, -0.5, 1.5, nrOfBinsSts,
                             -fStsOffsetRange, fStsOffsetRange);

  for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb) {
    fBmonStsDpbDiffEvo[uStsDpb] =
      new TH2F(Form("fBmonStsDpbDiffEvo%02u", uStsDpb), Form("Bmon-STS DPB %02u;TS; time diff [ns];Counts", uStsDpb),
               1800, 0, 180000, nrOfBinsSts, -fStsOffsetRange, fStsOffsetRange);
    fStsDpbCntsEvo[uStsDpb] =
      new TH1F(Form("fStsDpbCntsEvo%02u", uStsDpb), Form("Time STS DPB %02u;TS; Hit Counts", uStsDpb), 1800, 0, 180000);
  }  // for( UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb )

  // Bmon vs. Much for the different DPBs/AFCK
  fBmonMuchRocDiff = new TH2F("fBmonMuchRocDiff", "Bmon-Much;AFCK; time diff [ns];Counts", kuMaxNbMuchDpbs, -0.5,
                              kuMaxNbMuchDpbs - 0.5, nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);

  // Bmon vs. Much for the different ASICs
  fBmonMuchAsicDiff = new TH2F("fBmonMuchAsicDiff", "Bmon-Much;ASIC; time diff [ns];Counts", kuMaxNbMuchAsics, -0.5,
                               kuMaxNbMuchAsics - 0.5, nrOfBinsMuch, -fMuchOffsetRange, fMuchOffsetRange);

  for (UInt_t uMuchAsic = 0; uMuchAsic < kuMaxNbMuchAsics; ++uMuchAsic)
    fBmonMuchAsicDiffEvo[uMuchAsic] = new TH2F(Form("fBmonMuchAsicDiffEvo%02u", uMuchAsic),
                                               Form("Bmon-Much ASIC %02u;TS; time diff [ns];Counts", uMuchAsic), 1800,
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


  fBmonStsNb  = new TH2F("fBmonStsNb", "Bmon-STS;Nb Bmon; Nb STS;TS []", 100, 0, 100, 100, 0, 100);
  fBmonMuchNb = new TH2F("fBmonMuchNb", "Bmon-MUCH;Nb Bmon; Nb MUCH;TS []", 100, 0, 100, 100, 0, 100);
  fBmonTrdNb  = new TH2F("fBmonTrdNb", "Bmon-TRD;Nb Bmon; Nb TRD;TS []", 100, 0, 100, 100, 0, 100);
  fBmonTofNb  = new TH2F("fBmonTofNb", "Bmon-TOF;Nb Bmon; Nb TOF;TS []", 100, 0, 100, 100, 0, 100);
  fBmonRichNb = new TH2F("fBmonRichNb", "Bmon-RICH;Nb Bmon; Nb RICH;TS []", 100, 0, 100, 100, 0, 100);
  fBmonPsdNb  = new TH2F("fBmonPsdNb", "Bmon-PSD;Nb Bmon; Nb PSD;TS []", 100, 0, 100, 100, 0, 100);

  fBmonAddress = new TH1F("fBmonAddress", "Bmon address;address;Counts", 1000000, 0, 1000000.);

  fBmonChannel = new TH1F("fBmonChannel", "Bmon channel;channel nr;Counts", 100, -0.5, 99.5);

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

      server->Register("/CheckTiming", fBmonStsMeanEvo);
      server->Register("/CheckTiming", fBmonMuchMeanEvo);
      server->Register("/CheckTiming", fBmonTrdMeanEvo);
      server->Register("/CheckTiming", fBmonTofMeanEvo);
      server->Register("/CheckTiming", fBmonRichMeanEvo);
      server->Register("/CheckTiming", fBmonPsdMeanEvo);

      server->Register("/CheckTiming", fBmonBmonDiff);
      server->Register("/CheckTiming", fStsStsDiff);
      server->Register("/CheckTiming", fMuchMuchDiff);
      server->Register("/CheckTiming", fTrdTrdDiff);
      server->Register("/CheckTiming", fTofTofDiff);
      server->Register("/CheckTiming", fRichRichDiff);
      server->Register("/CheckTiming", fPsdPsdDiff);

      server->Register("/CheckTiming", fBmonStsNb);
      server->Register("/CheckTiming", fBmonMuchNb);
      server->Register("/CheckTiming", fBmonTrdNb);
      server->Register("/CheckTiming", fBmonTofNb);
      server->Register("/CheckTiming", fBmonRichNb);
      server->Register("/CheckTiming", fBmonPsdNb);

      server->Register("/CheckTiming", fBmonStsDpbDiff);
      for (UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb)
        server->Register("/CheckTiming/STS", fBmonStsDpbDiffEvo[uStsDpb]);

      server->Register("/CheckTiming", fBmonMuchRocDiff);
      server->Register("/CheckTiming", fBmonMuchAsicDiff);
      for (UInt_t uMuchAsic = 0; uMuchAsic < kuMaxNbMuchAsics; ++uMuchAsic)
        server->Register("/CheckTiming/MUCH", fBmonMuchAsicDiffEvo[uMuchAsic]);
    }
  }
  /*
  fDigisPerAsicEvo = new TH2F( "fDigisPerAsicEvo",
      "Digis per Asic evo; Time [ ns ]; ASIC []",
       5 * 10240, 0, 5 * 10240000.,
			kuMaxNbMuchAsics, 0, kuMaxNbMuchAsics);
  for( UInt_t uAsic = 0; uAsic < kuMaxNbMuchAsics; ++uAsic )
    for( UInt_t uChan = 0; uChan < kuNbChanSMX; ++uChan )
      {
         fdLastMuchDigi[ uAsic ][ uChan ] = 0.0;
         fdLastMuchDigiPulser[ uAsic ][ uChan ] = 0.0;
      } // loop on channel and asic

  fSameChanDigisDistEvo = new TH2F( "fSameChanDigisDistEvo",
      "Time dist of digis in same chan evo; Time [ ns ]; Same chan dist [ ns ]",
       5000, 0, 500000.,
       1000, 0., 10000. );

  fDigiTimeEvoBmon = new TH2F( "fDigiTimeEvoBmon",
      "Time of digi in Bmon vs ts index; TS [ ]; Digi time [ ns ]",
       10000, 0., 30000.,
       10000, 0., 300.e9 );
  fDigiTimeEvoSts = new TH2F( "fDigiTimeEvoSts",
      "Time of digi in Sts vs ts index; TS [ ]; Digi time [ ns ]",
       10000, 0., 30000.,
       10000, 0., 300.e9 );
  fDigiTimeEvoMuch = new TH2F( "fDigiTimeEvoMuch",
      "Time of digi in Much vs ts index; TS [ ]; Digi time [ ns ]",
       10000, 0., 30000.,
       10000, 0., 300.e9 );
  fDigiTimeEvoTof = new TH2F( "fDigiTimeEvoTof",
      "Time of digi in Tof vs ts index; TS [ ]; Digi time [ ns ]",
       10000, 0., 30000.,
       10000, 0., 300.e9 );
*/
}
// ---- ReInit  -------------------------------------------------------
InitStatus CbmMcbm2019CheckPulser::ReInit() { return kSUCCESS; }

// ---- Exec ----------------------------------------------------------
void CbmMcbm2019CheckPulser::Exec(Option_t* /*option*/)
{
  LOG(debug) << "executing TS " << fNrTs;

  if (0 < fNrTs && 0 == fNrTs % 1000) LOG(info) << Form("Processing TS %6d", fNrTs);

  /*
  /// Zoom in on Much jump in run 401
  if( fNrTs < 17600 || 17650 < fNrTs )
//  if( fNrTs < 17634 || 17637 < fNrTs )
    return;
*/

  CheckInterSystemOffset();
  /*
  Int_t nrBmonDigis=fBmonDigis->GetEntriesFast();
  for (Int_t iBmon = 0; iBmon < nrBmonDigis; ++iBmon)
  {
      CbmDigi* BmonDigi = static_cast<CbmDigi*>(fBmonDigis->At(iBmon));

      Double_t T0Time = BmonDigi->GetTime();
      fDigiTimeEvoBmon->Fill( fNrTs, T0Time );
  }

  Int_t nrStsDigis{0};
  if (fStsDigis) {
    nrStsDigis=fStsDigis->GetEntriesFast();
    LOG(debug) << "StsDigis: " << nrStsDigis;
    for (Int_t iMuch = 0; iMuch < nrStsDigis; ++iMuch) {

      CbmDigi* Digi = static_cast<CbmDigi*>(fStsDigis->At(iMuch));

      Double_t dTime = Digi->GetTime();
      fDigiTimeEvoSts->Fill( fNrTs, dTime );
    }
  }

  Int_t nrMuchDigis{0};
  if (fMuchDigis) {
    nrMuchDigis=fMuchDigis->GetEntriesFast();
    LOG(debug) << "MuchDigis: " << nrMuchDigis;
    for (Int_t iMuch = 0; iMuch < nrMuchDigis; ++iMuch) {

      CbmDigi* Digi = static_cast<CbmDigi*>(fMuchDigis->At(iMuch));

      Double_t dTime = Digi->GetTime();
      fDigiTimeEvoMuch->Fill( fNrTs, dTime );
    }
  }

  Int_t nrTofDigis{0};
  if (fTofDigis) {
    nrTofDigis=fTofDigis->GetEntriesFast();
    LOG(debug) << "TofDigis: " << nrTofDigis;
    for (Int_t iMuch = 0; iMuch < nrTofDigis; ++iMuch) {

      CbmDigi* Digi = static_cast<CbmDigi*>(fTofDigis->At(iMuch));

      Double_t dTime = Digi->GetTime();
      fDigiTimeEvoTof->Fill( fNrTs, dTime );
    }
  }
*/
  fNrTs++;

  if (0 < fNrTs && 0 == fNrTs % 90000) WriteHistos();
}

void CbmMcbm2019CheckPulser::CheckInterSystemOffset()
{
  LOG(debug) << "Begin";
  Int_t nrBmonDigis = 0;
  if (fBmonDigiVector) nrBmonDigis = fBmonDigiVector->size();
  else if (fBmonDigiArray)
    nrBmonDigis = fBmonDigiArray->GetEntriesFast();
  LOG(debug) << "BmonDigis: " << nrBmonDigis;

  Int_t nrStsDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kSts);
  Int_t nrMuchDigis = fDigiMan->GetNofDigis(ECbmModuleId::kMuch);
  Int_t nrTrdDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kTrd);
  Int_t nrTofDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kTof);
  Int_t nrRichDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
  Int_t nrPsdDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kPsd);

  /*
  if( 0 < nrBmonDigis )
  {
    LOG(info) << "TS:   " << fNrTs;
    LOG(info) << "Bmon:   " << nrBmonDigis;
    LOG(info) << "STS:  " << nrStsDigis;
    LOG(info) << "MUCH: " << nrMuchDigis;
    LOG(info) << "TRD:  " << nrTrdDigis;
    LOG(info) << "TOF:  " << nrTofDigis;
    LOG(info) << "RICH: " << nrRichDigis;
    LOG(info) << "PSD:  " << nrPsdDigis;
  }
*/
  //  if (nrBmonDigis < 100000) {
  if (nrBmonDigis < 1000000) {
    /// Re-initialize array references
    fPrevBmonFirstDigiSts  = 0.;
    fPrevBmonFirstDigiMuch = 0.;
    fPrevBmonFirstDigiTrd  = 0.;
    fPrevBmonFirstDigiTof  = 0.;
    fPrevBmonFirstDigiRich = 0.;
    fPrevBmonFirstDigiPsd  = 0.;

    pTsMetaData = dynamic_cast<TimesliceMetaData*>(fTimeSliceMetaDataArray->At(0));
    if (nullptr == pTsMetaData) LOG(fatal) << Form("No TS metadata found for TS %6u.", fNrTs);

    for (Int_t iBmon = 0; iBmon < nrBmonDigis; ++iBmon) {

      if (iBmon % 1000 == 0) LOG(debug) << "Executing entry " << iBmon;

      const CbmTofDigi* BmonDigi = nullptr;
      if (fBmonDigiVector) BmonDigi = &(fBmonDigiVector->at(iBmon));
      else if (fBmonDigiArray)
        BmonDigi = dynamic_cast<CbmTofDigi*>(fBmonDigiArray->At(iBmon));
      assert(BmonDigi);

      /// Keep only pulser Digis
      if (fuMaxTotPulserBmon < BmonDigi->GetCharge() || BmonDigi->GetCharge() < fuMinTotPulserBmon) continue;

      Double_t T0Time   = BmonDigi->GetTime();
      Int_t BmonAddress = BmonDigi->GetAddress();

      /// Keep only pulser Digis
      if (0x00005006 != BmonAddress && 0x04005006 != BmonAddress) continue;

      fiBmonNb++;

      fBmonAddress->Fill(BmonAddress);
      /*
      std::cout << Form( "Bmon pulser in TS %5d: address 0x%08X Bmon time %12.0f dt %12.0f",
                         fNrTs, BmonAddress, T0Time, T0Time - fdLastBmonDigiPulser )
                << std::endl;
*/
      fBmonBmonDiff->Fill(T0Time - fdLastBmonDigiPulser);
      fdLastBmonDigiPulser = T0Time;

      fBmonChannel->Fill(BmonDigi->GetChannel());

      if (nrStsDigis > 0 && nrStsDigis < 1000000 && fuMinAdcPulserSts < fuMaxAdcPulserSts)
        fPrevBmonFirstDigiSts = FillSystemOffsetHistos<CbmStsDigi>(
          fBmonStsDiff, fBmonStsDiffEvo, fBmonStsDiffEvoLong, fBmonStsMeanEvo, fBmonStsDpbDiff, T0Time, fStsOffsetRange,
          fPrevBmonFirstDigiSts, ECbmModuleId::kSts);
      if (nrMuchDigis > 0 && nrMuchDigis < 1000000 && fuMinAdcPulserMuch < fuMaxAdcPulserMuch)
        fPrevBmonFirstDigiMuch = FillSystemOffsetHistos<CbmMuchBeamTimeDigi>(
          fBmonMuchDiff, fBmonMuchDiffEvo, fBmonMuchDiffEvoLong, fBmonMuchMeanEvo, fBmonMuchRocDiff, T0Time,
          fMuchOffsetRange, fPrevBmonFirstDigiMuch, ECbmModuleId::kMuch);
      if (nrTrdDigis > 0 && nrTrdDigis < 1000000 && fuMinChargePulserTrd < fuMaxChargePulserTrd)
        fPrevBmonFirstDigiTrd = FillSystemOffsetHistos<CbmTrdDigi>(fBmonTrdDiff, fBmonTrdDiffEvo, fBmonTrdDiffEvoLong,
                                                                   fBmonTrdMeanEvo, nullptr, T0Time, fTrdOffsetRange,
                                                                   fPrevBmonFirstDigiTrd, ECbmModuleId::kTrd);
      if (nrTofDigis > 0 && nrTofDigis < 1000000 && fuMinTotPulserTof < fuMaxTotPulserTof)
        fPrevBmonFirstDigiTof = FillSystemOffsetHistos<CbmTofDigi>(fBmonTofDiff, fBmonTofDiffEvo, fBmonTofDiffEvoLong,
                                                                   fBmonTofMeanEvo, nullptr, T0Time, fTofOffsetRange,
                                                                   fPrevBmonFirstDigiTof, ECbmModuleId::kTof);
      if (nrRichDigis > 0 && nrRichDigis < 1000000 && fuMinTotPulserRich < fuMaxTotPulserRich)
        fPrevBmonFirstDigiRich = FillSystemOffsetHistos<CbmRichDigi>(
          fBmonRichDiff, fBmonRichDiffEvo, fBmonRichDiffEvoLong, fBmonRichMeanEvo, nullptr, T0Time, fRichOffsetRange,
          fPrevBmonFirstDigiRich, ECbmModuleId::kRich);
      if (nrPsdDigis > 0 && nrPsdDigis < 1000000 && fuMinAdcPulserPsd < fuMaxAdcPulserPsd)
        fPrevBmonFirstDigiPsd = FillSystemOffsetHistos<CbmPsdDigi>(fBmonPsdDiff, fBmonPsdDiffEvo, fBmonPsdDiffEvoLong,
                                                                   fBmonPsdMeanEvo, nullptr, T0Time, fPsdOffsetRange,
                                                                   fPrevBmonFirstDigiPsd, ECbmModuleId::kPsd);
    }

    /// Count pulser candidates for each system
    /// STS
    for (Int_t iDigi = 0; iDigi < nrStsDigis; ++iDigi) {
      const CbmStsDigi* digi = fDigiMan->Get<CbmStsDigi>(iDigi);

      UInt_t uAddr = digi->GetAddress();
      UInt_t uChan = digi->GetChannel();

      if ((kuDefaultAddress != fuStsAddress && uAddr != fuStsAddress)
          || (kuMaxChannelSts != fuStsFirstCha && uChan < fuStsFirstCha)
          || (kuMaxChannelSts != fuStsLastChan && fuStsLastChan < uChan))
        continue;

      if (fuMaxAdcPulserSts < digi->GetCharge() || digi->GetCharge() < fuMinAdcPulserSts) continue;

      fiStsNb++;
    }  // for( Int_t iDigi = 0; iDigi < nrStsDigis; ++iDigi )
    /// MUCH
    for (Int_t iDigi = 0; iDigi < nrMuchDigis; ++iDigi) {
      const CbmMuchBeamTimeDigi* digi = fDigiMan->Get<CbmMuchBeamTimeDigi>(iDigi);

      UInt_t uAsic = digi->GetNxId();
      UInt_t uChan = digi->GetNxCh();

      if ((kuMaxNbMuchAsics != fuMuchAsic && uAsic != fuMuchAsic)
          || (kuNbChanSMX != fuMuchFirstCha && uChan < fuMuchFirstCha)
          || (kuNbChanSMX != fuMuchLastChan && fuMuchLastChan < uChan))
        continue;

      if (fuMaxAdcPulserMuch < digi->GetCharge() || digi->GetCharge() < fuMinAdcPulserMuch) continue;

      fiMuchNb++;
    }  // for( Int_t iDigi = 0; iDigi < nrMuchDigis; ++iDigi )
    /// TRD
    for (Int_t iDigi = 0; iDigi < nrTrdDigis; ++iDigi) {
      const CbmTrdDigi* digi = fDigiMan->Get<CbmTrdDigi>(iDigi);

      UInt_t uAddr = digi->GetAddress();

      if ((kuDefaultAddress != fuTrdAddress && uAddr != fuTrdAddress)) continue;

      if (fuMaxChargePulserTrd < digi->GetCharge() || digi->GetCharge() < fuMinChargePulserTrd) continue;

      fiTrdNb++;
    }  // for( Int_t iDigi = 0; iDigi < nrTrdDigis; ++iDigi )
    /// TOF
    for (Int_t iDigi = 0; iDigi < nrTofDigis; ++iDigi) {
      const CbmTofDigi* digi = fDigiMan->Get<CbmTofDigi>(iDigi);

      if (fuMaxTotPulserTof < digi->GetCharge() || digi->GetCharge() < fuMinTotPulserTof) continue;

      fiTofNb++;
    }  // for( Int_t iDigi = 0; iDigi < nrTofDigis; ++iDigi )
    /// RICH
    for (Int_t iDigi = 0; iDigi < nrRichDigis; ++iDigi) {
      const CbmRichDigi* digi = fDigiMan->Get<CbmRichDigi>(iDigi);

      if (fuMaxTotPulserRich < digi->GetCharge() || digi->GetCharge() < fuMinTotPulserRich) continue;

      fiRichNb++;
    }  // for( Int_t iDigi = 0; iDigi < nrRichDigis; ++iDigi )
    /// PSD
    for (Int_t iDigi = 0; iDigi < nrPsdDigis; ++iDigi) {
      const CbmPsdDigi* digi = fDigiMan->Get<CbmPsdDigi>(iDigi);

      UInt_t uAddr = digi->GetAddress();

      if ((kuDefaultAddress != fuPsdAddress && uAddr != fuPsdAddress)) continue;

      if (fuMaxAdcPulserPsd < digi->GetCharge() || digi->GetCharge() < fuMinAdcPulserPsd) continue;

      fiPsdNb++;
    }  // for( Int_t iDigi = 0; iDigi < nrPsdDigis; ++iDigi )

    fBmonStsNb->Fill(fiBmonNb, fiStsNb);
    fBmonMuchNb->Fill(fiBmonNb, fiMuchNb);
    fBmonTrdNb->Fill(fiBmonNb, fiTrdNb);
    fBmonTofNb->Fill(fiBmonNb, fiTofNb);
    fBmonRichNb->Fill(fiBmonNb, fiRichNb);
    fBmonPsdNb->Fill(fiBmonNb, fiPsdNb);

    fiBmonNb = 0;
    fiStsNb  = 0;
    fiMuchNb = 0;
    fiTrdNb  = 0;
    fiTofNb  = 0;
    fiRichNb = 0;
    fiPsdNb  = 0;
  }  // if (nrBmonDigis < 1000000)

  /*
  for (Int_t iMuch = 0; iMuch < nrMuchDigis; ++iMuch) {

    CbmMuchBeamTimeDigi* Digi = static_cast<CbmMuchBeamTimeDigi*>(fMuchDigis->At(iMuch));

    Double_t dTime = Digi->GetTime();
    Double_t dAdc  = Digi->GetCharge();
    UInt_t uAsic = Digi->GetNxId();
    UInt_t uChan = Digi->GetNxCh();

    Double_t dStartJump = (17633. + 1969.) * 10240000.;
    Double_t dTimeSinceStart = dTime - dStartJump;
    fDigisPerAsicEvo->Fill( dTimeSinceStart, uAsic );

    if( 19132000. < dTimeSinceStart && dTimeSinceStart < 19600000 )
    {
//      std::cout << Form( "Much hit in TS %5d: asic %2u chan %3u Bmon time %12.0f ADC %2.0f",
//                            fNrTs, uAsic, uChan, (dTimeSinceStart - 19132000), dAdc )
//              << std::endl;
      Double_t dTimeDistLastDigi = dTimeSinceStart - fdLastMuchDigi[ uAsic ][ uChan ];
      fSameChanDigisDistEvo->Fill( dTimeSinceStart - 19132000,
                                   dTimeDistLastDigi < 10000 ? dTimeDistLastDigi : 9999 );
    }
    fdLastMuchDigi[ uAsic ][ uChan ] = dTimeSinceStart;

    if( ( kuMaxNbMuchAsics == fuMuchAsic && uAsic == fuMuchAsic ) &&
        ( kuNbChanSMX == fuMuchFirstCha || uChan >= fuMuchFirstCha ) &&
        ( kuNbChanSMX == fuMuchLastChan || fuMuchLastChan <= uChan )
      )
      continue;
    if( fuMaxAdcPulserMuch < Digi->GetCharge() || Digi->GetCharge() < fuMinAdcPulserMuch )
      continue;

        if( 32 != uChan )
          continue;
    std::cout << Form( "Much pulser in TS %5d: chan %3u Bmon time %12.0f ADC %2.0f dt %12.0f",
                       fNrTs, uChan, dTime, dAdc, dTime - fdLastMuchDigiPulser[ uAsic ][ uChan ] )
              << std::endl;
    fdLastMuchDigiPulser[ uAsic ][ uChan ] = dTime;
  }
*/
}

template<class Digi>
Int_t CbmMcbm2019CheckPulser::FillSystemOffsetHistos(TH1* histo, TH2* histoEvo, TH2* histoEvoLong,
                                                     TProfile* profMeanEvo, TH2* histoAFCK, const Double_t T0Time,
                                                     const Int_t offsetRange, Int_t iStartDigi, ECbmModuleId iDetId)
{

  Int_t nrDigis         = fDigiMan->GetNofDigis(iDetId);
  Int_t iFirstDigiInWin = iStartDigi;

  for (Int_t i = iStartDigi; i < nrDigis; ++i) {

    const Digi* digi = fDigiMan->Get<Digi>(i);

    switch (iDetId) {
      case ECbmModuleId::kSts:  ///< Silicon Tracking System
      {
        const CbmStsDigi* stsDigi = nullptr;
        try {
          stsDigi = boost::any_cast<const CbmStsDigi*>(digi);
        }
        catch (...) {
          LOG(fatal) << "Failed boost any_cast in "
                        "CbmMcbm2019CheckPulser::FillSystemOffsetHistos for a "
                        "digi of type "
                     << Digi::GetClassName();
        }  // try/catch
        assert(stsDigi);
        UInt_t uAddr = stsDigi->GetAddress();
        UInt_t uChan = stsDigi->GetChannel();

        if ((kuDefaultAddress != fuStsAddress && uAddr != fuStsAddress)
            || (kuMaxChannelSts != fuStsFirstCha && uChan < fuStsFirstCha)
            || (kuMaxChannelSts != fuStsLastChan && fuStsLastChan < uChan))
          continue;

        if (fuMaxAdcPulserSts < digi->GetCharge() || digi->GetCharge() < fuMinAdcPulserSts) continue;

        fiStsNb++;
        break;
      }                          // case ECbmModuleId::kSts:
      case ECbmModuleId::kMuch:  ///< Muon detection system
      {
        const CbmMuchBeamTimeDigi* muchDigi {nullptr};
        try {
          muchDigi = boost::any_cast<const CbmMuchBeamTimeDigi*>(digi);
        }
        catch (...) {
          LOG(fatal) << "Failed boost any_cast in "
                        "CbmMcbm2019CheckPulser::FillSystemOffsetHistos for a "
                        "digi of type "
                     << Digi::GetClassName();
        }  // try/catch
        assert(muchDigi);
        UInt_t uAsic = muchDigi->GetNxId();
        UInt_t uChan = muchDigi->GetNxCh();

        if ((kuMaxNbMuchAsics != fuMuchAsic && uAsic != fuMuchAsic)
            || (kuNbChanSMX != fuMuchFirstCha && uChan < fuMuchFirstCha)
            || (kuNbChanSMX != fuMuchLastChan && fuMuchLastChan < uChan))
          continue;

        if (fuMaxAdcPulserMuch < digi->GetCharge() || digi->GetCharge() < fuMinAdcPulserMuch) continue;

        fiMuchNb++;
        break;
      }                         // case ECbmModuleId::kMuch:
      case ECbmModuleId::kTrd:  ///< Time-of-flight Detector
      {
        /*
        const CbmTrdDigi* trdDigi;
        try {
          trdDigi =
              boost::any_cast<const CbmTrdDigi*>( digi );
        } catch( ... ) {
            LOG( fatal ) << "Failed boost any_cast in CbmMcbm2019CheckPulser::FillSystemOffsetHistos for a digi of type "
                         << Digi::GetClassName();
        } // try/catch
        assert(trdDigi);
*/
        UInt_t uAddr = digi->GetAddress();

        if ((kuDefaultAddress != fuTrdAddress && uAddr != fuTrdAddress)) continue;

        if (fuMaxChargePulserTrd < digi->GetCharge() || digi->GetCharge() < fuMinChargePulserTrd) continue;

        fiTrdNb++;
        break;
      }                         // case ECbmModuleId::kTrd:
      case ECbmModuleId::kTof:  ///< Time-of-flight Detector
      {
        if (fuMaxTotPulserTof < digi->GetCharge() || digi->GetCharge() < fuMinTotPulserTof) continue;

        fiTofNb++;
        break;
      }                          // case ECbmModuleId::kTof:
      case ECbmModuleId::kRich:  ///< Ring-Imaging Cherenkov Detector
      {
        if (fuMaxTotPulserRich < digi->GetCharge() || digi->GetCharge() < fuMinTotPulserRich) continue;

        fiRichNb++;
        break;
      }                         // case ECbmModuleId::kRich:
      case ECbmModuleId::kPsd:  ///< Projectile spectator detector
      {
        UInt_t uAddr = digi->GetAddress();

        if ((kuDefaultAddress != fuPsdAddress && uAddr != fuPsdAddress)) continue;

        if (fuMaxAdcPulserPsd < digi->GetCharge() || digi->GetCharge() < fuMinAdcPulserPsd) continue;
        if (digi->GetAddress() != (9 << 10) + 8) continue;

        fiPsdNb++;
        break;
      }  // case ECbmModuleId::kPsd:
      default: return 0;
    }  // switch( iDetId )

    Double_t diffTime = T0Time - digi->GetTime();
    //    profMeanEvo->Fill( T0Time * 1e-9 - fdStartTime, diffTime );

    if (diffTime > offsetRange) {
      ++iFirstDigiInWin;                 // Update Index of first digi in Win to next digi
      continue;                          // not yet in interesting range
    }                                    // if (diffTime > offsetRange)
    if (diffTime < -offsetRange) break;  // already past interesting range

    histo->Fill(diffTime);
    histoEvo->Fill(fNrTs, diffTime);
    histoEvoLong->Fill(fNrTs, diffTime);

    if (-1 == fdStartTime) fdStartTime = T0Time * 1e-9;
    profMeanEvo->Fill(T0Time * 1e-9 - fdStartTime, diffTime);
    /*
    if (ECbmModuleId::kMuch == iDetId )
      std::cout << Form( "MUCH coinc in TS %5d: %7.2f Bmon time %12.0f", fNrTs, diffTime, T0Time )
                << std::endl;
*/
    if (ECbmModuleId::kPsd == iDetId) fBmonPsdDiffCharge->Fill(diffTime, digi->GetCharge());

    /// STS DPB mapping: ladder 1 is in DPB 1
    if (ECbmModuleId::kSts == iDetId && histoAFCK) {
      UInt_t uDPB = (0 < (digi->GetAddress() & 0x00000400));
      histoAFCK->Fill(uDPB, diffTime);
      if (uDPB < kuMaxNbStsDpbs) fBmonStsDpbDiffEvo[uDPB]->Fill(fNrTs, diffTime);
    }  // if (ECbmModuleId::kSts == iDetId && histoAFCK)

    /// MUCH DPB mapping
    if (ECbmModuleId::kMuch == iDetId && histoAFCK) {
      const CbmMuchBeamTimeDigi* muchDigi {nullptr};
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
    }  // if (ECbmModuleId::kMuch == iDetId && histoAFCK)
  }

  return iFirstDigiInWin;
}

// ---- Finish --------------------------------------------------------
void CbmMcbm2019CheckPulser::Finish() { WriteHistos(); }

void CbmMcbm2019CheckPulser::WriteHistos()
{
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* outfile = TFile::Open(fOutFileName, "RECREATE");

  fBmonStsDiff->Write();
  fBmonMuchDiff->Write();
  fBmonTrdDiff->Write();
  fBmonTofDiff->Write();
  fBmonRichDiff->Write();
  fBmonPsdDiff->Write();
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

  fBmonStsMeanEvo->Write();
  fBmonMuchMeanEvo->Write();
  fBmonTrdMeanEvo->Write();
  fBmonTofMeanEvo->Write();
  fBmonRichMeanEvo->Write();
  fBmonPsdMeanEvo->Write();

  fBmonBmonDiff->Write();
  fStsStsDiff->Write();
  fMuchMuchDiff->Write();
  fTrdTrdDiff->Write();
  fTofTofDiff->Write();
  fRichRichDiff->Write();
  fPsdPsdDiff->Write();

  fBmonStsNb->Write();
  fBmonMuchNb->Write();
  fBmonTrdNb->Write();
  fBmonTofNb->Write();
  fBmonRichNb->Write();
  fBmonPsdNb->Write();

  fBmonAddress->Write();
  fBmonChannel->Write();

  fBmonStsDpbDiff->Write();
  /*
  for( UInt_t uStsDpb = 0; uStsDpb < kuMaxNbStsDpbs; ++uStsDpb )
  {
    fBmonStsDpbDiffEvo[uStsDpb]->Write();
    fStsDpbCntsEvo[uStsDpb]->Write();
  }
*/
  fBmonMuchRocDiff->Write();
  fBmonMuchAsicDiff->Write();
  /*
  for( UInt_t uMuchAsic = 0; uMuchAsic < kuMaxNbMuchAsics; ++uMuchAsic )
    fBmonMuchAsicDiffEvo[uMuchAsic]->Write();
*/
  /*
  fDigisPerAsicEvo->Write();
  fSameChanDigisDistEvo->Write();

     fDigiTimeEvoBmon ->Write();
     fDigiTimeEvoSts ->Write();
     fDigiTimeEvoMuch->Write();
     fDigiTimeEvoTof->Write();
*/
  outfile->Close();
  delete outfile;

  gFile      = oldFile;
  gDirectory = oldDir;
}

ClassImp(CbmMcbm2019CheckPulser)
