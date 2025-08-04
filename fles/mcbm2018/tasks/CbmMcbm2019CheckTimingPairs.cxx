/* Copyright (C) 2020-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#include "CbmMcbm2019CheckTimingPairs.h"

#include "CbmDigiManager.h"
#include "CbmFlesHistosTools.h"

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
CbmMcbm2019CheckTimingPairs::CbmMcbm2019CheckTimingPairs() : FairTask("CbmMcbm2019CheckTimingPairs") {}

// ---- Destructor ----------------------------------------------------
CbmMcbm2019CheckTimingPairs::~CbmMcbm2019CheckTimingPairs() {}

// ----  Initialisation  ----------------------------------------------
void CbmMcbm2019CheckTimingPairs::SetParContainers()
{
  // Load all necessary parameter containers from the runtime data base
  /*
  FairRunAna* ana = FairRunAna::Instance();
  FairRuntimeDb* rtdb=ana->GetRuntimeDb();

  <CbmMcbm2019CheckTimingPairsDataMember> = (<ClassPointer>*)
    (rtdb->getContainer("<ContainerName>"));
  */
}

// ---- Init ----------------------------------------------------------
InitStatus CbmMcbm2019CheckTimingPairs::Init()
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

void CbmMcbm2019CheckTimingPairs::CreateHistos()
{

  Double_t dHistLim = kdDefaultTimeWin + kdClockCycle / 2.0;
  for (UInt_t uDetA = 0; uDetA < fvsDetectors.size(); ++uDetA) {
    for (UInt_t uDetB = uDetA; uDetB < fvsDetectors.size(); ++uDetB) {
      std::string sName  = Form("hDt%s_Vs_%s", fvsDetectors[uDetA].c_str(), fvsDetectors[uDetB].c_str());
      std::string sTitle = Form("Time diff to Bmon for %s VS for %s; Dt %s [ns]; "
                                "dT %s [ns]; Possible pairs []",
                                fvsDetectors[uDetA].c_str(), fvsDetectors[uDetB].c_str(), fvsDetectors[uDetA].c_str(),
                                fvsDetectors[uDetB].c_str());
      fhDtADtB.push_back(new TH2D(sName.c_str(), sTitle.c_str(), kuNbBinsDefault + 1, -dHistLim, dHistLim,
                                  kuNbBinsDefault + 1, -dHistLim, dHistLim));
    }  // for( UInt_t uDetB = uDetA; uDetB < fvsDetectors.size(); ++uDetB )
  }    // for( UInt_t uDetA = 0; uDetA < fvsDetectors.size(); ++uDetA )


  /// Register the histos in the HTTP server
  FairRunOnline* run = FairRunOnline::Instance();
  if (run) {
    THttpServer* server = run->GetHttpServer();
    if (nullptr != server) {
      for (UInt_t uPair = 0; uPair < fhDtADtB.size(); ++uPair) {
        server->Register("/PairTiming", fhDtADtB[uPair]);
      }  // for( UInt_t uPair = 0; uPar < fhDtADtB.size(); ++uPair )
    }    // if( nullptr != server )
  }      // if( run )
}
// ---- ReInit  -------------------------------------------------------
InitStatus CbmMcbm2019CheckTimingPairs::ReInit() { return kSUCCESS; }

