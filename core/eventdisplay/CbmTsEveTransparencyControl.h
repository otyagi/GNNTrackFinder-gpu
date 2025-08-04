/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#ifndef CBMTSEVETRANSPARENCYCONTROL_H_
#define CBMTSEVETRANSPARENCYCONTROL_H_

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Bool_t
#include <TGFrame.h>     // for TGFrame (ptr only), TGHorizontalFrame
class TBuffer;
class TClass;
class TGCheckButton;
class TGNumberEntry;  // lines 16-16
class TMemberInspector;

/** @class CbmTsEveTransparencyControl
 ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
 ** @brief GUI elements to add global transparency control to CbmTimesliceManagerEditor. Cannot be used alone!
 ** @brief PAL 31/05/2023: clone of FairEveTransparencyControl (for FairEventManagerEditor) from FairRoot v18.6.7
 **/
class CbmTsEveTransparencyControl : public TGHorizontalFrame {
  TGCheckButton* fCheck;
  TGNumberEntry* fNumber;

public:
  CbmTsEveTransparencyControl(TGFrame const* parent, char const* label = "Transparency");

  TGCheckButton* GetCheck() const { return fCheck; }
  TGNumberEntry* GetNumber() const { return fNumber; }

  /**
   ** @brief Called when button clicked. RESERVED FOR GUI CALLS!
   **/
  void Toggled();  // SLOT to receive check button events
  /**
   ** @brief Called when transparency value changed. RESERVED FOR GUI CALLS!
   **/
  void ValueSet();  // SLOT to receive number entry events

  virtual ~CbmTsEveTransparencyControl() {};

  ClassDef(CbmTsEveTransparencyControl, 1)
};

#endif /* CBMTSEVETRANSPARENCYCONTROL_H_ */
