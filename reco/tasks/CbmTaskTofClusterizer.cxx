/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer], Pierre-Alain Loizeau, Norbert Herrmann */

#include "CbmTaskTofClusterizer.h"

// TOF Classes and includes
#include "CbmBmonDigi.h"  // in cbmdata/bmon
#include "CbmDigiManager.h"
#include "CbmEvent.h"
#include "CbmMatch.h"
#include "CbmTofHit.h"  // in cbmdata/tof
#include "CbmTsEventHeader.h"

// FAIR classes and includes
#include "FairRootFileSink.h"
#include "FairRootManager.h"
#include "FairRunAna.h"
#include "FairRuntimeDb.h"

#include <Logger.h>

// ROOT Classes and includes
#include "TClonesArray.h"
#include "TStopwatch.h"

// C++ Classes and includes
#include "compat/Filesystem.h"
#include "yaml/Yaml.h"

#include <iomanip>
#include <vector>


/************************************************************************************/
CbmTaskTofClusterizer::CbmTaskTofClusterizer() : CbmTaskTofClusterizer("TestbeamClusterizer", 0, 0) {}

CbmTaskTofClusterizer::CbmTaskTofClusterizer(const char* name, int32_t verbose, bool writeDataInOut)
  : FairTask(TString(name), verbose)
  , fTsHeader(NULL)
  , fDigiMan(nullptr)
  , fEventsColl(nullptr)
  , fbWriteHitsInOut(writeDataInOut)
  , fbWriteDigisInOut(writeDataInOut)
  , fTofHitsColl(NULL)
  , fTofDigiMatchColl(NULL)
  , fTofHitsCollOut(NULL)
  , fTofDigiMatchCollOut(NULL)
  , fiNbHits(0)
  , fdEvent(0)
  , fbSwapChannelSides(false)
  , fiOutputTreeEntry(0)
  , fiFileIndex(0)
{
}

CbmTaskTofClusterizer::~CbmTaskTofClusterizer() {}

/************************************************************************************/
// FairTasks inherited functions
InitStatus CbmTaskTofClusterizer::Init()
{
  LOG(info) << "CbmTaskTofClusterizer initializing... expect Digis in ns units! ";
  if (false == RegisterInputs()) return kFATAL;
  if (false == RegisterOutputs()) return kFATAL;
  if (false == InitAlgos()) return kFATAL;
  return kSUCCESS;
}


