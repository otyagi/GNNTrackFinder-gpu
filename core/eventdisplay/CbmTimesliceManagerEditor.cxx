/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */
/// Based on CbmTimesliceManagerEditor class of FairRoot v18.6.7

#include "CbmTimesliceManagerEditor.h"

#include "CbmEvent.h"                     // For CbmEvent
#include "CbmTimesliceManager.h"          // for CbmTimesliceManager
#include "CbmTsEveAnimationControl.h"     // for CbmTsEveAnimationControl
#include "CbmTsEveTransparencyControl.h"  // for CbmTsEveTransparencyControl

#include "FairRootManager.h"  // for FairRootManager
#include "FairRunAna.h"       // for FairRunAna
#include "FairTask.h"         // for FairTask

#include <RtypesCore.h>         // for Double_t, Int_t, UInt_t, Bool_t
#include <TChain.h>             // for TChain
#include <TClonesArray.h>       // for TClonesArray
#include <TEveManager.h>        // for TEveManager, gEve
#include <TFile.h>              // for TFile
#include <TGButton.h>           // for TGTextButton, TGCheckButton
#include <TGComboBox.h>         // for TGComboBox
#include <TGLabel.h>            // for TGLabel
#include <TGLayout.h>           // for TGLayoutHints, kLHintsExpandX
#include <TGNumberEntry.h>      // for TGNumberEntry, TGNumberFormat
#include <TGenericClassInfo.h>  // for TGenericClassInfo
#include <TGeoManager.h>        // for TGeoManager, gGeoManager
#include <TList.h>              // for TObjLink, TList
#include <TString.h>            // for TString, Form
#include <TSystem.h>            // for TSystem, gSystem

#include <chrono>
#include <iostream>
#include <memory>  // for unique_ptr
#include <thread>

class TGWindow;  // lines 36-36
class TObject;   // lines 37-37

#define MAXE 5000

// CbmTimesliceManagerEditor
//
// Specialization of TGedEditor for proper update propagation to TEveManager.

CbmTimesliceManagerEditor::CbmTimesliceManagerEditor(const TGWindow* p, Int_t width, Int_t height, UInt_t options,
                                                     Pixel_t back)
  : TGedFrame(p, width, height, options | kVerticalFrame, back)
  , fObject(0)
  , fManager(CbmTimesliceManager::Instance())
  , fCurrentEvent(0)
  , fGlobalTransparency(nullptr)
  , fEventTime(nullptr)
  , fScreenshotOpt(nullptr)
{
  Init();
}

void CbmTimesliceManagerEditor::SwitchBackground(Bool_t light_background)
{
  fManager->SwitchBackground(light_background);
}

void CbmTimesliceManagerEditor::SwitchPdgColorTrack(Bool_t pdg_color)
{
  // Forward to manager class which forwards to track display class
  fManager->SwitchPdgColorTrack(pdg_color);
}

