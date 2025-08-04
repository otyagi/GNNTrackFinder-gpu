/* Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#include "CbmCheckEvents.h"

//#include "CbmTofDigi.h"
#include "CbmDefs.h"
#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmMuchBeamTimeDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"

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
#include <typeinfo>

using std::fixed;
using std::setprecision;

// ---- Default constructor -------------------------------------------
CbmCheckEvents::CbmCheckEvents() : FairTask("CbmCheckEvents") {}

// ---- Destructor ----------------------------------------------------
CbmCheckEvents::~CbmCheckEvents() {}

// ----  Initialisation  ----------------------------------------------
void CbmCheckEvents::SetParContainers()
{
  // Load all necessary parameter containers from the runtime data base
  /*
  FairRunAna* ana = FairRunAna::Instance();
  FairRuntimeDb* rtdb=ana->GetRuntimeDb();

  <CbmCheckEventsDataMember> = (<ClassPointer>*)
    (rtdb->getContainer("<ContainerName>"));
  */
}

// ---- Init ----------------------------------------------------------
InitStatus CbmCheckEvents::Init()
{

  // Get a handle from the IO manager
  FairRootManager* ioman = FairRootManager::Instance();

  // DigiManager
  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->UseMuchBeamTimeDigi();
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

  fEvents = dynamic_cast<TClonesArray*>(ioman->GetObject("CbmEvent"));
  if (nullptr == fEvents) {

    if (nullptr != (ioman->GetObject("CbmEvent"))) {
      LOG(error) << "Got pointer of type" << typeid(ioman->GetObject("CbmEvent")).name();
      auto& tmp = *(ioman->GetObject("CbmEvent"));
      LOG(error) << "Got Object of type" << typeid(tmp).name();
    }  // if( nullptr != (ioman->GetObject("CbmEvent") )
    LOG(fatal) << "No TClonesArray with events found.";
  }  // if (nullptr == fEvents)


  CreateHistos();

  return kSUCCESS;
}

// ---- ReInit  -------------------------------------------------------
InitStatus CbmCheckEvents::ReInit() { return kSUCCESS; }

void CbmCheckEvents::CreateHistos()
{
  fEventSize   = new TH1F("fEventSize", "Event Size; # Digis; Counts", 1000, -0.5, 999.5);
  fEventLength = new TH1F("fEventLength", "Event Length; time [ns]; Counts", 1000, -0.5, 999.5);
  fEventsPerTS = new TH1F("fEventsPerTS", "Events per time slice; # Events; Counts", 1000, -0.5, 999.5);
  fBmonInEvent = new TH1F("fBmonInEvent", "Number of Bmon digis in Event; # digis; Counts", 1000, -0.5, 999.5);
  fStsInEvent  = new TH1F("fStsInEvent", "Number of Sts digis in Event; # digis; Counts", 1000, -0.5, 999.5);
  fMuchInEvent = new TH1F("fMuchInEvent", "Number of Much digis in Event; # digis; Counts", 1000, -0.5, 999.5);
  fTofInEvent  = new TH1F("fTofInEvent", "Number of Tof digis in Event; # digis; Counts", 1000, -0.5, 999.5);
  fBmonDeltaT =
    new TH1F("fBmonDeltaT", "Time diff between first and last Bmon digi;dt [ns]; Counts", 1000, -0.5, 999.5);
  fStsDeltaT   = new TH1F("fStsDeltaT", "Time diff between first and last Sts digi;dt [ns]; Counts", 1000, -0.5, 999.5);
  fMuchDeltaT =
    new TH1F("fMuchDeltaT", "Time diff between first and last Much digi;dt [ns]; Counts", 1000, -0.5, 999.5);
  fTofDeltaT = new TH1F("fTofDeltaT", "Time diff between first and last Tof digi;dt [ns]; Counts", 1000, -0.5, 999.5);

  fEventsvsTS = new TH2F("fEventsvsTS", "Nr. of events as fct. of TS", 10000, -0.5, 9999.5, 1000, -0.5, 999.5);
  fLengthvsTS = new TProfile("fLengthvsTS", "Length of events as fct. of TS", 10000, -0.5, 9999.5, -0.5, 999.5);
}
// ---- Exec ----------------------------------------------------------
void CbmCheckEvents::Exec(Option_t* /*option*/)
{

  LOG_IF(info, fNrTs % 1000 == 0) << "Analysing TS " << fNrTs;

  Int_t nrEvents = fEvents->GetEntriesFast();

  fEventsPerTS->Fill(nrEvents);
  fEventsvsTS->Fill(fNrTs, nrEvents);

  Int_t nrBmonDigis = -1;
  if (fBmonDigiVec) nrBmonDigis = fBmonDigiVec->size();
  else if (fBmonDigiArr)
    nrBmonDigis = fBmonDigiArr->GetEntriesFast();
  Int_t nrStsDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kSts);
  Int_t nrMuchDigis = fDigiMan->GetNofDigis(ECbmModuleId::kMuch);
  Int_t nrTofDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kTof);

  LOG(debug) << "Events: " << nrEvents;
  LOG(debug) << "BmonDigis: " << nrBmonDigis;
  LOG(debug) << "StsDigis: " << nrStsDigis;
  LOG(debug) << "MuchDigis: " << nrMuchDigis;
  LOG(debug) << "TofDigis: " << nrTofDigis;

  // Loop over all CbmEvents in the time slice
  for (Int_t iEvent = 0; iEvent < nrEvents; iEvent++) {
    CbmEvent* event = dynamic_cast<CbmEvent*>(fEvents->At(iEvent));
    fEventSize->Fill(event->GetNofData());
    fEventLength->Fill(event->GetEndTime() - event->GetStartTime());
    fLengthvsTS->Fill(fNrTs, event->GetEndTime() - event->GetStartTime(), 1);
    AnalyseEvent(event);
  }

  fNrTs++;
}

