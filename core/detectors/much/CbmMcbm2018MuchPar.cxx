/* Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Ajit Kumar, Florian Uhlig [committer] */

#include "CbmMcbm2018MuchPar.h"

#include "FairDetParIo.h"
#include "FairParIo.h"
#include "FairParamList.h"
#include <Logger.h>

#include "TMath.h"
#include "TString.h"

using namespace std;

// -----   Standard constructor   ------------------------------------------
CbmMcbm2018MuchPar::CbmMcbm2018MuchPar(const char* name, const char* title, const char* context)
  : FairParGenericSet(name, title, context)
  , fuNrOfDpbs(0)
  , fiDbpIdArray()
  , fiCrobActiveFlag()
  , fuFebsInGemA(0)
  , fuFebsInGemB(0)
  , fuFebsInRpc(0)
  , fnFebsIdsArrayGemA()
  , fnFebsIdsArrayGemB()
  , fnFebsIdsArrayRpc()
  , fChannelsToPadXA()
  , fChannelsToPadYA()
  , fChannelsToPadXB()
  , fChannelsToPadYB()
  , fChannelsToPadXRpc()
  , fChannelsToPadYRpc()
  , fRealX()
  , fRealPadSize()
{
  detName = "Much";
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmMcbm2018MuchPar::~CbmMcbm2018MuchPar() {}
// -------------------------------------------------------------------------


// -----   Public method clear   -------------------------------------------
void CbmMcbm2018MuchPar::clear()
{
  status = kFALSE;
  resetInputVersions();
}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------

void CbmMcbm2018MuchPar::putParams(FairParamList* l)
{
  if (!l) return;

  l->add("NrOfDpbs", fuNrOfDpbs);
  l->add("DbpIdArray", fiDbpIdArray);
  l->add("CrobActiveFlag", fiCrobActiveFlag);
  l->add("NrOfFebsInGemA", fuFebsInGemA);
  l->add("nFebsIdsArrayA", fnFebsIdsArrayGemA);
  l->add("NrOfFebsInGemB", fuFebsInGemB);
  l->add("nFebsIdsArrayB", fnFebsIdsArrayGemB);
  l->add("NrOfFebsInRpc", fuFebsInRpc);
  l->add("nFebsIdsArrayRpc", fnFebsIdsArrayRpc);
  l->add("ChannelsToPadXA", fChannelsToPadXA);
  l->add("ChannelsToPadYA", fChannelsToPadYA);
  l->add("ChannelsToPadXB", fChannelsToPadXB);
  l->add("ChannelsToPadYB", fChannelsToPadYB);
  l->add("ChannelsToPadXRpc", fChannelsToPadXRpc);
  l->add("ChannelsToPadYRpc", fChannelsToPadYRpc);
  l->add("RealX", fRealX);
  l->add("PadSize", fRealPadSize);
}

// -------------------------------------------------------------------------

Bool_t CbmMcbm2018MuchPar::getParams(FairParamList* l)
{

  if (!l) return kFALSE;

  if (!l->fill("NrOfDpbs", &fuNrOfDpbs)) return kFALSE;

  fiDbpIdArray.Set(fuNrOfDpbs);
  if (!l->fill("DbpIdArray", &fiDbpIdArray)) return kFALSE;

  fiCrobActiveFlag.Set(fuNrOfDpbs * kuNbCrobsPerDpb);
  if (!l->fill("CrobActiveFlag", &fiCrobActiveFlag)) return kFALSE;

  if (!l->fill("NrOfFebsInGemA", &fuFebsInGemA)) return kFALSE;

  fnFebsIdsArrayGemA.Set(GetNrOfFebsInGemA());
  if (!l->fill("nFebsIdsArrayA", &fnFebsIdsArrayGemA)) return kFALSE;

  if (!l->fill("NrOfFebsInGemB", &fuFebsInGemB)) return kFALSE;

  fnFebsIdsArrayGemB.Set(GetNrOfFebsInGemB());
  if (!l->fill("nFebsIdsArrayB", &fnFebsIdsArrayGemB)) return kFALSE;

  if (!l->fill("NrOfFebsInRpc", &fuFebsInRpc)) return kFALSE;

  fnFebsIdsArrayRpc.Set(GetNrOfFebsInRpc());
  if (!l->fill("nFebsIdsArrayRpc", &fnFebsIdsArrayRpc)) return kFALSE;

  fChannelsToPadXA.Set(GetNrOfFebs() * kuNbChanPerAsic);
  if (!l->fill("ChannelsToPadXA", &fChannelsToPadXA)) return kFALSE;

  fChannelsToPadYA.Set(GetNrOfFebs() * kuNbChanPerAsic);
  if (!l->fill("ChannelsToPadYA", &fChannelsToPadYA)) return kFALSE;

  fChannelsToPadXB.Set(GetNrOfFebs() * kuNbChanPerAsic);
  if (!l->fill("ChannelsToPadXB", &fChannelsToPadXB)) return kFALSE;

  fChannelsToPadYB.Set(GetNrOfFebs() * kuNbChanPerAsic);
  if (!l->fill("ChannelsToPadYB", &fChannelsToPadYB)) return kFALSE;

  fChannelsToPadXRpc.Set(GetNrOfFebsInRpc() * kuNbChanPerAsic);
  if (!l->fill("ChannelsToPadXRpc", &fChannelsToPadXRpc)) return kFALSE;

  fChannelsToPadYRpc.Set(GetNrOfFebsInRpc() * kuNbChanPerAsic);
  if (!l->fill("ChannelsToPadYRpc", &fChannelsToPadYRpc)) return kFALSE;

  fRealX.Set(2232);  // Number of Sectors in one GEM Module
  if (!l->fill("RealX", &fRealX)) return kFALSE;

  fRealPadSize.Set(2232);  // Number of Sectors in one GEM Module
  if (!l->fill("PadSize", &fRealPadSize)) return kFALSE;

  return kTRUE;
}
// -------------------------------------------------------------------------
Int_t CbmMcbm2018MuchPar::ElinkIdxToFebIdx(UInt_t uElink)
{
  //LOG(info) <<" uElink "<<uElink<<" kuNbElinksPerCrob "<<kuNbElinksPerCrob;
  if (uElink < kuNbElinksPerCrob) return kiCrobMapElinkFebIdx[uElink];
  else {
    LOG(warning) << "CbmMcbm2018MuchPar::ElinkIdxToFebIdx => Index out of bound, "
                 << "returning crazy value!";
    return -1;
  }  // else of if( uElink < kuNbElinksPerCrob )
}
// -------------------------------------------------------------------------
UInt_t CbmMcbm2018MuchPar::ElinkIdxToAsicIdxFebMuch(UInt_t uElink)
{
  if (uElink < kuNbElinksPerCrob) return kuCrobMapElinkFebMuch[uElink];
  else {
    LOG(warning) << "CbmMcbm2018MuchPar::ElinkIdxToAsicIdxFebMuch => Index out of bound, "
                 << "returning crazy value!";
    return 0xFFFF;
  }  // else of if( uElink < kuNbElinksPerCrob )
}
// -------------------------------------------------------------------------
UInt_t CbmMcbm2018MuchPar::GetDpbId(UInt_t uDpbIdx)
{
  if (uDpbIdx < fuNrOfDpbs) return fiDbpIdArray[uDpbIdx];
  else {
    LOG(warning) << "CbmMcbm2018MuchPar::GetDpbId => Index out of bound, "
                 << "returning crazy value!";
    return 0xFFFFFFFF;
  }  // else of if( uDpbIdx < fuNrOfDpbs )
}
Bool_t CbmMcbm2018MuchPar::IsCrobActive(UInt_t uDpbIdx, UInt_t uCrobIdx)
{
  if (uDpbIdx < fuNrOfDpbs) {
    if (uCrobIdx < kuNbCrobsPerDpb) return 0 < fiCrobActiveFlag[uDpbIdx * kuNbCrobsPerDpb + uCrobIdx] ? kTRUE : kFALSE;
    else {
      LOG(warning) << "CbmMcbm2018MuchPar::IsCrobActive => Crob Index out of bound, "
                   << "returning default inactive!";
      return kFALSE;
    }  // else of if( uCrobIdx < kuNbCrobsPerDpb )
  }    // if( uDpbIdx < fuNrOfDpbs )
  else {
    LOG(warning) << "CbmMcbm2018MuchPar::IsCrobActive => Dpb Index out of bound, "
                 << "returning default inactive!";
    return kFALSE;
  }  // else of if( uDpbIdx < fuNrOfDpbs )
}
Bool_t CbmMcbm2018MuchPar::IsFebActive(UInt_t uFebInSystIdx)
{

  if (uFebInSystIdx < GetNrOfFebs()) {
    /// Always return true for now
    return kTRUE;
  }  // if( uFebInSystIdx < GetNrOfFebs() )
  else {
    LOG(warning) << "CbmMcbm2018MuchPar::IsFebActive => Feb Index out of bound, "
                 << "returning default inactive!";
    return kFALSE;
  }  // else of if( uFebInSystIdx < GetNrOfFebs() )
}
Bool_t CbmMcbm2018MuchPar::IsFebActive(UInt_t uDpbIdx, UInt_t uCrobIdx, UInt_t uFebIdx)
{
  if (uDpbIdx < fuNrOfDpbs) {
    if (uCrobIdx < kuNbCrobsPerDpb) {
      if (uFebIdx < kuNbFebsPerCrob) {
        UInt_t uIdx = (uDpbIdx * kuNbCrobsPerDpb + uCrobIdx) * kuNbFebsPerCrob + uFebIdx;
        return IsFebActive(uIdx);
      }  // if( uFebIdx < kuNbFebsPerCrob )
      else {
        LOG(warning) << "CbmMcbm2018MuchPar::IsFebActive => Feb Index out of bound, "
                     << "returning default inactive!";
        return kFALSE;
      }  // else of if( uFebIdx < kuNbCrobsPerDpb )
    }    // if( uCrobIdx < kuNbCrobsPerDpb )
    else {
      LOG(warning) << "CbmMcbm2018MuchPar::IsFebActive => Crob Index out of bound, "
                   << "returning default inactive!";
      return kFALSE;
    }  // else of if( uCrobIdx < kuNbCrobsPerDpb )
  }    // if( uDpbIdx < fuNrOfDpbs )
  else {
    LOG(warning) << "CbmMcbm2018MuchPar::IsFebActive => Dpb Index out of bound, "
                 << "returning default inactive!";
    return kFALSE;
  }  // else of if( uDpbIdx < fuNrOfDpbs )
}
Short_t CbmMcbm2018MuchPar::GetPadXA(UShort_t febid, UShort_t channelid)
{
  if (fChannelsToPadXA.GetSize() <= static_cast<Int_t>((febid * kuNbChanPerAsic) + channelid)) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetPadXA => Index out of bounds: " << ((febid * kuNbChanPerAsic) + channelid)
               << " VS " << fChannelsToPadXA.GetSize() << " (" << febid << " and " << channelid << ")";
    return -2;
  }  // if( fChannelsToPadXA.GetSize () <= static_cast< Int_t >( (febid*kuNbChanPerAsic)+channelid ) )


  return fChannelsToPadXA[(febid * kuNbChanPerAsic) + channelid];
}
Short_t CbmMcbm2018MuchPar::GetPadYA(UShort_t febid, UShort_t channelid)
{
  if (fChannelsToPadXA.GetSize() <= static_cast<Int_t>((febid * kuNbChanPerAsic) + channelid)) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetPadYA => Index out of bounds: " << ((febid * kuNbChanPerAsic) + channelid)
               << " VS " << fChannelsToPadYA.GetSize() << " (" << febid << " and " << channelid << ")";
    return -2;
  }  // if( fChannelsToPadXA.GetSize () <= static_cast< Int_t >( (febid*kuNbChanPerAsic)+channelid ) )

  return fChannelsToPadYA[(febid * kuNbChanPerAsic) + channelid];
}

