/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */

/*
 * FairEveAnimationControl.cxx
 *
 *  Created on: 26 maj 2020
 *      Author: Daniel Wielanek
 *		E-mail: daniel.wielanek@gmail.com
 *		Warsaw University of Technology, Faculty of Physics
 */
/// PAL 01/06/2023: heavily based on FairEveAnimationControl from FairRoot v18.6.7, for usage with CbmTimesliceManager
#include "CbmTsEveAnimationControl.h"

CbmTsEveAnimationControl::CbmTsEveAnimationControl(TGedFrame* frame, TGCompositeFrame* tab, TString functionName,
                                                   TString name, Int_t width)
  : fWidth(width)
  , fParent(frame)
  , fFunctionName(functionName)
  , fTab(tab)
{
  SetName(name);
}

CbmTsEveAnimationControl::~CbmTsEveAnimationControl() {}

void CbmTsEveAnimationControl::Init()
{
  TGGroupFrame* animCtrl = new TGGroupFrame(fTab, GetName());
  animCtrl->SetTitlePos(TGGroupFrame::kCenter);

  // -------------------------------- Start "holy" button --------------------------------------------------------------
  fStartButton = new TGTextButton(animCtrl, "Start");
  fStartButton->Resize(80, 25);
  animCtrl->AddFrame(fStartButton, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 5, 5, 5, 5));
  fStartButton->Connect("Clicked()", fParent->ClassName(), fParent, fFunctionName);
  fStartButton->SetEnabled(kFALSE);  // Enabled only if TS number set to at least 1
  // -------------------------------------------------------------------------------------------------------------------

  // -------------------------------- Animation type -------------------------------------------------------------------
  TGCompositeFrame* frAnimType = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gAnimType           = new TGLabel(frAnimType, "Animation type:");
  frAnimType->AddFrame(gAnimType, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 1, 1));
  fTypeOpt = new TGComboBox(frAnimType);
  fTypeOpt->AddEntry("Evts in sel. TS", kEventsInTs);
  fTypeOpt->AddEntry("Timeslices", kTimeSlices);
  fTypeOpt->Select(kTimeSlices);
  fTypeOpt->Resize(100, 20);
  frAnimType->AddFrame(fTypeOpt, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));
  animCtrl->AddFrame(frAnimType, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));
  fTypeOpt->Connect("Selected (Int_t,Int_t)", this->ClassName(), this, "UpdateEnaDisButtons()");

  TGCompositeFrame* frSelAnimFrameSec = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelFrameSec             = new TGLabel(frSelAnimFrameSec, "Frame length (s):");
  frSelAnimFrameSec->AddFrame(gLabelFrameSec, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fAnimFrameSec = new TGNumberEntry(frSelAnimFrameSec, 5.0, 6, -1, TGNumberFormat::kNESReal,
                                    TGNumberFormat::kNEANonNegative, TGNumberFormat::kNELLimitMin, 0);
  fAnimFrameSec->SetNumber(5.0);
  frSelAnimFrameSec->AddFrame(fAnimFrameSec, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  animCtrl->AddFrame(frSelAnimFrameSec, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));
  // -------------------------------------------------------------------------------------------------------------------

  // -------------------------------- Event stepping buttons -----------------------------------------------------------
  TGCompositeFrame* frSelEvtMin = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelEvtMin         = new TGLabel(frSelEvtMin, "Min Evt:");
  frSelEvtMin->AddFrame(gLabelEvtMin, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fEvtMin = new TGNumberEntry(frSelEvtMin, 0, 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative,
                              TGNumberFormat::kNELLimitMinMax, 0, (fNbEvt ? fNbEvt - 1 : 0));
  fEvtMin->SetNumber(0);
  frSelEvtMin->AddFrame(fEvtMin, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  animCtrl->AddFrame(frSelEvtMin, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));

  TGCompositeFrame* frSelEvtMax = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelEvtMax         = new TGLabel(frSelEvtMax, "Max Evt:");
  frSelEvtMax->AddFrame(gLabelEvtMax, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fEvtMax = new TGNumberEntry(frSelEvtMax, 10, 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative,
                              TGNumberFormat::kNELLimitMinMax, 0, (fNbEvt ? fNbEvt - 1 : 0));
  fEvtMax->SetNumber((fNbEvt ? fNbEvt - 1 : 0));
  frSelEvtMax->AddFrame(fEvtMax, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  animCtrl->AddFrame(frSelEvtMax, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));

  TGCompositeFrame* frSelEvtStep = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelEvtStep         = new TGLabel(frSelEvtStep, "Step Evt:");
  frSelEvtStep->AddFrame(gLabelEvtStep, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fEvtStep = new TGNumberEntry(frSelEvtStep, 0, 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEAPositive,
                               TGNumberFormat::kNELLimitMinMax, 1, (fNbEvt ? fNbEvt - 1 : 1));
  fEvtStep->SetNumber(1);
  frSelEvtStep->AddFrame(fEvtStep, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  animCtrl->AddFrame(frSelEvtStep, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));

  /// In Timeslice mode, scan anyway on all events in each TS, in event mode wait for loading of first TS
  /// Not SetEnabled for TGNumberEntry, guessing equivalent is SetState
  fEvtMin->SetState(kFALSE);
  fEvtMax->SetState(kFALSE);
  fEvtStep->SetState(kFALSE);
  // -------------------------------------------------------------------------------------------------------------------

  // -------------------------------- TS stepping buttons --------------------------------------------------------------
  TGCompositeFrame* frSelTsMin = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelTsMin         = new TGLabel(frSelTsMin, "Min Ts:");
  frSelTsMin->AddFrame(gLabelTsMin, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fTsMin = new TGNumberEntry(frSelTsMin, 0, 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative,
                             TGNumberFormat::kNELLimitMinMax, 0, (fNbTs ? fNbTs - 1 : 0));
  fTsMin->SetNumber(0);
  frSelTsMin->AddFrame(fTsMin, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  animCtrl->AddFrame(frSelTsMin, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));

  TGCompositeFrame* frSelTsMax = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelTsMax         = new TGLabel(frSelTsMax, "Max Ts:");
  frSelTsMax->AddFrame(gLabelTsMax, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fTsMax = new TGNumberEntry(frSelTsMax, 10, 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative,
                             TGNumberFormat::kNELLimitMinMax, 0, (fNbTs ? fNbTs - 1 : 0));
  fTsMax->SetNumber((fNbTs ? fNbTs - 1 : 0));
  frSelTsMax->AddFrame(fTsMax, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  animCtrl->AddFrame(frSelTsMax, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));

  TGCompositeFrame* frSelTsStep = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelTsStep         = new TGLabel(frSelTsStep, "Step Ts:");
  frSelTsStep->AddFrame(gLabelTsStep, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fTsStep = new TGNumberEntry(frSelTsStep, 0, 6, -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEAPositive,
                              TGNumberFormat::kNELLimitMinMax, 1, (fNbTs ? fNbTs - 1 : 1));
  fTsStep->SetNumber(1);
  frSelTsStep->AddFrame(fTsStep, new TGLayoutHints(kLHintsRight | kLHintsExpandX));
  animCtrl->AddFrame(frSelTsStep, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));

  Bool_t bAtLeastOneTs = (0 < fNbTs);
  fTsMin->SetState(bAtLeastOneTs);
  fTsMax->SetState(bAtLeastOneTs);
  fTsStep->SetState(bAtLeastOneTs);
  fEvtStep->SetState(bAtLeastOneTs);
  fStartButton->SetEnabled(bAtLeastOneTs);
  // -------------------------------------------------------------------------------------------------------------------

  // -------------------------------- Screenshot Ena & type ------------------------------------------------------------
  fBtnScreenshotEna = new TGCheckButton(animCtrl, "Screenshot each event");
  fBtnScreenshotEna->SetOn();
  animCtrl->AddFrame(fBtnScreenshotEna, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));
  fBtnScreenshotEna->Connect("Clicked()", this->ClassName(), this, "UpdateEnaScreenshots()");

  TGCompositeFrame* frSceneType = new TGCompositeFrame(animCtrl, fWidth, 30, kHorizontalFrame | kFixedWidth);
  TGLabel* gLabelType           = new TGLabel(frSceneType, "Scene type:");
  frSceneType->AddFrame(gLabelType, new TGLayoutHints(kLHintsLeft | kLHintsExpandX, 1, 1, 2, 1));
  fSceneOpt = new TGComboBox(frSceneType);
  fSceneOpt->AddEntry("3D", k3D);
  if (fbMcbmViewersEna) {
    fSceneOpt->AddEntry("ZX", kZX);
    fSceneOpt->AddEntry("ZY", kZY);
  }
  else {
    fSceneOpt->AddEntry("RPhi", kXY);
    fSceneOpt->AddEntry("RhoZ", kZ);
  }
  fSceneOpt->AddEntry("All", kAll);
  fSceneOpt->Select(k3D);
  fSceneOpt->Resize(100, 20);
  frSceneType->AddFrame(fSceneOpt, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));
  animCtrl->AddFrame(frSceneType, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));
  // -------------------------------------------------------------------------------------------------------------------

  // -------------------------------- Display clear ctrl ---------------------------------------------------------------
  fBtnClearBuffer = new TGCheckButton(animCtrl, "Clear Buffer at Start");
  fBtnClearBuffer->SetOn();
  animCtrl->AddFrame(fBtnClearBuffer, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));

  fBtnRunContinuous = new TGCheckButton(animCtrl, "Run Continuous (stack events)");
  fBtnRunContinuous->SetOn(kFALSE);
  animCtrl->AddFrame(fBtnRunContinuous, new TGLayoutHints(kLHintsLeft | kLHintsExpandX));
  // -------------------------------------------------------------------------------------------------------------------

  fTab->AddFrame(animCtrl, new TGLayoutHints(kLHintsRight | kLHintsExpandX, 1, 1, 2, 1));

  UpdateTsLimits();

  fbInitDone = true;
}

CbmTsEveAnimationControl::eAnimationType CbmTsEveAnimationControl::GetAnimationType() const
{
  return static_cast<eAnimationType>(fTypeOpt->GetSelected());
}

Double_t CbmTsEveAnimationControl::GetAnimFrameSec() { return fAnimFrameSec->GetNumber(); }

Bool_t CbmTsEveAnimationControl::GetScreenshotEna() { return fBtnScreenshotEna->IsOn(); }

CbmTsEveAnimationControl::eScreenshotType CbmTsEveAnimationControl::GetScreenshotType() const
{
  return static_cast<eScreenshotType>(fSceneOpt->GetSelected());
}

Int_t CbmTsEveAnimationControl::GetEventMin() { return fEvtMin->GetNumber(); }

Int_t CbmTsEveAnimationControl::GetEventMax() { return fEvtMax->GetNumber(); }

Int_t CbmTsEveAnimationControl::GetEventStep() { return fEvtStep->GetNumber(); }

Int_t CbmTsEveAnimationControl::GetTsMin() { return fTsMin->GetNumber(); }

Int_t CbmTsEveAnimationControl::GetTsMax() { return fTsMax->GetNumber(); }

Int_t CbmTsEveAnimationControl::GetTsStep() { return fTsStep->GetNumber(); }

Bool_t CbmTsEveAnimationControl::GetRunContinuous() { return fBtnRunContinuous->IsOn(); }

Bool_t CbmTsEveAnimationControl::GetClearBuffer() { return fBtnClearBuffer->IsOn(); }

void CbmTsEveAnimationControl::UpdateEnaScreenshots() { fSceneOpt->SetEnabled(fBtnScreenshotEna->IsOn()); }

void CbmTsEveAnimationControl::UpdateEnaDisButtons()
{
  /// Prevent invalid accesses
  Bool_t bAtLeastOneTs = (0 < fNbTs);
  if (bAtLeastOneTs) {
    fStartButton->SetEnabled(kTRUE);

    Bool_t bTsScanMode = (kTimeSlices == GetAnimationType());

    /// In Timeslice mode, scan anyway on all events in each TS in range
    fEvtMin->SetState(!bTsScanMode);
    fEvtMax->SetState(!bTsScanMode);
    fEvtStep->SetState(kTRUE);

    /// In Event mode, scan only on events within selected TS
    fTsMin->SetState(bTsScanMode);
    fTsMax->SetState(bTsScanMode);
    fTsStep->SetState(bTsScanMode);
  }
  else {
    fStartButton->SetEnabled(kFALSE);
    fEvtMin->SetState(kFALSE);
    fEvtMax->SetState(kFALSE);
    fEvtStep->SetState(kFALSE);
    fTsMin->SetState(kFALSE);
    fTsMax->SetState(kFALSE);
    fTsStep->SetState(kFALSE);
  }
}

void CbmTsEveAnimationControl::UpdateEventLimits()
{
  fEvtMin->SetLimitValues(0, fNbEvt - 1);
  fEvtMax->SetLimitValues(0, fNbEvt - 1);
  fEvtStep->SetLimitValues(1, fNbEvt - 1);

  fEvtMin->SetNumber(0);
  fEvtMax->SetNumber(fNbEvt - 1);
  fEvtStep->SetNumber(1);

  UpdateEnaDisButtons();
}

void CbmTsEveAnimationControl::UpdateTsLimits()
{
  fTsMin->SetLimitValues(0, fNbTs - 1);
  fTsMax->SetLimitValues(0, fNbTs - 1);
  fTsStep->SetLimitValues(1, fNbTs - 1);

  fTsMin->SetNumber(0);
  fTsMax->SetNumber(fNbTs - 1);
  fTsStep->SetNumber(1);

  UpdateEnaDisButtons();
}
