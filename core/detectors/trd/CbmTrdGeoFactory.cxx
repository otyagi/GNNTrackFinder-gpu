/* Copyright (C) 2024 National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer] */

/**
 * \file CbmTrdGeoFactory.cxx
 * \brief TRD chamber manager class.
 * \author Alexandru Bercuci <abercuci@niham.nipne.ro>
 * \date 01/10/2023
*/
#include "CbmTrdGeoFactory.h"

// #include "CbmTrdConstructionDB.h"
#include "CbmTrdAddress.h"

#include <Logger.h>  // for LOG, Logger

#include <TGeoBBox.h>
#include <TGeoCompositeShape.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoMedium.h>
#include <TGeoVolume.h>
#include <TROOT.h>

#include <cassert>
using namespace cbm::trd;
using namespace cbm::trd::geo;

const char* ChamberBuilder::Component::fgName[(int) ChamberBuilder::eGeoPart::kNparts] = {"Radiator", "Window",
                                                                                          "Volume", "BackPanel", "FEB"};
//________________________________________________________________________________________
const TGeoMedium* cbm::trd::geo::GetMaterial(const char* mname)
{
  if (fMaterial.find(mname) == fMaterial.end()) {
    LOG(error) << "GetMaterial(" << mname << ") failed.";
    return nullptr;
  }
  return fMaterial.at(mname);
}

//__________________________________________________________________
bool cbm::trd::geo::ReadModuleInfo(const char* modTxt, info_t& info)
{
  // parse module info
  string modName  = modTxt;
  auto posstart   = modName.find("module") + 6;
  uint ndigits    = 0;
  auto partoftype = modName.at(posstart);
  while (std::isdigit(partoftype) && (ndigits + posstart) < modName.size()) {
    partoftype = modName.at(posstart + ndigits);
    ++ndigits;
  }
  info.type = stoi(modName.substr(posstart, ndigits));  // 6th element+ module type

  posstart += ndigits;
  int modCopyNo = stoi(modName.substr(posstart, string::npos));
  if ((modCopyNo / 100000000) < 0) {
    LOG(warning) << modTxt << " Module in ancient (< 2013) format. Ask expert.";
    return false;
  }

  // >= 2014 format has 9 digits
  // In TGeoManager numbering starts with 1, so we have to subtract 1.
  // int modCopy   = ((modCopyNo / 1000000) % 100);  // from module copy number
  info.id       = (modCopyNo % 1000) - 1;
  info.superId  = ((modCopyNo / 1000) % 100) - 1;
  info.rotation = ((modCopyNo / 100000) % 10);  // from module copy number
  info.address  = CbmTrdAddress::GetAddress(info.superId, info.id, 0, 0, 0);
  return true;
}

//__________________________________________________________________
bool cbm::trd::geo::ReadFebInfo(const char* febTxt, info_t& info)
{
  // parse FEB info
  string febName = febTxt;
  auto posstart  = febName.find("FEB") + 3;
  try {
    info.type      = stoi(febName.substr(posstart, 2));
    info.superType = info.type / 10;
    if (info.superType < 0 || info.superType > 2) throw eException::invalid_type;
    info.type = info.type % 10;

    int febNo = stoi(febName.substr(posstart + 3, string::npos));
    info.id   = febNo % 100;
    if (info.superType == 0 && info.id >= faspFeb[info.type].nmax) throw eException::invalid_id;
    info.superId  = (febNo / 100) % 1000;
    info.rotation = (febNo / 100000) % 10;
    if (info.rotation > 1) throw eException::invalid_id;
  }
  catch (invalid_argument a) {  // syntax exceptions
    LOG(error) << "FEB name does not follow syntax rules : \"FEBxy_identifier\", \"x\" = superType, \"y\" = Type. "
                  "Couldn't initialize.";
    return false;
  }
  catch (eException e) {  // TRD exceptions
    LOG(error) << "FEB name does not follow syntax rules";
    switch (e) {
      case eException::invalid_type: LOG(info) << " : Family type not recognized"; break;
      case eException::invalid_id: LOG(info) << " : Local id not recognized"; break;
      default: LOG(info) << " : Un-identified violation."; break;
    }
    return false;
  }

  return true;
}

//__________________________________________________________________
int cbm::trd::geo::WriteModuleInfo(info_t* /*info*/)
{
  /**  Encryption of module info into the node number
 * format        : CCRLMMMM
 * info.address  : not stored, derived from  CbmTrdAddress::GetAddress(ly, mod, 0, 0, 0)
 * info.superId  : layer_id [L]  
 * info.id       : mod_id / layer [M]
 * info.type     : type according to family
 * info.rotation : rotation in 90deg steps [R]
 * copy number   : [C]
 */
  int modInfo = 0;  //info.id + info.superId * 100;
  return modInfo;
}

//__________________________________________________________________
int cbm::trd::geo::WriteFebInfo(info_t* info)
{
  /**  Encryption of FEB info into the node number
 * format        : RPPPII
 * info.address  : 
 * info.superId  : identification of FEB in production DB [P]   
 * info.id       : identification of FEB placement on the module  [I] 
 * info.type     : Board version [T]
 * info.rotation : rotation in 180deg steps [R]
 * copy number   : not used
 */
  if (info == nullptr) {
    LOG(error) << "WriteFebInfo : Info for FEB is missing";
    return -1;
  }
  if (info->id < 0 || info->superId < 0 || info->rotation < 0) {
    LOG(warning) << "WriteFebInfo : Info for FEB is incomplete : id=" << info->id << " superId=" << info->superId
                 << " rotatation=" << info->rotation;
    return -1;
  }
  int febInfo = info->id + 100 * info->superId + 100000 * info->rotation;
  return febInfo;
}