Short_t CbmMcbm2018MuchPar::GetPadXB(UShort_t febid, UShort_t channelid)
{
  if (fChannelsToPadXB.GetSize() <= static_cast<Int_t>((febid * kuNbChanPerAsic) + channelid)) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetPadXB => Index out of bounds: " << ((febid * kuNbChanPerAsic) + channelid)
               << " VS " << fChannelsToPadXB.GetSize() << " (" << febid << " and " << channelid << ")";
    return -2;
  }  // if( fChannelsToPadXB.GetSize () <= static_cast< Int_t >( (febid*kuNbChanPerAsic)+channelid ) )


  return fChannelsToPadXB[(febid * kuNbChanPerAsic) + channelid];
}
Short_t CbmMcbm2018MuchPar::GetPadYB(UShort_t febid, UShort_t channelid)
{
  if (fChannelsToPadXB.GetSize() <= static_cast<Int_t>((febid * kuNbChanPerAsic) + channelid)) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetPadYB => Index out of bounds: " << ((febid * kuNbChanPerAsic) + channelid)
               << " VS " << fChannelsToPadYB.GetSize() << " (" << febid << " and " << channelid << ")";
    return -2;
  }  // if( fChannelsToPadXB.GetSize () <= static_cast< Int_t >( (febid*kuNbChanPerAsic)+channelid ) )

  return fChannelsToPadYB[(febid * kuNbChanPerAsic) + channelid];
}


