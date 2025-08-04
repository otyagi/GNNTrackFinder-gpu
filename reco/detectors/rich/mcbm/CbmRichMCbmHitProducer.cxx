/* Copyright (C) 2019-2024 UGiessen/JINR-LIT, Giessen/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Adrian Amatus Weber, Semen Lebedev [committer], Martin Beyer */

#include "CbmRichMCbmHitProducer.h"

#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmRichDetectorData.h"  // for CbmRichPmtData, CbmRichPixelData
#include "CbmRichDigi.h"
#include "CbmRichDigiMapManager.h"
#include "CbmRichGeoManager.h"
#include "CbmRichHit.h"
#include "CbmRichPoint.h"
#if HAVE_ONNXRUNTIME
#include "CbmRichMCbmDenoiseCnn.h"
#endif

#include <FairRootManager.h>
#include <Logger.h>

#include <TClonesArray.h>
#include <TStopwatch.h>

#include <iomanip>
#include <iostream>

using std::fixed;
using std::right;
using std::setprecision;
using std::setw;

using namespace std;


CbmRichMCbmHitProducer::CbmRichMCbmHitProducer()
  : FairTask("CbmRichMCbmHitProducer")
  , fRichHits(NULL)
  , fNofTs(0)
  , fHitError(0.6 / sqrt(12.))
  ,
  //fMappingFile("mRICH_Mapping_vert_20190318.geo")
  fMappingFile("mRICH_Mapping_vert_20190318_elView.geo")
  , fICD_offset_read()
{
}

CbmRichMCbmHitProducer::~CbmRichMCbmHitProducer()
{
  FairRootManager* manager = FairRootManager::Instance();
  manager->Write();
}

void CbmRichMCbmHitProducer::SetParContainers() {}

InitStatus CbmRichMCbmHitProducer::Init()
{
  FairRootManager* manager = FairRootManager::Instance();

  fCbmEvents = dynamic_cast<TClonesArray*>(manager->GetObject("CbmEvent"));
  if (fCbmEvents == nullptr) {
    LOG(info) << GetName() << "::Init() CbmEvent NOT found \n";
  }
  else {
    LOG(info) << GetName() << "::Init() CbmEvent found";
  }

  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();
  if (!fDigiMan->IsPresent(ECbmModuleId::kRich)) Fatal("CbmRichMCbmHitProducer::Init", "No RichDigi array!");

  fRichHits = new TClonesArray("CbmRichHit");
  manager->Register("RichHit", "RICH", fRichHits, IsOutputBranchPersistent("RichHit"));

  InitMapping();

  for (auto& a : fICD_offset_read)
    a = 0.;
  if (fDoICD) read_ICD(fICD_offset_read, 0);

#if HAVE_ONNXRUNTIME
  if (fUseDenoiseNN) {
    fDenoiseCnn = std::make_unique<CbmRichMCbmDenoiseCnn>();
    fDenoiseCnn->Init();
    fDenoiseCnn->SetClassificationThreshold(fDenoiseCnnThreshold);
    LOG(info) << "CbmRichMCbmHitProducer: Initialized CbmRichMCbmDenoiseCnn";
  }
#endif

  return kSUCCESS;
}

void CbmRichMCbmHitProducer::InitMapping()
{

  string line;
  ifstream file(fMappingFile);
  if (!file.is_open()) {
    std::cout << "<CbmRichMCbmHitProducer::InitMapping>: Unable to open mapping file:" << fMappingFile.c_str()
              << std::endl;
  }

  fRichMapping.clear();

  while (getline(file, line)) {

    istringstream iss(line);
    vector<std::string> results(istream_iterator<string>{iss}, std::istream_iterator<string>());
    if (results.size() != 8) continue;

    CbmRichMCbmMappingData data;
    data.fTrbId   = stoi(results[0], nullptr, 16);
    data.fChannel = stoi(results[1]);
    data.fX       = stod(results[6]);
    data.fY       = stod(results[7]);
    data.fZ       = 348.;

    data.fX -= 6.3;  //Shift by 1Pmt + PmtGap + 1cm

    Int_t adr = ((data.fTrbId << 16) | (data.fChannel & 0x00FF));

    // cout <<  data.fTrbId << " " << data.fChannel << " " << data.fX << " " << data.fY << " " << adr << endl;

    fRichMapping[adr] = data;
  }
  file.close();

  //cout << "Mapping size:" << fRichMapping.size() <<endl;
}