void CbmTimesliceManagerEditor::Init()
{
  FairRootManager* rootManager = FairRootManager::Instance();
  TChain* chain                = rootManager->GetInChain();
  fNbTs                        = chain->GetEntriesFast();

  MakeTitle("CbmTimesliceManager Editor");
  TGVerticalFrame* fInfoFrame = CreateEditorTabSubFrame("Info TS");
  TGCompositeFrame* title1 =
    new TGCompositeFrame(fInfoFrame, fWidth, 10, kVerticalFrame | kLHintsExpandX | kFixedWidth | kOwnBackground);

  TString Infile = "Input file : ";
  TFile* file    = FairRootManager::Instance()->GetInChain()->GetFile();
  Infile += file->GetName();
  TGLabel* TFName = new TGLabel(title1, Infile.Data());
  title1->AddFrame(TFName);

  Int_t nodes    = gGeoManager->GetNNodes();
  TString NNodes = "No. of Nodes : ";
  NNodes += nodes;
  TGLabel* NoNode = new TGLabel(title1, NNodes.Data());
  title1->AddFrame(NoNode);

  UInt_t RunId = FairRunAna::Instance()->getRunId();
  TString run  = "Run Id : ";
  run += RunId;
  TGLabel* TRunId = new TGLabel(title1, run.Data());
  title1->AddFrame(TRunId);

  TString ntimeslices = "No of timeslices : ";
  ntimeslices += fNbTs;
  TGLabel* TNbTimeslices = new TGLabel(title1, ntimeslices.Data());
  title1->AddFrame(TNbTimeslices);

  TGHorizontalFrame* fTsIdx = new TGHorizontalFrame(title1);
  TGLabel* lTsIdx           = new TGLabel(fTsIdx, "Current TS:");
  fTsIdx->AddFrame(lTsIdx, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 1, 2, 1, 1));
  fCurrentTimeslice = new TGNumberEntry(fTsIdx, 0., 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative,
                                        TGNumberFormat::kNELLimitMinMax, 0, fNbTs);
  fTsIdx->AddFrame(fCurrentTimeslice, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
  fCurrentTimeslice->Connect("ValueSet(Long_t)", "CbmTimesliceManagerEditor", this, "SelectSingleTimeslice()");
  title1->AddFrame(fTsIdx);

  TGHorizontalFrame* fTsTime = new TGHorizontalFrame(title1);
  TGLabel* TsTimeLabel       = new TGLabel(fTsTime, "TS Time: ");
  fTimesliceTime             = new TGLabel(fTsTime, "");
  fTsTime->AddFrame(TsTimeLabel);
  fTsTime->AddFrame(fTimesliceTime);
  title1->AddFrame(fTsTime);

  TString nevent = "No of events in TS : ";
  nevent += (Int_t)(-1);
  fEventNb = new TGLabel(title1, nevent.Data());
  title1->AddFrame(fEventNb);

  fSelTs = new TGTextButton(title1, "Select TS");
  fSelTs->Connect("Clicked()", "CbmTimesliceManagerEditor", this, "SelectSingleTimeslice()");
  title1->AddFrame(fSelTs, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));

  fPrevTs = new TGTextButton(title1, "Prev TS");
  fPrevTs->Connect("Clicked()", "CbmTimesliceManagerEditor", this, "PrevTimeslice()");
  title1->AddFrame(fPrevTs, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));

  fNextTs = new TGTextButton(title1, "Next TS");
  fNextTs->Connect("Clicked()", "CbmTimesliceManagerEditor", this, "NextTimeslice()");
  title1->AddFrame(fNextTs, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));


  TGHorizontalFrame* fEvtIdx = new TGHorizontalFrame(title1);
  TGLabel* lEvtIdx           = new TGLabel(fEvtIdx, "Current Event:");
  fEvtIdx->AddFrame(lEvtIdx, new TGLayoutHints(kLHintsLeft | kLHintsCenterY, 1, 2, 1, 1));
  fCurrentEvent = new TGNumberEntry(fEvtIdx, 0., 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative,
                                    TGNumberFormat::kNELLimitMinMax, 0, -1);
  fEvtIdx->AddFrame(fCurrentEvent, new TGLayoutHints(kLHintsLeft, 1, 1, 1, 1));
  fCurrentTimeslice->Connect("ValueSet(Long_t)", "CbmTimesliceManagerEditor", this, "SelectSingleTimeslice()");
  title1->AddFrame(fEvtIdx);


  TGHorizontalFrame* fEvtTime = new TGHorizontalFrame(title1);
  TGLabel* EventTimeLabel     = new TGLabel(fEvtTime, "Event Time: ");
  fEventTime                  = new TGLabel(fEvtTime, "");
  fEvtTime->AddFrame(EventTimeLabel);
  fEvtTime->AddFrame(fEventTime);
  title1->AddFrame(fEvtTime);

  fUpdateEvent = new TGTextButton(title1, "Update Event");
  fUpdateEvent->Connect("Clicked()", "CbmTimesliceManagerEditor", this, "SelectSingleEvent()");
  title1->AddFrame(fUpdateEvent, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));

  fPrevEvent = new TGTextButton(title1, "Prev Event");
  fPrevEvent->Connect("Clicked()", "CbmTimesliceManagerEditor", this, "PrevEvent()");
  title1->AddFrame(fPrevEvent, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));

  fNextEvent = new TGTextButton(title1, "Next Event");
  fNextEvent->Connect("Clicked()", "CbmTimesliceManagerEditor", this, "NextEvent()");
  title1->AddFrame(fNextEvent, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));

  /// Disable all Prev/Next TS buttons until first TS loaded/selected!
  fPrevTs->SetEnabled(kFALSE);
  fNextTs->SetEnabled(kFALSE);

  /// Disable all Events buttons until first TS loaded/selected!
  fUpdateEvent->SetEnabled(kFALSE);
  fPrevEvent->SetEnabled(kFALSE);
  fNextEvent->SetEnabled(kFALSE);


  fInfoFrame->AddFrame(title1, new TGLayoutHints(kLHintsTop, 0, 0, 2, 0));

  //=============== graphics =============================
  TGVerticalFrame* scene_conf = CreateEditorTabSubFrame("Graphics");
  // TGHorizontalFrame* transparency_frame = new TGHorizontalFrame(scene_conf); /// PAL: Unused in FairRoot original?!?

  std::unique_ptr<CbmTsEveTransparencyControl> transparency(
    new CbmTsEveTransparencyControl(scene_conf, "Global transparency"));
  scene_conf->AddFrame(transparency.release(), new TGLayoutHints(kLHintsNormal, 5, 5, 1, 1));

  TGCheckButton* backgroundButton = new TGCheckButton(scene_conf, "Light background");
  scene_conf->AddFrame(backgroundButton, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));
  backgroundButton->Connect("Toggled(Bool_t)", this->ClassName(), this, "SwitchBackground(Bool_t)");

  TGCheckButton* pdgColorTrackButton = new TGCheckButton(scene_conf, "Track w/ PDG color");
  scene_conf->AddFrame(pdgColorTrackButton, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 5, 5, 1, 1));
  pdgColorTrackButton->Connect("Toggled(Bool_t)", this->ClassName(), this, "SwitchPdgColorTrack(Bool_t)");

  TGGroupFrame* frame_screenshot = new TGGroupFrame(scene_conf, "Screenshot");
  frame_screenshot->SetTitlePos(TGGroupFrame::kCenter);

  frame_screenshot->SetLayoutManager(new TGHorizontalLayout(frame_screenshot));

  TGTextButton* Screen = new TGTextButton(frame_screenshot, "Make");
  frame_screenshot->AddFrame(Screen, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 20, 2, 2, 2));
  Screen->Connect("Clicked()", this->ClassName(), this, "MakeScreenshot()");

  fScreenshotOpt = new TGComboBox(frame_screenshot);
  fScreenshotOpt->AddEntry("3D", CbmTsEveAnimationControl::eScreenshotType::k3D);
  if (CbmTimesliceManager::Instance()->GetMcbmViewersMode()) {
    fScreenshotOpt->AddEntry("ZX", CbmTsEveAnimationControl::eScreenshotType::kZX);
    fScreenshotOpt->AddEntry("ZY", CbmTsEveAnimationControl::eScreenshotType::kZY);
  }
  else {
    fScreenshotOpt->AddEntry("RPhi", CbmTsEveAnimationControl::eScreenshotType::kXY);
    fScreenshotOpt->AddEntry("RhoZ", CbmTsEveAnimationControl::eScreenshotType::kZ);
  }
  fScreenshotOpt->AddEntry("All", CbmTsEveAnimationControl::eScreenshotType::kAll);
  fScreenshotOpt->Select(0);
  fScreenshotOpt->Resize(40, 30);
  frame_screenshot->AddFrame(fScreenshotOpt, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  scene_conf->AddFrame(frame_screenshot, new TGLayoutHints(kLHintsTop | kLHintsExpandX, 1, 1, 2, 1));

  fAnimation = new CbmTsEveAnimationControl(this, scene_conf, "StartAnimation()", "Animation", fWidth);
  fAnimation->SetTsNb(fNbTs);
  fAnimation->SetDisplayMcbm(CbmTimesliceManager::Instance()->GetMcbmViewersMode());
  fAnimation->Init();

  // TODO: fine-tune the position of the elements in the GUI tab
  //  scene_conf->AddFrame(online_screenshot,new TGLayoutHints(kLHintsRight|kLHintsExpandX));


  fCbmEvents = dynamic_cast<TClonesArray*>(rootManager->GetObject("CbmEvent"));

  if (nullptr == fCbmEvents) {
    LOG(fatal) << "CbmTimesliceManager::Init() => CbmEvents branch not found! Task will be deactivated";
  }
}

