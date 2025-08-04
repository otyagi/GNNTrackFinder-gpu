/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer], Norbert Herrmann */

#include "CbmTimesliceManager.h"

#include "CbmEvent.h"                     // For CbmEvent
#include "CbmTimeslicePixelHitSetDraw.h"  // For CbmTimeslicePixelHitSetDraw
#include "CbmTimesliceRecoTracks.h"       // For CbmTimesliceRecoTracks
#include "CbmTsDisTofTracklets.h"         // for CbmTofTracklets
#include "CbmTsPointSetArrayDraw.h"       // For CbmTsPointSetArrayDraw
#include "FairRootManager.h"              // for FairRootManager
#include "FairTask.h"                     // for FairTask
#include "FairXMLNode.h"                  // for FairXMLNode and FairXMLFile

#include <TClonesArray.h>  // for TClonesArray
#include <TDatabasePDG.h>  // for TDatabasePDG
#include <TEveBrowser.h>
#include <TEveGeoNode.h>  // for TEveGeoTopNode
#include <TEveManager.h>  // for TEveManager, gEve
#include <TEveProjectionManager.h>
#include <TEveProjections.h>  // for TEveProjection, TEveProjection::k...
#include <TEveScene.h>
#include <TEveText.h>
#include <TEveTrans.h>
#include <TEveViewer.h>
#include <TEveWindow.h>  // for TEveWindowPack, TEveWindowSlot
#include <TGFileDialog.h>
#include <TGLAnnotation.h>
#include <TGLCameraOverlay.h>
#include <TGLClip.h>  // for TGLClip, TGLClip::kClipPlane, TGL...
#include <TGLFontManager.h>
#include <TGLLightSet.h>
#include <TGLViewer.h>
#include <TGeoBBox.h>
#include <TGeoManager.h>  // for gGeoManager, TGeoManager
#include <TGeoNode.h>
#include <TGeoVolume.h>  // for TGeoVolume
#include <TVector3.h>

// +++++++++++++++++++++++++++++++++++++++++++++ Public +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
CbmTimesliceManager* CbmTimesliceManager::gRinstanceTsMan = 0;

CbmTimesliceManager* CbmTimesliceManager::Instance() { return gRinstanceTsMan; }

CbmTimesliceManager::CbmTimesliceManager() : TEveEventManager("CbmTimesliceManager", "")
{
  /// Initialize instance pointer with this object when create directly
  gRinstanceTsMan = this;

  fRunAna      = FairRunAna::Instance();
  fRootManager = FairRootManager::Instance();

  AddParticlesToPdgDataBase();
  InitPdgColorMap();
}