void CbmRichMCbmHitProducer::Exec(Option_t* /*option*/)
{
  fNofDigis = 0;
  TStopwatch timer;
  timer.Start();
  fRichHits->Delete();

  // if CbmEvent does not exist then process standard event.
  // if CbmEvent exists then proceed all events in time slice.
  Int_t nUnits = fCbmEvents ? fCbmEvents->GetEntriesFast() : 1;
  if (fCbmEvents) fNofEvents += nUnits;
  for (Int_t iUnit = 0; iUnit < nUnits; iUnit++) {
    CbmEvent* event = fCbmEvents ? static_cast<CbmEvent*>(fCbmEvents->At(iUnit)) : nullptr;
    ProcessData(event);
  }
  timer.Stop();
  fTotalTime += timer.RealTime();

  std::stringstream logOut;
  logOut << setw(20) << left << GetName() << "[";
  logOut << fixed << setw(8) << setprecision(1) << right << timer.RealTime() * 1000. << " ms] ";
  logOut << "TS " << fNofTs;
  if (fCbmEvents) logOut << ", events " << nUnits;
  logOut << ", digis " << fNofDigis;
  logOut << ", hits " << fRichHits->GetEntriesFast();
  LOG(info) << logOut.str();
  fTotalNofDigis += fNofDigis;
  fTotalNofHits += fRichHits->GetEntriesFast();
  fNofTs++;
}

void CbmRichMCbmHitProducer::ProcessData(CbmEvent* event)
{
  TStopwatch timer;
  timer.Start();
  if (event != NULL) {
    LOG(debug) << "CbmRichMCbmHitProducer CbmEvent mode. CbmEvent # " << event->GetNumber();
    Int_t nofDigis = event->GetNofData(ECbmDataType::kRichDigi);
    //LOG(info) << "nofDigis: " << nofDigis;

    fNofHits = 0;
    for (Int_t iDigi = 0; iDigi < nofDigis; iDigi++) {
      Int_t digiIndex = event->GetIndex(ECbmDataType::kRichDigi, iDigi);
      ProcessDigi(event, digiIndex);
    }
    LOG(debug) << "nofDigis: " << nofDigis << "\t\t "
               << "nofHits : " << fNofHits;
    fNofHits = 0;
  }
  else {
    for (Int_t iDigi = 0; iDigi < fDigiMan->GetNofDigis(ECbmModuleId::kRich); iDigi++) {
      ProcessDigi(event, iDigi);
    }
  }
  timer.Stop();
  fHitProducerTime += timer.RealTime();

// Process all hits with DenoiseNN, setting value RichHit.fIsNoiseNN
#if HAVE_ONNXRUNTIME
  if (fUseDenoiseNN) {
    timer.Start();
    fDenoiseCnn->Process(event, fRichHits);
    timer.Stop();
    fDenoiseCnnTime += timer.RealTime();
  }
#endif
}