void CbmTimesliceManagerEditor::SelectSingleTimeslice()
{
  fManager->SetClearHandler(kTRUE);
  SelectTimeslice();
}

void CbmTimesliceManagerEditor::SelectTimeslice()
{
  fManager->GotoTimeslice(fCurrentTimeslice->GetIntNumber());
  SetTimesliceTimeLabel(CbmTimesliceManager::Instance()->GetTimesliceTime());
  CbmTimesliceManager::Instance()->SetTsTimeText(CbmTimesliceManager::Instance()->GetTimesliceTime());
  CbmTimesliceManager::Instance()->SetTsNumberText(fCurrentTimeslice->GetIntNumber());

  /// Enable/Disable buttons to avoid invalid accesses
  fPrevTs->SetEnabled((0 < fCurrentTimeslice->GetIntNumber()) ? kTRUE : kFALSE);
  fNextTs->SetEnabled((fCurrentTimeslice->GetIntNumber() < (fNbTs - 1)) ? kTRUE : kFALSE);

  fNbEventsInTs = fCbmEvents->GetEntriesFast();

  TString nevent = "No of events in TS : ";
  nevent += fNbEventsInTs;
  fEventNb->ChangeText(nevent.Data());
  fCurrentEvent->SetLimitValues(0, fNbEventsInTs);
  fCurrentEvent->SetIntNumber(0);
  fAnimation->SetEvtNb(fNbEventsInTs);

  /// Enable/Disable buttons to avoid invalid accesses
  if (0 < fNbEventsInTs) {
    fUpdateEvent->SetEnabled(kTRUE);
    fPrevEvent->SetEnabled(kFALSE);
    if (1 < fNbEventsInTs) {  //
      fNextEvent->SetEnabled(kTRUE);
    }
  }
  else {
    fUpdateEvent->SetEnabled(kFALSE);
    fPrevEvent->SetEnabled(kFALSE);
    fNextEvent->SetEnabled(kFALSE);
  }

  /// Load first event of this TS if possible and Update info text
  if (0 < fNbEventsInTs) {  //
    SelectEvent();
  }
}

