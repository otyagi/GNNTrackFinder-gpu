/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */

#include "CbmTsEveTransparencyControl.h"

#include "CbmTimesliceManager.h"  // for CbmTimesliceManager

#include <TGButton.h>       // for TGCheckButton
#include <TGNumberEntry.h>  // for TGNumberEntry, TGNumberFormat, TGNumbe...

CbmTsEveTransparencyControl::CbmTsEveTransparencyControl(TGFrame const* parent, char const* label)
  : TGHorizontalFrame(parent)
  , fCheck(new TGCheckButton(this, label))
  , fNumber(new TGNumberEntry(this,
                              80.,  // initial number
                              6,    // digitwidth
                              -1, TGNumberFormat::kNESInteger, TGNumberFormat::kNEANonNegative,
                              TGNumberFormat::kNELLimitMinMax,
                              0,     // min
                              100))  // max
{
  SetCleanup(kDeepCleanup);

  // display
  AddFrame(fCheck);   // takes ownership
  AddFrame(fNumber);  // takes ownership

  // wire up observers
  fCheck->Connect("Toggled(Bool_t)", this->ClassName(), this, "Toggled()");
  fNumber->Connect("ValueSet(Long_t)", this->ClassName(), this, "ValueSet()");
}

void CbmTsEveTransparencyControl::Toggled()
{
  if (fCheck->IsOn()) {  //
    CbmTimesliceManager::Instance()->SetTransparency(kFALSE, fNumber->GetIntNumber());
  }
  else {
    CbmTimesliceManager::Instance()->SetTransparency(kTRUE, fNumber->GetIntNumber());
  }
}

void CbmTsEveTransparencyControl::ValueSet()
{
  if (fCheck->IsOn()) {  //
    CbmTimesliceManager::Instance()->SetTransparency(kFALSE, fNumber->GetIntNumber());
  }
  else {
    CbmTimesliceManager::Instance()->SetTransparency(kTRUE, fNumber->GetIntNumber());
  }
}
