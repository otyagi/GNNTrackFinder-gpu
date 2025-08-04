/* Copyright (C) 2020-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmMcbm2019CheckDtInDet.h"

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
CbmMcbm2019CheckDtInDet::CbmMcbm2019CheckDtInDet() : FairTask("CbmMcbm2019CheckDtInDet") {}

// ---- Destructor ----------------------------------------------------
CbmMcbm2019CheckDtInDet::~CbmMcbm2019CheckDtInDet() {}

// ----  Initialisation  ----------------------------------------------
void CbmMcbm2019CheckDtInDet::SetParContainers()
{
  // Load all necessary parameter containers from the runtime data base
  /*
  FairRunAna* ana = FairRunAna::Instance();
  FairRuntimeDb* rtdb=ana->GetRuntimeDb();

  <CbmMcbm2019CheckDtInDetDataMember> = (<ClassPointer>*)
    (rtdb->getContainer("<ContainerName>"));
  */
}

// ---- Init ----------------------------------------------------------
InitStatus CbmMcbm2019CheckDtInDet::Init()
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
  }  // if ( ! fDigiMan->IsPresent( ECbmModuleId::kTrd) )
  else {
    /// The TRD digi time is relative to the TS start, so we need the metadata to offset it
    fTimeSliceMetaDataArray = dynamic_cast<TClonesArray*>(ioman->GetObject("TimesliceMetaData"));
    if (!fTimeSliceMetaDataArray) LOG(fatal) << "No TS metadata input found while TRD needs it.";
  }  // else of if ( ! fDigiMan->IsPresent( ECbmModuleId::kTrd) )

  if (!fDigiMan->IsPresent(ECbmModuleId::kTof)) { LOG(info) << "No TOF digi input found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kRich)) { LOG(info) << "No RICH digi input found."; }

  if (!fDigiMan->IsPresent(ECbmModuleId::kPsd)) { LOG(info) << "No PSD digi input found."; }

  CreateHistos();

  return kSUCCESS;
}