void CbmTimesliceManagerEditor::PrevTimeslice()
{
  LOG(debug1) << "CbmTimesliceManager::PrevTimeslice() => At timeslice " << fCurrentTimeslice->GetIntNumber();
  uint32_t uTsIdx = fCurrentTimeslice->GetIntNumber();
  if (0 < uTsIdx) {  // Should be protected by button enabling/disabling, but better safe than sorry
    uTsIdx -= 1;
    fCurrentTimeslice->SetIntNumber(uTsIdx);
    LOG(debug1) << "CbmTimesliceManager::PrevTimeslice() => Setting timeslice to " << uTsIdx << " result "
                << fCurrentTimeslice->GetIntNumber();

    SelectSingleTimeslice();
  }
}

void CbmTimesliceManagerEditor::NextTimeslice()
{
  LOG(debug1) << "CbmTimesliceManager::NextTimeslice() => At timeslice " << fCurrentTimeslice->GetIntNumber();
  uint32_t uTsIdx = fCurrentTimeslice->GetIntNumber();
  if (uTsIdx < fNbTs - 1) {  // Should be protected by button enabling/disabling, but better safe than sorry
    uTsIdx += 1;
    fCurrentTimeslice->SetIntNumber(uTsIdx);
    LOG(debug1) << "CbmTimesliceManager::NextTimeslice() => Setting timeslice to " << uTsIdx << " result "
                << fCurrentTimeslice->GetIntNumber();

    SelectSingleTimeslice();
  }
}