Short_t CbmMcbm2018MuchPar::GetPadXRpc(UShort_t febid, UShort_t channelid)
{
  if (fChannelsToPadXRpc.GetSize() <= static_cast<Int_t>((febid * kuNbChanPerAsic) + channelid)) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetPadXRpc => Index out of bounds: " << ((febid * kuNbChanPerAsic) + channelid)
               << " VS " << fChannelsToPadXRpc.GetSize() << " (" << febid << " and " << channelid << ")";
    return -2;
  }  // if( fChannelsToPadXB.GetSize () <= static_cast< Int_t >( (febid*kuNbChanPerAsic)+channelid ) )


  return fChannelsToPadXRpc[(febid * kuNbChanPerAsic) + channelid];
}
Short_t CbmMcbm2018MuchPar::GetPadYRpc(UShort_t febid, UShort_t channelid)
{
  if (fChannelsToPadXRpc.GetSize() <= static_cast<Int_t>((febid * kuNbChanPerAsic) + channelid)) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetPadYRpc => Index out of bounds: " << ((febid * kuNbChanPerAsic) + channelid)
               << " VS " << fChannelsToPadYRpc.GetSize() << " (" << febid << " and " << channelid << ")";
    return -2;
  }  // if( fChannelsToPadXB.GetSize () <= static_cast< Int_t >( (febid*kuNbChanPerAsic)+channelid ) )

  return fChannelsToPadYRpc[(febid * kuNbChanPerAsic) + channelid];
}


