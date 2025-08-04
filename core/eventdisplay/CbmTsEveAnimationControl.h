/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */

/*
 * FairEveAnimationControl.h
 *
 *  Created on: 26 maj 2020
 *      Author: Daniel Wielanek
 *		E-mail: daniel.wielanek@gmail.com
 *		Warsaw University of Technology, Faculty of Physics
 */

#ifndef CBMTSEVEANIMATIONCONTROL_H_
#define CBMTSEVEANIMATIONCONTROL_H_

#include <Rtypes.h>
#include <RtypesCore.h>
#include <TGButton.h>
#include <TGButton.h>  // for TGTextButton, TGCheckButton
#include <TGComboBox.h>
#include <TGDoubleSlider.h>
#include <TGFrame.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGedFrame.h>  // for TGedFrame
#include <TObject.h>

#include <GuiTypes.h>

/** @class CbmTsEveAnimationControl
 ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
 ** @brief GUI elements to add animation and screenshot controls to CbmTimesliceManagerEditor. Cannot be used alone!
 ** @brief PAL 31/05/2023: Heavily based on FairEveAnimationControl (for FairEventManagerEditor) from FairRoot v18.6.7
 **/
class CbmTsEveAnimationControl : public TNamed {
public:
  enum eAnimationType
  {
    kEventsInTs = 0,
    kTimeSlices = 1
  };
  enum eScreenshotType
  {
    kAll = 0,
    k3D  = 1,
    kZX  = 2,
    kZY  = 3,
    kXY  = 4,
    kZ   = 5
  };
  CbmTsEveAnimationControl(TGedFrame* frame, TGCompositeFrame* tab, TString functionName, TString name = "",
                           Int_t width = 170);

  virtual ~CbmTsEveAnimationControl();

  /**
   ** @brief Tune list of allowed screenshot types depending on available CBM/mCBM views.
   **/
  void SetDisplayMcbm(bool bEna) { fbMcbmViewersEna = bEna; }

  /**
   * set name of function called when button is pressed
   * @param name
   */
  void SetFunctionName(TString name) { fFunctionName = name; };

  void SetEvtNb(uint32_t uNbEvt)
  {
    fNbEvt = uNbEvt;

    if (fbInitDone) {  //
      UpdateEventLimits();
    }
  };

  void SetTsNb(uint32_t uTsNb)
  {
    fNbTs = uTsNb;

    if (fbInitDone) {  //
      UpdateTsLimits();
    }
  };

  void Init();

  eAnimationType GetAnimationType() const;
  Double_t GetAnimFrameSec();
  Bool_t GetScreenshotEna();
  eScreenshotType GetScreenshotType() const;
  Int_t GetEventMin();
  Int_t GetEventMax();
  Int_t GetEventStep();
  Int_t GetTsMin();
  Int_t GetTsMax();
  Int_t GetTsStep();
  Bool_t GetRunContinuous();
  Bool_t GetClearBuffer();

  /**
   ** @brief Update GUI elements to enforce options/limits. RESERVED FOR GUI CALLS!
   **/
  void UpdateEnaScreenshots();
  /**
   ** @brief Update GUI elements to enforce options/limits. RESERVED FOR GUI CALLS!
   **/
  void UpdateEnaDisButtons();
  /**
   ** @brief Update GUI elements to enforce options/limits. RESERVED FOR GUI CALLS!
   **/
  void UpdateEventLimits();
  /**
   ** @brief Update GUI elements to enforce options/limits. RESERVED FOR GUI CALLS!
   **/
  void UpdateTsLimits();

private:
  const Int_t fWidth;
  uint32_t fNbEvt = 0;
  uint32_t fNbTs  = 0;

  TGedFrame* fParent    = nullptr;
  TString fFunctionName = "";

  bool fbMcbmViewersEna = false;  //!
  bool fbInitDone       = false;

  TGCompositeFrame* fTab           = nullptr;
  TGTextButton* fStartButton       = nullptr;
  TGComboBox* fTypeOpt             = nullptr;
  TGNumberEntry* fAnimFrameSec     = nullptr;
  TGCheckButton* fBtnScreenshotEna = nullptr;
  TGComboBox* fSceneOpt            = nullptr;
  TGNumberEntry* fEvtMin           = nullptr;
  TGNumberEntry* fEvtMax           = nullptr;
  TGNumberEntry* fEvtStep          = nullptr;
  TGNumberEntry* fTsMin            = nullptr;
  TGNumberEntry* fTsMax            = nullptr;
  TGNumberEntry* fTsStep           = nullptr;
  TGCheckButton* fBtnRunContinuous = nullptr;
  TGCheckButton* fBtnClearBuffer   = nullptr;

  ClassDef(CbmTsEveAnimationControl, 1)
};

#endif /* CBMTSEVEANIMATIONCONTROL_H_ */