void CbmTimesliceManagerEditor::SelectSingleEvent()
{
  fManager->SetClearHandler(kTRUE);
  SelectEvent();
}

void CbmTimesliceManagerEditor::SelectEvent()
{
  LOG(debug1) << "CbmTimesliceManager::SelectEvent() => Going to event " << fCurrentEvent->GetIntNumber();
  fManager->GotoEvent(fCurrentEvent->GetIntNumber());
  SetEventTimeLabel(CbmTimesliceManager::Instance()->GetEventTime());
  CbmTimesliceManager::Instance()->SetEvtTimeText(CbmTimesliceManager::Instance()->GetEventTime());
  CbmTimesliceManager::Instance()->SetEvtNumberText(fCurrentEvent->GetIntNumber());

  // Enable/Disable buttons to avoid invalid accesses
  fPrevEvent->SetEnabled((0 < fCurrentEvent->GetIntNumber()) ? kTRUE : kFALSE);
  fNextEvent->SetEnabled((fCurrentEvent->GetIntNumber() < (fNbEventsInTs - 1)) ? kTRUE : kFALSE);
}

void CbmTimesliceManagerEditor::PrevEvent()
{
  LOG(debug1) << "CbmTimesliceManager::PrevEvent() => At event " << fCurrentEvent->GetIntNumber();
  uint32_t uEventIdx = fCurrentEvent->GetIntNumber();
  if (0 < uEventIdx) {  // Should be protected by button enabling/disabling, but better safe than sorry
    uEventIdx -= 1;
    fCurrentEvent->SetIntNumber(uEventIdx);
    LOG(debug1) << "CbmTimesliceManager::PrevEvent() => Setting event to " << uEventIdx << " result "
                << fCurrentEvent->GetIntNumber();

    SelectSingleEvent();
  }
}

void CbmTimesliceManagerEditor::NextEvent()
{
  LOG(debug1) << "CbmTimesliceManager::NextEvent() => At event " << fCurrentEvent->GetIntNumber();
  uint32_t uEventIdx = fCurrentEvent->GetIntNumber();
  if (uEventIdx < fNbEventsInTs - 1) {  // Should be protected by button enabling/disabling, but better safe than sorry
    uEventIdx += 1;
    fCurrentEvent->SetIntNumber(uEventIdx);
    LOG(debug1) << "CbmTimesliceManager::NextEvent() => Setting event to " << uEventIdx << " result "
                << fCurrentEvent->GetIntNumber();

    SelectSingleEvent();
  }
}

void CbmTimesliceManagerEditor::SetTimesliceTimeLabel(Double_t time)
{
  TString stime;
  stime.Form("%.2f", time);
  stime += " ns";
  fTimesliceTime->SetText(stime.Data());
  Update();
}
void CbmTimesliceManagerEditor::SetEventTimeLabel(Double_t time)
{
  TString stime;
  stime.Form("%.2f", time);
  stime += " ns";
  fEventTime->SetText(stime.Data());
  Update();
}

void CbmTimesliceManagerEditor::SetModel(TObject* obj) { fObject = obj; }

