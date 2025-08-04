/* Copyright (C) 2019-2020 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

// -----------------------------------------------------------------------------
// -----                                                                   -----
// -----                     CbmMcbm2018UnpackerTaskTof                    -----
// -----               Created 10.02.2019 by P.-A. Loizeau                 -----
// -----                                                                   -----
// -----------------------------------------------------------------------------

#include "CbmMcbm2018UnpackerTaskTof.h"

#include "CbmMcbm2018TofPar.h"
#include "CbmMcbm2018UnpackerAlgoTof.h"

#include "FairParGenericSet.h"
#include "FairRootManager.h"
#include "FairRun.h"
#include "FairRunOnline.h"
#include "FairRuntimeDb.h"
#include <Logger.h>

#include "TClonesArray.h"
#include "TH1.h"
#include "THttpServer.h"
#include "TROOT.h"
#include "TString.h"
#include <TFile.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include <stdint.h>

Bool_t bMcbm2018UnpackerTaskTofResetHistos = kFALSE;

CbmMcbm2018UnpackerTaskTof::CbmMcbm2018UnpackerTaskTof(UInt_t /*uNbGdpb*/)
  : CbmMcbmUnpack()
  , fbMonitorMode(kFALSE)
  , fbDebugMonitorMode(kFALSE)
  , fbSeparateArrayBmon(kFALSE)
  , fbWriteOutput(kTRUE)
  , fuDigiMaskedIdBmon(0x00002806)
  , fuDigiMaskId(0x001FFFFF)
  , fulTsCounter(0)
  , fUnpackerAlgo(nullptr)
{
  fUnpackerAlgo = new CbmMcbm2018UnpackerAlgoTof();
}

CbmMcbm2018UnpackerTaskTof::~CbmMcbm2018UnpackerTaskTof() { delete fUnpackerAlgo; }

Bool_t CbmMcbm2018UnpackerTaskTof::Init()
{
  LOG(info) << "CbmMcbm2018UnpackerTaskTof::Init";
  LOG(info) << "Initializing mCBM TOF 2018 Unpacker";

  FairRootManager* ioman = FairRootManager::Instance();
  if (NULL == ioman) { LOG(fatal) << "No FairRootManager instance"; }

  fpvDigiTof = new std::vector<CbmTofDigi>();
  ioman->RegisterAny("TofDigi", fpvDigiTof, fbWriteOutput);

  if (kTRUE == fbSeparateArrayBmon) {
    fpvDigiBmon = new std::vector<CbmTofDigi>();
    ioman->RegisterAny("BmonDigi", fpvDigiBmon, fbWriteOutput);
  }  // if( kTRUE == fbSeparateArrayBmon )

  return kTRUE;
}

void CbmMcbm2018UnpackerTaskTof::SetParContainers()
{
  LOG(info) << "Setting parameter containers for " << GetName();

  TList* fParCList = fUnpackerAlgo->GetParList();

  for (Int_t iparC = 0; iparC < fParCList->GetEntries(); ++iparC) {
    FairParGenericSet* tempObj = (FairParGenericSet*) (fParCList->At(iparC));
    fParCList->Remove(tempObj);

    std::string sParamName {tempObj->GetName()};
    FairParGenericSet* newObj =
      dynamic_cast<FairParGenericSet*>(FairRun::Instance()->GetRuntimeDb()->getContainer(sParamName.data()));

    if (nullptr == newObj) {
      LOG(error) << "Failed to obtain parameter container " << sParamName << ", for parameter index " << iparC;
      return;
    }  // if( nullptr == newObj )

    fParCList->AddAt(newObj, iparC);
    //      delete tempObj;
  }  // for( Int_t iparC = 0; iparC < fParCList->GetEntries(); ++iparC )
}

