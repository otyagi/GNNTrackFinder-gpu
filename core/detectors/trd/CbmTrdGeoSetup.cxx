/* Copyright (C) 2024 National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer] */

#include "CbmTrdGeoSetup.h"

#include "CbmTrdGeoFactory.h"

#include <FairParGenericSet.h>  // for FairParGenericSet
#include <FairParamList.h>      // for FairParamList
#include <FairRunAna.h>         // for FairRunAna
#include <FairRuntimeDb.h>      // for FairRuntimeDb
#include <Logger.h>             // for LOG

#include <TArrayI.h>      // for TArrayI
#include <TDatime.h>      // for TDatime::AsString()
#include <TGeoElement.h>  // for active volume material interogation
#include <TGeoManager.h>  // for TGeoManager, gGeoManager
#include <TGeoNode.h>     // for TGeoNode
#include <TObjArray.h>    // for TObjArray
#include <TSystem.h>      // for TSystem::GetUserInfo()

using namespace cbm::trd::geo;


//__________________________________________________________________
InitStatus SetupManager::Init()
{
  InitStatus stat;
  TGeoNode* topNode = gGeoManager->GetTopNode();
  TObjArray* nodes  = topNode->GetNodes();
  for (Int_t iNode = 0; iNode < nodes->GetEntriesFast(); iNode++) {
    TGeoNode* trdGeo = static_cast<TGeoNode*>(nodes->At(iNode));
    if (string(trdGeo->GetName()).find("trd") != 0) continue;  // trd_vXXy top node, e.g. trd_v13a, trd_v14b
    fGeoTag = trdGeo->GetName();

    // init the setup. Collect meta info
    string uname = "John Doe", ucontact = "not-defined", udescr = "not-defined";
    // retrieve user name from the system
    auto uinfo = gSystem->GetUserInfo(gSystem->GetEffectiveUid());
    if (!uinfo)
      LOG(warning) << "Couldn't read user_name.";
    else
      uname = uinfo->fRealName;
    if (fContact.compare("") == 0)
      LOG(warning) << "No contact info were provided for responsible " << uname;
    else
      ucontact = fContact;
    if (fDescription.compare("") == 0)
      LOG(error) << "No comments provided for setup. Please add them by SetupManager::SetDescription()";
    else
      udescr = fDescription;

    // build setup adding all meta info
    uname += ";";
    uname += ucontact;
    TDatime d;
    uname += ";";
    uname += d.AsString();
    uname += ";";
    uname += udescr;
    Setup* setup = new Setup(fGeoTag.data(), uname.data());
    Setup::Module modSetup;
    //fHardwareSetup.SelectComponentIdMap(fGeometryTag);
    TObjArray* layers = trdGeo->GetNodes();
    for (Int_t iLayer = 0; iLayer < layers->GetEntriesFast(); iLayer++) {
      TGeoNode* lyGeo = static_cast<TGeoNode*>(layers->At(iLayer));
      if (string(lyGeo->GetName()).find("layer") != 0) continue;  // only layers

      TObjArray* modules = lyGeo->GetNodes();
      for (Int_t iModule = 0; iModule < modules->GetEntriesFast(); iModule++) {
        TGeoNode* modGeo = static_cast<TGeoNode*>(modules->At(iModule));
        // skip geo elements which are not modules
        if (string(modGeo->GetName()).find("module") != 0) continue;

        LOG(info) << " Reading module " << modGeo->GetName() << " [" << lyGeo->GetName() << "].";
        stat = modSetup.init(modGeo);
        switch (stat) {
          case kERROR: continue;
          case kFATAL: LOG(fatal) << GetName() << " Couldn't process geometry file."; break;
          case kSUCCESS: setup->addParam(new Setup::Module(modSetup)); break;
        }
      }     // loop over TRD modules / layers
    }       // loop over TRD layers
    break;  // loop over CBM systems
  }
  return kSUCCESS;
}

//__________________________________________________________________
void SetupManager::SetParContainers()
{
  FairRuntimeDb* rtdb = FairRunAna::Instance()->GetRuntimeDb();
  fSetup              = (Setup*) (rtdb->getContainer("TrdSetup"));
}

//__________________________________________________________________
void SetupManager::Finish()
{
  FairRuntimeDb* rtdb = FairRunAna::Instance()->GetRuntimeDb();
  fSetup              = (Setup*) (rtdb->getContainer("TrdSetup"));
  //fSetup->Print();
}