void CbmRichMCbmHitProducer::ProcessDigi(CbmEvent* event, Int_t digiIndex)
{
  const CbmRichDigi* digi = fDigiMan->Get<CbmRichDigi>(digiIndex);
  if (digi == nullptr) return;
  if (digi->GetAddress() < 0) return;
  Int_t DiRICH_Add = (digi->GetAddress() >> 16) & 0xFFFF;
  if ((0x7901 <= DiRICH_Add && DiRICH_Add <= 0x7905) || (0x9991 <= DiRICH_Add && DiRICH_Add <= 0x9998)) {
    // TRBaddresses 0x7901 to 0x7905 are for FSD/NCAL
    // TRBaddresses 0x9991 to 0x9998 are for PASTA
    return;
  }
  fNofDigis++;
  if (isInToT(digi->GetToT())) {
    TVector3 posPoint;
    CbmRichPixelData* data = CbmRichDigiMapManager::GetInstance().GetPixelDataByAddress(digi->GetAddress());

    if (data != nullptr) {
      posPoint.SetXYZ(data->fX, data->fY, data->fZ);
    }
    else {
      LOG(info) << "CbmRichMCbmHitProducer: No Node for 0x" << std::hex << digi->GetAddress() << std::dec
                << " found. Using ASCII File! ";
      CbmRichMCbmMappingData dataAscii = fRichMapping[digi->GetAddress()];

      /* if (dataAscii == NULL) {
               LOG(error) << "CbmRichMCbmHitProducer: No Position for "<<digi->GetAddress()<<"found! ";
               return;
           }*/
      posPoint.SetXYZ(dataAscii.fX, dataAscii.fY, dataAscii.fZ);
    }

    //std::cout<<std::hex<<digi->GetAddress()<<std::dec<<"    "<<data->fX<<"   "<<data->fY<<"   "<<data->fZ<<std::endl;
    if (!RestrictToAcc(posPoint)) return;
    if (!RestrictToFullAcc(posPoint)) return;
    if (!RestrictToAerogelAccDec2019(posPoint)) return;
    AddHit(event, posPoint, digi, digiIndex, data->fPmtId);
  }
}


void CbmRichMCbmHitProducer::AddHit(CbmEvent* event, TVector3& posHit, const CbmRichDigi* digi, Int_t index,
                                    Int_t PmtId)
{
  Int_t nofHits = fRichHits->GetEntriesFast();

  int32_t DiRICH_Addr = digi->GetAddress();
  unsigned int addr   = (((DiRICH_Addr >> 24) & 0xF) * 18 * 32) + (((DiRICH_Addr >> 20) & 0xF) * 2 * 32)
                      + (((DiRICH_Addr >> 16) & 0xF) * 32) + ((DiRICH_Addr & 0xFFFF) - 0x1);

  new ((*fRichHits)[nofHits]) CbmRichHit();
  CbmRichHit* hit = (CbmRichHit*) fRichHits->At(nofHits);
  hit->SetPosition(posHit);
  hit->SetDx(fHitError);
  hit->SetDy(fHitError);
  hit->SetRefId(index);
  if (fDoICD) {
    hit->SetTime(digi->GetTime() - fICD_offset_read.at(addr));
  }
  else {
    hit->SetTime(digi->GetTime());
  }

  hit->SetToT(digi->GetToT());
  hit->SetAddress(digi->GetAddress());
  hit->SetPmtId(PmtId);
  // Not Implemented in Digi: hit->SetPmtId(digi->GetPmtId());

  if (event != NULL) {
    event->AddData(ECbmDataType::kRichHit, nofHits);
    fNofHits++;
  }
}


void CbmRichMCbmHitProducer::Finish()
{
  fRichHits->Clear();
  std::cout << std::endl;
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Run summary";
  LOG(info) << "Time slices     : " << fNofTs;
  LOG(info) << "Digis     / TS  : " << fixed << setprecision(2) << fTotalNofDigis / Double_t(fNofTs);
  LOG(info) << "Hits      / TS  : " << fixed << setprecision(2) << fTotalNofHits / Double_t(fNofTs);
  LOG(info) << "Time      / TS  : " << fixed << setprecision(2) << 1000. * fTotalTime / Double_t(fNofTs) << " ms";
  if (fCbmEvents) {
    LOG(info) << "Events          : " << fNofEvents;
    LOG(info) << "Events    / TS  : " << fixed << setprecision(2) << fNofEvents / Double_t(fNofTs);
    if (fNofEvents > 0) {
      LOG(info) << "Digis     / ev  : " << fixed << setprecision(2) << fTotalNofDigis / Double_t(fNofEvents);
      LOG(info) << "Hits      / ev  : " << fixed << setprecision(2) << fTotalNofHits / Double_t(fNofEvents);
      LOG(info) << "Time      / ev  : " << fixed << setprecision(2) << 1000. * fTotalTime / Double_t(fNofEvents)
                << " ms";
    }
  }

  TString eventOrTsStr    = fCbmEvents && fNofEvents > 0 ? "ev: " : "TS: ";
  Double_t eventOrTsValue = Double_t(fCbmEvents && fNofEvents > 0 ? fNofEvents : fNofTs);
  if (fTotalTime != 0.) {
    LOG(info) << "===== Time by task (real time) ======";
    LOG(info) << "HitProducer        / " << eventOrTsStr << fixed << setprecision(2) << setw(9) << right
              << 1000. * fHitProducerTime / eventOrTsValue << " ms [" << setw(5) << right
              << 100 * fHitProducerTime / fTotalTime << " %]";
#if HAVE_ONNXRUNTIME
    if (fUseDenoiseNN)
      LOG(info) << "Denoise NN         / " << eventOrTsStr << fixed << setprecision(2) << setw(9) << right
                << 1000. * fDenoiseCnnTime / eventOrTsValue << " ms [" << setw(5) << right
                << 100 * fDenoiseCnnTime / fTotalTime << " %]";
#endif
  }
  LOG(info) << "=====================================\n";
}


