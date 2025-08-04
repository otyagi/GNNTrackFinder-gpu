/* Copyright (C) 2021 Goethe-University Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig [committer], Alexandru Bercuci */


#include "CbmTrdUnpackFaspAlgo.h"

#include "CbmTrdDigi.h"
#include "CbmTrdParFasp.h"
#include "CbmTrdParModAsic.h"
#include "CbmTrdParModDigi.h"
#include "CbmTrdParSetDigi.h"
#include "CbmTrdParSetGain.h"
#include "CbmTrdParSetGas.h"
#include "CbmTrdParSpadic.h"

#include <FairParAsciiFileIo.h>
#include <FairParGenericSet.h>
#include <FairParamList.h>
#include <FairRuntimeDb.h>
#include <FairTask.h>
#include <Logger.h>

#include <Rtypes.h>
#include <RtypesCore.h>

#include <boost/format.hpp>

// select verbosity level
// 0 : none
// 1 : data unpacker
// 2 : digi packing
#define VERBOSE 0

using namespace std;

CbmTrdUnpackFaspAlgo::CbmTrdUnpackFaspAlgo() : CbmRecoUnpackAlgo("CbmTrdUnpackFaspAlgo") {}

//_________________________________________________________________________________
CbmTrdUnpackFaspAlgo::~CbmTrdUnpackFaspAlgo() {}

//_________________________________________________________________________________
Bool_t CbmTrdUnpackFaspAlgo::initParSet(FairParGenericSet* parset)
{
  Int_t nModules(0), nAsics(0);
  if (strcmp(parset->ClassName(), "CbmTrdParSetAsic") == 0) {
    CbmTrdParSetAsic* setPar = static_cast<CbmTrdParSetAsic*>(parset);
    for (auto did : fModuleId) {
      auto setDet = static_cast<CbmTrdParModAsic*>(setPar->GetModulePar(did));
      if (!setDet) continue;
      if (setDet->GetAsicType() != CbmTrdDigi::eCbmTrdAsicType::kFASP) continue;
      if (fMonitor) fMonitor->addParam(did, setDet);
      fAsicSet.addParam(setDet);
      nAsics += setDet->GetNofAsicsOnModule();
      nModules++;
    }
    // fAsicSet.printParams();
    LOG(info) << GetName() << "::initParSet - for container " << parset->ClassName() << " modules " << nModules
              << " asics " << nAsics;
  }
  else if (strcmp(parset->ClassName(), "CbmTrdParSetDigi") == 0) {
    fDigiSet                          = static_cast<CbmTrdParSetDigi*>(parset);
    map<Int_t, CbmTrdParMod*> digiPar = fDigiSet->GetModuleMap();
    for (auto digi : digiPar) {
      fModuleId.emplace_back(digi.first);
      if (fMonitor) fMonitor->addParam(digi.first, (CbmTrdParModDigi*) digi.second);
    }
    // setPar->printParams();
    LOG(info) << GetName() << "::initParSet - for container " << parset->ClassName() << " modules " << fModuleId.size();
  }
  else if (strcmp(parset->ClassName(), "CbmTrdParSetGas") == 0) {
    CbmTrdParSetGas* setPar = static_cast<CbmTrdParSetGas*>(parset);
    setPar->printParams();
    nModules = setPar->GetNrOfModules();
  }
  else if (strcmp(parset->ClassName(), "CbmTrdParSetGain") == 0) {
    CbmTrdParSetGain* setPar = static_cast<CbmTrdParSetGain*>(parset);
    setPar->printParams();
    nModules = setPar->GetNrOfModules();
  }
  else {
    LOG(error) << "Parameter set " << parset->ClassName() << " not known. Skip.";
    return kFALSE;
  }
  return kTRUE;
}

//_________________________________________________________________________________
std::vector<std::pair<std::string, std::shared_ptr<FairParGenericSet>>>*
CbmTrdUnpackFaspAlgo::GetParContainerRequest(std::string geoTag, std::uint32_t runId)
{
  LOG(info) << GetName() << "::GetParContainerRequest - for container " << geoTag.data() << " run " << runId << "  "
            << fParFilesBasePath.data();

  // Basepath for default Trd parameter sets (those connected to a geoTag)
  std::string basepath = Form("%s/trd_%s", fParFilesBasePath.data(), geoTag.data());
  std::string temppath = "";

  // Digest the runId information in case of runId = 0 we use the default fall back
  std::string runpath = "";
  if (runId != 0) {
    runpath = ".run" + std::to_string(runId);
  }

  temppath = basepath + runpath + ".digi" + ".par";
  fParContVec.emplace_back(std::make_pair(temppath, std::make_shared<CbmTrdParSetDigi>()));
  temppath = basepath + runpath + ".asic" + ".par";
  fParContVec.emplace_back(std::make_pair(temppath, std::make_shared<CbmTrdParSetAsic>()));
  //   temppath = basepath + runpath + ".gas" + ".par";
  //   fParContVec.emplace_back(std::make_pair(temppath, std::make_shared<CbmTrdParSetGas>()));
  //   temppath = basepath + runpath + ".gain" + ".par";
  //   fParContVec.emplace_back(std::make_pair(temppath, std::make_shared<CbmTrdParSetGain>()));

  return &fParContVec;
}