Bool_t CbmMcbm2018UnpackerTaskTof::InitContainers()
{
  LOG(info) << "Init parameter containers for " << GetName();

  /// Control flags
  CbmMcbm2018TofPar* pUnpackPar =
    dynamic_cast<CbmMcbm2018TofPar*>(FairRun::Instance()->GetRuntimeDb()->getContainer("CbmMcbm2018TofPar"));
  if (nullptr == pUnpackPar) {
    LOG(error) << "Failed to obtain parameter container CbmMcbm2018TofPar";
    return kFALSE;
  }  // if( nullptr == pUnpackPar )
     /*
   fbMonitorMode = pUnpackPar->GetMonitorMode();
   LOG(info) << "Monitor mode:       "
             << ( fbMonitorMode ? "ON" : "OFF" );

   fbDebugMonitorMode = pUnpackPar->GetDebugMonitorMode();
   LOG(info) << "Debug Monitor mode: "
             << ( fbDebugMonitorMode ? "ON" : "OFF" );
*/
  Bool_t initOK = fUnpackerAlgo->InitContainers();

  /// If monitor mode enabled, trigger histos creation, obtain pointer on them and add them to the HTTP server
  if (kTRUE == fbMonitorMode) {
    /// Trigger histo creation on all associated algos
    initOK &= fUnpackerAlgo->CreateHistograms();

    fhArraySize = new TH1I("fhArraySize", "Size of the Array VS TS index; TS index; Size [bytes]", 10000, 0., 10000.);
    fhArrayCapacity =
      new TH1I("fhArrayCapacity", "Size of the Array VS TS index; TS index; Size [bytes]", 10000, 0., 10000.);

    /// Obtain vector of pointers on each histo from the algo (+ optionally desired folder)
    std::vector<std::pair<TNamed*, std::string>> vHistos = fUnpackerAlgo->GetHistoVector();

    /// Register the histos in the HTTP server
    THttpServer* server = FairRunOnline::Instance()->GetHttpServer();
    if (nullptr != server) {
      for (UInt_t uHisto = 0; uHisto < vHistos.size(); ++uHisto) {
        server->Register(Form("/%s", vHistos[uHisto].second.data()), vHistos[uHisto].first);
      }  // for( UInt_t uHisto = 0; uHisto < vHistos.size(); ++uHisto )

      server->RegisterCommand("/Reset_UnpTof_Hist", "bMcbm2018UnpackerTaskTofResetHistos=kTRUE");
      server->Restrict("/Reset_UnpTof_Hist", "allow=admin");

      server->Register("/Array", fhArraySize);
      server->Register("/Array", fhArrayCapacity);
    }  // if( nullptr != server )
  }    // if( kTRUE == fbMonitorMode )

  fUnpackerAlgo->SetMonitorMode(fbMonitorMode);

  return initOK;
}

Bool_t CbmMcbm2018UnpackerTaskTof::ReInitContainers()
{
  LOG(info) << "ReInit parameter containers for " << GetName();
  Bool_t initOK = fUnpackerAlgo->ReInitContainers();

  return initOK;
}

void CbmMcbm2018UnpackerTaskTof::AddMsComponentToList(size_t component, UShort_t usDetectorId)
{
  fUnpackerAlgo->AddMsComponentToList(component, usDetectorId);
}