void CbmTaskTofClusterizer::Exec(Option_t* option)
{
  if (fTofCalDigiVecOut) fTofCalDigiVecOut->clear();
  if (fEventsColl) {
    if (NULL != fTsHeader)
      LOG(info) << "New Ts " << iNbTs << ", size " << fEventsColl->GetSize() << " at " << fTsHeader->GetTsStartTime()
                << " with " << fEventsColl->GetEntriesFast() << " events, " << fDigiMan->GetNofDigis(ECbmModuleId::kTof)
                << " TOF digis + " << fDigiMan->GetNofDigis(ECbmModuleId::kBmon) << " Bmon digis ";

    TStopwatch timerTs;
    timerTs.Start();

    iNbTs++;
    fiHitStart          = 0;
    int32_t iNbHits     = 0;
    int32_t iNbCalDigis = 0;
    fTofDigiMatchCollOut->Delete();  // costly, FIXME
    fTofHitsCollOut->Delete();       // costly, FIXME

    for (int32_t iEvent = 0; iEvent < fEventsColl->GetEntriesFast(); iEvent++) {
      CbmEvent* tEvent = dynamic_cast<CbmEvent*>(fEventsColl->At(iEvent));
      fTofDigiVec.clear();
      LOG(debug) << "TS event " << iEvent << " with " << tEvent->GetNofData(ECbmDataType::kBmonDigi) << " Bmon and "
                 << tEvent->GetNofData(ECbmDataType::kTofDigi) << " Tof digis ";

      for (size_t iDigi = 0; iDigi < tEvent->GetNofData(ECbmDataType::kBmonDigi); iDigi++) {
        int32_t iDigiIndex = static_cast<int32_t>(tEvent->GetIndex(ECbmDataType::kBmonDigi, iDigi));
        CbmTofDigi tDigi(fDigiMan->Get<CbmBmonDigi>(iDigiIndex));
        if (tDigi.GetType() != 5) {
          tDigi.SetAddress(0, 0, 0, 0, 5);  // convert to Tof Address
        }
        if (tDigi.GetSide() == 1) {  // HACK for May22 setup
          tDigi.SetAddress(tDigi.GetSm(), tDigi.GetRpc(), tDigi.GetChannel() + 6, 0, tDigi.GetType());
        }
        fTofDigiVec.push_back(tDigi);
      }
      for (size_t iDigi = 0; iDigi < tEvent->GetNofData(ECbmDataType::kTofDigi); iDigi++) {
        int32_t iDigiIndex      = static_cast<int32_t>(tEvent->GetIndex(ECbmDataType::kTofDigi, iDigi));
        const CbmTofDigi* tDigi = fDigiMan->Get<CbmTofDigi>(iDigiIndex);
        fTofDigiVec.push_back(CbmTofDigi(*tDigi));
      }

      ExecEvent(option);

      // --- In event-by-event mode: copy caldigis, hits and matches to output array and register them to event
      uint uDigi0 = fTofCalDigiVecOut->size();  //starting index of current event
      LOG(debug) << "Append " << fTofCalDigiVec->size() << " CalDigis to event " << tEvent->GetNumber();
      for (uint32_t index = 0; index < fTofCalDigiVec->size(); index++) {
        CbmTofDigi* tDigi = &(fTofCalDigiVec->at(index));
        tEvent->AddData(ECbmDataType::kTofCalDigi, iNbCalDigis);
        fTofCalDigiVecOut->push_back(CbmTofDigi(*tDigi));
        iNbCalDigis++;
      }

      int iHit0 = iNbHits;  //starting index of current event
      LOG(debug) << "Assign " << fTofHitsColl->GetEntriesFast() << " hits to event " << tEvent->GetNumber();
      for (int32_t index = 0; index < fTofHitsColl->GetEntriesFast(); index++) {
        CbmTofHit* pHit = (CbmTofHit*) fTofHitsColl->At(index);
        new ((*fTofHitsCollOut)[iNbHits]) CbmTofHit(*pHit);
        tEvent->AddData(ECbmDataType::kTofHit, iNbHits);

        CbmMatch* pDigiMatch = (CbmMatch*) fTofDigiMatchColl->At(index);

        // update content of match object to TS array
        CbmMatch* mDigiMatch = new CbmMatch();
        for (int32_t iLink = 0; iLink < pDigiMatch->GetNofLinks(); iLink++) {
          CbmLink Link = pDigiMatch->GetLink(iLink);
          Link.SetIndex(Link.GetIndex() + uDigi0);
          mDigiMatch->AddLink(Link);
        }

        new ((*fTofDigiMatchCollOut)[iNbHits]) CbmMatch(*mDigiMatch);
        delete mDigiMatch;
        iNbHits++;
      }

      fiHitStart += iNbHits - iHit0;
      fTofDigiVec.clear();
      fTofCalDigiVec->clear();
      fTofHitsColl->Delete();       //Clear("C");
      fTofDigiMatchColl->Delete();  //Clear("C");
    }

    /// PAL: add TS statistics for monitoring and perf evaluation
    timerTs.Stop();
    LOG(debug) << GetName() << "::Exec: real time=" << timerTs.RealTime() << " CPU time=" << timerTs.CpuTime();
    fProcessTime += timerTs.RealTime();
    fuNbDigis += fDigiMan->GetNofDigis(ECbmModuleId::kTof)      // TOF
                 + fDigiMan->GetNofDigis(ECbmModuleId::kBmon);  // Bmon
    fuNbHits += fiHitStart;

    std::stringstream logOut;
    logOut << std::setw(20) << std::left << GetName() << " [";
    logOut << std::fixed << std::setw(8) << std::setprecision(1) << std::right << timerTs.RealTime() * 1000. << " ms] ";
    logOut << "TS " << iNbTs;
    logOut << ", events " << fEventsColl->GetEntriesFast();
    logOut << ", hits " << fiHitStart << ", time/1k-hit " << std::setprecision(4)
           << timerTs.RealTime() * 1e6 / fiHitStart << " [ms]";
    LOG(info) << logOut.str();
  }
  else {
    // fTofDigisColl=fTofRawDigisColl;
    // (VF) This does not work here. The digi manager does not foresee to add
    // new data to the input array. So, I here copy the input digis into
    // the array fTofDigisColl. Not very efficient, but temporary only, until
    // also the internal data representations are changed to std::vectors.

    fTofDigiVec.clear();
    if (NULL != fBmonDigiVec) {  // 2022 data
      for (int32_t iDigi = 0; iDigi < (int) (fBmonDigiVec->size()); iDigi++) {
        CbmTofDigi tDigi = fBmonDigiVec->at(iDigi);
        if (tDigi.GetType() != 5)
          LOG(fatal) << "Wrong Bmon type " << tDigi.GetType() << ", Addr 0x" << std::hex << tDigi.GetAddress();
        if (tDigi.GetSide() == 1) {  // HACK for May22 setup
          tDigi.SetAddress(tDigi.GetSm(), tDigi.GetRpc(), tDigi.GetChannel() + 6, 0, tDigi.GetType());
        }
        fTofDigiVec.push_back(CbmTofDigi(tDigi));
      }
    }
    for (int32_t iDigi = 0; iDigi < fDigiMan->GetNofDigis(ECbmModuleId::kTof); iDigi++) {
      const CbmTofDigi* tDigi = fDigiMan->Get<CbmTofDigi>(iDigi);
      fTofDigiVec.push_back(CbmTofDigi(*tDigi));

      //if (iDigi == 10000) break;  // Only use 10000 digis for now. D.Smith
    }
    ExecEvent(option);
  }
}