//_________________________________________________________________________________
bool CbmTrdUnpackFaspAlgo::pushDigis(std::vector<CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage> messes, const uint16_t mod_id)
{
  const UChar_t lFasp             = messes[0].getFaspIdMod();
  const CbmTrdParModAsic* asicPar = (CbmTrdParModAsic*) fAsicSet.GetModulePar(mod_id);
  const CbmTrdParFasp* faspPar    = (CbmTrdParFasp*) asicPar->GetAsicPar(mod_id * 1000 + lFasp);
  const CbmTrdParModDigi* digiPar = (CbmTrdParModDigi*) fDigiSet->GetModulePar(mod_id);

  // link data to the position on the padplane
  if (!faspPar) {
    LOG(error) << GetName() << "::pushDigis - Par for FASP " << (int) lFasp << " in module " << mod_id
               << " missing. Skip.";
    return false;
  }
  if (!digiPar) {
    LOG(error) << GetName() << "::pushDigis - DIGI par for module " << mod_id << " missing. Skip.";
    return false;
  }

  ULong64_t tdaqOffset(0);
  if (fMess->version == eMessageVersion::kMessLegacy) {
    // Add DAQ time calibration for legacy FASPRO.
    if (digiPar->GetPadRow(faspPar->GetPadAddress(messes[0].ch)) % 2 == 0) tdaqOffset = 3;
  }
  if (VERBOSE) faspPar->Print();

  for (auto imess : messes) {
    const Int_t pad                     = faspPar->GetPadAddress(imess.ch);
    const CbmTrdParFaspChannel* chCalib = faspPar->GetChannel(imess.ch);
    if (chCalib->IsMasked()) {
      LOG(warn) << GetName() << "::pushDigis - FASP par " << mod_id * 1000 + lFasp << " ch " << int(imess.ch)
                << " masked but have data. Masks need attention.";
    }
    const ULong64_t lTime               = fTime + tdaqOffset + imess.tlab;
    const UShort_t lchR                 = chCalib->HasPairingR() ? imess.data : 0;
    const UShort_t lchT                 = chCalib->HasPairingR() ? 0 : imess.data;
    std::vector<CbmTrdDigi>& digiBuffer = fDigiBuffer[pad];

    if (VERBOSE) {
      const Int_t ch = 2 * pad + chCalib->HasPairingR();
      imess.print();
      printf("fasp[%2d] ch[%4d / %2d] pad[%4d] row[%2d] col[%2d] %c[%4d]\n", lFasp, ch, imess.ch, pad,
             digiPar->GetPadRow(pad), digiPar->GetPadColumn(pad), (chCalib->HasPairingT() ? 'T' : 'R'),
             lchT > 0 ? lchT : lchR);
    }

    if (digiBuffer.size() == 0) {  // init pad position in map and build digi for message
      digiBuffer.emplace_back(pad, lchT, lchR, lTime);
      digiBuffer.back().SetAddressModule(mod_id);
      continue;
    }

    // check if last digi has both R/T message components. Update if not and is within time window
    auto id = digiBuffer.rbegin();  // Should always be valid here.
                                    // No need to extra check
    Double_t r, t;
    Int_t dt;
    const Int_t dtime = (*id).GetTime() - lTime;
    bool use(false);

    if (TMath::Abs(dtime) < 5) {  // test message part of (last) digi
      r = (*id).GetCharge(t, dt);
      if (lchR && r < 0.1) {  // set R charge on an empty slot
        (*id).SetCharge(t, lchR, -dtime);
        use = true;
      }
      else if (lchT && t < 0.1) {  // set T charge on an empty slot
        (*id).SetCharge(lchT, r, +dtime);
        (*id).SetTime(lTime);
        use = true;
      }
    }

    // build digi for message when update failed
    if (!use) {
      digiBuffer.emplace_back(pad, lchT, lchR, lTime);
      digiBuffer.back().SetAddressModule(mod_id);
      id = digiBuffer.rbegin();
    }

    // LEGACY CODE:
    if (fMess->version == eMessageVersion::kMessLegacy) {
      // update charge for previously allocated digis
      // to account for FASPRO ADC buffering and read-out feature
      for (++id; id != digiBuffer.rend(); ++id) {
        r = (*id).GetCharge(t, dt);
        if (lchR && int(r)) {  // update R charge and mark on digi
          (*id).SetCharge(t, lchR, dt);
          (*id).SetFlag(1);
          break;
        }
        else if (lchT && int(t)) {  // update T charge and mark on digi
          (*id).SetCharge(lchT, r, dt);
          (*id).SetFlag(0);
          break;
        }
      }
    }
  }
  messes.clear();

  return true;
}