Bool_t CbmMcbm2018UnpackerTaskTof::DoUnpack(const fles::Timeslice& ts, size_t /*component*/)
{
  if (fbMonitorMode && bMcbm2018UnpackerTaskTofResetHistos) {
    LOG(info) << "Reset TOF unpacker histos ";
    fUnpackerAlgo->ResetHistograms();
    bMcbm2018UnpackerTaskTofResetHistos = kFALSE;
  }  // if( fbMonitorMode && bMcbm2018UnpackerTaskTofResetHistos )

  if (kFALSE == fUnpackerAlgo->ProcessTs(ts)) {
    LOG(error) << "Failed processing TS " << ts.index() << " in unpacker algorithm class";
    return kTRUE;
  }  // if( kFALSE == fUnpackerAlgo->ProcessTs( ts ) )

  /// Copy the digis in the DaqBuffer
  std::vector<CbmTofDigi>& vDigi = fUnpackerAlgo->GetVector();
  LOG(debug) << "Found " << vDigi.size() << " TOF digis during unpacking";

  /*
   /// Sort the buffers of hits due to the time offsets applied
   => Done in the algo!!!
   sort(vDigi.begin(), vDigi.end(),
        [](const CbmTofDigi & a, const CbmTofDigi & b) -> bool
        {
          return a.GetTime() < b.GetTime();
        });
*/

  for (auto digi : vDigi) {
    if (kTRUE == fbSeparateArrayBmon && fuDigiMaskedIdBmon == (digi.GetAddress() & fuDigiMaskId)) {
      /// Insert data in Bmon output container
      LOG(debug) << "Fill digi Bmon vector with " << Form("0x%08x", digi.GetAddress()) << " at " << fpvDigiBmon->size();
      fpvDigiBmon->emplace_back(digi);
    }  // if( kTRUE == fbSeparateArrayBmon && fuDigiMaskedIdBmon == ( digi.GetAddress() & fuDigiMaskId ) )
    else {
      /// Insert data in TOF output container
      LOG(debug) << "Fill digi TOF vector with " << Form("0x%08x", digi.GetAddress()) << " at " << fpvDigiTof->size();
      fpvDigiTof->emplace_back(digi);
    }  // else of if( kTRUE == fbSeparateArrayBmon && fuDigiMaskedIdBmon == ( digi.GetAddress() & fuDigiMaskId ) )
  }

  vDigi.clear();
  fUnpackerAlgo->ClearVector();
  if (kTRUE == fbMonitorMode) {
    if (kTRUE == fbSeparateArrayBmon) {
      fhArraySize->Fill(fulTsCounter, fpvDigiTof->size() + fpvDigiBmon->size());
      fhArrayCapacity->Fill(fulTsCounter, fpvDigiTof->capacity() + fpvDigiBmon->capacity());
    }  // if( kTRUE == fbSeparateArrayBmon )
    else {
      fhArraySize->Fill(fulTsCounter, fpvDigiTof->size());
      fhArrayCapacity->Fill(fulTsCounter, fpvDigiTof->capacity());
    }  // else of if( kTRUE == fbSeparateArrayBmon )
  }    // if( kTRUE == fbMonitorMode )

  if (0 == fulTsCounter % 10000) LOG(info) << "Processed " << fulTsCounter << "TS";
  fulTsCounter++;

  return kTRUE;
}

void CbmMcbm2018UnpackerTaskTof::Reset()
{
  fpvDigiTof->clear();

  if (kTRUE == fbSeparateArrayBmon) { fpvDigiBmon->clear(); }  // if( kTRUE == fbSeparateArrayBmon )
}


void CbmMcbm2018UnpackerTaskTof::Finish()
{
  /// If monitor mode enabled, trigger histos creation, obtain pointer on them and add them to the HTTP server
  if (kTRUE == fbMonitorMode) {
    /// Obtain vector of pointers on each histo from the algo (+ optionally desired folder)
    std::vector<std::pair<TNamed*, std::string>> vHistos = fUnpackerAlgo->GetHistoVector();

    /// Save old global file and folder pointer to avoid messing with FairRoot
    TFile* oldFile     = gFile;
    TDirectory* oldDir = gDirectory;

    TFile* histoFile = nullptr;

    // open separate histo file in recreate mode
    histoFile = new TFile("data/HistosUnpackerTof.root", "RECREATE");
    histoFile->cd();

    /// Register the histos in the HTTP server
    for (UInt_t uHisto = 0; uHisto < vHistos.size(); ++uHisto) {
      /// Make sure we end up in chosen folder
      TString sFolder = vHistos[uHisto].second.data();
      if (nullptr == gDirectory->Get(sFolder)) gDirectory->mkdir(sFolder);
      gDirectory->cd(sFolder);

      /// Write plot
      vHistos[uHisto].first->Write();

      histoFile->cd();
    }  // for( UInt_t uHisto = 0; uHisto < vHistos.size(); ++uHisto )

    fhArraySize->Write();
    fhArrayCapacity->Write();

    /// Restore old global file and folder pointer to avoid messing with FairRoot
    gFile      = oldFile;
    gDirectory = oldDir;

    histoFile->Close();
  }  // if( kTRUE == fbMonitorMode )
}

void CbmMcbm2018UnpackerTaskTof::SetIgnoreOverlapMs(Bool_t bFlagIn) { fUnpackerAlgo->SetIgnoreOverlapMs(bFlagIn); }

void CbmMcbm2018UnpackerTaskTof::SetTimeOffsetNs(Double_t dOffsetIn) { fUnpackerAlgo->SetTimeOffsetNs(dOffsetIn); }
void CbmMcbm2018UnpackerTaskTof::SetDiamondDpbIdx(UInt_t uIdx) { fUnpackerAlgo->SetDiamondDpbIdx(uIdx); }

ClassImp(CbmMcbm2018UnpackerTaskTof)