//__________________________________________________________________
Setup::Setup(const char* n, const char* t) : FairParGenericSet(n, t, "default")
{
  /** Define complementing info besides the geo description for the TRD system setup.
   * n = geo tag; e.g. for "trd_v22d_mcbm.geo.root" name = "v22d_mcbm"
   * t = meta info; list of ";" separated information in the following order
   *  - "name"  : responsible person's name
   *  - "email" : responsible person's email
   *  - "date"  : date of creation
   */
  fMetaFields.push_back("name");
  fMetaFields.push_back("email");
  fMetaFields.push_back("date");
  size_t ii(0);
  if ((ii = Parse()) != fMetaFields.size()) {
    Help();
    LOG(warning) << "Number of info fields " << ii << " less than required (" << fMetaFields.size()
                 << "). Info might be spoiled. Check input.\n";
  }
  else {
    for (auto info : fMeta) {
      if (strcmp(info.second.data(), "") != 0) continue;
      Help(info.first.data());
    }
  }
}

//__________________________________________________________________
const char* Setup::GetInfo(const char* label) const
{
  if (fMeta.find(label) == fMeta.end()) {
    Help(label);
    return nullptr;
  }
  return fMeta.at(label).data();
}

//__________________________________________________________________
void Setup::Help(const char* lab) const
{
  LOG(info) << "* meta info; list of \";\" separated information in the following order:";
  LOG(info) << "*  - \"name\"  : responsible person's name";
  LOG(info) << "*  - \"email\" : responsible person's email";
  LOG(info) << "*  - \"date\"  : date of creation";

  if (lab) {
    if (fMeta.find(lab) == fMeta.end())
      LOG(error) << "Meta info \"" << lab << "\" not defined.\n";
    else
      LOG(warning) << "Meta info for\"" << lab << "\" not registered.\n";
  }
}

//__________________________________________________________________
size_t Setup::Parse()
{
  size_t idx(0);
  string s(GetTitle());
  char* p = strtok(s.data(), ";");
  while (p != nullptr && idx < fMetaFields.size()) {
    fMeta[fMetaFields[idx++]] = p;
    p                         = strtok(nullptr, ";");
  }
  return idx;
}

//_______________________________________________________________________________
Setup::~Setup() { fModule.clear(); }

//_______________________________________________________________________________
int Setup::GetModuleId(int i) const
{
  if (i < 0 || i >= (int) GetNrOfModules()) return -1;
  uint16_t id = fModule[i]->GetModuleId();
  if (id == 0xffff)
    return -1;
  else
    return id;
}

//_______________________________________________________________________________
const Setup::Module* Setup::GetModulePar(int detId) const
{
  for (auto mod : fModule)
    if (mod->GetModuleId() == detId) return mod;
  return nullptr;
}

//_______________________________________________________________________________
bool Setup::getParams(FairParamList* l)
{
  if (!l) return false;
  if (!l->fill("Version", &fVersion)) {
    LOG(error) << GetName() << "::getParams : Couldn't find \"Version\"";
    return false;
  }
  int nmods(0);
  if (!l->fill("NrOfModules", &nmods)) {
    LOG(error) << GetName() << "::getParams : Couldn't find \"NrOfModules\"";
    return false;
  }
  TArrayI modId(nmods), typId(nmods), rot(nmods);
  if (!l->fill("Trd.Id", &modId)) {
    LOG(error) << GetName() << "::getParams : Couldn't find \"Trd.Id\"";
    return false;
  }
  if (!l->fill("Trd.Type", &typId)) {
    LOG(error) << GetName() << "::getParams : Couldn't find \"Trd.Type\"";
    return false;
  }
  if (!l->fill("Trd.Rot", &rot)) {
    LOG(error) << GetName() << "::getParams : Couldn't find \"Trd.Rot\"";
    return false;
  }
  Text_t textIn[100];
  if (!l->fill("Trd.Gas", textIn, 100)) {
    LOG(error) << GetName() << "::getParams : Couldn't find \"Trd.Gas\"";
    return false;
  }
  if (textIn[0] == 'A' && textIn[1] == 'r')
    fGas = eGas::kAr;
  else
    fGas = eGas::kXe;

  for (int imod(0); imod < nmods; imod++) {
    fModule.push_back(new Module(Form("Trd%s.%d", (modId[imod] < 0 ? "2D" : "1D"), abs(modId[imod])), GetTitle()));
    auto mod   = fModule.back();
    mod->fType = typId[imod];
    mod->fRot  = rot[imod];
    mod->getParams(l);
  }
  return true;
}


//_______________________________________________________________________________
void Setup::putParams(FairParamList* l)
{
  if (!l) return;
  l->add("Version", fVersion);
  int ii(0), nmods = (int) GetNrOfModules();
  l->add("NrOfModules", nmods);
  TArrayI modId(nmods), typId(nmods), rot(nmods);
  for (auto mod : fModule) {
    modId[ii] = mod->GetModuleId() * (mod->GetFamily() == ePadPlane::k1d ? 1 : -1);
    typId[ii] = mod->GetType();
    rot[ii]   = mod->GetRotation();
    ii++;
  }
  l->add("Trd.Id", modId);
  l->add("Trd.Type", typId);
  l->add("Trd.Rot", rot);
  if (fGas == eGas::kAr)
    l->add("Trd.Gas", "ArCO2");
  else
    l->add("Trd.Gas", "XeCO2");
  for (auto mod : fModule)
    mod->putParams(l);
}