void CbmTaskTofClusterizer::ExecEvent(Option_t* /*option*/)
{
  // Clear output arrays
  fTofCalDigiVec->clear();
  fTofHitsColl->Delete();  // Computationally costly!, but hopefully safe
  fTofDigiMatchColl->Delete();
  FairRootFileSink* sink = (FairRootFileSink*) FairRootManager::Instance()->GetSink();
  if (sink) {
    fiOutputTreeEntry = sink->GetOutTree()->GetEntries();
  }

  fiNbHits = 0;

  BuildClusters();
}

/************************************************************************************/
void CbmTaskTofClusterizer::Finish()
{
  LOG(info) << "Finish with " << fdEvent << " processed events";

  /// PAL: add run statistics for monitoring and perf evaluation
  LOG(info) << "=====================================";
  LOG(info) << GetName() << ": Finish run";
  LOG(info) << GetName() << ": Run summary ";
  LOG(info) << GetName() << ": Processing time      : " << std::fixed << std::setprecision(3) << fProcessTime;
  LOG(info) << GetName() << ": Nr of events         : " << fdEvent;
  LOG(info) << GetName() << ": Nr of input digis    : " << fuNbDigis;
  LOG(info) << GetName() << ": Nr of produced hits  : " << fuNbHits;
  LOG(info) << GetName() << ": Nr of hits / event   : " << std::fixed << std::setprecision(2)
            << (fdEvent > 0 ? fuNbHits / fdEvent : 0);
  LOG(info) << GetName() << ": Nr of hits / digis   : " << std::fixed << std::setprecision(2)
            << (fuNbDigis > 0 ? fuNbHits / (double) fuNbDigis : 0);
  LOG(info) << "=====================================";
}