void CbmMcbm2019CheckDtInDet::CreateHistos()
{
  /// Logarithmic bining for self time comparison
  uint32_t iNbBinsLog = 0;
  /// Parameters are NbDecadesLog, NbStepsDecade, NbSubStepsInStep
  std::vector<double> dBinsLogVector = GenerateLogBinArray(9, 9, 1, iNbBinsLog);
  double* dBinsLog                   = dBinsLogVector.data();
  //  double * dBinsLog = GenerateLogBinArray( 9, 9, 1, iNbBinsLog );

  /// Proportion of hits with same time
  // Bmon vs. Bmon
  fBmonBmonSameTime = new TH1F("fBmonBmonSameTime", "Fract. same time Bmon;Same Time? [];Counts", 2, -0.5, 1.5);
  // sts vs. Sts
  fStsStsSameTime = new TH1F("fStsStsSameTime", "Fract. same time Sts;Same Time? [];Counts", 2, -0.5, 1.5);
  // Much vs. Much
  fMuchMuchSameTime = new TH1F("fMuchMuchSameTime", "Fract. same time Much;Same Time? [];Counts", 2, -0.5, 1.5);
  // Trd vs. Trd
  fTrdTrdSameTime = new TH1F("fTrdTrdSameTime", "Fract. same time Trd;Same Time? [];Counts", 2, -0.5, 1.5);
  // Tof vs. Tof
  fTofTofSameTime = new TH1F("fTofTofSameTime", "Fract. same time Tof;Same Time? [];Counts", 2, -0.5, 1.5);
  // Rich vs. Rich
  fRichRichSameTime = new TH1F("fRichRichSameTime", "Fract. same time Rich;Same Time? [];Counts", 2, -0.5, 1.5);
  // Psd vs. Psd
  fPsdPsdSameTime = new TH1F("fPsdPsdSameTime", "Fract. same time Psd;Same Time? [];Counts", 2, -0.5, 1.5);

  /// Per detector
  // Bmon vs. Bmon
  fBmonBmonDiff = new TH1F("fBmonBmonDiff", "Bmon-Bmon_prev;time diff [ns];Counts", 10001, -0.5, 10000.5);
  // sts vs. Sts
  fStsStsDiff = new TH1F("fStsStsDiff", "Sts-Sts_prev;time diff [ns];Counts", 10001, -0.5, 10000.5);
  // Much vs. Much
  fMuchMuchDiff = new TH1F("fMuchMuchDiff", "Much-Much_prev;time diff [ns];Counts", 10001, -0.5, 10000.5);
  // Trd vs. Trd
  fTrdTrdDiff = new TH1F("fTrdTrdDiff", "Trd-Trd_prev;time diff [ns];Counts", 10001, -0.5, 10000.5);
  // Tof vs. Tof
  fTofTofDiff = new TH1F("fTofTofDiff", "Tof-Tof_prev;time diff [ns];Counts", 10001, -0.5, 10000.5);
  // Rich vs. Rich
  fRichRichDiff = new TH1F("fRichRichDiff", "Rich-Rich_prev;time diff [ns];Counts", 10001, -0.5, 10000.5);
  // Psd vs. Psd
  fPsdPsdDiff = new TH1F("fPsdPsdDiff", "Psd-Psd_prev;time diff [ns];Counts", 10001, -0.5, 10000.5);
  // Bmon vs. Bmon
  fBmonBmonDiffLog = new TH1F("fBmonBmonDiffLog", "Bmon-Bmon_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // sts vs. Sts
  fStsStsDiffLog = new TH1F("fStsStsDiffLog", "Sts-Sts_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Much vs. Much
  fMuchMuchDiffLog = new TH1F("fMuchMuchDiffLog", "Much-Much_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Trd vs. Trd
  fTrdTrdDiffLog = new TH1F("fTrdTrdDiffLog", "Trd-Trd_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Tof vs. Tof
  fTofTofDiffLog = new TH1F("fTofTofDiffLog", "Tof-Tof_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Rich vs. Rich
  fRichRichDiffLog = new TH1F("fRichRichDiffLog", "Rich-Rich_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);
  // Psd vs. Psd
  fPsdPsdDiffLog = new TH1F("fPsdPsdDiffLog", "Psd-Psd_prev;time diff [ns];Counts", iNbBinsLog, dBinsLog);

  /// Per channel
  // Bmon vs. Bmon
  fBmonBmonDiffPerChan =
    new TH2F("fBmonBmonDiffPerChan", "Bmon-Bmon_prev Per Channel;time diff [ns]; Channel [];Counts", iNbBinsLog,
             dBinsLog, fuNbChanBmon, 0, fuNbChanBmon);
  // sts vs. Sts
  fStsStsDiffPerChan = new TH2F("fStsStsDiffPerChan", "Sts-Sts_prev Per Channel;time diff [ns]; Channel [];Counts",
                                iNbBinsLog, dBinsLog, fuNbChanSts, 0, fuNbChanSts);
  // Much vs. Much
  fMuchMuchDiffPerChan =
    new TH2F("fMuchMuchDiffPerChan", "Much-Much_prev Per Channel;time diff [ns]; Channel [];Counts", iNbBinsLog,
             dBinsLog, fuNbChanMuch, 0, fuNbChanMuch);
  // Trd vs. Trd
  fTrdTrdDiffPerChan = new TH2F("fTrdTrdDiffPerChan", "Trd-Trd_prev Per Channel;time diff [ns]; Channel [];Counts",
                                iNbBinsLog, dBinsLog, fuNbChanTrd, 0, fuNbChanTrd);
  // Tof vs. Tof
  fTofTofDiffPerChan = new TH2F("fTofTofDiffPerChan", "Tof-Tof_prev Per Channel;time diff [ns]; Channel [];Counts",
                                iNbBinsLog, dBinsLog, fuNbChanTof, 0, fuNbChanTof);
  // Rich vs. Rich
  fRichRichDiffPerChan =
    new TH2F("fRichRichDiffPerChan", "Rich-Rich_prev Per Channel;time diff [ns]; Channel [];Counts", iNbBinsLog,
             dBinsLog, fuNbChanRich, 0, fuNbChanRich);
  // Psd vs. Psd
  fPsdPsdDiffPerChan = new TH2F("fPsdPsdDiffPerChan", "Psd-Psd_prev Per Channel;time diff [ns]; Channel [];Counts",
                                iNbBinsLog, dBinsLog, fuNbChanPsd, 0, fuNbChanPsd);

  /// Register the histos in the HTTP server
  FairRunOnline* run = FairRunOnline::Instance();
  if (run) {
    THttpServer* server = run->GetHttpServer();
    if (nullptr != server) {

      server->Register("/Dt", fBmonBmonDiff);
      server->Register("/Dt", fStsStsDiff);
      server->Register("/Dt", fMuchMuchDiff);
      server->Register("/Dt", fTrdTrdDiff);
      server->Register("/Dt", fTofTofDiff);
      server->Register("/Dt", fRichRichDiff);
      server->Register("/Dt", fPsdPsdDiff);

      server->Register("/Dt", fBmonBmonDiffLog);
      server->Register("/Dt", fStsStsDiffLog);
      server->Register("/Dt", fMuchMuchDiffLog);
      server->Register("/Dt", fTrdTrdDiffLog);
      server->Register("/Dt", fTofTofDiffLog);
      server->Register("/Dt", fRichRichDiffLog);
      server->Register("/Dt", fPsdPsdDiffLog);

      server->Register("/DtPerChan", fBmonBmonDiffPerChan);
      server->Register("/DtPerChan", fStsStsDiffPerChan);
      server->Register("/DtPerChan", fMuchMuchDiffPerChan);
      server->Register("/DtPerChan", fTrdTrdDiffPerChan);
      server->Register("/DtPerChan", fTofTofDiffPerChan);
      server->Register("/DtPerChan", fRichRichDiffPerChan);
      server->Register("/DtPerChan", fPsdPsdDiffPerChan);
    }
  }
}
// ---- ReInit  -------------------------------------------------------
InitStatus CbmMcbm2019CheckDtInDet::ReInit() { return kSUCCESS; }

// ---- Exec ----------------------------------------------------------
void CbmMcbm2019CheckDtInDet::Exec(Option_t* /*option*/)
{
  LOG(debug) << "executing TS " << fNrTs;

  if (0 < fNrTs && 0 == fNrTs % 1000) LOG(info) << Form("Processing TS %6d", fNrTs);

  /// Get nb entries per detector
  LOG(debug) << "Begin";
  Int_t nrBmonDigis = 0;
  if (fBmonDigiVector) nrBmonDigis = fBmonDigiVector->size();
  else if (fBmonDigiArray)
    nrBmonDigis = fBmonDigiArray->GetEntriesFast();
  LOG(debug) << "BmonDigis: " << nrBmonDigis;

  /*
  Int_t nrStsDigis  = fDigiMan->GetNofDigis( ECbmModuleId::kSts);
  Int_t nrMuchDigis = fDigiMan->GetNofDigis( ECbmModuleId::kMuch);
  Int_t nrTrdDigis  = fDigiMan->GetNofDigis( ECbmModuleId::kTrd);
  Int_t nrTofDigis  = fDigiMan->GetNofDigis( ECbmModuleId::kTof);
  Int_t nrRichDigis = fDigiMan->GetNofDigis( ECbmModuleId::kRich);
  Int_t nrPsdDigis  = fDigiMan->GetNofDigis( ECbmModuleId::kPsd);
*/

  /// Check dT in Bmon
  for (Int_t iBmon = 0; iBmon < nrBmonDigis; ++iBmon) {

    if (iBmon % 1000 == 0) LOG(debug) << "Executing entry " << iBmon;

    const CbmTofDigi* BmonDigi = nullptr;
    if (fBmonDigiVector) BmonDigi = &(fBmonDigiVector->at(iBmon));
    else if (fBmonDigiArray)
      BmonDigi = dynamic_cast<CbmTofDigi*>(fBmonDigiArray->At(iBmon));
    assert(BmonDigi);

    Double_t T0Time = BmonDigi->GetTime();
    //    Int_t BmonAddress = BmonDigi->GetAddress();

    Double_t T0TimeDiff = T0Time - fPrevTimeBmon;

    if (0 < iBmon) {
      fBmonBmonDiff->Fill(T0TimeDiff);
      if (0 < T0TimeDiff) {
        fBmonBmonSameTime->Fill(0);
        fBmonBmonDiffLog->Fill(T0TimeDiff);
      }  // if( 0 < T0TimeDiff)
      else
        fBmonBmonSameTime->Fill(1);
    }  // if( 0 < iBmon )

    fPrevTimeBmon = T0Time;
  }  // for( Int_t iBmon = 0; iBmon < nrBmonDigis; ++iBmon )

  /// Check dT in the other channels
  FillHistosPerDet<CbmStsDigi>(fStsStsSameTime, fStsStsDiff, fStsStsDiffLog, fStsStsDiffPerChan, ECbmModuleId::kSts);
  FillHistosPerDet<CbmMuchBeamTimeDigi>(fMuchMuchSameTime, fMuchMuchDiff, fMuchMuchDiffLog, fMuchMuchDiffPerChan,
                                        ECbmModuleId::kMuch);
  FillHistosPerDet<CbmTrdDigi>(fTrdTrdSameTime, fTrdTrdDiff, fTrdTrdDiffLog, fTrdTrdDiffPerChan, ECbmModuleId::kTrd);
  FillHistosPerDet<CbmTofDigi>(fTofTofSameTime, fTofTofDiff, fTofTofDiffLog, fTofTofDiffPerChan, ECbmModuleId::kTof);
  FillHistosPerDet<CbmRichDigi>(fRichRichSameTime, fRichRichDiff, fRichRichDiffLog, fRichRichDiffPerChan,
                                ECbmModuleId::kRich);
  FillHistosPerDet<CbmPsdDigi>(fPsdPsdSameTime, fPsdPsdDiff, fPsdPsdDiffLog, fPsdPsdDiffPerChan, ECbmModuleId::kPsd);

  fNrTs++;

  if (0 < fNrTs && 0 == fNrTs % 90000) WriteHistos();
}


template<class Digi>
void CbmMcbm2019CheckDtInDet::FillHistosPerDet(TH1* histoSameTime, TH1* histoDt, TH1* histoDtLog,
                                               TH2* /*histoDtPerChan*/, ECbmModuleId iDetId)
{
  UInt_t uNrDigis = fDigiMan->GetNofDigis(iDetId);

  Double_t dPrevTime = -1;

  for (UInt_t uDigiIdx = 0; uDigiIdx < uNrDigis; ++uDigiIdx) {
    const Digi* digi = fDigiMan->Get<Digi>(uDigiIdx);

    Double_t dNewDigiTime = digi->GetTime();

    if (0 < uDigiIdx) {
      Double_t dTimeDiff = dNewDigiTime - dPrevTime;

      histoDt->Fill(dTimeDiff);
      if (0 < dTimeDiff) {
        histoSameTime->Fill(0);
        histoDtLog->Fill(dTimeDiff);
      }  // if( 0 < dTimeDiff )
      else
        histoSameTime->Fill(1);
    }  //
       /*
    /// Fill histos per Channel
    switch( iDetId )
    {
      case kSts: ///< Silicon Tracking System
      {
        const CbmStsDigi* stsDigi;
        try {
          stsDigi =
              boost::any_cast<const CbmStsDigi*>( digi );
        } catch( ... ) {
            LOG( fatal ) << "Failed boost any_cast in CbmMcbm2019CheckPulser::FillSystemOffsetHistos for a digi of type "
                         << Digi::GetClassName();
        } // try/catch
        assert(stsDigi);
        UInt_t uAddr = stsDigi->GetAddress();
        UInt_t uChan = stsDigi->GetChannel();

        break;
      } // case kSts:
      case kMuch:        ///< Muon detection system
      {
        const CbmMuchBeamTimeDigi* muchDigi;
        try {
          muchDigi =
              boost::any_cast<const CbmMuchBeamTimeDigi*>( digi );
        } catch( ... ) {
            LOG( fatal ) << "Failed boost any_cast in CbmMcbm2019CheckPulser::FillSystemOffsetHistos for a digi of type "
                         << Digi::GetClassName();
        } // try/catch
        assert(muchDigi);
        UInt_t uAsic = muchDigi->GetNxId();
        UInt_t uChan = muchDigi->GetNxCh();

        break;
      } // case kMuch:
      case kTrd:         ///< Time-of-flight Detector
      {
        UInt_t uAddr = digi->GetAddress();
        break;
      } // case kTrd:
      case kTof:         ///< Time-of-flight Detector
      {

        break;
      } // case kTof:
      case kRich:        ///< Ring-Imaging Cherenkov Detector
      {

        break;
      } // case kRich:
      case kPsd:         ///< Projectile spectator detector
      {
        UInt_t uAddr = digi->GetAddress();

        break;
      } // case kPsd:
      default:
        return 0;
    } // switch( iDetId )
*/
    dPrevTime = dNewDigiTime;
  }  // for( UInt_t uDigiIdx = 0; uDigiIdx < uNrDigis; ++uDigiIdx )
}

// ---- Finish --------------------------------------------------------
void CbmMcbm2019CheckDtInDet::Finish() { WriteHistos(); }

void CbmMcbm2019CheckDtInDet::WriteHistos()
{
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;


  TFile* outfile = TFile::Open(fOutFileName, "RECREATE");

  fBmonBmonSameTime->Write();
  fStsStsSameTime->Write();
  fMuchMuchSameTime->Write();
  fTrdTrdSameTime->Write();
  fTofTofSameTime->Write();
  fRichRichSameTime->Write();
  fPsdPsdSameTime->Write();

  fBmonBmonDiff->Write();
  fStsStsDiff->Write();
  fMuchMuchDiff->Write();
  fTrdTrdDiff->Write();
  fTofTofDiff->Write();
  fRichRichDiff->Write();
  fPsdPsdDiff->Write();

  fBmonBmonDiffLog->Write();
  fStsStsDiffLog->Write();
  fMuchMuchDiffLog->Write();
  fTrdTrdDiffLog->Write();
  fTofTofDiffLog->Write();
  fRichRichDiffLog->Write();
  fPsdPsdDiffLog->Write();

  fBmonBmonDiffPerChan->Write();
  fStsStsDiffPerChan->Write();
  fMuchMuchDiffPerChan->Write();
  fTrdTrdDiffPerChan->Write();
  fTofTofDiffPerChan->Write();
  fRichRichDiffPerChan->Write();
  fPsdPsdDiffPerChan->Write();

  outfile->Close();
  delete outfile;

  gFile      = oldFile;
  gDirectory = oldDir;
}

ClassImp(CbmMcbm2019CheckDtInDet)