bool CbmRichMCbmHitProducer::isInToT(const double ToT)
{

  if (!fDoToT) return true;

  if ((ToT > fToTLimitLow) && (ToT < fToTLimitHigh)) {
    return true;
  }
  else {
    return false;
  }
}


bool CbmRichMCbmHitProducer::RestrictToAcc(TVector3& pos)
{
  Double_t x = pos.X();
  Double_t y = pos.Y();

  return this->RestrictToAcc(x, y);
}

bool CbmRichMCbmHitProducer::RestrictToAcc(Double_t x, Double_t y)
{  //check if Track is in mRICH acceptance
  if (fRestrictToAcc == false) return true;
  bool inside = false;
  if (x >= -6.25 && x <= -1.05) {
    // left part of mRICH
    if (y >= 8 && y <= 15.9) {
      inside = true;
    }
  }
  else if (x > -1.05 && x <= 4.25) {
    //right part
    if (y >= 8 && y <= 13.2) {
      inside = true;
    }
  }

  return inside;
}

bool CbmRichMCbmHitProducer::RestrictToFullAcc(TVector3& pos)
{
  Double_t x = pos.X();
  Double_t y = pos.Y();

  return this->RestrictToFullAcc(x, y);
}

bool CbmRichMCbmHitProducer::RestrictToFullAcc(Double_t x, Double_t y)
{  //check if Track is in mRICH acceptance
  if (fRestrictToFullAcc == false) return true;
  bool inside = false;
  if (x >= -16.85 && x <= 4.25) {  //TODO:check values
    // left part of mRICH
    if (y >= -23.8 && y <= 23.8) {
      inside = true;
    }
  }

  return inside;
}

bool CbmRichMCbmHitProducer::RestrictToAerogelAccDec2019(TVector3& pos)
{
  Double_t x = pos.X();
  Double_t y = pos.Y();

  return this->RestrictToAerogelAccDec2019(x, y);
}

bool CbmRichMCbmHitProducer::RestrictToAerogelAccDec2019(Double_t x, Double_t y)
{  //check if Track is in mRICH acceptance
  if (fRestrictToAerogelAccDec2019 == false) return true;
  //bool inside = false;

  if (y > 13.5) return false;
  if (y > 8.0 && x < 12.5) return false;

  return true;
}

void CbmRichMCbmHitProducer::read_ICD(std::array<Double_t, 2304>& ICD_offsets, unsigned int iteration)
{
  std::string line;
  std::string filename = ("" == fIcdFilenameBase ? "icd_offset_it" : fIcdFilenameBase);
  filename += Form("_%u.data", iteration);
  std::ifstream icd_file(filename);
  unsigned int lineCnt = 0;
  if (icd_file.is_open()) {
    while (getline(icd_file, line)) {
      if (line[0] == '#') continue;  // just a comment
      std::istringstream iss(line);
      unsigned int addr = 0;
      Double_t value;
      if (!(iss >> addr >> value)) {
        LOG(info) << "A Problem accured in line " << lineCnt;
        break;
      }  // error
      lineCnt++;
      ICD_offsets.at(addr) += value;
    }
    icd_file.close();
    LOG(info) << "Loaded inter channel delay file " << filename << " for RICH.";
  }
  else {
    LOG(info) << "Unable to open inter channel delay file " << filename;
  }
}

ClassImp(CbmRichMCbmHitProducer)