uint32_t CbmTrdUnpackFaspAlgo::ResetTimeslice()
{
  uint32_t uNbLostDigis = 0;
  /// PAL 03/08/2022: clear internal buffer at latest between two timeslices (TS are self contained!)
  /// D.Smith: As of 27.4.2023 only loops over pads of a single component.

  for (auto pad_id(0); pad_id < NFASPMOD * NFASPCH; pad_id++) {
    if (!fDigiBuffer[pad_id].size()) continue;

    LOG(warn) << fName << "::ResetTimeslice - buffered digi pad=" << pad_id << " store " << fDigiBuffer[pad_id].size()
              << " unprocessed digi.";
    uNbLostDigis += fDigiBuffer[pad_id].size();

    fDigiBuffer[pad_id].clear();
  }
  return uNbLostDigis;
}

void CbmTrdUnpackFaspAlgo::FinalizeComponent()
{
  Double_t r, t;
  Int_t dt;
  // push finalized digits to the next level
  for (uint16_t ipad(0); ipad < NFASPMOD * NFASPCH; ipad++) {
    if (!fDigiBuffer[ipad].size()) continue;
    uint nIncomplete(0);
    for (auto id = fDigiBuffer[ipad].begin(); id != fDigiBuffer[ipad].end(); id++) {
      r = (*id).GetCharge(t, dt);

      // LEGACY CODE:
      if (fMess->version == eMessageVersion::kMessLegacy) {
        // check if digi has all signals CORRECTED
        if (((t > 0) != (*id).IsFlagged(0)) || ((r > 0) != (*id).IsFlagged(1))) {
          nIncomplete++;
          continue;
        }
      }
      if (fMonitor) fMonitor->FillHistos((&(*id)));
      // reset flags as they were used only to mark the correctly setting of the charge/digi
      (*id).SetFlag(0, false);
      (*id).SetFlag(1, false);
      /** Convert global time from clk temporary representation to the finale version in [ns]
      * Correct the time with the system time offset which is derived in calibration.*/
      uint64_t gtime = (*id).GetTime() * 12.5;  // fAsicClockPeriod;
      if (gtime >= uint64_t(fSystemTimeOffset)) gtime -= fSystemTimeOffset;
      (*id).SetTime(gtime);
      fOutputVec.emplace_back(std::move((*id)));
    }
    // clear digi buffer wrt the digi which was forwarded to higher structures
    fDigiBuffer[ipad].clear();
    if (nIncomplete > 2) {
      LOG(warn) << fName << "FinalizeComponent() skip " << nIncomplete << " incomplete digi at pad " << ipad << ".\n";
    }
  }
}