void CbmCheckEvents::AnalyseEvent(CbmEvent* event)
{
  // Loop over the the digis and extract the maximum time
  // difference between the digis
  GetTimeDiffBmon(event, fBmonDeltaT, fBmonInEvent);
  GetTimeDiff<CbmStsDigi>(event, fStsDeltaT, fStsInEvent, ECbmDataType::kStsDigi);
  GetTimeDiff<CbmMuchBeamTimeDigi>(event, fMuchDeltaT, fMuchInEvent, ECbmDataType::kMuchDigi);
  GetTimeDiff<CbmTofDigi>(event, fTofDeltaT, fTofInEvent, ECbmDataType::kTofDigi);
}

template<class Digi>
void CbmCheckEvents::GetTimeDiff(CbmEvent* event, TH1* deltaT, TH1* size, ECbmDataType dataType)
{
  Double_t startTime {1.e18};
  Double_t stopTime {0.};
  Int_t nDigis = event->GetNofData(dataType);
  size->Fill(nDigis);
  for (Int_t iDigi = 0; iDigi < nDigis; iDigi++) {
    UInt_t index     = event->GetIndex(dataType, iDigi);
    const Digi* digi = fDigiMan->Get<Digi>(index);
    assert(digi);
    if (digi->GetTime() < startTime) startTime = digi->GetTime();
    if (digi->GetTime() > stopTime) stopTime = digi->GetTime();
  }
  deltaT->Fill(stopTime - startTime);
}


void CbmCheckEvents::GetTimeDiffBmon(CbmEvent* event, TH1* deltaT, TH1* size)
{
  Double_t startTime {1.e18};
  Double_t stopTime {0.};
  Int_t nDigis = event->GetNofData(ECbmDataType::kBmonDigi);
  size->Fill(nDigis);
  for (Int_t iDigi = 0; iDigi < nDigis; iDigi++) {
    UInt_t index = event->GetIndex(ECbmDataType::kBmonDigi, iDigi);
    //Double_t digiTime; (VF) not used
    const CbmTofDigi* digi = nullptr;
    if (fBmonDigiVec) digi = &(fBmonDigiVec->at(index));
    else if (fBmonDigiArr)
      digi = dynamic_cast<CbmTofDigi*>(fBmonDigiArr->At(index));
    assert(digi);
    if (digi->GetTime() < startTime) startTime = digi->GetTime();
    if (digi->GetTime() > stopTime) stopTime = digi->GetTime();
  }
  deltaT->Fill(stopTime - startTime);
}

// ---- Finish --------------------------------------------------------
void CbmCheckEvents::Finish()
{
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* outfile = TFile::Open("test2.root", "RECREATE");

  fEventSize->Write();
  fEventLength->Write();
  fEventsPerTS->Write();

  fBmonInEvent->Write();
  fStsInEvent->Write();
  fMuchInEvent->Write();
  fTofInEvent->Write();

  fBmonDeltaT->Write();
  fStsDeltaT->Write();
  fMuchDeltaT->Write();
  fTofDeltaT->Write();

  fEventsvsTS->Write();
  fLengthvsTS->Write();

  outfile->Close();
  delete outfile;

  gFile      = oldFile;
  gDirectory = oldDir;
}

ClassImp(CbmCheckEvents)
