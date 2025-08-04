/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci */

#include "CbmTrdParSetAsic.h"

#include "CbmTrdDigi.h"       // for eCbmTrdAsicType
#include "CbmTrdParAsic.h"    // for CbmTrdParAsic
#include "CbmTrdParFasp.h"    // for CbmTrdParFasp, NFASPCH, CbmTrdParFasp...
#include "CbmTrdParMod.h"     // for CbmTrdParMod
#include "CbmTrdParModAsic.h"  // for CbmTrdParModAsic
#include "CbmTrdParSpadic.h"  // for CbmTrdParSpadic, NSPADICCH

#include <FairParamList.h>  // for FairParamList
#include <Logger.h>         // for Logger, LOG

#include <TArrayI.h>            // for TArrayI
#include <TGenericClassInfo.h>  // for TGenericClassInfo
#include <TObjArray.h>
#include <TObjString.h>
#include <TString.h>            // for Form
#include <TSystem.h>

#include <utility>  // for pair

#include <stdint.h>  // for size_t
#include <stdio.h>   // for printf
#include <string.h>  // for strcmp

using std::map;
//_______________________________________________________________________________
CbmTrdParSetAsic::CbmTrdParSetAsic(const char* name, const char* title, const char* context)
  : CbmTrdParSet(name, title, context)
{
}

//_______________________________________________________________________________
int CbmTrdParSetAsic::FindModuleByEqId(uint16_t eqid, uint8_t& crob_id, uint8_t& lnk_id) const
{
  for (auto mod : fModuleMap) {
    int crob = ((const CbmTrdParModAsic*) mod.second)->HasEqId(eqid, lnk_id);
    if (crob < 0) continue;
    crob_id = crob;
    return mod.first;
  }
  return -1;
}

//_______________________________________________________________________________
Bool_t CbmTrdParSetAsic::getParams(FairParamList* l)
{
  // LOG(info) << GetName() << "::getParams(FairParamList*)";
  if (!l) return kFALSE;
  if (!l->fill("NrOfModules", &fNrOfModules)) return kFALSE;
  TArrayI moduleId(fNrOfModules);
  if (!l->fill("ModuleId", &moduleId)) return kFALSE;
  TArrayI nAsic(fNrOfModules);
  if (!l->fill("nAsic", &nAsic)) return kFALSE;
  TArrayI typeAsic(fNrOfModules);
  if (!l->fill("typeAsic", &typeAsic)) return kFALSE;

  Int_t maxNrAsics {0};
  for (Int_t imod = 0; imod < fNrOfModules; imod++) {
    if (nAsic[imod] > maxNrAsics) maxNrAsics = nAsic[imod];
  }
  Int_t address(0);

  CbmTrdParModAsic* mod(nullptr);
  CbmTrdParAsic* asic(nullptr);
  for (Int_t i = 0; i < fNrOfModules; i++) {
    mod = new CbmTrdParModAsic(GetName(), Form("%s for Module %d", GetTitle(), moduleId[i]));
    mod->SetChamberType(typeAsic[i]);
    mod->SetModuleId(moduleId[i]);
    // only for FASP
    if (9 == typeAsic[i]) {
      TArrayI crobAddress(int(NCROBMOD));
      if (!l->fill(Form("CrobInfo - Module %d", moduleId[i]), &crobAddress)) continue;
      mod->SetCrobAddresses(crobAddress.GetArray());
      Int_t maxValues = maxNrAsics * int(NFASPPARS);
      TArrayI values(maxValues);
      if (!l->fill(Form("FaspInfo - Module %d", moduleId[i]), &values)) continue;
      for (Int_t iasic = 0; iasic < nAsic[i]; iasic++) {
        Int_t offset = iasic * int(NFASPPARS);
        address      = values[offset++];
        if (address == moduleId[i] * 1000 + 999) continue;
        asic         = new CbmTrdParFasp(address);
        static_cast<CbmTrdParFasp*>(asic)->LoadParams(&(values.GetArray()[offset]));
        mod->SetAsicPar(asic);
      }
    }
    else {
      Int_t maxValues = maxNrAsics * (5 + NSPADICCH);
      TArrayI values(maxValues);

      if (!l->fill(Form("SpadicInfo - Module %d", moduleId[i]), &values)) continue;
      for (Int_t iasic = 0; iasic < nAsic[i]; iasic++) {
        Int_t offset = iasic * (5 + NSPADICCH);
        address      = values[offset + 0];
        asic         = new CbmTrdParSpadic(address);
        asic->SetComponentId(static_cast<CbmTrdParSpadic*>(asic)->CreateComponentId(
          values[offset + 1], values[offset + 2], values[offset + 3], values[offset + 4]));
        std::vector<Int_t> addresses {};
        for (Int_t j = offset + 5; j < offset + 5 + NSPADICCH; j++) {
          addresses.push_back(values[j]);
        }
        asic->SetChannelAddresses(addresses);
        mod->SetAsicPar(asic);
      }
    }
    fModuleMap[moduleId[i]] = mod;
  }
  return kTRUE;
}