//_______________________________________________________________________________
void Setup::addParam(Module* mod) { fModule.push_back(mod); }

//_______________________________________________________________________________
Setup::Module::Module(const char* n, const char* t) : TNamed(n, t)
{
  //   string name = n;
  //   if (name.size() > 6) {
  //     if (name.substr(3, 2).compare("1D") == 0) fFamily = ePadPlane::k1d;
  //     else
  //       fFamily = ePadPlane::k2d;
  //     fId = stoi(name.substr(6, 10));
  //   }
}

//_______________________________________________________________________________
Setup::Module::Module(const Module& m0) : TNamed((const TNamed&) m0)
{
  fId      = m0.fId;
  fFee     = m0.fFee;
  fFamily  = m0.fFamily;
  fWindow  = m0.fWindow;
  fGas     = m0.fGas;
  fType    = m0.fType;
  fFeeType = m0.fFeeType;
  fRot     = m0.fRot;
  fDaq     = m0.fDaq;
  fFEE     = m0.fFEE;
}

//_______________________________________________________________________________
InitStatus Setup::Module::init(TGeoNode* n)
{
  bool hasRadiator(false);
  info_t info;
  if (!ReadModuleInfo(n->GetName(), info)) return kERROR;

  TObjArray* components = n->GetNodes();
  for (Int_t icomp = 0; icomp < components->GetEntriesFast(); icomp++) {
    TGeoNode* comp = static_cast<TGeoNode*>(components->At(icomp));
    string cname(comp->GetName());

    if (cname.find(ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kRadiator]) == 0) {
      LOG(debug4) << GetName() << " Initialize " << comp->GetName() << " with "
                  << ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kRadiator];
      hasRadiator = true;  // to be used when defining entrance window params
    }
    else if (cname.find(ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kWindow]) == 0) {
      LOG(debug4) << GetName() << " Initialize " << comp->GetName() << " with "
                  << ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kWindow];
      if (!hasRadiator)
        fWindow = eWindow::kNotSet;
      else {  // determine if the module have a thin (1D) or thick (2D) entrance window
        int selectFeature(0);
        TIter ipcs(comp->GetNodes());
        while (auto pcs = (TGeoNode*) ipcs()) {
          if (string(pcs->GetName()).find("winIn_C") == 0)
            selectFeature++;
          else if (string(pcs->GetName()).find("winIn_HC") == 0)
            selectFeature++;
        }
        if (selectFeature == 2)
          fWindow = eWindow::kThick;
        else
          fWindow = eWindow::kThin;

        LOG(info) << GetName() << " Initialize TR absorption in " << (fWindow == eWindow::kThick ? "thick" : "thin")
                  << " entrance window.";
      }
    }
    else if (cname.find(ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kVolume]) == 0) {
      LOG(debug4) << GetName() << " Initialize " << comp->GetName() << " with "
                  << ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kVolume];
      // determine the active gas
      auto gas = (TGeoNode*) comp->GetNodes()->FindObject("gas_0");
      if (!gas)
        LOG(error) << GetName() << " Couldn't find \"gas_0\" in " << comp->GetName();
      else {
        auto activeGas = gas->GetMedium()->GetMaterial();
        for (int imat(0); imat < activeGas->GetNelements(); imat++) {
          auto elem = activeGas->GetElement(imat);
          switch (elem->Z()) {
            case 6:
            case 8: break;  // do nothing for C and O
            case 18:        // found Ar
              fGas = eGas::kAr;
              break;
            case 54:  // found Xe
              fGas = eGas::kXe;
              break;
            default:
              elem->Print();
              LOG(error) << GetName() << " Couldn't found element in active gas from " << comp->GetName();
              break;
          }
        }
      }
      if (fGas == eGas::kNotSet) {
        LOG(warning) << GetName() << " Couldn't identify active gas type. Use legacy mode.";
        return kERROR;
      }
      LOG(info) << GetName() << " Initialize active gas to " << (fGas == eGas::kXe ? "XeCO2." : "ArCO2.");
    }
    else if (cname.find(ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kBackPanel]) == 0) {
      LOG(debug4) << GetName() << " Initialize " << comp->GetName() << " with "
                  << ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kBackPanel];
      // find the type of the pad-plane and consequently the TRD family
      TIter ibkp(comp->GetNodes());
      while (auto bkp = (TGeoNode*) ibkp()) {
        if (string(bkp->GetName()).find("pp_pcb2d") == 0) {
          fFamily = ePadPlane::k2d;  // 2d
          SetName(Form("Trd2D.%d", info.address));
          break;
        }
        else if (string(bkp->GetName()).find("pp_pcb1d") == 0) {
          fFamily = ePadPlane::k1d;  // 1d
          SetName(Form("Trd1D.%d", info.address));
          break;
        }
      }
      if (fFamily == ePadPlane::kNotSet) {
        LOG(error) << GetName() << " Couldn't identify TRD module family for " << n->GetName() << " " << n->GetTitle();
        return kFATAL;
      }
      LOG(info) << GetName() << " Initialize TRD family to " << (fFamily == ePadPlane::k2d ? "2D." : "1D;");
    }
    else if (cname.find(ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kFEB]) == 0) {
      LOG(info) << GetName() << " Initialize " << comp->GetName() << " with "
                << ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kFEB];
      // find the FEE family, number of ASICs / module and eventually mapping
      if (!ReadFebInfo(comp->GetName(), info)) return kERROR;
    }
    else {
      LOG(info) << GetName() << "Geometry of module " << n->GetName() << " uses legacy setup.";
      return kERROR;
    }
    //           if (!TString(part->GetName()).BeginsWith("gas_")) continue;  // only active gas volume
    //
    //           // Put together the full path to the interesting volume, which
    //           // is needed to navigate with the geomanager to this volume.
    //           // Extract the geometry information (size, global position)
    //           // from this volume.
    //           TString path = TString("/") + topNode->GetName() + "/" + station->GetName() + "/" + layer->GetName() + "/"
    //                          + module->GetName() + "/" + part->GetName();
    //
    //           CreateModuleParameters(path);
  }
  return kSUCCESS;
}