// ----unpack----
bool CbmTrdUnpackFaspAlgo::unpack(const fles::Timeslice* ts, std::uint16_t icomp, UInt_t imslice)
{
  if (VERBOSE) printf("CbmTrdUnpackFaspAlgo::unpack 0x%04x %d\n", icomp, imslice);
  LOG(debug2) << "Component " << icomp << " connected to config CbmTrdUnpackConfig2D. Slice " << imslice;

  bool unpackOk = true;
  //Double_t fdMsSizeInNs = 1.28e6;

  auto msdesc = ts->descriptor(icomp, imslice);

  // Cast required to silence a warning on macos (there a uint64_t is a llu)
  if (VERBOSE)
    printf("time start %lu system[0x%x] version[0x%x]\n", static_cast<size_t>(msdesc.idx), msdesc.sys_id,
           msdesc.sys_ver);

  // this is executed only once, at the beginning, after discovering the packing version of the data
  if (!fMess) {
    switch (msdesc.sys_ver) {
      case (int) eMessageVersion::kMessLegacy:
        LOG(info) << "CbmTrdUnpackFaspAlgo::unpack : Legacy version.";
        fMess = new CbmTrdFaspMessage();
        break;
      case (int) eMessageVersion::kMess24:
        LOG(info) << "CbmTrdUnpackFaspAlgo::unpack : 06.2024 version.";
        fMess = new CbmTrdFaspMessage24();
        break;
      default: LOG(fatal) << "CbmTrdUnpackFaspAlgo::unpack : Un-registered version " << msdesc.sys_ver; break;
    }
  }
  // define time wrt start of time slice in TRD/FASP clks [80 MHz]
  fTime = uint64_t((msdesc.idx - fTsStartTime) / 25);
  fTime <<= 1;

  // get MOD_id and ROB id from the equipment, using the comp map: eq_id -> (mod_id, rob_id)
  fMess->mod = fAsicSet.FindModuleByEqId(msdesc.eq_id, fMess->rob, fMess->elink);
  //printf("AB :: eq_id[0x%x] -> rob[%d] link[%d] mod[%d]\n", msdesc.eq_id, fMess->rob, fMess->elink, fMess->mod);
  // Get the µslice size in bytes to calculate the number of completed words
  auto mssize = msdesc.size;

  // Get the number of complete words in the input MS buffer.
  std::uint32_t nwords = mssize / 4;  //fBytesPerWord;

  const auto mspointer = ts->content(icomp, imslice);

  // We have 32 bit spadic frames in this readout version
  const auto mscontent = reinterpret_cast<const size_t*>(mspointer);

  const uint32_t* wd = reinterpret_cast<const uint32_t*>(mscontent);


  uint8_t lFaspOld(0xff);
  vector<CbmTrdFaspMessage> vMess;
  for (uint64_t j = 0; j < nwords; j++, wd++) {
    uint32_t w = *wd;
    // Select the appropriate conversion type of the word according to
    // the message version and type
    switch (fMess->getType(w)) {
      case eMessageType::kData: fMess->readDW(w); break;
      case eMessageType::kEpoch: fMess->readEW(w); break;
      default: break;
    }

    // uint8_t ch_id   = w & 0xf;
    // uint8_t isaux   = (w >> 4) & 0x1;
    // uint8_t slice   = (w >> 5) & 0x7f;
    // uint16_t data   = (w >> 12) & 0x3fff;
    // uint32_t epoch  = (w >> 5) & 0x1fffff;
    // uint8_t fasp_id = ((w >> 26) & 0x3f) + crob_id * NFASPCROB;
    // std::cout<<"fasp_id="<<static_cast<unsigned int>(fasp_id)<<" ch_id="<<static_cast<unsigned int>(ch_id)<<" isaux="<<static_cast<unsigned int>(isaux)<<std::endl;
    if (fMess->type == (int) eMessageType::kEpoch) {
      if (!fMess->ch) {
        // clear buffer
        if (vMess.size()) pushDigis(vMess, fMess->mod);
        vMess.clear();

        if (VERBOSE) fMess->print();

        lFaspOld = 0xff;
        fTime += FASP_EPOCH_LENGTH;
      }
      else {
        LOG(error) << "CbmTrdUnpackFaspAlgo::unpack : Epoch message with wrong signature. Ask an expert.";
      }
      continue;
    }

    if (lFaspOld != fMess->getFaspIdMod()) {
      // push
      if (vMess.size()) pushDigis(vMess, fMess->mod);
      vMess.clear();
      lFaspOld = fMess->getFaspIdMod();
    }
    if (VERBOSE) fMess->print();
    vMess.emplace_back(*fMess);
    //vMess.emplace_back(crob_id, lFaspOld, ch_id, kData, slice, data >> 1);
  }
  return unpackOk;
}

//_________________________________________________________________________________
CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage::CbmTrdFaspMessage(uint8_t rdaq, uint8_t asic, uint8_t c, uint8_t typ,
                                                           uint8_t t, uint16_t d, uint8_t lnk)
  : ch(c)
  , type(typ)
  , tlab(t)
  , data(d)
  , rob(rdaq)
  , elink(lnk)
  , fasp(asic)
{
}

//_________________________________________________________________________________
CbmTrdUnpackFaspAlgo::eMessageType CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage::getType(uint32_t wd) const
{
  if ((wd >> (int) eMessageLength::kMessCh) & 0x1) return eMessageType::kEpoch;
  return eMessageType::kData;
}