void CbmTimesliceManager::SetDisplayCbmElectron()
{
  CbmTimeslicePixelHitSetDraw* drawStsHit  = new CbmTimeslicePixelHitSetDraw("StsHit", kBlue, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawRichHit = new CbmTimeslicePixelHitSetDraw("RichHit", kCyan, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawTrdHit  = new CbmTimeslicePixelHitSetDraw("TrdHit", kYellow, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawTofHit  = new CbmTimeslicePixelHitSetDraw("TofHit", kRed, kFullCircle);
  CbmTimeslicePixelHitSetDraw* drawPsdHit  = new CbmTimeslicePixelHitSetDraw("PsdHit", kOrange, kFullSquare);

  CbmTimesliceRecoTracks* drawTrack = new CbmTimesliceRecoTracks();

  AddTask(drawStsHit);
  AddTask(drawRichHit);
  AddTask(drawTrdHit);
  AddTask(drawTofHit);
  AddTask(drawPsdHit);

  AddTask(drawTrack);
}

void CbmTimesliceManager::SetDisplayCbmMuon()
{
  CbmTimeslicePixelHitSetDraw* drawMvdHit  = new CbmTimeslicePixelHitSetDraw("MvdHit", kBlue, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawStsHit  = new CbmTimeslicePixelHitSetDraw("StsHit", kBlue, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawMuchHit = new CbmTimeslicePixelHitSetDraw("MuchHit", kCyan, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawTrdHit  = new CbmTimeslicePixelHitSetDraw("TrdHit", kYellow, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawTofHit  = new CbmTimeslicePixelHitSetDraw("TofHit", kRed, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawPsdHit  = new CbmTimeslicePixelHitSetDraw("PsdHit", kOrange, kFullSquare);

  AddTask(drawMvdHit);
  AddTask(drawStsHit);
  AddTask(drawMuchHit);
  AddTask(drawTrdHit);
  AddTask(drawTofHit);
  AddTask(drawPsdHit);

  CbmTimesliceRecoTracks* drawTrack = new CbmTimesliceRecoTracks();
  AddTask(drawTrack);
}

void CbmTimesliceManager::SetDisplayMcbm(bool bLowRate, bool bMuch)
{
  CbmTimeslicePixelHitSetDraw* drawStsHit  = new CbmTimeslicePixelHitSetDraw("StsHit", kBlue, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawMuchHit = new CbmTimeslicePixelHitSetDraw("MuchHit", kCyan, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawTrdHit  = new CbmTimeslicePixelHitSetDraw("TrdHit", kYellow, kFullSquare);
  CbmTimeslicePixelHitSetDraw* drawTofHit  = new CbmTimeslicePixelHitSetDraw("TofHit", kRed, kFullCircle);
  CbmTimeslicePixelHitSetDraw* drawRichHit = new CbmTimeslicePixelHitSetDraw("RichHit", kOrange, kFullSquare);

  AddTask(drawStsHit);
  if (bMuch) {  //
    AddTask(drawMuchHit);
  }
  AddTask(drawTrdHit);
  AddTask(drawTofHit);
  if (bLowRate) {  //
    AddTask(drawRichHit);
  }

  CbmTimesliceRecoTracks* drawTrack = new CbmTimesliceRecoTracks();
  AddTask(drawTrack);

  CbmTsDisTofTracklets* TofTracks =
    new CbmTsDisTofTracklets("TofTracks", 1, kFALSE, kTRUE);  //name, verbosity, RnrChildren points, RnrChildren track
  AddTask(TofTracks);

  CbmTsPointSetArrayDraw* drawTofUHit = new CbmTsPointSetArrayDraw(
    "TofUHit", kBlue, kCross, 1, kTRUE);  //name, colorMode, markerMode, verbosity, RnrChildren
  //CbmTimeslicePixelHitSetDraw* drawTofUHit  = new CbmTimeslicePixelHitSetDraw("TofUHit", kBlue, kCross);
  AddTask(drawTofUHit);

  fbMcbmViewersEna = true;
}

void CbmTimesliceManager::SetDisplayTofCosmicsHd()
{
  CbmTimeslicePixelHitSetDraw* drawTofHit = new CbmTimeslicePixelHitSetDraw("TofHit", kRed, kFullSquare);
  AddTask(drawTofHit);

  CbmTsDisTofTracklets* TofTracks =
    new CbmTsDisTofTracklets("TofTracks", 1, kFALSE, kTRUE);  //name, verbosity, RnrChildren points, RnrChildren track
  AddTask(TofTracks);

  //CbmTsPointSetArrayDraw* drawTofUHit = new CbmTsPointSetArrayDraw("TofUHit", kBlue, kCross, 1, kTRUE);  //name, colorMode, markerMode, verbosity, RnrChildren
  //AddTask(drawTofUHit);

  // Display L1 tracks
  //CbmTimesliceRecoTracks* drawTrack = new CbmTimesliceRecoTracks();
  //AddTask(drawTrack);

  fbMcbmViewersEna = true;
}

void CbmTimesliceManager::SetTransparency(Bool_t use_xml, Int_t trans)
{
  if (use_xml == kFALSE) {
    // high transparency
    Int_t vis_level = gGeoManager->GetVisLevel();
    TGeoNode* top   = gGeoManager->GetTopNode();
    SetTransparencyForLayer(top, vis_level, trans);
  }
  else {
    // normal transparency
    if (fXMLConfig != "") {  //
      LoadXMLSettings();
    }
    else {
      Int_t vis_level = gGeoManager->GetVisLevel();
      TGeoNode* top   = gGeoManager->GetTopNode();
      SetTransparencyForLayer(top, vis_level, 0);
    }
  }
  if (gEve->GetGlobalScene()->GetRnrState()) {
    gEve->GetGlobalScene()->SetRnrState(kFALSE);
    gEve->GetGlobalScene()->SetRnrState(kTRUE);
    gEve->Redraw3D();
  }
}

void CbmTimesliceManager::SwitchBackground(Bool_t /*light*/)
{
  /// PAL 31/05/2023: No parameter possible to SwitchColorSet of TEveViewerList in recent root versions !?!
  gEve->GetViewers()->SwitchColorSet();
}

void CbmTimesliceManager::SwitchPdgColorTrack(Bool_t pdg_color)
{
  /// Get List of tasks from FairRunAna
  TList* taskList = FairRunAna::Instance()->GetMainTask()->GetListOfTasks();

  /// Tell the one(s) displaying reco tracks to use pdg_color or not
  for (TObject* task : *taskList) {
    if (nullptr != dynamic_cast<CbmTimesliceRecoTracks*>(task)) {
      dynamic_cast<CbmTimesliceRecoTracks*>(task)->SwitchPdgColorTrack(pdg_color);
    }
  }
}

void CbmTimesliceManager::Init(Int_t visopt, Int_t vislvl, Int_t maxvisnds)
{
  TEveManager::Create();
  fRunAna->Init();

  if (!InitializeMainView(visopt, vislvl, maxvisnds)) {
    LOG(fatal) << "CbmTimesliceManager::Init() => Failed main view initialization";
  }

  if (fbMcbmViewersEna) {  //
    InitializeViewsMcbm();
  }
  else {
    InitializeViewsCbm();
  }

  fCbmEvents = dynamic_cast<TClonesArray*>(FairRootManager::Instance()->GetObject("CbmEvent"));

  if (nullptr == fCbmEvents) {
    LOG(fatal) << "CbmTimesliceManager::Init() => CbmEvents branch not found! Task will be deactivated";
  }
}

void CbmTimesliceManager::Open() {}
void CbmTimesliceManager::UpdateEditor() {}
void CbmTimesliceManager::Close() {}
void CbmTimesliceManager::DisplaySettings() {}

Int_t CbmTimesliceManager::Color(int pdg)
{
  if (fPDGToColor.find(pdg) != fPDGToColor.end()) {  //
    return fPDGToColor[pdg];
  }
  return 0;
}

void CbmTimesliceManager::GotoTimeslice(uint32_t event)
{
  fTimesliceIdx = event;
  /// This will force all added tasks to load first event of this TS if possible
  FairRunAna::Instance()->Run(static_cast<Long64_t>(fTimesliceIdx));
}

void CbmTimesliceManager::NextTimeslice()
{
  /// Check if possible (min/max)

  /// Re-use main method
  int NbEventsInTs = 0;
  while (NbEventsInTs == 0) {
    GotoTimeslice(GetCurrentTimeslice() + 1);
    NbEventsInTs = fCbmEvents->GetEntriesFast();
  }
  LOG(debug) << "Display TS " << GetCurrentTimeslice() << " with " << NbEventsInTs << " events";
  SetTsTimeText(GetTimesliceTime());
  SetTsNumberText(GetCurrentTimeslice());
  SetEvtTimeText(GetEventTime());
  SetEvtNumberText(GetCurrentEvent());
}

void CbmTimesliceManager::PrevTimeslice()
{
  /// Check if possible (min/max)

  /// Re-use main method
  GotoTimeslice(GetCurrentTimeslice() - 1);
}

void CbmTimesliceManager::GotoEvent(Int_t event)
{
  fEventIdx = event;

  CbmEvent* pEvent   = dynamic_cast<CbmEvent*>(fCbmEvents->At(fEventIdx));
  Int_t nofTofTracks = pEvent->GetNofData(ECbmDataType::kTofTracklet);
  LOG(debug) << GetName() << " : Ts_nofTofTracks " << nofTofTracks << " in event " << fEventIdx;

  /// Get List of tasks from FairRunAna
  TList* taskList = FairRunAna::Instance()->GetMainTask()->GetListOfTasks();

  /// Tell each of them to go to selected event
  for (TObject* task : *taskList) {
    if (nullptr != dynamic_cast<CbmTimesliceRecoTracks*>(task)) {
      dynamic_cast<CbmTimesliceRecoTracks*>(task)->GotoEvent(event);
    }
    else if (nullptr != dynamic_cast<CbmTimeslicePixelHitSetDraw*>(task)) {
      LOG(debug) << GetName() << ": call " << task->GetName();
      dynamic_cast<CbmTimeslicePixelHitSetDraw*>(task)->GotoEvent(event);
    }
    else if (nullptr != dynamic_cast<CbmTsPointSetArrayDraw*>(task)) {
      LOG(debug) << GetName() << ": call " << task->GetName();
      //dynamic_cast<CbmTsPointSetArrayDraw*>(task)->GotoEvent(event);      // FIXME: does not work yet
    }
    else if (nullptr != dynamic_cast<CbmTsDisTofTracklets*>(task)) {
      LOG(debug) << GetName() << ": call " << task->GetName();
      dynamic_cast<CbmTsDisTofTracklets*>(task)->GotoEvent(event);
    }
  }
}

void CbmTimesliceManager::NextEvent()
{
  /// Check if possible (min/max)

  /// Re-use main method
  GotoEvent(fEventIdx + 1);
}

void CbmTimesliceManager::PrevEvent()
{
  /// Check if possible (min/max)

  /// Re-use main method
  GotoEvent(fEventIdx - 1);
}

double_t CbmTimesliceManager::GetTimesliceTime()
{
  fTimeTimeslice = FairRootManager::Instance()->GetEventTime();
  return fTimeTimeslice;
}

double_t CbmTimesliceManager::GetEventTime()
{
  const CbmEvent* event = dynamic_cast<const CbmEvent*>(fCbmEvents->At(fEventIdx));
  fTimeEvent            = event->GetTzero();
  return fTimeEvent;
}

void CbmTimesliceManager::SetTsTimeText(Double_t time)
{
  TString stime;
  stime.Form("TS Time: %.2f", time);
  stime += " ns";
  fTimesliceTimeText->SetText(stime);
}

void CbmTimesliceManager::SetTsNumberText(Int_t evtNumber)
{
  TString text = "TS: ";
  text += evtNumber;
  fTimesliceNumberText->SetText(text);
}

void CbmTimesliceManager::SetEvtTimeText(Double_t time)
{
  TString stime;
  stime.Form("Event Time: %.2f", time);
  stime += " ns";
  fEventTimeText->SetText(stime);
}

void CbmTimesliceManager::SetEvtNumberText(Int_t evtNumber)
{
  TString text = "Event: ";
  text += evtNumber;
  fEventNumberText->SetText(text);
}

void CbmTimesliceManager::MakeScreenshot(CbmTsEveAnimationControl::eScreenshotType screenshotType, TString path)
{
  TString filename;
  if (path == "") {
    const char* filetypes[] = {"PNG", "*.png", "JPG", "*.jpg", 0, 0};
    TGFileInfo fi;
    fi.fFileTypes = filetypes;
    fi.fIniDir    = StrDup(".");
    new TGFileDialog(gClient->GetRoot(), gEve->GetMainWindow(), kFDSave, &fi);
    if (fi.fFilename == nullptr) {  //
      return;
    }
    filename = fi.fFilename;
  }
  else {
    filename = path;
  }

  if (fbMcbmViewersEna) {
    switch (screenshotType) {
      case CbmTsEveAnimationControl::eScreenshotType::k3D: {
        gEve->GetDefaultGLViewer()->SavePicture(filename);
        break;
      }
      case CbmTsEveAnimationControl::eScreenshotType::kZX: {
        TGLViewer* gl = fViewZX->GetGLViewer();
        gl->SavePicture(filename);
        break;
      }
      case CbmTsEveAnimationControl::eScreenshotType::kZY: {
        TGLViewer* gl = fViewZY->GetGLViewer();
        gl->SavePicture(filename);
        break;
      }
      case CbmTsEveAnimationControl::eScreenshotType::kAll: {
        TString filename_path = filename(0, filename.Last('.'));
        TString filename_ext  = filename(filename.Last('.') + 1, 3);
        TString filename3d    = Form("%s_3d.%s", filename_path.Data(), filename_ext.Data());
        TString filenameZY    = Form("%s_ZY.%s", filename_path.Data(), filename_ext.Data());
        TString filenameZX    = Form("%s_ZX.%s", filename_path.Data(), filename_ext.Data());
        gEve->GetDefaultGLViewer()->SavePicture(filename3d);
        TGLViewer* gl = fViewZY->GetGLViewer();
        gl->SavePicture(filenameZY);
        gl = fViewZX->GetGLViewer();
        gl->SavePicture(filenameZX);
        break;
      }
      default:
        /// Type missmatch, should never happen if Comboboxes properly set but better safe than sorry
        break;
    }
  }
  else {
    switch (screenshotType) {
      case CbmTsEveAnimationControl::eScreenshotType::k3D: {
        gEve->GetDefaultGLViewer()->SavePicture(filename);
        break;
      }
      case CbmTsEveAnimationControl::eScreenshotType::kXY: {
        TGLViewer* gl = fRPhiView->GetGLViewer();
        gl->SavePicture(filename);
        break;
      }
      case CbmTsEveAnimationControl::eScreenshotType::kZ: {
        TGLViewer* gl = fRhoZView->GetGLViewer();
        gl->SavePicture(filename);
        break;
      }
      case CbmTsEveAnimationControl::eScreenshotType::kAll: {
        TString filename_path = filename(0, filename.Last('.'));
        TString filename_ext  = filename(filename.Last('.') + 1, 3);
        TString filename3d    = Form("%s_3d.%s", filename_path.Data(), filename_ext.Data());
        TString filenameRphi  = Form("%s_XY.%s", filename_path.Data(), filename_ext.Data());
        TString filenameRhoz  = Form("%s_Z.%s", filename_path.Data(), filename_ext.Data());
        gEve->GetDefaultGLViewer()->SavePicture(filename3d);
        TGLViewer* gl = fRPhiView->GetGLViewer();
        gl->SavePicture(filenameRphi);
        gl = fRhoZView->GetGLViewer();
        gl->SavePicture(filenameRhoz);
        break;
      }
      default:
        /// Type missmatch, should never happen if Comboboxes properly set but better safe than sorry
        break;
    }
  }
}
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

// ++++++++++++++++++++++++++++++++++++++++++++ Protected +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
void CbmTimesliceManager::LoadXMLSettings()
{
  /// Complement the FairEventManager with a new keyword for setting the node visibility (+ daughters if recursive > 0)
  FairXMLFile xmlfile(fXMLConfig, "read");
  FairXMLNode* xml = xmlfile.GetRootNode();
  for (int i = 0; i < xml->GetNChildren(); i++) {
    TString nodename = xml->GetChild(i)->GetName();
    if (nodename.EqualTo("Detectors")) {
      TGeoNode* top        = gGeoManager->GetTopNode();
      FairXMLNode* top_xml = xml->GetChild(i)->GetChild(0);
      if (top_xml != nullptr) {  //
        LoadXMLDetector(top, top_xml);
      }
    }
    else if (nodename.EqualTo("MCTracksColors")) {
      FairXMLNode* colors = xml->GetChild(i);
      for (int j = 0; j < colors->GetNChildren(); j++) {
        FairXMLNode* color = colors->GetChild(j);
        TString pgd_code   = color->GetAttrib("pdg")->GetValue();
        TString color_code = color->GetAttrib("color")->GetValue();
        //fPDGToColor[pgd_code.Atoi()] = StringToColor(color_code);
      }
    }
  }
  if (gEve->GetGlobalScene()->GetRnrState()) {
    gEve->GetGlobalScene()->SetRnrState(kFALSE);
    gEve->GetGlobalScene()->SetRnrState(kTRUE);
    gEve->Redraw3D();
  }
}

void CbmTimesliceManager::LoadXMLDetector(TGeoNode* node, FairXMLNode* xml, Int_t depth)
{
  /// Complement the FairEventManager with a new keyword for setting the node visibility (+ daughters if recursive > 0)
  TString name      = xml->GetAttrib("name")->GetValue();
  TString node_name = node->GetName();
  Bool_t recursive  = (xml->GetAttrib("recursive")->GetValue().Length() != 0 && !name.EqualTo(node_name));
  if (recursive && depth == 0) {  //
    return;
  }
  TString transparency  = xml->GetAttrib("transparency")->GetValue();
  TString color         = xml->GetAttrib("color")->GetValue();
  TString visibility    = xml->GetAttrib("visibility")->GetValue();
  TString recursive_val = xml->GetAttrib("recursive")->GetValue();
  if (!recursive && "0" != recursive_val) {  //
    LOG(info) << "LoadXMLDetector called for " << node_name;
  }
  if (!color.EqualTo("")) {
    node->GetVolume()->SetFillColor(StringToColor(color));
    node->GetVolume()->SetLineColor(StringToColor(color));
  }
  if (!transparency.EqualTo("")) {  //
    node->GetVolume()->SetTransparency((Char_t)(transparency.Atoi()));
  }
  if (!visibility.EqualTo("")) {
    bool bVisVal = (0 < visibility.Atoi());
    node->SetVisibility(bVisVal);  // Declared as deprecated in the ROOT documentation, but needed for real invisibility
    node->GetVolume()->SetVisibility(bVisVal);
    if (!recursive) {                                                                //
      LOG(info) << "Setting " << node_name << (bVisVal ? " Visible" : " Invisible")  //
                << ("0" != recursive_val ? " and its daughters also" : "");
    }
  }
  if (recursive_val.Length() > 0) {
    Int_t xml_depth = recursive_val.Atoi();
    /*
    /// Original FairRoot counting, led to troubles in my case
    if (recursive) {
        xml_depth = depth - 1;
    }
    */
    if (0 < xml_depth) {
      for (int i = 0; i < node->GetNdaughters(); i++) {
        TGeoNode* daughter_node = node->GetDaughter(i);
        LoadXMLDetector(daughter_node, xml, xml_depth);
      }
    }
  }
  if (xml->GetNChildren() > 0 && !recursive) {
    for (int i = 0; i < node->GetNdaughters(); i++) {
      TString subdetector_name = node->GetDaughter(i)->GetName();
      for (int j = 0; j < xml->GetNChildren(); j++) {
        FairXMLNode* subnode = xml->GetChild(j);
        TString subnode_name = subnode->GetAttrib("name")->GetValue();
        if (subnode_name == subdetector_name) {  //
          LoadXMLDetector(node->GetDaughter(i), subnode);
        }
      }
    }
  }
}

Int_t CbmTimesliceManager::StringToColor(TString color) const
{
  if (color.Contains("k")) {
    Int_t plus_index  = color.First('+');
    Int_t minus_index = color.First('-');
    Int_t cut         = plus_index;
    if (cut == -1) {  //
      cut = minus_index;
    }
    if (cut == -1) {  //
      cut = color.Length();
    }
    TString col_name(color(0, cut));
    Int_t col_val = 0;
    if (col_name.EqualTo("kWhite")) {  //
      col_val = 0;
    }
    else if (col_name.EqualTo("kBlack")) {
      col_val = 1;
    }
    else if (col_name.EqualTo("kGray")) {
      col_val = 920;
    }
    else if (col_name.EqualTo("kRed")) {
      col_val = 632;
    }
    else if (col_name.EqualTo("kGreen")) {
      col_val = 416;
    }
    else if (col_name.EqualTo("kBlue")) {
      col_val = 600;
    }
    else if (col_name.EqualTo("kYellow")) {
      col_val = 400;
    }
    else if (col_name.EqualTo("kMagenta")) {
      col_val = 616;
    }
    else if (col_name.EqualTo("kCyan")) {
      col_val = 432;
    }
    else if (col_name.EqualTo("kOrange")) {
      col_val = 800;
    }
    else if (col_name.EqualTo("kSpring")) {
      col_val = 820;
    }
    else if (col_name.EqualTo("kTeal")) {
      col_val = 840;
    }
    else if (col_name.EqualTo("kAzure")) {
      col_val = 860;
    }
    else if (col_name.EqualTo("kViolet")) {
      col_val = 880;
    }
    else if (col_name.EqualTo("kPink")) {
      col_val = 900;
    }
    TString col_num(color(cut + 1, color.Length()));
    if (col_num.Length() > 0) {   //
      if (color.Contains("+")) {  //
        col_val += col_num.Atoi();
      }
      else {
        col_val -= col_num.Atoi();
      }
    }
    return col_val;
  }
  else {
    return color.Atoi();
  }
}
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

// +++++++++++++++++++++++++++++++++++++++++++++ Private ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //
void CbmTimesliceManager::AddParticlesToPdgDataBase()
{
  // Add particles to the PDG data base
  TDatabasePDG* pdgDB = TDatabasePDG::Instance();

  const Double_t kAu2Gev   = 0.9314943228;
  const Double_t khSlash   = 1.0545726663e-27;
  const Double_t kErg2Gev  = 1 / 1.6021773349e-3;
  const Double_t khShGev   = khSlash * kErg2Gev;
  const Double_t kYear2Sec = 3600 * 24 * 365.25;

  // Ions
  if (!pdgDB->GetParticle(1000010020)) {  //
    pdgDB->AddParticle("Deuteron", "Deuteron", 2 * kAu2Gev + 8.071e-3, kTRUE, 0, 3, "Ion", 1000010020);
  }

  if (!pdgDB->GetParticle(1000010030)) {  //
    pdgDB->AddParticle("Triton", "Triton", 3 * kAu2Gev + 14.931e-3, kFALSE, khShGev / (12.33 * kYear2Sec), 3, "Ion",
                       1000010030);
  }

  if (!pdgDB->GetParticle(1000020040)) {  //
    pdgDB->AddParticle("Alpha", "Alpha", 4 * kAu2Gev + 2.424e-3, kTRUE, khShGev / (12.33 * kYear2Sec), 6, "Ion",
                       1000020040);
  }

  if (!pdgDB->GetParticle(1000020030)) {  //
    pdgDB->AddParticle("He3", "He3", 3 * kAu2Gev + 14.931e-3, kFALSE, 0, 6, "Ion", 1000020030);
  }

  // Special particles
  if (!pdgDB->GetParticle(50000050)) {  //
    pdgDB->AddParticle("Cherenkov", "Cherenkov", 0, kFALSE, 0, 0, "Special", 50000050);
  }

  if (!pdgDB->GetParticle(50000051)) {  //
    pdgDB->AddParticle("FeedbackPhoton", "FeedbackPhoton", 0, kFALSE, 0, 0, "Special", 50000051);
  }
}

void CbmTimesliceManager::InitPdgColorMap()
{
  fPDGToColor[22]    = 623;  // photon
  fPDGToColor[-2112] = 2;    // anti-neutron
  fPDGToColor[-11]   = 3;    // e+
  fPDGToColor[-3122] = 4;    // anti-lambda
  fPDGToColor[11]    = 5;    // e-
  fPDGToColor[-3222] = 6;    // Sigma -
  fPDGToColor[12]    = 7;    // e-neutrino
  fPDGToColor[-3212] = 8;    //  Sigma0
  fPDGToColor[-13]   = 9;    // mu+
  fPDGToColor[-3112] = 10;   // Sigma+ (PB
  fPDGToColor[13]    = 11;   //  mu-
  fPDGToColor[-3322] = 12;   //  Xi0
  fPDGToColor[111]   = 13;   // pi0
  fPDGToColor[-3312] = 14;   //  Xi+
  fPDGToColor[211]   = 15;   // pi+
  fPDGToColor[-3334] = 16;   //  Omega+ (PB)
  fPDGToColor[-211]  = 17;   // pi-
  fPDGToColor[-15]   = 18;   // tau+
  fPDGToColor[130]   = 19;   // K long
  fPDGToColor[15]    = 20;   //  tau -
  fPDGToColor[321]   = 21;   // K+
  fPDGToColor[411]   = 22;   // D+
  fPDGToColor[-321]  = 23;   // K-
  fPDGToColor[-411]  = 24;   // D-
  fPDGToColor[2112]  = 25;   // n
  fPDGToColor[421]   = 26;   // D0
  fPDGToColor[2212]  = 27;   // p
  fPDGToColor[-421]  = 28;   // D0
  fPDGToColor[-2212] = 29;   //  anti-proton
  fPDGToColor[431]   = 30;   // Ds+
  fPDGToColor[310]   = 31;   // K short
  fPDGToColor[-431]  = 32;   // anti Ds-
  fPDGToColor[221]   = 33;   // eta
  fPDGToColor[4122]  = 34;   // Lambda_C+
  fPDGToColor[3122]  = 35;   //  Lambda
  fPDGToColor[24]    = 36;   // W+
  fPDGToColor[3222]  = 37;   // Sigma+
  fPDGToColor[-24]   = 38;   //  W-
  fPDGToColor[3212]  = 39;   // Sigma0
  fPDGToColor[23]    = 40;   //  Z
  fPDGToColor[3112]  = 41;   // Sigma -
  fPDGToColor[3322]  = 42;   // Xi0
  fPDGToColor[3312]  = 43;   // Xi-
  fPDGToColor[3334]  = 44;   // Omega- (PB)

  fPDGToColor[50000050] = 801;  // Cerenkov

  fPDGToColor[1000010020] = 45;  // PAL 31/05/2023: ???
  fPDGToColor[1000010030] = 48;  // PAL 31/05/2023: ???
  fPDGToColor[1000020040] = 50;  // PAL 31/05/2023: ???
  fPDGToColor[1000020030] = 55;  // PAL 31/05/2023: ???
}

void CbmTimesliceManager::SetTransparencyForLayer(TGeoNode* node, Int_t depth, Char_t transparency)
{
  node->GetVolume()->SetTransparency(transparency);
  if (depth <= 0) {  //
    return;
  }
  for (int i = 0; i < node->GetNdaughters(); i++) {
    TGeoNode* dau = node->GetDaughter(i);
    SetTransparencyForLayer(dau, depth - 1, transparency);
  }
}

bool CbmTimesliceManager::InitializeMainView(Int_t visopt, Int_t vislvl, Int_t maxvisnds)
{
  if (gGeoManager == nullptr) {  //
    return false;
  }
  TGeoNode* N          = gGeoManager->GetTopNode();
  TEveGeoTopNode* TNod = new TEveGeoTopNode(gGeoManager, N, visopt, vislvl, maxvisnds);

  if (!fXMLConfig.EqualTo("")) {  //
    LoadXMLSettings();
  }

  gEve->AddGlobalElement(TNod);

  gEve->FullRedraw3D(kTRUE);
  fEvent = gEve->AddEvent(this);

  fTimesliceNumberText = new TGLAnnotation(gEve->GetDefaultGLViewer(), "TS Number: ", 0.01, 0.94);
  fTimesliceNumberText->SetTextSize(0.03);  // % of window diagonal
  fTimesliceNumberText->SetTextColor(kOrange - 2);

  fTimesliceTimeText = new TGLAnnotation(gEve->GetDefaultGLViewer(), "TS Time: ", 0.01, 0.90);
  fTimesliceTimeText->SetTextSize(0.03);  // % of window diagonal
  fTimesliceTimeText->SetTextColor(kOrange - 2);

  fEventNumberText = new TGLAnnotation(gEve->GetDefaultGLViewer(), "Event Number: ", 0.01, 0.86);
  fEventNumberText->SetTextSize(0.03);  // % of window diagonal
  fEventNumberText->SetTextColor(kOrange - 2);

  fEventTimeText = new TGLAnnotation(gEve->GetDefaultGLViewer(), "Event Time: ", 0.01, 0.82);
  fEventTimeText->SetTextSize(0.03);  // % of window diagonal
  fEventTimeText->SetTextColor(kOrange - 2);

  return true;
}

void CbmTimesliceManager::InitializeViewsCbm()
{
  fRPhiProjManager = new TEveProjectionManager(TEveProjection::kPT_RPhi);
  fRhoZProjManager = new TEveProjectionManager(TEveProjection::kPT_RhoZ);
  gEve->AddToListTree(fRPhiProjManager, kFALSE);
  gEve->AddToListTree(fRhoZProjManager, kFALSE);
  fAxesPhi = new TEveProjectionAxes(fRPhiProjManager);
  fAxesRho = new TEveProjectionAxes(fRhoZProjManager);

  fRPhiView  = gEve->SpawnNewViewer("RPhi View", "");
  fRPhiScene = gEve->SpawnNewScene("RPhi", "Scene holding axis.");
  fRPhiScene->AddElement(fAxesPhi);

  fRhoZView  = gEve->SpawnNewViewer("RhoZ View", "");
  fRhoZScene = gEve->SpawnNewScene("RhoZ", "Scene holding axis.");
  fRhoZScene->AddElement(fAxesRho);

  SetViewers(fRPhiView, fRhoZView);

  TEveWindowSlot* MultiSlot = TEveWindow::CreateWindowInTab(gEve->GetBrowser()->GetTabRight());
  TEveWindowPack* MultiPack = MultiSlot->MakePack();
  MultiPack->SetElementName("Multi View");
  MultiPack->SetHorizontal();
  MultiPack->SetShowTitleBar(kFALSE);
  MultiPack->NewSlot()->MakeCurrent();
  fMultiView = gEve->SpawnNewViewer("3D View (multi)", "");
  // switch off left and right light sources for 3D MultiView
  // TODO: investigate and tune which light source directions should be used for best visibility
  fMultiView->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  fMultiView->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  // add 3D scenes (first tab) to 3D MultiView
  fMultiView->AddScene(gEve->GetGlobalScene());
  fMultiView->AddScene(gEve->GetEventScene());

  // add slot for RPhi projection on Multi View tab
  MultiPack = MultiPack->NewSlot()->MakePack();
  MultiPack->SetShowTitleBar(kFALSE);
  MultiPack->NewSlot()->MakeCurrent();
  fMultiRPhiView = gEve->SpawnNewViewer("RPhi View (multi)", "");
  MultiPack->NewSlot()->MakeCurrent();
  fMultiRhoZView = gEve->SpawnNewViewer("RhoZ View (multi)", "");

  SetViewers(fMultiRPhiView, fMultiRhoZView);

  // don't change reposition camera on each update
  fRPhiView->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fRhoZView->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMultiView->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMultiRPhiView->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMultiRhoZView->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMultiView->GetEveFrame()->HideAllDecorations();
  fMultiRPhiView->GetEveFrame()->HideAllDecorations();
  fMultiRhoZView->GetEveFrame()->HideAllDecorations();
}

void CbmTimesliceManager::SetViewers(TEveViewer* RPhi, TEveViewer* RhoZ)
{
  RPhi->GetGLViewer()->SetCurrentCamera(fRphiCam);
  // set camera parameters
  RPhi->GetGLViewer()->GetCameraOverlay()->SetOrthographicMode(TGLCameraOverlay::kAxis);
  RPhi->GetGLViewer()->GetCameraOverlay()->SetShowOrthographic(kTRUE);
  // switch off left, right, top and bottom light sources
  // TODO: investigate and tune which light source directions should be used for best visibility
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightTop, false);
  RPhi->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightBottom, false);

  RhoZ->GetGLViewer()->SetCurrentCamera(fRhoCam);
  // set camera parameters
  RhoZ->GetGLViewer()->GetCameraOverlay()->SetOrthographicMode(TGLCameraOverlay::kAxis);
  RhoZ->GetGLViewer()->GetCameraOverlay()->SetShowOrthographic(kTRUE);
  // switch off left, right and front light sources
  // TODO: investigate and tune which light source directions should be used for best visibility
  RhoZ->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  RhoZ->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  RhoZ->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightFront, false);

  RPhi->AddScene(fRPhiScene);
  RPhi->AddScene(gEve->GetGlobalScene());
  RPhi->AddScene(gEve->GetEventScene());
  RhoZ->AddScene(fRhoZScene);
  RhoZ->AddScene(gEve->GetGlobalScene());
  RhoZ->AddScene(gEve->GetEventScene());
}

void CbmTimesliceManager::InitializeViewsMcbm()
{
  // FIXME: available only starting from Fairsoft apr22 (did not check in which ROOT version it was introduced)
  //fProjManagerZY = new TEveProjectionManager(TEveProjection::kPT_ZY);
  fProjManagerZY = new TEveProjectionManager(TEveProjection::kPT_RhoZ);
  //fProjManagerZX = new TEveProjectionManager(TEveProjection::kPT_ZX);
  fProjManagerZX = new TEveProjectionManager(TEveProjection::kPT_RhoZ);
  gEve->AddToListTree(fProjManagerZY, kFALSE);
  gEve->AddToListTree(fProjManagerZX, kFALSE);
  fAxesZY = new TEveProjectionAxes(fProjManagerZY);
  fAxesZX = new TEveProjectionAxes(fProjManagerZX);

  fViewZY  = gEve->SpawnNewViewer("ZY View", "");
  fSceneZY = gEve->SpawnNewScene("ZY", "Scene holding axis.");
  fSceneZY->AddElement(fAxesZY);

  fViewZX  = gEve->SpawnNewViewer("ZX View", "");
  fSceneZX = gEve->SpawnNewScene("ZX", "Scene holding axis.");
  fSceneZX->AddElement(fAxesZX);

  SetMcbmViewers(fViewZY, fViewZX);

  TEveWindowSlot* McbmSlot = TEveWindow::CreateWindowInTab(gEve->GetBrowser()->GetTabRight());
  TEveWindowPack* McbmPack = McbmSlot->MakePack();
  McbmPack->SetElementName("mCBM View");
  McbmPack->SetHorizontal();
  McbmPack->SetShowTitleBar(kFALSE);
  McbmPack->NewSlot()->MakeCurrent();
  fMcbmView = gEve->SpawnNewViewer("3D View (multi)", "");
  // switch off left and right light sources for 3D MultiView
  // TODO: investigate and tune which light source directions should be used for best visibility
  //fMcbmView->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  //fMcbmView->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  // add 3D scenes (first tab) to 3D MultiView
  fMcbmView->AddScene(gEve->GetGlobalScene());
  fMcbmView->AddScene(gEve->GetEventScene());
  // Center the 3D MultiView
  fMcbmView->GetGLViewer()->CurrentCamera().SetCenterVecWarp(0.0, 0.0, 150.0);

  // add slot for XY projection on Multi View tab
  McbmPack = McbmPack->NewSlot()->MakePack();
  McbmPack->SetShowTitleBar(kFALSE);
  McbmPack->NewSlot()->MakeCurrent();
  fMcbmViewZY = gEve->SpawnNewViewer("ZY View (multi)", "");
  McbmPack->NewSlot()->MakeCurrent();
  fMcbmViewZX = gEve->SpawnNewViewer("ZX View (multi)", "");

  SetMcbmViewers(fMcbmViewZY, fMcbmViewZX);

  // don't change reposition camera on each update
  fViewZY->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fViewZX->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMcbmView->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMcbmViewZY->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMcbmViewZX->GetGLViewer()->SetResetCamerasOnUpdate(kFALSE);
  fMcbmView->GetEveFrame()->HideAllDecorations();
  fMcbmViewZY->GetEveFrame()->HideAllDecorations();
  fMcbmViewZX->GetEveFrame()->HideAllDecorations();

  // Set clear background
  fViewZY->GetGLViewer()->SetClearColor(kYellow - 10);
  fViewZX->GetGLViewer()->SetClearColor(kYellow - 10);
  fMcbmView->GetGLViewer()->SetClearColor(kYellow - 10);
  fMcbmViewZY->GetGLViewer()->SetClearColor(kYellow - 10);
  fMcbmViewZX->GetGLViewer()->SetClearColor(kYellow - 10);
}

void CbmTimesliceManager::SetMcbmViewers(TEveViewer* ZY, TEveViewer* ZX)
{
  ZY->GetGLViewer()->SetCurrentCamera(fCamZY);
  // set camera parameters
  ZY->GetGLViewer()->GetCameraOverlay()->SetOrthographicMode(TGLCameraOverlay::kAxis);
  ZY->GetGLViewer()->GetCameraOverlay()->SetShowOrthographic(kTRUE);
  // switch off left, right and front light sources
  // TODO: investigate and tune which light source directions should be used for best visibility
  ZY->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  //ZY->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  ZY->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightFront, false);
  // Set Camera Center
  ZY->GetGLViewer()->CurrentCamera().SetCenterVecWarp(0.0, 0.0, 150.0);

  ZY->AddScene(fSceneZY);
  ZY->AddScene(gEve->GetGlobalScene());
  ZY->AddScene(gEve->GetEventScene());

  ZX->GetGLViewer()->SetCurrentCamera(fCamZX);
  // set camera parameters
  ZX->GetGLViewer()->GetCameraOverlay()->SetOrthographicMode(TGLCameraOverlay::kAxis);
  ZX->GetGLViewer()->GetCameraOverlay()->SetShowOrthographic(kTRUE);
  // switch off left, right, top and bottom light sources
  // TODO: investigate and tune which light source directions should be used for best visibility
  ZX->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightLeft, false);
  //ZX->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightRight, false);
  ZX->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightTop, false);
  ZX->GetGLViewer()->GetLightSet()->SetLight(TGLLightSet::kLightBottom, false);
  // Set Camera Center
  ZX->GetGLViewer()->CurrentCamera().SetCenterVecWarp(0.0, 0.0, 150.0);

  ZX->AddScene(fSceneZX);
  ZX->AddScene(gEve->GetGlobalScene());
  ZX->AddScene(gEve->GetEventScene());
}
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

ClassImp(CbmTimesliceManager)
