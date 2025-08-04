/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci */

#include "CbmTrdParFasp.h"

#include <FairParamList.h>  // for FairParamList

#include <TArrayI.h>            // for TArrayI
#include <TGenericClassInfo.h>  // for TGenericClassInfo
#include <TMathBase.h>          // for Min
#include <TString.h>            // for Form

#include <stdio.h>  // for printf

Double_t CbmTrdParFasp::fgSizeX = 2.;
Double_t CbmTrdParFasp::fgSizeY = 2.;
Double_t CbmTrdParFasp::fgSizeZ = 0.5;
//___________________________________________________________________
CbmTrdParFasp::CbmTrdParFasp(Int_t address, Int_t FebGrouping, Double_t x, Double_t y, Double_t z)
  : CbmTrdParAsic(address, FebGrouping, x, y, z)
{
  SetNameTitle(Form("FASP%d_%d", address / 1000, address % 1000), "FASP definition");
}

//___________________________________________________________________
const CbmTrdParFaspChannel* CbmTrdParFasp::GetChannel(Int_t pad_address, UChar_t pairing) const
{
  Int_t id = QueryChannel(2 * pad_address + pairing);
  if (id < 0) return nullptr;
  return &fCalib[id];
}

//___________________________________________________________________
const CbmTrdParFaspChannel* CbmTrdParFasp::GetChannel(Int_t ch_address) const
{
  if (ch_address < 0 || ch_address >= NFASPCH) return nullptr;
  return &fCalib[ch_address];
}

//___________________________________________________________________
bool CbmTrdParFasp::IsChannelMasked(int chId) const
{
  if (chId < 0 || chId >= NFASPCH) return false;
  return fCalib[chId].IsMasked();
}

//___________________________________________________________________
void CbmTrdParFasp::LoadParams(FairParamList* l)
{
  // printf("CbmTrdParFasp::LoadParams(FairParamList*)\n");
  TArrayI value(NFASPCH);
  if (l->fill(Form("%dCHS", fAddress), &value)) {
    for (Int_t ich(0); ich < NFASPCH; ich++) {
      Int_t pair = ich % 2;
      // printf("  ch[%2d] pair[%d] value[%d] ChAddress=%d\n", ich, pair, value[ich], 2 * value[ich] + pair);
      SetChannelAddress(2 * value[ich] + pair);
      fCalib[ich].SetPairing(pair);
    }
  }
  if (l->fill(Form("%dPUT", fAddress), &value))
    for (Int_t ich(0); ich < NFASPCH; ich++)
      fCalib[ich].fPileUpTime = value[ich];
  if (l->fill(Form("%dTHR", fAddress), &value))
    for (Int_t ich(0); ich < NFASPCH; ich++)
      fCalib[ich].fThreshold = value[ich];
  if (l->fill(Form("%dMDS", fAddress), &value))
    for (Int_t ich(0); ich < NFASPCH; ich++)
      fCalib[ich].fMinDelaySignal = value[ich];
}

//___________________________________________________________________
void CbmTrdParFasp::LoadParams(int* valArray)
{
  // printf("CbmTrdParFasp::LoadParams(%d)\n", fAddress);
  int offset(0);
  SetChannelMask(valArray[offset++]);
  for (Int_t ich(0); ich < NFASPCH; ich++) {
    int chAddress = valArray[offset++];
    SetChannelAddress(abs(chAddress));
    fCalib[ich].SetPairing(bool(chAddress > 0));
  }
  for (Int_t ich(0); ich < NFASPCH; ich++) {
    fCalib[ich].fPileUpTime = valArray[offset++];
  }
  for (Int_t ich(0); ich < NFASPCH; ich++) {
    fCalib[ich].fThreshold = valArray[offset++];
  }
  for (Int_t ich(0); ich < NFASPCH; ich++) {
    fCalib[ich].fMinDelaySignal = valArray[offset++];
  }
}

//___________________________________________________________________
Bool_t CbmTrdParFasp::SetCalibParameters(Int_t ch, Double_t const* par)
{
  /**  The list of channel parameter should be arranged as follows:
 * 0 : Signal formation time in [ns]
 * 1 : Length of Flat-Top in [clocks]
 * 2 : Threshold in [ADC units]
 * 3 : Signal @ minimum delay i.e. fPileUpTime [ADC units] 
 * 4 : Factor of parabolic dependence dt=fdt*(s-s0)^2 to calculate trigger [a.u.]
 * 5 : paring type ; 0 = tilt, 1 = rect
 */
  if (ch < 0 || ch >= NFASPCH) return kFALSE;
  fCalib[ch].fPileUpTime     = UShort_t(par[0]);
  fCalib[ch].fFlatTop        = UChar_t(par[1]);
  fCalib[ch].fThreshold      = UShort_t(par[2]);
  fCalib[ch].fMinDelaySignal = UShort_t(par[3]);
  fCalib[ch].fMinDelayParam  = par[4];
  if (par[5] > 0) fCalib[ch].SetPairing(kTRUE);
  else
    fCalib[ch].SetPairing(kFALSE);
  return kTRUE;
}

//___________________________________________________________________
void CbmTrdParFasp::SetChannelMask(uint32_t mask)
{
  for (Int_t ich(0); ich < NFASPCH; ich++) {
    bool on = !bool((mask >> ich) & 0x1);
    fCalib[ich].SetMask(on);
  }
}

//___________________________________________________________________
uint32_t CbmTrdParFasp::GetChannelMask() const
{
  uint32_t mask(0);
  for (Int_t ich(0); ich < NFASPCH; ich++) {
    if (!fCalib[ich].IsMasked()) mask |= 0x1;
    mask <<= 1;
  }
  return mask;
}

//___________________________________________________________________
void CbmTrdParFasp::Print(Option_t* opt) const
{
  CbmTrdParAsic::Print("TrdParFasp");
  printf("  Nchannels[%2d]\n", (Int_t) fChannelAddresses.size());
  for (Int_t ich(0); ich < TMath::Min((Int_t) GetNchannels(), (Int_t) fChannelAddresses.size()); ich++) {
    printf("  %2d pad_addr[%4d]", ich, GetPadAddress(ich));
    fCalib[ich].Print(opt);
  }
}

//___________________________________________________________________
CbmTrdParFaspChannel::CbmTrdParFaspChannel(Int_t pup, Int_t ft, Int_t thr, Int_t mds, Float_t mdp)
  : TObject()
  , fPileUpTime(pup)
  , fFlatTop(ft)
  , fConfig(0)
  , fThreshold(thr)
  , fMinDelaySignal(mds)
  , fMinDelayParam(mdp)
{
}

//___________________________________________________________________
void CbmTrdParFaspChannel::Print(Option_t* /*opt*/) const
{
  printf("[%c]; MASK{%c}; CALIB{ PUT[ns]=%3d FT[clk]=%2d THR[ADC]=%4d MDS[ADC]=%4d }\n", (HasPairingR() ? 'R' : 'T'),
         (IsMasked() ? 'X' : ' '), fPileUpTime, fFlatTop, fThreshold, fMinDelaySignal);
}

ClassImp(CbmTrdParFasp);
ClassImp(CbmTrdParFaspChannel);