//_____________________________________________________________
void CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage::readDW(uint32_t w)
{
  uint8_t shift(0);
  ch = w & 0xf;
  shift += uint8_t(eMessageLength::kMessCh);
  type = (w >> shift) & 0x1;
  shift += uint8_t(eMessageLength::kMessType);
  tlab = (w >> shift) & 0x7f;
  shift += uint8_t(eMessageLength::kMessTlab);
  data = (w >> shift) & 0x3fff;
  data = data >> 1;
  shift += uint8_t(eMessageLength::kMessData);
  fasp = ((w >> shift) & 0x3f);

  if (VERBOSE >= 2) {
    printf("legacyMess_readDW[%x]\n", w);
    print();
  }
}

//_________________________________________________________________________________
void CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage::readEW(uint32_t w)
{
  uint8_t shift(0);
  ch = w & 0xf;
  shift += uint8_t(eMessageLength::kMessCh);
  type = (w >> shift) & 0x1;
  shift += uint8_t(eMessageLength::kMessType);
  epoch = (w >> shift) & 0x1fffff;
  shift += uint8_t(eMessageLength::kMessEpoch);
  fasp = (w >> shift) & 0x3f;

  if (VERBOSE >= 2) {
    printf("legacyMess_readEW[%x]\n", w);
    print();
  }
}

//_________________________________________________________________________________
void CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage::print() const
{
  if (type == (uint8_t) eMessageType::kData)
    cout << Form("    DATA : rob=%d%c fasp_id=%02d [%03d] ch_id=%02d tclk=%03d data=%4d\n", rob, (elink ? 'u' : 'd'),
                 fasp, getFaspIdMod(), ch, tlab, data);
  else if (type == (uint8_t) eMessageType::kEpoch)
    cout << Form("    EPOCH: eq_id=%d%c ch_id=%02d epoch=%05d\n", rob, (elink ? 'u' : 'd'), ch, epoch);
  else
    cout << "    MTYPE: unknown";
}

//_________________________________________________________________________________
CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage24::CbmTrdFaspMessage24() : CbmTrdFaspMessage()
{
  version = eMessageVersion::kMess24;
}

//_________________________________________________________________________________
CbmTrdUnpackFaspAlgo::eMessageType CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage24::getType(uint32_t wd) const
{
  //printf("mess_type[%x]\n", wd);
  if ((wd >> 31) & 0x1) return eMessageType::kEpoch;
  return eMessageType::kData;
}

//_____________________________________________________________
void CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage24::readDW(uint32_t w)
{
  uint8_t shift(0);
  uint16_t adc_data = (w >> shift) & 0x3fff;
  // TODO This data format version delivers the ADC value as bit_sgn + 13 significant bits
  // TODO The CbmTrdDigi supports digi data with only 12bits unsigned. The first tests will
  // TODO convert the measurement to the old format leaving the implementation of the new storage to // TODO later time.  (AB 14.06.2024)
  uint16_t sign = adc_data >> 13;  // sign
  int value_i;
  if (!sign)
    value_i = adc_data;
  else
    value_i = (-1) * ((adc_data ^ 0xffff) & 0x1fff);
  // convert to 12bit unsigned
  data = (value_i + 0x1fff) >> 2;
  shift += uint8_t(eMessageLength::kMessData);
  tlab = (w >> shift) & 0x7f;
  shift += uint8_t(eMessageLength::kMessTlab);
  ch = (w >> shift) & 0xf;
  shift += uint8_t(eMessageLength::kMessCh);
  fasp = ((w >> shift) & 0x3f);
  shift += uint8_t(eMessageLength::kMessFasp);
  type = (w >> shift) & 0x1;
  shift += uint8_t(eMessageLength::kMessType);

  if (VERBOSE >= 2) {
    printf("v06.24Mess_readDW[%x] signed charge = %+d\n", w, value_i);
    print();
  }
}

//_________________________________________________________________________________
void CbmTrdUnpackFaspAlgo::CbmTrdFaspMessage24::readEW(uint32_t w)
{
  uint8_t shift(0);
  epoch = (w >> shift) & 0x1fffff;
  shift += uint8_t(eMessageLength::kMessEpoch);
  ch = (w >> shift) & 0xf;
  shift += uint8_t(eMessageLength::kMessCh);
  fasp = (w >> shift) & 0x3f;
  shift += uint8_t(eMessageLength::kMessFasp);
  type = (w >> shift) & 0x1;
  shift += uint8_t(eMessageLength::kMessType);

  if (VERBOSE >= 2) {
    printf("v06.24Mess_readEW[%x]\n", w);
    print();
  }
}

ClassImp(CbmTrdUnpackFaspAlgo)