//_______________________________________________________________________________
void Setup::Module::putParams(FairParamList* l)
{
  int ii(0);
  TArrayI daqId(fDaq.size());
  for (auto ic : fDaq)
    daqId[ii++] = ic;
  l->add(Form("%s.DaqInfo", GetName()), daqId);

  ii        = 0;
  auto pads = fFEE[0]->GetPads();
  TArrayI asicSetup(fFEE.size() * (pads.size() + 2));
  for (auto asic : fFEE) {
    asicSetup[ii++] = asic->GetId();  // asic id in the module
    asicSetup[ii++] = asic->GetMask();
    pads            = asic->GetPads();
    for (auto pad : pads)
      asicSetup[ii++] = pad;
  }
  l->add(Form("%s.%sInfo", GetName(), (fFee == eAsic::kSpadic ? "Spadic" : "Fasp")), asicSetup);
}

//_______________________________________________________________________________
bool Setup::Module::getParams(FairParamList* l)
{
  READOUT pMod = (fFamily == ePadPlane::k1d ? mod1D[fType] : mod2D[fType]);
  FEB pFeb     = faspFeb[fFeeType];
  int ndaq     = pMod.ndaq;
  int nasic    = pMod.nasic;
  int nch      = pFeb.nchannels;
  TArrayI daqId(ndaq);
  if (!l->fill(Form("%s.DaqInfo", GetName()), &daqId)) {
    LOG(error) << GetName() << "::getParams : Couldn't find \"DaqInfo\"";
    return false;
  }
  fDaq.assign(daqId.GetArray(), daqId.GetArray() + ndaq);

  TArrayI asicInfo(nasic);
  if (!l->fill(Form("%s.SpadicInfo", GetName()), &asicInfo)) fFee = eAsic::kSpadic;
  if (!l->fill(Form("%s.FaspInfo", GetName()), &asicInfo))
    fFee = eAsic::kFasp;
  else {
    LOG(error) << GetName() << "::getParams : Couldn't find \"AsicInfo\"";
    return false;
  }
  for (int i(0), k(0); i < nasic; i++) {
    fFEE.push_back(new Asic(Form("%s%d", (fFee == eAsic::kSpadic ? "Spadic" : "Fasp"), i), GetName()));
    auto asic     = fFEE.back();
    asic->fUnique = asicInfo[k++];
    asic->fMask   = asicInfo[k++];
    asic->fPad.assign(&asicInfo[k], &asicInfo[k + nch]);
    k += nch;
  }

  return true;
}

//_______________________________________________________________________________
Setup::Asic::Asic(const char* n, const char* t) : TNamed(n, t)
{
  string name = n;
  // fFamily = eAsic::kFasp;
  int nskip(4);
  if (name.find("Spadic") == 0) {
    // fFamily = eAsic::kSpadic;
    nskip = 6;
  }
  fId = stoi(name.substr(nskip, 10));
}

/* clang-format off */
// NamespaceImp(cbm::trd::geo)
ClassImp(cbm::trd::geo::SetupManager)
ClassImp(cbm::trd::geo::Setup)
ClassImp(cbm::trd::geo::Setup::Module)
ClassImp(cbm::trd::geo::Setup::Asic)
  /* clang-format on */