void CbmTimesliceManagerEditor::StartAnimation()
{
  CbmTsEveAnimationControl::eScreenshotType screen = fAnimation->GetScreenshotType();

  std::chrono::duration<double> secSleepTime(fAnimation->GetAnimFrameSec());
  bool bSleep = (0.0 < fAnimation->GetAnimFrameSec());

  switch (fAnimation->GetAnimationType()) {
    case CbmTsEveAnimationControl::eAnimationType::kEventsInTs: {  // Events in selected timeslice
      /// Get minimum and maximum event indices, ensured to fit range in TS by buttons updates
      uint32_t iEvtMin  = static_cast<uint32_t>(fAnimation->GetEventMin());
      uint32_t iEvtMax  = static_cast<uint32_t>(fAnimation->GetEventMax());
      uint32_t iEvtStep = static_cast<uint32_t>(fAnimation->GetEventStep());

      if (fAnimation->GetScreenshotEna()) {  //
        gSystem->mkdir("event_animations");
      }
      for (uint32_t iEvt = iEvtMin; iEvt < iEvtMax; iEvt += iEvtStep) {
        fCurrentEvent->SetIntNumber(iEvt);

        if ((iEvtMin == iEvt && kTRUE == fAnimation->GetClearBuffer()) || kFALSE == fAnimation->GetRunContinuous()) {
          /// Clear display at startup if requested (should be default for most uses)
          /// Clear display after each event if requested (should be default for most uses)
          fManager->SetClearHandler(kTRUE);
        }
        else {
          /// Do not clear display between events = accumulate/stack them
          fManager->SetClearHandler(kFALSE);
        }

        SelectEvent();
        gEve->FullRedraw3D();
        if (fAnimation->GetScreenshotEna()) {  //
          fManager->MakeScreenshot(screen, Form("event_animations/event_%05i.png", iEvt));
        }
        if (bSleep) {
          // sleep between events to be able to see them
          std::this_thread::sleep_for(secSleepTime);
        }
      }
      break;
    }
    case CbmTsEveAnimationControl::eAnimationType::kTimeSlices: {  // Events in timeslice range
      /// Get minimum and maximum TS indices, ensured to fit range in run by buttons updates
      uint32_t iTsMin  = static_cast<uint32_t>(fAnimation->GetTsMin());
      uint32_t iTsMax  = static_cast<uint32_t>(fAnimation->GetTsMax());
      uint32_t iTsStep = static_cast<uint32_t>(fAnimation->GetTsStep());

      /// Get only event step, min and max set to content of each TS
      uint32_t iEvtStep = static_cast<int32_t>(fAnimation->GetEventStep());

      if (fAnimation->GetScreenshotEna()) {  //
        gSystem->mkdir("timeslice_animations");
      }
      for (uint32_t iTs = iTsMin; iTs < iTsMax; iTs += iTsStep) {
        fCurrentTimeslice->SetIntNumber(iTs);

        if (iTsMin == iTs && kTRUE == fAnimation->GetClearBuffer()) {
          /// Clear display at startup if requested (should be default for most uses)
          fManager->SetClearHandler(kTRUE);
        }

        SelectTimeslice();  /// <= This should update all events limits to appropriate values
        for (uint32_t iEvt = 0; iEvt < fNbEventsInTs; iEvt += iEvtStep) {
          fCurrentEvent->SetIntNumber(iEvt);

          if (kFALSE == fAnimation->GetRunContinuous()) {
            /// Clear display after each event if requested (should be default for most uses)
            fManager->SetClearHandler(kTRUE);
          }
          else {
            /// Do not clear display between events = accumulate/stack them
            fManager->SetClearHandler(kFALSE);
          }

          SelectEvent();
          gEve->FullRedraw3D();
          if (fAnimation->GetScreenshotEna()) {  //
            fManager->MakeScreenshot(screen, Form("timeslice_animations/ts_%05i_event_%05i.png", iTs, iEvt));
          }
          if (bSleep) {
            // sleep between events to be able to see them
            std::this_thread::sleep_for(secSleepTime);
          }
        }  // Event loop
      }    // TS loop
      break;
    }
  }
  LOG(info) << "CbmTimesliceManager::StartAnimation() => Done with animation";
}

void CbmTimesliceManagerEditor::MakeScreenshot()
{
  fManager->MakeScreenshot(static_cast<CbmTsEveAnimationControl::eScreenshotType>(fScreenshotOpt->GetSelected()));
}

ClassImp(CbmTimesliceManagerEditor)
