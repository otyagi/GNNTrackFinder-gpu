/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */

#ifndef CbmTimesliceManagerEditor_H
#define CbmTimesliceManagerEditor_H

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Bool_t, Double_t, UInt_t
#include <TGFrame.h>     // for kChildFrame
#include <TGedFrame.h>   // for TGedFrame

#include <GuiTypes.h>  // for Pixel_t

class CbmTimesliceManager;
class CbmTsEveAnimationControl;
class TBuffer;
class TClass;
class TClonesArray;
class TGComboBox;
class TGLabel;
class TGNumberEntry;
class TGWindow;
class TMemberInspector;
class TObject;

/** @class CbmTimesliceManagerEditor
 ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
 ** @brief GUI elements for CbmTimesliceManager. Automatically loaded by TBrowser. Cannot be used standalone!
 ** @brief Based on FairEventManagerEditor class of FairRoot v18.6.7
 **/
class CbmTimesliceManagerEditor : public TGedFrame {
public:
  CbmTimesliceManagerEditor(const TGWindow* p = 0, Int_t width = 250, Int_t height = 30, UInt_t options = kChildFrame,
                            Pixel_t back = GetDefaultFrameBackground());
  virtual ~CbmTimesliceManagerEditor() = default;

  CbmTimesliceManagerEditor(const CbmTimesliceManagerEditor&) = delete;
  CbmTimesliceManagerEditor& operator=(const CbmTimesliceManagerEditor&) = delete;

  /**
   ** switch background color: to be called by GUI element
   ** @param light use white if true
   **/
  void SwitchBackground(Bool_t light_background);

  /**
   ** switch track color: to be called by GUI element
   ** @param PDG color if true, red if false (see TimesliceRecoTracks)
   **/
  void SwitchPdgColorTrack(Bool_t pdg_color);

  virtual void Init();

  /**
   ** @brief Select timeslice depending on GUI elements values, after clearing display. RESERVED FOR GUI CALLS!
   **/
  virtual void SelectSingleTimeslice();
  /**
   ** @brief Select timeslice depending on GUI elements values and load its first event. RESERVED FOR GUI CALLS!
   **/
  virtual void SelectTimeslice();
  /**
   ** @brief Select previous timeslice and load its first event. RESERVED FOR GUI CALLS!
   **/
  virtual void PrevTimeslice();
  /**
   ** @brief Select next timeslice and load its first event. RESERVED FOR GUI CALLS!
   **/
  virtual void NextTimeslice();
  /**
   ** @brief Select event in timeslice depending on GUI elements values, after clearing display. RESERVED FOR GUI CALLS!
   **/
  virtual void SelectSingleEvent();
  /**
   ** @brief Select event in timeslice depending on GUI elements values. RESERVED FOR GUI CALLS!
   **/
  virtual void SelectEvent();
  /**
   ** @brief Select previous event in timeslice. RESERVED FOR GUI CALLS!
   **/
  virtual void PrevEvent();
  /**
   ** @brief Select next event in timeslice. RESERVED FOR GUI CALLS!
   **/
  virtual void NextEvent();

  void SetModel(TObject* obj);

  /**
   ** @brief Start display: to be called by GUI element, see HowTo for description of options
   **/
  virtual void StartAnimation();

  /**
    ** @brief Screenshot(s) generation: to be called by GUI element
    **/
  void MakeScreenshot();

protected:
  TObject* fObject                   = nullptr;  //!
  CbmTimesliceManager* fManager      = nullptr;  //!
  TGNumberEntry* fCurrentTimeslice   = nullptr;  //!
  TGNumberEntry* fCurrentEvent       = nullptr;  //!
  TGNumberEntry* fGlobalTransparency = nullptr;  //!
  TGLabel* fTimesliceTime            = nullptr;  //!
  TGTextButton* fSelTs               = nullptr;  //!
  TGTextButton* fPrevTs              = nullptr;  //!
  TGTextButton* fNextTs              = nullptr;  //!
  TGLabel* fEventNb                  = nullptr;  //!
  TGLabel* fEventTime                = nullptr;  //!
  TGTextButton* fUpdateEvent         = nullptr;  //!
  TGTextButton* fPrevEvent           = nullptr;  //!
  TGTextButton* fNextEvent           = nullptr;  //!

  TGComboBox* fScreenshotOpt           = nullptr;  //!
  CbmTsEveAnimationControl* fAnimation = nullptr;  //!

  uint32_t fNbTs           = 0;
  TClonesArray* fCbmEvents = nullptr;  //!
  uint32_t fNbEventsInTs   = 0;

  void SetTimesliceTimeLabel(Double_t time);
  void SetEventTimeLabel(Double_t time);

private:
  ClassDef(CbmTimesliceManagerEditor, 1);
};

#endif  // CbmTimesliceManagerEditor_H