//________________________________________________________________________________________
ChamberBuilder::ChamberBuilder(int typ) : FairTask(Form("module%d", typ))
{
  fChmbTyp = typ;

  fMaterial["air"]           = nullptr;
  fMaterial["TRDpefoam20"]   = nullptr;
  fMaterial["TRDG10"]        = nullptr;
  fMaterial["TRDkapton"]     = nullptr;
  fMaterial["TRDgas"]        = nullptr;
  fMaterial["TRDcopper"]     = nullptr;
  fMaterial["TRDaramide"]    = nullptr;
  fMaterial["TRDcarbon"]     = nullptr;
  fMaterial["aluminium"]     = nullptr;
  fMaterial["polypropylene"] = nullptr;
  fMaterial["silicon"]       = nullptr;

  fComponent[(int) eGeoPart::kRadiator]  = nullptr;
  fComponent[(int) eGeoPart::kWindow]    = new Window;
  fComponent[(int) eGeoPart::kVolume]    = new Volume;
  fComponent[(int) eGeoPart::kBackPanel] = new BackPanel;
  fComponent[(int) eGeoPart::kFEB]       = nullptr;
}

//________________________________________________________________________________________
InitStatus ChamberBuilder::Init()
{
  if (HasRadiator()) fComponent[(int) eGeoPart::kRadiator] = new Radiator;
  if (HasFEB()) fComponent[(int) eGeoPart::kFEB] = new FEB;
  switch (fChmbTyp) {
    case 1:
      LOG(info) << "Init for TRD2D.";
      SetFEE(fConfig, (bool) eAsic::kFasp);
      SetPP(fConfig, (bool) ePadPlane::k2d);
      activeAreaX = activeAreaY = 54;
      sizeX = sizeY = 57;
      break;
    case (int) eModuleTypes1D::kLowChDensitySmallR:
    case (int) eModuleTypes1D::kHighChDensityLargeR:
    case (int) eModuleTypes1D::kLowChDensityLargeR:
      SetFEE(fConfig, (bool) eAsic::kSpadic);
      SetPP(fConfig, (bool) ePadPlane::k1d);
      LOG(info) << "Init for TRD1D type " << fChmbTyp << ".";
      break;
    default: LOG(fatal) << "Unknown TRD chamber type " << fChmbTyp << ". Abort."; break;
  }

  // Activate materials od the TRD geometry
  TGeoManager* gGeoMan = (TGeoManager*) gROOT->FindObject("FAIRGeom");
  assert(gGeoMan);
  for (auto& imat : fMaterial) {
    imat.second = gGeoMan->GetMedium(imat.first.data());
    assert(imat.second);
  }

  // Instantiate the construction data base of the TRD system
  // if (!gDB) {
  //   assert((gDB = ConstructionDB::Instantiate()));
  // }

  // Init sub-components
  for (auto icomp : fComponent) {
    if (!icomp) continue;
    auto status = icomp->Init();
    if (status != kSUCCESS) return status;
  }
  return kSUCCESS;
}

//________________________________________________________________________________________
void ChamberBuilder::Exec(Option_t*)
{
  // estimate total module height and define center
  int idx(0);
  double vh[(int) eGeoPart::kNparts];
  bool kAdd(true);
  double hOffset(0.), hTot(0.);
  for (auto icomp : fComponent) {
    if (!icomp) continue;
    vh[idx] = icomp->GetHeight();
    hTot += vh[idx];
    if (kAdd) {
      if (idx == (int) eGeoPart::kVolume) {
        hOffset += vh[idx] / 2;
        kAdd = false;
      }
      else
        hOffset += vh[idx];
    }
    idx++;
  }
  // add global z offset
  hOffset = hTot / 2;
  fVol    = new TGeoVolume(Form("module%d", fChmbTyp), new TGeoBBox("", sizeX / 2, sizeY / 2, hTot / 2),
                        cbm::trd::geo::GetMaterial("air"));
  fVol->SetLineColor(kGreen);
  fVol->SetTransparency(80);

  // Stack-up all sub-components
  idx = 0;
  double hh(-hTot / 2);
  info_t infoFeb;
  for (auto icomp : fComponent) {
    if (!icomp) continue;
    hh += 0.5 * vh[idx];
    switch (idx) {
      case (int) eGeoPart::kFEB: {  // special case for feb multiple placement.
        // FEB characteristics and identification stored in geometry
        auto vFeb = new TGeoVolume(icomp->GetName(), new TGeoBBox("", sizeX / 2, sizeY / 2, vh[idx] / 2));
        for (int ifeb(0), jfeb(0); ifeb < Nfebs; ifeb++) {
          infoFeb.id       = ifeb;
          infoFeb.superId  = 0;  // gDB.GetFebId(imod, ifeb);
          infoFeb.rotation = 0;  // gDB.GetFebRot(imod, ifeb);
          if ((jfeb = WriteFebInfo(&infoFeb)) < 0) continue;
          vFeb->AddNode(icomp->fVol, jfeb, new TGeoTranslation("", feb_pos[ifeb][0], feb_pos[ifeb][1], 0.));
        }
        fVol->AddNode(vFeb, 1, new TGeoTranslation("", 0, 0, hh));
      } break;
      default: fVol->AddNode(icomp->fVol, 1, new TGeoTranslation("", 0., 0., hh)); break;
    }

    hh += 0.5 * vh[idx++];
  }
}
//________________________________________________________________________________________
double ChamberBuilder::Component::GetCenter() const
{
  if (!fVol) return 0.;
  double zlo, zhi;
  fVol->GetShape()->GetAxisRange(3., zlo, zhi);
  return 0.5 * (zlo + zhi);
}
// double ChamberBuilder::Component::GetHeight() const
// {
//   if (!fVol) return 0.;
//   double zlo, zhi, hh = fVol->GetShape()->GetAxisRange(2, zlo, zhi);
//   return hh;
// }