// ---- Exec ----------------------------------------------------------
void CbmMcbm2019CheckTimingPairs::Exec(Option_t* /*option*/)
{
  LOG(debug) << "executing TS " << fNrTs;

  if (0 < fNrTs && 0 == fNrTs % 1000) LOG(info) << Form("Processing TS %6d", fNrTs);

  /// Fill buffers of hits in correlation to Bmon
  UInt_t uNbBmonDigis = 0;
  if (fBmonDigiVector) uNbBmonDigis = fBmonDigiVector->size();
  else if (fBmonDigiArray)
    uNbBmonDigis = fBmonDigiArray->GetEntriesFast();

  /*
   UInt_t uNbStsDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kSts);
   UInt_t uNbMuchDigis = fDigiMan->GetNofDigis(ECbmModuleId::kMuch);
   UInt_t uNbTrdDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kTrd);
   UInt_t uNbTofDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kTof);
   UInt_t uNbRichDigis = fDigiMan->GetNofDigis(ECbmModuleId::kRich);
   UInt_t uNbPsdDigis  = fDigiMan->GetNofDigis(ECbmModuleId::kPsd);
*/

  fuNbDigisWithCoincBmon = 0;
  for (UInt_t uBmonDigi = 0; uBmonDigi < uNbBmonDigis; ++uBmonDigi) {
    const CbmTofDigi* pDigiBmon = nullptr;
    if (fBmonDigiVector) pDigiBmon = &(fBmonDigiVector->at(uBmonDigi));
    else if (fBmonDigiArray)
      pDigiBmon = dynamic_cast<CbmTofDigi*>(fBmonDigiArray->At(uBmonDigi));
    assert(pDigiBmon);

    //      UInt_t uChannel = pDigiBmon->GetChannel();
    Double_t dTimeBmon = pDigiBmon->GetTime();

    fuNbCoincDigisSts  = 0;
    fuNbCoincDigisMuch = 0;
    fuNbCoincDigisTrd  = 0;
    fuNbCoincDigisTof  = 0;
    fuNbCoincDigisRich = 0;
    fuNbCoincDigisPsd  = 0;
    for (UInt_t uIndexDet = 0; uIndexDet < fvsDetectors.size(); ++uIndexDet) {
      if ("STS" == fvsDetectors[uIndexDet]) {
        fvuPrevBmonFirstDigiDet[uIndexDet] = FillCorrBuffer<CbmStsDigi>(
          dTimeBmon, fvuPrevBmonFirstDigiDet[uIndexDet], -fdStsTimeWin, fdStsTimeWin, fvDigisSts, ECbmModuleId::kSts);
      }  // if( "STS" == fvsDetectors[ uIndexDet ] )
      else if ("MUCH" == fvsDetectors[uIndexDet]) {
        fvuPrevBmonFirstDigiDet[uIndexDet] =
          FillCorrBuffer<CbmMuchBeamTimeDigi>(dTimeBmon, fvuPrevBmonFirstDigiDet[uIndexDet], -fdMuchTimeWin,
                                              fdMuchTimeWin, fvDigisMuch, ECbmModuleId::kMuch);
      }  // else if( "MUCH" == fvsDetectors[ uIndexDet ] )
      else if ("TRD" == fvsDetectors[uIndexDet]) {
        fvuPrevBmonFirstDigiDet[uIndexDet] = FillCorrBuffer<CbmTrdDigi>(
          dTimeBmon, fvuPrevBmonFirstDigiDet[uIndexDet], -fdTrdTimeWin, fdTrdTimeWin, fvDigisTrd, ECbmModuleId::kTrd);
      }  // else if( "TRD" == fvsDetectors[ uIndexDet ] )
      else if ("TOF" == fvsDetectors[uIndexDet]) {
        fvuPrevBmonFirstDigiDet[uIndexDet] = FillCorrBuffer<CbmTofDigi>(
          dTimeBmon, fvuPrevBmonFirstDigiDet[uIndexDet], -fdTofTimeWin, fdTofTimeWin, fvDigisTof, ECbmModuleId::kTof);
      }  // else if( "TOF" == fvsDetectors[ uIndexDet ] )
      else if ("RICH" == fvsDetectors[uIndexDet]) {
        fvuPrevBmonFirstDigiDet[uIndexDet] =
          FillCorrBuffer<CbmRichDigi>(dTimeBmon, fvuPrevBmonFirstDigiDet[uIndexDet], -fdRichTimeWin, fdRichTimeWin,
                                      fvDigisRich, ECbmModuleId::kRich);
      }  // else if( "RICH" == fvsDetectors[ uIndexDet ] )
      else if ("PSD" == fvsDetectors[uIndexDet]) {
        fvuPrevBmonFirstDigiDet[uIndexDet] = FillCorrBuffer<CbmPsdDigi>(
          dTimeBmon, fvuPrevBmonFirstDigiDet[uIndexDet], -fdPsdTimeWin, fdPsdTimeWin, fvDigisPsd, ECbmModuleId::kPsd);
      }  // else if( "PSD" == fvsDetectors[ uIndexDet ] )
      else
        LOG(fatal) << "CbmMcbm2019CheckTimingPairs => Unknown detector";
    }  // for( UInt_t uIndexDet = 0; uIndexDet < fvsDetectors.size(); ++uIndexDet )

    /// Store also the Bmon Hit if any STS or MUCH coincidence
    if (0 < fuNbCoincDigisSts || 0 < fuNbCoincDigisMuch || 0 < fuNbCoincDigisTrd || 0 < fuNbCoincDigisTof
        || 0 < fuNbCoincDigisRich || 0 < fuNbCoincDigisPsd) {
      fvDigisBmon.push_back(*pDigiBmon);
      ++fuNbDigisWithCoincBmon;

      /// Make sure we keep both vector in sync at the same size
      if (0 == fuNbCoincDigisSts) fvDigisSts.push_back(std::vector<CbmStsDigi>());
      if (0 == fuNbCoincDigisMuch) fvDigisMuch.push_back(std::vector<CbmMuchBeamTimeDigi>());
      if (0 == fuNbCoincDigisTrd) fvDigisTrd.push_back(std::vector<CbmTrdDigi>());
      if (0 == fuNbCoincDigisTof) fvDigisTof.push_back(std::vector<CbmTofDigi>());
      if (0 == fuNbCoincDigisRich) fvDigisRich.push_back(std::vector<CbmRichDigi>());
      if (0 == fuNbCoincDigisPsd) fvDigisPsd.push_back(std::vector<CbmPsdDigi>());
    }  // if( 0 < uNbCoincDigisSts || 0 < uNbCoincDigisMuch )
  }    // for( UInt_t uBmonDigi = 0; uBmonDigi < uNbBmonDigis; ++uBmonDigi )

  /// Fill plots from buffers f correlated hits
  for (UInt_t uIndexBmon = 0; uIndexBmon < fvDigisBmon.size(); ++uIndexBmon) {
    UInt_t uHistoIdx = 0;
    for (UInt_t uIndexDetA = 0; uIndexDetA < fvsDetectors.size(); ++uIndexDetA) {
      for (UInt_t uIndexDetB = uIndexDetA; uIndexDetB < fvsDetectors.size(); ++uIndexDetB) {
        if ("STS" == fvsDetectors[uIndexDetA]) {
          FillHistosInter<CbmStsDigi>(uIndexBmon, uIndexDetA, uIndexDetB, fvDigisSts[uIndexBmon], uHistoIdx);
        }  // if( "STS" == fvsDetectors[ uIndexDetA ] )
        else if ("MUCH" == fvsDetectors[uIndexDetA]) {
          FillHistosInter<CbmMuchBeamTimeDigi>(uIndexBmon, uIndexDetA, uIndexDetB, fvDigisMuch[uIndexBmon], uHistoIdx);
        }  // else if( "MUCH" == fvsDetectors[ uIndexDetA ] )
        else if ("TRD" == fvsDetectors[uIndexDetA]) {
          FillHistosInter<CbmTrdDigi>(uIndexBmon, uIndexDetA, uIndexDetB, fvDigisTrd[uIndexBmon], uHistoIdx);
        }  // else if( "TRD" == fvsDetectors[ uIndexDetA ] )
        else if ("TOF" == fvsDetectors[uIndexDetA]) {
          FillHistosInter<CbmTofDigi>(uIndexBmon, uIndexDetA, uIndexDetB, fvDigisTof[uIndexBmon], uHistoIdx);
        }  // else if( "TOF" == fvsDetectors[ uIndexDetA ] )
        else if ("RICH" == fvsDetectors[uIndexDetA]) {
          FillHistosInter<CbmRichDigi>(uIndexBmon, uIndexDetA, uIndexDetB, fvDigisRich[uIndexBmon], uHistoIdx);
        }  // else if( "RICH" == fvsDetectors[ uIndexDetA ] )
        else if ("PSD" == fvsDetectors[uIndexDetA]) {
          FillHistosInter<CbmPsdDigi>(uIndexBmon, uIndexDetA, uIndexDetB, fvDigisPsd[uIndexBmon], uHistoIdx);
        }  // else if( "PSD" == fvsDetectors[ uIndexDetA ] )
        else
          LOG(fatal) << "CbmMcbm2019CheckTimingPairs => Unknown detector";

        uHistoIdx++;
      }  // for( UInt_t uIndexDetB = uIndexDetA; uIndexDetB < fvsDetectors.size(); ++uIndexDetB )
    }    // for( UInt_t uIndexDetA = 0; uIndexDetA < fvsDetectors.size(); ++uIndexDetA )

    /// Cleanup buffers
    fvDigisSts[uIndexBmon].clear();
    fvDigisMuch[uIndexBmon].clear();
    fvDigisTrd[uIndexBmon].clear();
    fvDigisTof[uIndexBmon].clear();
    fvDigisRich[uIndexBmon].clear();
    fvDigisPsd[uIndexBmon].clear();
  }  // for( UInt_t uIndexBmon = 0; uIndexBmon < fvDigisBmon.size(); ++uIndexBmon )
  /// Cleanup buffers
  fvDigisBmon.clear();
  fvDigisSts.clear();
  fvDigisMuch.clear();
  fvDigisTrd.clear();
  fvDigisTof.clear();
  fvDigisRich.clear();
  fvDigisPsd.clear();

  fNrTs++;

  if (0 < fNrTs && 0 == fNrTs % 10000) WriteHistos();
}