UInt_t CbmMcbm2018MuchPar::GetFebId(UInt_t uAsicIdx)
{
  //22022022:- Not enabled GEM2 (GemB) Working for GemA and RPC under one CRI
  //LOG(info)<<" fnFebsIdsArrayGemA.GetSize() "<<fnFebsIdsArrayGemA.GetSize()<<" fnFebsIdsArrayGemB.GetSize()"<<fnFebsIdsArrayGemB.GetSize();
  if (uAsicIdx >= GetNrOfFebsInGemA()) {
    if ((uAsicIdx % GetNrOfFebsInGemA()) < GetNrOfFebsInRpc()) return fnFebsIdsArrayRpc[uAsicIdx % GetNrOfFebsInGemA()];
    else {
      LOG(error) << "CbmMcbm2018MuchPar::GetFebId => Index out of bounds: " << uAsicIdx << " VS " << GetNrOfFebsInGemA()
                 << " and " << GetNrOfFebsInRpc() << " => Returning crazy value!!!";
      return 10000 * (GetNrOfFebsInGemA() + GetNrOfFebsInRpc());
    }  // else of if( ( uAsicIdx % GetNrOfFebsInGemA() ) < GetNrOfFebsInGemB() )
  }    // if(uAsicIdx >= GetNrOfFebsInGemA())
  else
    return fnFebsIdsArrayGemA[uAsicIdx];

  //uncomment below when GemB also visible with another CRI
  /*
  if (uAsicIdx >= GetNrOfFebsInGemA()) {
    if ((uAsicIdx % GetNrOfFebsInGemA()) < GetNrOfFebsInGemB())
      return fnFebsIdsArrayGemB[uAsicIdx % GetNrOfFebsInGemA()];
    else {
     // LOG(error) << "CbmMcbm2018MuchPar::GetFebId => Index out of bounds: " << uAsicIdx << " VS " << GetNrOfFebsInGemA()
       //          << " and " << GetNrOfFebsInGemB() << " => Returning crazy value!!!";
      return 10000 * (GetNrOfFebsInGemA() + GetNrOfFebsInGemB());
    }  // else of if( ( uAsicIdx % GetNrOfFebsInGemA() ) < GetNrOfFebsInGemB() )
  }    // if(uAsicIdx >= GetNrOfFebsInGemA())
  else
    return fnFebsIdsArrayGemA[uAsicIdx];
  */
}

