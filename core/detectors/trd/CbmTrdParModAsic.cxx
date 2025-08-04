/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer]  */

#include "CbmTrdParModAsic.h"

#include "CbmTrdDigi.h"       // for eCbmTrdAsicType
#include "CbmTrdParAsic.h"    // for CbmTrdParAsic
#include "CbmTrdParFasp.h"    // for CbmTrdParFasp, NFASPCH, CbmTrdParFasp...
#include "CbmTrdParMod.h"     // for CbmTrdParMod
#include "CbmTrdParSpadic.h"  // for CbmTrdParSpadic, NSPADICCH

#include <FairParamList.h>  // for FairParamList
#include <Logger.h>         // for Logger, LOG

#include <TArrayI.h>            // for TArrayI
#include <TGenericClassInfo.h>  // for TGenericClassInfo
#include <TObjArray.h>
#include <TObjString.h>
#include <TString.h>  // for Form
#include <TSystem.h>

#include <utility>  // for pair

#include <stdint.h>  // for size_t
#include <stdio.h>   // for printf
#include <string.h>  // for strcmp

using std::map;
//_______________________________________________________________________________
CbmTrdParModAsic::CbmTrdParModAsic(const char* name, const char* title)
  : CbmTrdParMod(name, title)
  , fType(8)
  , fCrobAdd()
  , fModPar()
{
}

//_______________________________________________________________________________
void CbmTrdParModAsic::clear()
{
  for (auto asic : fModPar)
    delete asic.second;
  fModPar.clear();
}

//_______________________________________________________________________________
bool CbmTrdParModAsic::GetFaspChannelPar(int pad, const CbmTrdParFaspChannel*& tilt,
                                         const CbmTrdParFaspChannel*& rect) const
{
  if (pad < 0 || pad >= NFASPMOD * NFASPCH) return false;
  for (int ich(0), ch(pad << 1); ich < 2; ich++, ch++) {
    int faspAddress        = GetAsicAddress(ch);
    const CbmTrdParFasp* p = static_cast<const CbmTrdParFasp*>(GetAsicPar(faspAddress));
    if (!p) {
      LOG(debug) << GetName() << "::GetFaspChannelPar : Could not find FASP params for address=" << faspAddress
                 << " @ pad=" << pad;
      continue;
    }
    if (ich) rect = p->GetChannel(pad, ich);
    else
      tilt = p->GetChannel(pad, ich);
  }
  return true;
}

//_______________________________________________________________________________
Int_t CbmTrdParModAsic::GetAsicAddress(Int_t chAddress) const
{
  /** Query the ASICs in the module set for the specified read-out channel. 
 * Returns the id of the ASIC within the module or -1 if all returns false.   
 */

  for (auto asic : fModPar) {
    if (asic.second->QueryChannel(chAddress) >= 0) return asic.first;
  }
  return -1;
}

//_______________________________________________________________________________
void CbmTrdParModAsic::GetAsicAddresses(std::vector<Int_t>* a) const
{
  /** Query the ASICs in the module set for their addresses. 
 * Returns the list of these addresses in the vector prepared by the user   
 */
  for (auto asic : fModPar)
    a->push_back(asic.first);
}

//_______________________________________________________________________________
const CbmTrdParAsic* CbmTrdParModAsic::GetAsicPar(Int_t address) const
{
  if (fModPar.find(address) == fModPar.end()) return nullptr;
  return fModPar.at(address);
}

//_______________________________________________________________________________
CbmTrdParAsic* CbmTrdParModAsic::GetAsicPar(Int_t address)
{
  if (fModPar.find(address) == fModPar.end()) return nullptr;
  return fModPar[address];
}

//_______________________________________________________________________________
int CbmTrdParModAsic::GetNofAsics() const
{
  switch (fType) {
    case 1: return 80;
    case 3: return 20;
    case 5: return 180;
    case 7: return 36;
    case 8: return 24;
    case 9: return 180;
    default:
      LOG(warn) << GetName() << "::GetNofAsics : The chamber type " << fType << " has no mapping to ASICs. Skip.";
  }
  return 0;
}

//_______________________________________________________________________________
CbmTrdDigi::eCbmTrdAsicType CbmTrdParModAsic::GetAsicType() const
{
  switch (fType) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8: return CbmTrdDigi::eCbmTrdAsicType::kSPADIC;
    case 9: return CbmTrdDigi::eCbmTrdAsicType::kFASP;
    default:
      LOG(warn) << GetName() << "::GetAsicType : The chamber type " << fType << " has no mapping to ASICs. Skip.";
  }
  return CbmTrdDigi::eCbmTrdAsicType::kNTypes;
}

//_______________________________________________________________________________
int CbmTrdParModAsic::HasEqId(uint16_t eq, uint8_t& lnk) const
{
  int idx(0);
  for (auto add : fCrobAdd) {
    if ((add & 0xffff) == eq) {
      lnk = 0;
      return idx;
    }
    else if (((add >> 16) & 0xffff) == eq) {
      lnk = 1;
      return idx;
    }
    idx++;
  }
  return -1;
}

//_______________________________________________________________________________
void CbmTrdParModAsic::Print(Option_t* opt) const
{
  printf(" %s Asics[%d] Crobs[%lu]\n", GetName(), GetNofAsics(), fCrobAdd.size());
  printf("   ");
  for (auto eqid : fCrobAdd)
    printf(" %d", eqid);
  printf("\n");
  if (strcmp(opt, "all") == 0) {
    for (auto asic : fModPar)
      asic.second->Print(opt);
  }
}

//_______________________________________________________________________________
void CbmTrdParModAsic::SetAsicPar(CbmTrdParAsic* p)
{
  int address = p->GetAddress();
  if (fModPar.find(address) != fModPar.end()) {
    if (address % 1000 == 999) return;
    LOG(warn) << GetName() << "::SetAsicPar : The ASIC @ " << address << " already initialized. Skip.";
    return;
  }
  fModPar[address] = p;
}

//_______________________________________________________________________________
void CbmTrdParModAsic::SetCrobAddresses(int* addresses)
{
  switch (fType) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8: fCrobAdd.push_back(addresses[0]); break;
    case 9:
      for (int i(0); i < NCROBMOD; i++)
        fCrobAdd.push_back(addresses[i]);
      break;
    default:
      LOG(warn) << GetName() << "::SetCrobAddresses : The chamber type " << fType << " has no mapping to CROBs. Skip.";
  }
}


ClassImp(CbmTrdParModAsic)