template<class Digi>
UInt_t CbmMcbm2019CheckTimingPairs::FillCorrBuffer(Double_t dTimeBmon, UInt_t uIndexStart, Double_t dWinStartTime,
                                                   Double_t dWinStopTime, std::vector<std::vector<Digi>>& vDigi,
                                                   ECbmModuleId iDetId)
{

  UInt_t nrDigis         = fDigiMan->GetNofDigis(iDetId);
  UInt_t uFirstDigiInWin = uIndexStart;

  for (UInt_t iDigi = uIndexStart; iDigi < nrDigis; ++iDigi) {
    const Digi* digi = fDigiMan->Get<Digi>(iDigi);

    Double_t dTimeDet  = digi->GetTime();
    Double_t dTimeDiff = dTimeDet - dTimeBmon;

    if (dTimeDiff < dWinStartTime) {
      uFirstDigiInWin = iDigi;
      continue;
    }  // if( dTimeDiff < dWinStartTime  )
    else if (dWinStopTime < dTimeDiff) {
      break;
    }  // else if( dWinStopTime < dTimeDiff ) of if( dTimeDiff < dWinStartTime  )

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
        //            UInt_t uChan = stsDigi->GetChannel();

        /// Reject pulser digis
        if ((kuDefaultAddress != fuStsAddress && uAddr == fuStsAddress)) continue;

        /// Concidence candidate, store it!
        if (0 == fuNbCoincDigisSts) vDigi.push_back(std::vector<Digi>());

        vDigi[fuNbDigisWithCoincBmon].push_back((*digi));
        ++fuNbCoincDigisSts;

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
        //            UInt_t uChan = muchDigi->GetNxCh();

        /// Reject pulser digis
        if ((kuMaxNbMuchAsics != fuMuchAsic && uAsic == fuMuchAsic)) continue;

        /// Concidence candidate, store it!
        if (0 == fuNbCoincDigisMuch) vDigi.push_back(std::vector<Digi>());

        vDigi[fuNbDigisWithCoincBmon].push_back((*digi));
        ++fuNbCoincDigisMuch;

        break;
      }                         // case ECbmModuleId::kMuch:
      case ECbmModuleId::kTrd:  ///< Time-of-flight Detector
      {
        /*
            UInt_t uAddr = digi->GetAddress();

            if( ( kuDefaultAddress != fuTrdAddress && uAddr == fuTrdAddress ) )
             continue;

            /// Reject pulser digis
            if( fuMinChargePulserTrd < digi->GetCharge() && digi->GetCharge() < fuMaxChargePulserTrd )
             continue;
*/
        /// Concidence candidate, store it!
        if (0 == fuNbCoincDigisTrd) vDigi.push_back(std::vector<Digi>());

        vDigi[fuNbDigisWithCoincBmon].push_back((*digi));
        ++fuNbCoincDigisTrd;

        break;
      }                         // case ECbmModuleId::kTrd:
      case ECbmModuleId::kTof:  ///< Time-of-flight Detector
      {
        /// Reject pulser digis
        if (fuMinTotPulserTof < digi->GetCharge() && digi->GetCharge() < fuMaxTotPulserTof) continue;

        /// Concidence candidate, store it!
        if (0 == fuNbCoincDigisTof) vDigi.push_back(std::vector<Digi>());

        vDigi[fuNbDigisWithCoincBmon].push_back((*digi));
        ++fuNbCoincDigisTof;

        break;
      }                          // case ECbmModuleId::kTof:
      case ECbmModuleId::kRich:  ///< Ring-Imaging Cherenkov Detector
      {
        /// Reject pulser digis
        if (fuMinTotPulserRich < digi->GetCharge() && digi->GetCharge() < fuMaxTotPulserRich) continue;

        /// Concidence candidate, store it!
        if (0 == fuNbCoincDigisRich) vDigi.push_back(std::vector<Digi>());

        vDigi[fuNbDigisWithCoincBmon].push_back((*digi));
        ++fuNbCoincDigisRich;

        break;
      }                         // case ECbmModuleId::kRich:
      case ECbmModuleId::kPsd:  ///< Projectile spectator detector
      {
        UInt_t uAddr = digi->GetAddress();

        /// Reject pulser digis
        if ((kuDefaultAddress != fuPsdAddress && uAddr == fuPsdAddress)) continue;
        if (fuMinAdcPulserPsd < digi->GetCharge() && digi->GetCharge() < fuMaxAdcPulserPsd) continue;
        /*
            if( digi->GetAddress() == (9<<10)+8 )
             continue;
*/
        /// Concidence candidate, store it!
        if (0 == fuNbCoincDigisPsd) vDigi.push_back(std::vector<Digi>());

        vDigi[fuNbDigisWithCoincBmon].push_back((*digi));
        ++fuNbCoincDigisPsd;

        break;
      }  // case ECbmModuleId::kPsd:
      default: return 0;
    }  // switch( iDetId )
  }    // for( UInt_t iDigi = uIndexStart; iDigi < nrDigis; ++iDigi )

  return uFirstDigiInWin;
}