void CbmTaskTofClusterizer::Finish(double calMode)
{
  SetCalMode(calMode);
  Finish();
}

/************************************************************************************/
// Functions common for all clusters approximations
bool CbmTaskTofClusterizer::RegisterInputs()
{
  FairRootManager* fManager = FairRootManager::Instance();

  if (NULL == fManager) {
    LOG(error) << "CbmTaskTofClusterizer::RegisterInputs => Could not find "
                  "FairRootManager!!!";
    return false;
  }

  fEventsColl = dynamic_cast<TClonesArray*>(fManager->GetObject("Event"));
  if (NULL == fEventsColl) fEventsColl = dynamic_cast<TClonesArray*>(fManager->GetObject("CbmEvent"));

  if (NULL == fEventsColl) LOG(info) << "CbmEvent not found in input file, assume eventwise input";

  fDigiMan = CbmDigiManager::Instance();
  fDigiMan->Init();
  if (!fDigiMan->IsPresent(ECbmModuleId::kTof)) {
    LOG(error) << GetName() << ": No Tof digi input!";
    return false;
  }
  if (fDigiMan->IsPresent(ECbmModuleId::kBmon)) {
    LOG(info) << GetName() << ": separate Bmon digi input!";
    //fBmonDigiVec = fManager->InitObjectAs<std::vector<CbmTofDigi> const*>("TzdDigi");
  }
  else {
    LOG(info) << "No separate Bmon digi input found.";
  }  // if( ! fBmonDigiVec )

  fTsHeader = fManager->InitObjectAs<CbmTsEventHeader const*>("EventHeader.");  //for data
  if (NULL == fTsHeader) {
    LOG(info) << "CbmTaskTofClusterizer::RegisterInputs => Could not get TsHeader Object";
  }

  return true;
}
bool CbmTaskTofClusterizer::RegisterOutputs()
{
  FairRootManager* rootMgr = FairRootManager::Instance();
  rootMgr->InitSink();

  fTofCalDigiVec    = new std::vector<CbmTofDigi>();
  fTofHitsColl      = new TClonesArray("CbmTofHit", 100);
  fTofDigiMatchColl = new TClonesArray("CbmMatch", 100);

  if (NULL == fEventsColl) {
    // Flag check to control whether digis are written in output root file
    rootMgr->RegisterAny("TofCalDigi", fTofCalDigiVec, fbWriteDigisInOut);

    // Flag check to control whether digis are written in output root file
    rootMgr->Register("TofHit", "Tof", fTofHitsColl, fbWriteHitsInOut);
    fTofHitsCollOut = fTofHitsColl;

    rootMgr->Register("TofHitCalDigiMatch", "Tof", fTofDigiMatchColl, fbWriteHitsInOut);
    LOG(info) << Form("Register DigiMatch in TS mode at %p with %d", fTofDigiMatchColl, (int) fbWriteHitsInOut);
    fTofDigiMatchCollOut = fTofDigiMatchColl;
  }
  else {  // CbmEvent - mode
    fTofCalDigiVecOut    = new std::vector<CbmTofDigi>();
    fTofHitsCollOut      = new TClonesArray("CbmTofHit", 10000);
    fTofDigiMatchCollOut = new TClonesArray("CbmMatch", 10000);

    rootMgr->RegisterAny("TofCalDigi", fTofCalDigiVecOut, fbWriteDigisInOut);
    rootMgr->Register("TofHit", "Tof", fTofHitsCollOut, fbWriteHitsInOut);
    rootMgr->Register("TofHitCalDigiMatch", "Tof", fTofDigiMatchCollOut, fbWriteHitsInOut);
    LOG(info) << Form("Register DigiMatch in event mode at %p with %d ", fTofDigiMatchCollOut, (int) fbWriteHitsInOut);
  }
  return true;
}