//_______________________________________________________________________________
void CbmTrdParSetAsic::putParams(FairParamList* l)
{
  if (!l) return;
  // LOG(info) << GetName() << "::putParams(FairParamList*)";

  Int_t idx(0);
  TArrayI moduleId(fNrOfModules), nAsic(fNrOfModules), typeChmb(fNrOfModules);
  for (auto mod : fModuleMap) {
    moduleId[idx] = mod.first;
    nAsic[idx]    = ((CbmTrdParModAsic*) mod.second)->GetNofAsics();
    typeChmb[idx] = ((CbmTrdParModAsic*) mod.second)->GetChamberType();
    idx++;
  }
  l->add("NrOfModules", fNrOfModules);
  l->add("ModuleId", moduleId);
  l->add("nAsic", nAsic);
  l->add("typeAsic", typeChmb);

  idx = 0;
  for (auto mapEntry : fModuleMap) {
    Int_t iAsicNr(0);
    Int_t currentAsicAddress(-1);

    CbmTrdParModAsic* mod = (CbmTrdParModAsic*) mapEntry.second;
    if (mod->GetAsicType() == CbmTrdDigi::eCbmTrdAsicType::kSPADIC) {
      // Calculate the size of the array to hold all values realted to all
      // asics of a full detector module
      // each ASCIC has a maximum of NSPADICCH channels attached + 5 values
      // for AsicID, CriId, CrobId, CrobNumber and ElinkId
      Int_t sizePerSpadic = 5 + NSPADICCH;
      Int_t fullSize      = nAsic[idx] * sizePerSpadic;
      TArrayI asicInfo(fullSize);
      iAsicNr = 0;
      for (auto iModuleIt : mod->fModPar) {
        size_t asicComponentId(100098);  // 100098 = undefined
        currentAsicAddress   = iModuleIt.first;
        asicComponentId      = ((CbmTrdParSpadic*) iModuleIt.second)->GetComponentId();
        int offset           = iAsicNr * sizePerSpadic;
        asicInfo[offset]     = currentAsicAddress;
        asicInfo[offset + 1] = CbmTrdParSpadic::GetCriId(asicComponentId);
        asicInfo[offset + 2] = CbmTrdParSpadic::GetCrobId(asicComponentId);
        asicInfo[offset + 3] = CbmTrdParSpadic::GetCrobNumber(asicComponentId);
        asicInfo[offset + 4] = CbmTrdParSpadic::GetElinkId(asicComponentId, 0);

        if ((((CbmTrdParSpadic*) iModuleIt.second)->GetNchannels()) != NSPADICCH) {
          LOG(fatal) << "Number of channels found " << ((CbmTrdParSpadic*) iModuleIt.second)->GetNchannels()
                     << " is differnt from the expected " << NSPADICCH;
        }

        Int_t iAsicChannel(0);
        for (auto channelAddressIt : ((CbmTrdParSpadic*) iModuleIt.second)->GetChannelAddresses()) {
          asicInfo[offset + 5 + iAsicChannel] = channelAddressIt;
          iAsicChannel++;
        }
        iAsicNr++;
      }
      l->add(Form("SpadicInfo - Module %d", mapEntry.first), asicInfo);
    }
    if (mod->GetAsicType() == CbmTrdDigi::eCbmTrdAsicType::kFASP) {
      TArrayI crobAddress(int(NCROBMOD));
      crobAddress.Set(int(NCROBMOD), mod->GetCrobAddresses());
      l->add(Form("CrobInfo - Module %d", mapEntry.first), crobAddress);
      int fullSize = nAsic[idx] * int(NFASPPARS);
      TArrayI asicInfo(fullSize);

      iAsicNr = 0;
      for (auto iModuleIt : mod->fModPar) {
        int offset         = iAsicNr * int(NFASPPARS);
        asicInfo[offset++] = iModuleIt.first;

        CbmTrdParFasp* fasp = (CbmTrdParFasp*) iModuleIt.second;
        asicInfo[offset++]  = fasp->GetChannelMask();
        Int_t ich(0);
        for (auto chAddress : fasp->GetChannelAddresses()) {
          const CbmTrdParFaspChannel* ch = fasp->GetChannel(ich);
          if (!ch) {
            LOG(info) << "Missing calib for Fasp[" << offset << "] pad " << chAddress;
            ich++;
            continue;
          }
          asicInfo[offset + ich]                 = (ch->HasPairingT() ? -1 : 1) * chAddress;
          asicInfo[offset + (1 * NFASPCH) + ich] = ch->GetPileUpTime();
          asicInfo[offset + (2 * NFASPCH) + ich] = ch->GetThreshold();
          asicInfo[offset + (3 * NFASPCH) + ich] = ch->GetMinDelaySignal();
          ich++;
        }
        iAsicNr++;
      }
      l->add(Form("FaspInfo - Module %d", mapEntry.first), asicInfo);
    }
    idx++;
  }
}

ClassImp(CbmTrdParSetAsic)