template<class DigiA>
void CbmMcbm2019CheckTimingPairs::FillHistosInter(UInt_t uIndexBmon, UInt_t uIndexDetA, UInt_t uIndexDetB,
                                                  std::vector<DigiA>& vCorrDigA, UInt_t uHistoIdx)
{
  if ("STS" == fvsDetectors[uIndexDetB]) {
    FillHistos<DigiA, CbmStsDigi>(uIndexBmon, uIndexDetA, uIndexDetB, vCorrDigA, fvDigisSts[uIndexBmon], uHistoIdx);
  }  // if( "STS" == fvsDetectors[ uIndexDetB ] )
  else if ("MUCH" == fvsDetectors[uIndexDetB]) {
    FillHistos<DigiA, CbmMuchBeamTimeDigi>(uIndexBmon, uIndexDetA, uIndexDetB, vCorrDigA, fvDigisMuch[uIndexBmon],
                                           uHistoIdx);
  }  // else if( "MUCH" == fvsDetectors[ uIndexDetB ] )
  else if ("TRD" == fvsDetectors[uIndexDetB]) {
    FillHistos<DigiA, CbmTrdDigi>(uIndexBmon, uIndexDetA, uIndexDetB, vCorrDigA, fvDigisTrd[uIndexBmon], uHistoIdx);
  }  // else if( "TRD" == fvsDetectors[ uIndexDetB ] )
  else if ("TOF" == fvsDetectors[uIndexDetB]) {
    FillHistos<DigiA, CbmTofDigi>(uIndexBmon, uIndexDetA, uIndexDetB, vCorrDigA, fvDigisTof[uIndexBmon], uHistoIdx);
  }  // else if( "TOF" == fvsDetectors[ uIndexDetB ] )
  else if ("RICH" == fvsDetectors[uIndexDetB]) {
    FillHistos<DigiA, CbmRichDigi>(uIndexBmon, uIndexDetA, uIndexDetB, vCorrDigA, fvDigisRich[uIndexBmon], uHistoIdx);
  }  // else if( "RICH" == fvsDetectors[ uIndexDetB ] )
  else if ("PSD" == fvsDetectors[uIndexDetB]) {
    FillHistos<DigiA, CbmPsdDigi>(uIndexBmon, uIndexDetA, uIndexDetB, vCorrDigA, fvDigisPsd[uIndexBmon], uHistoIdx);
  }  // else if( "PSD" == fvsDetectors[ uIndexDetB ] )
  else
    LOG(fatal) << "CbmMcbm2019CheckTimingPairs => Unknown detector";
}
template<class DigiA, class DigiB>
void CbmMcbm2019CheckTimingPairs::FillHistos(UInt_t uIndexBmon, UInt_t /*uIndexDetA*/, UInt_t /*uIndexDetB*/,
                                             std::vector<DigiA>& vCorrDigA, std::vector<DigiB>& vCorrDigB,
                                             UInt_t uHistoIdx)
{
  Double_t dTimeBmon = fvDigisBmon[uIndexBmon].GetTime();
  /*
   std::vector< DigiA > vCorrDigA;
   std::vector< DigiB > vCorrDigB;

   /// Get detector A
   if( "STS" == fvsDetectors[ uIndexDetA ] )
   {
      vCorrDigA = fvDigisSts[ uIndexBmon ];
   } // if( "STS" == fvsDetectors[ uIndexDetA ] )
   else if( "MUCH" == fvsDetectors[ uIndexDetA ] )
   {
      vCorrDigA = fvDigisMuch[ uIndexBmon ];
   } // else if( "MUCH" == fvsDetectors[ uIndexDetA ] )
   else if( "TRD" == fvsDetectors[ uIndexDetA ] )
   {
      vCorrDigA = fvDigisTrd[ uIndexBmon ];
   } // else if( "TRD" == fvsDetectors[ uIndexDetA ] )
   else if( "TOF" == fvsDetectors[ uIndexDetA ] )
   {
      vCorrDigA = fvDigisTof[ uIndexBmon ];
   } // else if( "TOF" == fvsDetectors[ uIndexDetA ] )
   else if( "RICH" == fvsDetectors[ uIndexDetA ] )
   {
      vCorrDigA = fvDigisRich[ uIndexBmon ];
   } // else if( "RICH" == fvsDetectors[ uIndexDetA ] )
   else if( "PSD" == fvsDetectors[ uIndexDetA ] )
   {
      vCorrDigA = fvDigisPsd[ uIndexBmon ];
   } // else if( "PSD" == fvsDetectors[ uIndexDetA ] )
      else LOG( fatal ) << "CbmMcbm2019CheckTimingPairs => Unknown detector";

   if( "STS" == fvsDetectors[ uIndexDetB ] )
   {
      vCorrDigB = fvDigisSts[ uIndexBmon ];
   } // if( "STS" == fvsDetectors[ uIndexDetB ] )
   else if( "MUCH" == fvsDetectors[ uIndexDetB ] )
   {
      vCorrDigB = fvDigisMuch[ uIndexBmon ];
   } // else if( "MUCH" == fvsDetectors[ uIndexDetB ] )
   else if( "TRD" == fvsDetectors[ uIndexDetB ] )
   {
      vCorrDigB = fvDigisTrd[ uIndexBmon ];
   } // else if( "TRD" == fvsDetectors[ uIndexDetB ] )
   else if( "TOF" == fvsDetectors[ uIndexDetB ] )
   {
      vCorrDigB = fvDigisTof[ uIndexBmon ];
   } // else if( "TOF" == fvsDetectors[ uIndexDetB ] )
   else if( "RICH" == fvsDetectors[ uIndexDetB ] )
   {
      vCorrDigB = fvDigisRich[ uIndexBmon ];
   } // else if( "RICH" == fvsDetectors[ uIndexDetB ] )
   else if( "PSD" == fvsDetectors[ uIndexDetB ] )
   {
      vCorrDigB = fvDigisPsd[ uIndexBmon ];
   } // else if( "PSD" == fvsDetectors[ uIndexDetB ] )
      else LOG( fatal ) << "CbmMcbm2019CheckTimingPairs => Unknown detector";
*/
  for (UInt_t uIdxDetA = 0; uIdxDetA < vCorrDigA.size(); ++uIdxDetA) {
    Double_t dTimeDetA = vCorrDigA[uIdxDetA].GetTime();
    Double_t dDtDetA   = dTimeDetA - dTimeBmon;

    for (UInt_t uIdxDetB = 0; uIdxDetB < vCorrDigB.size(); ++uIdxDetB) {
      Double_t dTimeDetB = vCorrDigB[uIdxDetB].GetTime();
      Double_t dDtDetB   = dTimeDetB - dTimeBmon;

      fhDtADtB[uHistoIdx]->Fill(dDtDetA, dDtDetB);
    }  // for( UInt_t uIdxDetB = 0; uIdxDetB < vCoincDigisDetB[ uEvent ].size(); ++vCoincDigisDetB )
  }    // for( UInt_t uIdxDetA = 0; uIdxDetA < vCoincDigisDetA[ uEvent ].size(); ++vCoincDigisDetA )
}


// ---- Finish --------------------------------------------------------
void CbmMcbm2019CheckTimingPairs::Finish() { WriteHistos(); }

void CbmMcbm2019CheckTimingPairs::WriteHistos()
{
  TFile* oldFile     = gFile;
  TDirectory* oldDir = gDirectory;

  TFile* outfile = TFile::Open(fOutFileName, "RECREATE");

  for (UInt_t uPair = 0; uPair < fhDtADtB.size(); ++uPair) {
    fhDtADtB[uPair]->Write();
  }  // for( UInt_t uPair = 0; uPair < fhDtADtB.size(); ++uPair )

  outfile->Close();
  delete outfile;

  gFile      = oldFile;
  gDirectory = oldDir;
}

ClassImp(CbmMcbm2019CheckTimingPairs)