UInt_t CbmMcbm2018MuchPar::GetModule(UInt_t uAsicIdx)
{
  if (uAsicIdx >= GetNrOfFebsInGemA()) {
    //if ((uAsicIdx % GetNrOfFebsInGemA()) < GetNrOfFebsInGemB()) return 1;
    if ((uAsicIdx % GetNrOfFebsInGemA()) < GetNrOfFebsInRpc()) return 1;
    else
      return 2;
  }  // if(uAsicIdx >= GetNrOfFebsInGemA())
  else
    return 0;
}

Double_t CbmMcbm2018MuchPar::GetRealX(Int_t SectorIndex)
{

  //LOG(info)<<" fChannelsToPadX.GetSize() "<<fChannelsToPadX.GetSize();
  if (SectorIndex < 0 || SectorIndex <= 97) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetRealX => Index out of bounds: ";
    return -2;
  }  // if( fChannelsToPadY.GetSize () <= (febid*kuNbChanPerAsic)+channelid )

  return fRealX[SectorIndex];
}

Double_t CbmMcbm2018MuchPar::GetRealPadSize(Int_t SectorIndex)
{

  //LOG(info)<<" fChannelsToPadX.GetSize() "<<fChannelsToPadX.GetSize();
  if (SectorIndex < 0 || SectorIndex <= 97) {
    LOG(debug) << "CbmMcbm2018MuchPar::GetRealX => Index out of bounds: ";
    return -2;
  }  // if( fChannelsToPadY.GetSize () <= (febid*kuNbChanPerAsic)+channelid )

  return fRealPadSize[SectorIndex];
}

Double_t CbmMcbm2018MuchPar::GetRealX(Int_t Channel, Int_t Sector)
{
  Int_t PadIndex = Channel + 97 * Sector;
  if (Channel < 0 || Sector < 0) return -2;
  if (fRealX.GetSize() <= PadIndex) {
    LOG(info) << "CbmMcbm2018MuchPar::GetRealX => Index out of bounds: " << Channel << " " << Sector << " " << PadIndex;
    return -1;
  }  // if( fRealX.Size() <= PadIndex )

  return fRealX[PadIndex];
}
Double_t CbmMcbm2018MuchPar::GetRealPadSize(Int_t Channel, Int_t Sector)
{
  Int_t PadIndex = Channel + 97 * Sector;
  if (Channel < 0 || Sector < 0) return -2;
  if (fRealPadSize.GetSize() <= PadIndex) {
    LOG(info) << "CbmMcbm2018MuchPar::GetRealPadSize => Index out of bounds: " << Channel << " " << Sector << " "
              << PadIndex;
    return -1;
  }  // if( fRealPadSize.Size() <= PadIndex )

  return fRealPadSize[PadIndex];
}

ClassImp(CbmMcbm2018MuchPar)