//________________________________________________________________________________________
ChamberBuilder::Window::Window() : Component("Window") { ; }

InitStatus ChamberBuilder::Window::Init()
{
  const double hc_size_x  = activeAreaX;
  const double hc_size_y  = 2 * 0.3 + activeAreaY;
  const double win_size_x = hc_size_x + 2 * WIN_FrameX_thickness;
  const double win_size_y = hc_size_y + 2 * WIN_FrameY_thickness;

  // Carbon fiber layers
  TGeoBBox* winIn_C = new TGeoBBox("winIn_C", win_size_x / 2., win_size_y / 2., winIn_C_thickness / 2.);
  TGeoVolume* vol_winIn_C =
    new TGeoVolume("winIn_C", winIn_C, cbm::trd::geo::GetMaterial("TRDcarbon") /*carbonVolMed*/);
  vol_winIn_C->SetLineColor(kBlack);  //kGray);
  fHeight = winIn_C_thickness;

  // Honeycomb layer
  TGeoBBox* winIn_HC       = new TGeoBBox("winIn_HC", hc_size_x / 2., hc_size_y / 2., winIn_HC_thickness / 2.);
  TGeoVolume* vol_winIn_HC = new TGeoVolume("winIn_HC", winIn_HC, cbm::trd::geo::GetMaterial("TRDaramide"));
  vol_winIn_HC->SetLineColor(kOrange);
  fHeight += winIn_HC_thickness;

  // framex
  TGeoBBox* winIn_fx = new TGeoBBox("winIn_fx", (hc_size_x + 2 * WIN_FrameX_thickness) / 2, WIN_FrameY_thickness / 2,
                                    winIn_HC_thickness / 2.);
  TGeoVolume* vol_winIn_fx = new TGeoVolume("winIn_fx", winIn_fx, /*frameVolMed*/ cbm::trd::geo::GetMaterial("TRDG10"));
  vol_winIn_fx->SetLineColor(kBlue);
  TGeoBBox* winIn_xout = new TGeoBBox("winIn_xout", hc_size_x / 2 + 2 * WIN_FrameX_thickness, WIN_OutY_thickness / 2,
                                      winIn_HC_thickness / 2.);
  TGeoVolume* vol_winIn_xout =
    new TGeoVolume("winIn_xout", winIn_xout, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_winIn_xout->SetLineColor(kBlue + 2);

  // framey
  TGeoBBox* winIn_fy       = new TGeoBBox("winIn_fy", WIN_FrameX_thickness / 2, hc_size_y / 2, winIn_HC_thickness / 2.);
  TGeoVolume* vol_winIn_fy = new TGeoVolume("winIn_fy", winIn_fy, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_winIn_fy->SetLineColor(kCyan);
  TGeoBBox* winIn_k =
    new TGeoBBox("winIn_k", WIN_FrameX_thickness / 2, hc_size_y / 2 + WIN_FrameY_thickness, winIn_HC_thickness / 2.);
  TGeoVolume* vol_winIn_k = new TGeoVolume("winIn_k", winIn_k, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_winIn_k->SetLineColor(kViolet);
  TGeoBBox* winIn_yout =
    new TGeoBBox("winIn_yout", WIN_OutX_thickness / 2, hc_size_y / 2 + WIN_FrameY_thickness + WIN_OutY_thickness,
                 winIn_HC_thickness / 2.);
  TGeoVolume* vol_winIn_yout =
    new TGeoVolume("winIn_yout", winIn_yout, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_winIn_yout->SetLineColor(kViolet + 5);

  // Add up all sub-components
  fVol =
    new TGeoVolume(GetName(), new TGeoBBox("", sizeX / 2, sizeY / 2, fHeight / 2), cbm::trd::geo::GetMaterial("air"));
  fVol->SetLineColor(kOrange);
  fVol->SetTransparency(50);

  double x, y;
  fHeight = -fHeight / 2 + winIn_HC_thickness / 2;
  fVol->AddNode(vol_winIn_HC, 1, new TGeoTranslation("", 0., 0., fHeight));
  y = (hc_size_y + WIN_FrameY_thickness) / 2.;
  fVol->AddNode(vol_winIn_fx, 1, new TGeoTranslation("", 0., y, fHeight));
  fVol->AddNode(vol_winIn_fx, 2, new TGeoTranslation("", 0., -y, fHeight));
  y += 0.5 * (WIN_FrameY_thickness + WIN_OutY_thickness);
  fVol->AddNode(vol_winIn_xout, 1, new TGeoTranslation("", 0., y, fHeight));
  fVol->AddNode(vol_winIn_xout, 2, new TGeoTranslation("", 0., -y, fHeight));
  x = (hc_size_x + WIN_FrameX_thickness) / 2.;
  fVol->AddNode(vol_winIn_fy, 1, new TGeoTranslation("", x, 0., fHeight));
  fVol->AddNode(vol_winIn_fy, 2, new TGeoTranslation("", -x, 0., fHeight));
  x += WIN_FrameX_thickness;
  fVol->AddNode(vol_winIn_k, 1, new TGeoTranslation("", x, 0., fHeight));
  fVol->AddNode(vol_winIn_k, 2, new TGeoTranslation("", -x, 0., fHeight));
  x += 0.5 * (WIN_FrameX_thickness + WIN_OutX_thickness);
  fVol->AddNode(vol_winIn_yout, 1, new TGeoTranslation("", x, 0., fHeight));
  fVol->AddNode(vol_winIn_yout, 2, new TGeoTranslation("", -x, 0., fHeight));

  fHeight += 0.5 * (winIn_HC_thickness + winIn_C_thickness);
  fVol->AddNode(vol_winIn_C, 1, new TGeoTranslation("", 0., 0., fHeight));
  fHeight += 0.5 * winIn_C_thickness;
  fHeight *= 2;
  return kSUCCESS;
}

//________________________________________________________________________________________
ChamberBuilder::Volume::Volume() : Component("Volume") { ; }

InitStatus ChamberBuilder::Volume::Init()
{
  // Gas. The volume has to be defined only for pads (read-out) area. Take care in the DigiPara definition
  TGeoBBox* gas       = new TGeoBBox("trd_gas", 0.5 * activeAreaX, 0.5 * activeAreaY, 0.5 * gas_thickness);
  TGeoVolume* vol_gas = new TGeoVolume("gas", gas, cbm::trd::geo::GetMaterial("TRDgas") /*gasVolMed*/);
  vol_gas->SetLineColor(kRed);
  //vol_gas->SetTransparency(80);
  TGeoBBox* gas_ext       = new TGeoBBox("trd_gas_dstr", 0.5 * activeAreaX, 0.5 * gas_extra, 0.5 * gas_thickness);
  TGeoVolume* vol_gas_ext = new TGeoVolume("gas_ext", gas_ext, cbm::trd::geo::GetMaterial("TRDgas") /*gasVolMed*/);
  vol_gas_ext->SetLineColor(kMagenta);
  //vol_gas_ext->SetTransparency(80);
  fHeight = gas_thickness;

  const double gas_size_x = activeAreaX;
  const double gas_size_y = activeAreaY + 2 * gas_extra;

  // framex
  auto* gas_xin = new TGeoBBox("gas_xin", gas_size_x / 2 + cathode_width, WIN_OutY_thickness / 2, gas_thickness / 2.);
  auto* vol_gas_xin = new TGeoVolume("gas_xin", gas_xin, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_gas_xin->SetLineColor(kViolet + 5);
  auto* gas_xout           = new TGeoBBox("gas_xout", gas_size_x / 2 + cathode_width, WIN_OutY_thickness / 2,
                                (gas_thickness /*+ ridge_height*/) / 2.);
  TGeoVolume* vol_gas_xout = new TGeoVolume("gas_xout", gas_xout, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_gas_xout->SetLineColor(kViolet + 5);
  // framey
  TGeoBBox* gas_k       = new TGeoBBox("gas_k", cathode_width / 2, gas_size_y / 2, ledge_thickness / 2.);
  TGeoVolume* vol_gas_k = new TGeoVolume("gas_k", gas_k, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_gas_k->SetLineColor(kViolet);
  TGeoBBox* gas_a       = new TGeoBBox("gas_a", anode_width / 2, gas_size_y / 2, ledge_thickness / 2.);
  TGeoVolume* vol_gas_a = new TGeoVolume("gas_a", gas_a, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_gas_a->SetLineColor(kViolet + 2);
  TGeoBBox* gas_d       = new TGeoBBox("gas_d", dist_width / 2, gas_size_y / 2, ledge_thickness / 2.);
  TGeoVolume* vol_gas_d = new TGeoVolume("gas_d", gas_d, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_gas_d->SetLineColor(kViolet + 4);
  TGeoBBox* gas_yout       = new TGeoBBox("gas_yout", WIN_OutX_thickness / 2, 2 * WIN_OutY_thickness + gas_size_y / 2,
                                    (gas_thickness /*+ ridge_height*/) / 2.);
  TGeoVolume* vol_gas_yout = new TGeoVolume("gas_yout", gas_yout, cbm::trd::geo::GetMaterial("TRDG10") /*frameVolMed*/);
  vol_gas_yout->SetLineColor(kViolet + 5);

  // Add up all sub-components
  fVol =
    new TGeoVolume(GetName(), new TGeoBBox("", sizeX / 2, sizeY / 2, fHeight / 2), cbm::trd::geo::GetMaterial("air"));
  fVol->SetLineColor(kYellow);
  fVol->SetTransparency(50);

  double x, y;
  fHeight = 0.;
  fVol->AddNode(vol_gas, 0, new TGeoTranslation("", 0., 0., fHeight));
  x = 0.5 * (gas_size_x + cathode_width);
  fVol->AddNode(vol_gas_k, 1, new TGeoTranslation("", x, 0., fHeight - ledge_thickness));
  fVol->AddNode(vol_gas_k, 2, new TGeoTranslation("", -x, 0., fHeight - ledge_thickness));
  x = 0.5 * (gas_size_x + anode_width);
  fVol->AddNode(vol_gas_a, 1, new TGeoTranslation("", x, 0., fHeight));
  fVol->AddNode(vol_gas_a, 2, new TGeoTranslation("", -x, 0., fHeight));
  x = 0.5 * (gas_size_x + dist_width);
  fVol->AddNode(vol_gas_d, 1, new TGeoTranslation("", x, 0., fHeight + ledge_thickness));
  fVol->AddNode(vol_gas_d, 2, new TGeoTranslation("", -x, 0., fHeight + ledge_thickness));
  x = 0.5 * gas_size_x + cathode_width + 0.5 * WIN_OutX_thickness;
  fVol->AddNode(vol_gas_yout, 1, new TGeoTranslation("", x, 0., fHeight /* + ridge_height / 2*/));
  fVol->AddNode(vol_gas_yout, 2, new TGeoTranslation("", -x, 0., fHeight /* + ridge_height / 2*/));
  y = 0.5 * (activeAreaY + gas_extra);
  fVol->AddNode(vol_gas_ext, 0, new TGeoTranslation("", 0., y, fHeight));
  fVol->AddNode(vol_gas_ext, 1, new TGeoTranslation("", 0., -y, fHeight));
  y += 0.5 * (gas_extra + WIN_OutY_thickness);
  fVol->AddNode(vol_gas_xin, 1, new TGeoTranslation("", 0, y, fHeight));
  fVol->AddNode(vol_gas_xin, 2, new TGeoTranslation("", 0, -y, fHeight));
  y += WIN_OutY_thickness;
  fVol->AddNode(vol_gas_xout, 1, new TGeoTranslation("", 0, y, fHeight /* + ridge_height / 2*/));
  fVol->AddNode(vol_gas_xout, 2, new TGeoTranslation("", 0, -y, fHeight /* + ridge_height / 2*/));
  fHeight += gas_thickness;
  return kSUCCESS;
}

//________________________________________________________________________________________
ChamberBuilder::BackPanel::BackPanel() : Component("BackPanel") { ; }
InitStatus ChamberBuilder::BackPanel::Init()
{
  const double hc_size_x  = activeAreaX;
  const double hc_size_y  = activeAreaY - 2 * BKP_OutY_correct;
  const double bkp_size_x = hc_size_x + 2 * BKP_Frame_width;
  const double bkp_size_y = hc_size_y + 2 * BKP_Frame_width;

  // Pad Copper
  TGeoBBox* pp       = new TGeoBBox("pp_cu", activeAreaX / 2., activeAreaY / 2., pp_pads_thickness / 2.);
  TGeoVolume* vol_pp = new TGeoVolume("pp_cu", pp, /*padcopperVolMed*/ cbm::trd::geo::GetMaterial("TRDcopper"));
  vol_pp->SetLineColor(kBlue);
  fHeight = pp_pads_thickness;

  // Pad Plane
  TGeoBBox* pp_PCB       = new TGeoBBox("pp_pcb", bkp_size_x / 2., bkp_size_y / 2., pp_pcb_thickness / 2.);
  TGeoVolume* vol_pp_PCB = new TGeoVolume("pp_pcb2d", pp_PCB, /*padpcbVolMed*/ cbm::trd::geo::GetMaterial("TRDG10"));
  vol_pp_PCB->SetLineColor(kGreen);
  fHeight += pp_pcb_thickness;

  // Perforated BackPanel structure
  auto vol_bp      = new TGeoVolumeAssembly("bkp_int");
  auto vol_bp_ASIC = new TGeoVolumeAssembly("");
  // HC : 3 components (HC, PCB, Cu) with 2 sizes
  Color_t bp_col[3]      = {kOrange, kGray, kRed + 2};
  const char* matName[3] = {"TRDaramide", "TRDG10", "TRDcopper"};
  // build one BP unit
  double dxHC[2] = {hc_unitx / 2., (hc_unitx - hc_holex) / 4.}, dyHC[2] = {(hc_unity - hc_holey) / 4., hc_holey / 2.},
         hHCx((hc_unitx + hc_holex) / 4.), hHCy((hc_unity + hc_holey) / 4.), hHC(0.),
         hHCz[] = {hc_thickness, cu_pcb_thickness, cu_thickness};
  for (int ibpz(0); ibpz < 3; ibpz++) {
    hHC += 0.5 * hHCz[ibpz];
    for (int ibpy(-1), jbp(0); ibpy < 2; ibpy += 2) {
      auto bpShp   = new TGeoBBox("", dxHC[0], dyHC[0], hHCz[ibpz] / 2.);
      auto vol_cmp = new TGeoVolume("", bpShp, cbm::trd::geo::GetMaterial(matName[ibpz]));
      vol_cmp->SetLineColor(bp_col[ibpz]);
      vol_bp_ASIC->AddNode(vol_cmp, jbp++, new TGeoTranslation("", 0., ibpy * hHCy, hHC));
    }
    for (int ibpx(-1), jbp(0); ibpx < 2; ibpx += 2) {
      auto bpShp   = new TGeoBBox("", dxHC[1], dyHC[1], hHCz[ibpz] / 2.);
      auto vol_cmp = new TGeoVolume("", bpShp, cbm::trd::geo::GetMaterial(matName[ibpz]));
      vol_cmp->SetLineColor(bp_col[ibpz]);
      vol_bp_ASIC->AddNode(vol_cmp, jbp++, new TGeoTranslation("", ibpx * hHCx, 0., hHC));
    }
    hHC += 0.5 * hHCz[ibpz];
  }
  // build BP plate
  for (Int_t c(0), ifc(0); c < 9; c++) {
    for (Int_t r(0); r < 10; r++) {
      vol_bp->AddNode(vol_bp_ASIC, ifc++, new TGeoTranslation("", (c - 4) * hc_unitx, hc_unity * (0.5 + r), 0.));
      vol_bp->AddNode(vol_bp_ASIC, ifc++, new TGeoTranslation("", (c - 4) * hc_unitx, -hc_unity * (r + 0.5), 0.));
    }
  }
  fHeight += hHC;

  // framex
  auto xoutBd =
    new TGeoBBox("", hc_size_x / 2, (BKP_OutY_thickness - BKP_OutY_correct) / 2, (hHC - BKP_Frame_closure) / 2.);
  auto xvolBd       = new TGeoVolume("", xoutBd, cbm::trd::geo::GetMaterial("TRDG10"));
  auto xoutFc       = new TGeoBBox("", hc_size_x / 2, (BKP_Frame_width - BKP_OutY_correct) / 2, BKP_Frame_closure / 2.);
  auto xvolFc       = new TGeoVolume("", xoutFc, cbm::trd::geo::GetMaterial("TRDG10"));
  auto vol_bkp_xout = new TGeoVolumeAssembly("bkp_xout");
  vol_bkp_xout->AddNode(
    xvolFc, 1, new TGeoTranslation("", 0, (BKP_Frame_width - BKP_OutY_thickness) / 2, (BKP_Frame_closure - hHC) / 2));
  vol_bkp_xout->AddNode(xvolBd, 1, new TGeoTranslation("", 0., 0., BKP_Frame_closure / 2));
  vol_bkp_xout->SetLineColor(kViolet + 2);

  // framey
  auto youtBd =
    new TGeoBBox("", BKP_OutX_thickness / 2, hc_size_y / 2 + BKP_OutY_thickness, (hHC - BKP_Frame_closure) / 2.);
  auto yvolBd       = new TGeoVolume("", youtBd, cbm::trd::geo::GetMaterial("TRDG10"));
  auto youtFc       = new TGeoBBox("", BKP_Frame_width / 2, hc_size_y / 2 + BKP_Frame_width, BKP_Frame_closure / 2.);
  auto yvolFc       = new TGeoVolume("", youtFc, cbm::trd::geo::GetMaterial("TRDG10"));
  auto vol_bkp_yout = new TGeoVolumeAssembly("bkp_yout");
  vol_bkp_yout->AddNode(yvolFc, 1,
                        new TGeoTranslation("t_bkp_yout_fc", (BKP_Frame_width - BKP_OutX_thickness) / 2, 0.,
                                            (BKP_Frame_closure - hHC) / 2));
  vol_bkp_yout->AddNode(yvolBd, 1, new TGeoTranslation("t_bkp_yout_bd", 0., 0., BKP_Frame_closure / 2));
  vol_bkp_yout->SetLineColor(kViolet + 2);

  // Add up all components
  fVol = new TGeoVolume(GetName(), new TGeoBBox("", bkp_size_x / 2, bkp_size_y / 2, fHeight / 2),
                        cbm::trd::geo::GetMaterial("air"));
  fVol->SetLineColor(kOrange);
  fVol->SetTransparency(50);

  double x, y;
  fHeight = -fHeight / 2 + 0.5 * pp_pads_thickness;
  fVol->AddNode(vol_pp, 1, new TGeoTranslation("", 0., 0., fHeight));
  fHeight += 0.5 * (pp_pads_thickness + pp_pcb_thickness);
  fVol->AddNode(vol_pp_PCB, 1, new TGeoTranslation("", 0., 0., fHeight));
  fHeight += 0.5 * pp_pcb_thickness;
  fVol->AddNode(vol_bp, 1, new TGeoTranslation("", 0., 0., fHeight));
  fHeight += 0.5 * hHC;

  x            = 0.5 * (hc_size_x + BKP_OutX_thickness);
  auto* fy_tra = new TGeoTranslation("", x, 0., fHeight);
  fVol->AddNode(vol_bkp_yout, 1, fy_tra);
  auto* fy_rot = new TGeoRotation();
  fy_rot->RotateZ(180.);
  auto* fy_mat = new TGeoHMatrix("");
  (*fy_mat)    = (*fy_rot) * (*fy_tra);
  fVol->AddNode(vol_bkp_yout, 2, fy_mat);
  y      = 0.5 * (hc_size_y + BKP_OutY_thickness + BKP_OutY_correct);
  fy_tra = new TGeoTranslation("", 0., y, fHeight);
  fVol->AddNode(vol_bkp_xout, 1, fy_tra);
  fy_mat    = new TGeoHMatrix("");
  (*fy_mat) = (*fy_rot) * (*fy_tra);
  fVol->AddNode(vol_bkp_xout, 2, fy_mat);
  fHeight += 0.5 * hHC;
  fHeight *= 2;
  return kSUCCESS;
}

//________________________________________________________________________________________
ChamberBuilder::FEB::FEB() : Component("FEB") { ; }

InitStatus ChamberBuilder::FEB::Init()
{
  // Create the FASPRO FEBs out of all CU/PCB layers
  fHeight = FASPRO_zspace;

  TString scu = "", spcb = "";
  TGeoTranslation* tr(nullptr);
  double FASPRO_thickness(0.);
  for (int ily(0); ily < FASPRO_Nly; ily++) {
    // effective Cu layer thickness  = h [um * 10-4] * coverage [% * 10-2]
    double lyThickEff = FASPRO_ly_cu[ily][0] * FASPRO_ly_cu[ily][1] * 1.e-6;
    new TGeoBBox(Form("faspro_ly%02d", ily), FASPRO_length / 2., FASPRO_width / 2., lyThickEff / 2.);
    FASPRO_thickness += lyThickEff / 2;
    tr = new TGeoTranslation(Form("t_faspro_ly%02d", ily), 0., 0., FASPRO_thickness);
    tr->RegisterYourself();
    scu += Form("%cfaspro_ly%02d:t_faspro_ly%02d", (ily ? '+' : ' '), ily, ily);
    FASPRO_thickness += lyThickEff / 2;
    if (ily == FASPRO_Nly - 1) break;  // skip for FR4
    // the FEB dielectric made of PCB
    double lyThickPcb = 1.e-4 * FASPRO_ly_pcb[ily] / 2.;
    new TGeoBBox(Form("faspro_ly%02d_pcb", ily), FASPRO_length / 2., FASPRO_width / 2., lyThickPcb);
    FASPRO_thickness += lyThickPcb;
    tr = new TGeoTranslation(Form("t_faspro_ly%02d_pcb", ily), 0., 0., FASPRO_thickness);
    tr->RegisterYourself();
    spcb += Form("%cfaspro_ly%02d_pcb:t_faspro_ly%02d_pcb", (ily ? '+' : ' '), ily, ily);
    FASPRO_thickness += lyThickPcb;
  }
  for (int ihole(0); ihole < FASPRO_Nfasp; ihole++) {
    new TGeoBBox(Form("faspro_hole%d", ihole), FASPRO_hole_x / 2., FASPRO_hole_y / 2., 1.e-4 + FASPRO_thickness / 2.);
    tr =
      new TGeoTranslation(Form("t_faspro_hole%d", ihole), HOLE_pos[ihole][0], HOLE_pos[ihole][1], FASPRO_thickness / 2);
    tr->RegisterYourself();
    scu += Form("-faspro_hole%d:t_faspro_hole%d", ihole, ihole);
    spcb += Form("-faspro_hole%d:t_faspro_hole%d", ihole, ihole);
  }
  auto faspro_cu     = new TGeoCompositeShape("faspro_cu", scu.Data());
  auto vol_faspro_cu = new TGeoVolume("faspro_cu", faspro_cu, cbm::trd::geo::GetMaterial("TRDcopper"));
  vol_faspro_cu->SetLineColor(kRed - 3);  //vol_faspro_cu->SetTransparency(50);
  auto faspro_pcb     = new TGeoCompositeShape("faspro_pcb", spcb.Data());
  auto vol_faspro_pcb = new TGeoVolume("faspro_pcb", faspro_pcb,
                                       cbm::trd::geo::GetMaterial("TRDG10") /*febVolMed*/);  // the FEB made of PCB
  vol_faspro_pcb->SetLineColor(kGreen + 3);
  //vol_faspro_pcb->SetTransparency(50);
  fHeight += FASPRO_thickness;

  // create FASP ASIC
  auto fasp     = new TGeoBBox("fasp", FASP_x / 2., FASP_y / 2., FASP_z / 2.);
  auto vol_fasp = new TGeoVolume("fasp", fasp, cbm::trd::geo::GetMaterial("silicon"));
  vol_fasp->SetLineColor(kBlack);
  // create ADC ASIC
  auto adc     = new TGeoBBox("adc", ADC_x / 2., ADC_y / 2., ADC_z / 2.);
  auto vol_adc = new TGeoVolume("adc", adc, cbm::trd::geo::GetMaterial("silicon"));
  vol_adc->SetLineColor(kBlack);
  // create FPGA ASIC
  auto fpga     = new TGeoBBox("fpga", FPGA_x / 2., FPGA_y / 2., FPGA_z / 2.);
  auto vol_fpga = new TGeoVolume("fpga", fpga, cbm::trd::geo::GetMaterial("silicon"));
  vol_fpga->SetLineColor(kBlack);
  // create DCDC ASIC
  auto dcdc     = new TGeoBBox("dcdc", DCDC_x / 2., DCDC_y / 2., DCDC_z / 2.);
  auto vol_dcdc = new TGeoVolume("dcdc", dcdc, cbm::trd::geo::GetMaterial("silicon"));
  vol_dcdc->SetLineColor(kBlack);
  // create FC Coonector
  auto connFc      = new TGeoBBox("connFc", ConnFC_x / 2., ConnFC_y / 2., ConnFC_z / 2.);
  auto vol_conn_fc = new TGeoVolume("connFc", connFc, cbm::trd::geo::GetMaterial("polypropylene"));
  vol_conn_fc->SetLineColor(kYellow);
  // create BRIDGE Coonector
  auto connBrg      = new TGeoBBox("connBrg", ConnBRG_x / 2., ConnBRG_y / 2., ConnBRG_z / 2.);
  auto vol_conn_brg = new TGeoVolume("connBrg", connBrg, cbm::trd::geo::GetMaterial("polypropylene"));
  vol_conn_brg->SetLineColor(kYellow + 2);
  fHeight += ConnBRG_z;

  // Init volume:
  // FEB family FASPRO
  // FEB type v1 (12 FASPs)
  int fType = 1;
  fVol      = new TGeoVolumeAssembly(Form("%s1%d", GetName(), fType));
  fVol->SetLineColor(kGreen);
  fVol->SetTransparency(50);

  // Add up all components
  fHeight = -0.5 * fHeight + FASPRO_zspace;
  fVol->AddNode(vol_faspro_cu, 1, new TGeoTranslation("", 0., 0., fHeight));
  fVol->AddNode(vol_faspro_pcb, 1, new TGeoTranslation("", 0., 0., fHeight));
  // add FASPs on the back side of the FEB
  info_t infoAsic;
  for (int ifasp(0), jfasp(0); ifasp < faspFeb[fType].nasic; ifasp++) {
    vol_fasp->SetTitle(Form("%x", 0xff /*gDB->GetASICMask*/));
    // if ((jfasp = WriteAsicInfo(&infoAsic)) < 0) continue;
    fVol->AddNode(vol_fasp, jfasp,
                  new TGeoTranslation("", FASP_pos[ifasp][0], FASP_pos[ifasp][1], fHeight - FASP_z / 2));
  }
  fHeight += FASPRO_thickness;
  // add ADCs, FPGAs and DCDC converters on the tob side of the FEB
  for (int iadc(0); iadc < FASPRO_Nadc; iadc++)
    fVol->AddNode(vol_adc, iadc + 1, new TGeoTranslation("", ADC_pos[iadc][0], ADC_pos[iadc][1], fHeight + ADC_z / 2));
  for (int ifpga(0); ifpga < FASPRO_Nfpga; ifpga++)
    fVol->AddNode(vol_fpga, ifpga + 1,
                  new TGeoTranslation("", FPGA_pos[ifpga][0], FPGA_pos[ifpga][1], fHeight + FPGA_z / 2));
  for (int idcdc(0); idcdc < FASPRO_Ndcdc; idcdc++)
    fVol->AddNode(vol_dcdc, idcdc + 1,
                  new TGeoTranslation("", DCDC_pos[idcdc][0], DCDC_pos[idcdc][1], fHeight + DCDC_z / 2));
  // add connectors to the FEB
  for (int ifasp(0); ifasp < FASPRO_Nfasp; ifasp++)
    fVol->AddNode(vol_conn_fc, ifasp + 1,
                  new TGeoTranslation("", ConnFC_pos[ifasp][0], ConnFC_pos[ifasp][1], fHeight + ConnFC_z / 2));
  for (int iconn(0); iconn < 2; iconn++)
    fVol->AddNode(vol_conn_brg, iconn + 1,
                  new TGeoTranslation("", ConnBRG_pos[iconn][0], ConnBRG_pos[iconn][1], fHeight + ConnBRG_z / 2));
  fHeight += ConnBRG_z;
  fHeight *= 2;
  return kSUCCESS;
}

//________________________________________________________________________________________
ChamberBuilder::Radiator::Radiator() : Component("Radiator") { ; }

InitStatus ChamberBuilder::Radiator::Init()
{
  TGeoBBox* trd_radiator = new TGeoBBox("trd_radiator", sizeX / 2., sizeY / 2., radiator_thickness / 2.);
  fVol = new TGeoVolume("Radiator", trd_radiator, cbm::trd::geo::GetMaterial("TRDpefoam20") /*radVolMed*/);
  fVol->SetLineColor(kRed);
  //trdmod1_radvol->SetTransparency(50);  // set transparency for the TRD radiator

  fHeight = radiator_thickness;
  return kSUCCESS;
}
/* clang-format off */
// NamespaceImp(cbm::trd::geo)
ClassImp(cbm::trd::geo::ChamberBuilder)
ClassImp(cbm::trd::geo::ChamberBuilder::Component)
ClassImp(cbm::trd::geo::ChamberBuilder::Radiator)
ClassImp(cbm::trd::geo::ChamberBuilder::Window)
ClassImp(cbm::trd::geo::ChamberBuilder::Volume)
ClassImp(cbm::trd::geo::ChamberBuilder::BackPanel)
ClassImp(cbm::trd::geo::ChamberBuilder::FEB)
  /* clang-format on */