bool CbmTaskTofClusterizer::InitAlgos()
{
  // Read hitfinder parameters and initialize algo
  fAlgo = std::make_unique<cbm::algo::tof::Hitfind>(
    cbm::algo::yaml::ReadFromFile<cbm::algo::tof::HitfindSetup>("TofHitfinderPar.yaml"));

  // Read calibration parameters initialize algo
  fCalibrate = std::make_unique<cbm::algo::tof::Calibrate>(
    cbm::algo::yaml::ReadFromFile<cbm::algo::tof::CalibrateSetup>("TofCalibratePar.yaml"));

  return true;
}


/************************************************************************************/
bool CbmTaskTofClusterizer::BuildClusters()
{
  if (fTofDigiVec.empty()) {
    LOG(info) << " No RawDigis defined ! Check! ";
    return false;
  }
  LOG(info) << "Build clusters from " << fTofDigiVec.size() << " digis in event " << fdEvent + 1;

  if (bAddBeamCounterSideDigi) {
    // Duplicate type "5" - digis
    for (size_t iDigInd = 0; iDigInd < fTofDigiVec.size(); iDigInd++) {
      CbmTofDigi* pDigi = &(fTofDigiVec.at(iDigInd));
      if (pDigi->GetType() == 5) {  // || pDigi->GetType() == 8) {
        if (pDigi->GetSide() == 1) {
          bAddBeamCounterSideDigi = false;  // disable for current data set
          LOG(info) << "Start counter digi duplication disabled";
          break;
        }
        fTofDigiVec.push_back(CbmTofDigi(*pDigi));
        CbmTofDigi* pDigiN = &(fTofDigiVec.back());
        pDigiN->SetAddress(pDigi->GetSm(), pDigi->GetRpc(), pDigi->GetChannel(), (0 == pDigi->GetSide()) ? 1 : 0,
                           pDigi->GetType());
        LOG(debug) << "Duplicated digi " << fTofDigiVec.size() << " with address 0x" << std::hex
                   << pDigiN->GetAddress();
      }
    }
  }

  //Calibrate digis
  auto [caldigis, calmonitor] = (*fCalibrate)(fTofDigiVec);
  *fTofCalDigiVec             = std::move(caldigis);
  LOG(info) << calmonitor.print();

  //Call cluster finder
  auto [clusters, monitor, indices] = (*fAlgo)(*fTofCalDigiVec);
  LOG(info) << monitor.print();

  //Store hits and match
  size_t indexOffset = 0;
  for (auto const& cluster : clusters.Data()) {
    const int32_t hitIndex = fTofHitsColl->GetEntriesFast();
    TVector3 hitpos        = TVector3(cluster.hitPos.X(), cluster.hitPos.Y(), cluster.hitPos.Z());
    TVector3 hiterr        = TVector3(cluster.hitPosErr.X(), cluster.hitPosErr.Y(), cluster.hitPosErr.Z());
    new ((*fTofHitsColl)[hitIndex])
      CbmTofHit(cluster.address, hitpos, hiterr, fiNbHits, cluster.hitTime, cluster.hitTimeErr, cluster.numChan() * 2,
                int32_t(cluster.weightsSum * 10.));

    fiNbHits++;
    //if (event) event->AddData(ECbmDataType::kTofHit, hitIndex);

    CbmMatch* digiMatch = new ((*fTofDigiMatchColl)[hitIndex]) CbmMatch();
    for (int32_t i = 0; i < cluster.numChan() * 2; i++) {
      size_t digiInd = indices.at(indexOffset + i);
      double tot     = fTofCalDigiVec->at(digiInd).GetTot();
      digiMatch->AddLink(CbmLink(tot, digiInd, fiOutputTreeEntry, fiFileIndex));
    }
    indexOffset += cluster.numChan() * 2;
  }

  fdEvent++;
  return true;
}
