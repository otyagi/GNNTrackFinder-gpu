/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer], Norbert Herrmann */

#ifndef CbmTimesliceManager_H
#define CbmTimesliceManager_H

#include "CbmTsEveAnimationControl.h"

#include "FairRunAna.h"  // for FairRunAna

#include <Rtypes.h>            // for Float_t, Int_t, Bool_t, etc
#include <TEveEventManager.h>  // for TEveEventManager
#include <TEveProjectionAxes.h>
#include <TGLViewer.h>

#include <map>

class FairRootManager;
class FairRunAna;
class FairTask;
class FairXMLNode;

class TClonesArray;
class TEveProjectionAxes;
class TEveProjectionManager;
class TEveScene;
class TEveText;
class TEveViewer;
class TGeoNode;
class TGLAnnotation;
class TGListTreeItem;

/** @class CbmTimesliceManager
 ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
 ** @brief TBrowser Event display for Timeslices as Tree entry with CbmEvents in container. Function as unique Instance.
 ** @brief Based on FairEventManager class of FairRoot v18.6.7
 **/
class CbmTimesliceManager : public TEveEventManager {
public:
  static CbmTimesliceManager* Instance();
  CbmTimesliceManager();
  virtual ~CbmTimesliceManager() = default;

  CbmTimesliceManager(const CbmTimesliceManager&) = delete;
  CbmTimesliceManager& operator=(const CbmTimesliceManager&) = delete;

  /**
   ** @brief Set path to xml configuration file for control of transparency and visibility of individual geometry nodes.
   ** @param Full path to the xml file (without usage of variables such as ~ or $XXXX)
   **/
  virtual void SetXMLConfig(TString xml_config) { fXMLConfig = xml_config; }
  /**
   ** @brief Select/add default set of views and some Hit and Tracks drawing classes for the CBM SIS100 electron setup.
   **/
  void SetDisplayCbmElectron();
  /**
   ** @brief Select/add default set of views and some Hit and Tracks drawing classes for the CBM SIS100 muon setup.
   **/
  void SetDisplayCbmMuon();
  /**
   ** @brief Select/add default set of views and some Hit and Tracks drawing classes for the mCBM 2022+ setups.
   **/
  void SetDisplayMcbm(bool bLowRate = true, bool bMuch = false);
  /**
   ** @brief Select/add default set of views and some Hit and Tracks drawing classes for the HD 2023+ cosmics setups.
   **/
  void SetDisplayTofCosmicsHd();
  /**
   * set detector's transparency, typically called during processing of XML config file
   * @param use_xml  use xml colors if available
   * @param trans    transparency for detector (if xml not used)
   */
  virtual void SetTransparency(Bool_t use_xml, Int_t trans);
  /**
   ** switch background color: to be called by GUI element
   ** @param light use white if true
   **/
  virtual void SwitchBackground(Bool_t /*light*/);

  /**
   ** switch track color: to be called by GUI element
   ** @param PDG color if true, red if false (see TimesliceRecoTracks)
   **/
  void SwitchPdgColorTrack(Bool_t pdg_color);

  virtual void Init(Int_t visopt = 1, Int_t vislvl = 3, Int_t maxvisnds = 10000);
  void AddTask(FairTask* t) { fRunAna->AddTask(t); }

  virtual void Open();
  void UpdateEditor();
  virtual void Close();
  virtual Int_t Color(Int_t pdg);
  virtual void DisplaySettings();                  // *MENU*
  virtual void GotoTimeslice(uint32_t timeslice);  // *MENU*
  virtual void NextTimeslice();                    // *MENU*
  virtual void PrevTimeslice();                    // *MENU*
  virtual void GotoEvent(Int_t event);             // *MENU*
  virtual void NextEvent();                        // *MENU*
  virtual void PrevEvent();                        // *MENU*

  bool GetMcbmViewersMode() const { return fbMcbmViewersEna; }
  virtual uint32_t GetCurrentTimeslice() const { return fTimesliceIdx; }
  virtual Int_t GetCurrentEvent() const { return fEventIdx; }
  double_t GetTimesliceTime();  ///< current time in ns to display in the event display.
  double_t GetEventTime();      ///< current time in ns to display in the event display.

  virtual void SetTsTimeText(Double_t time);
  virtual void SetTsNumberText(Int_t tsNumber);
  virtual void SetEvtTimeText(Double_t time);
  virtual void SetEvtNumberText(Int_t evtNumber);

  void SetClearHandler(Bool_t val) { fClearHandler = val; }  ///< Used to indicate to subtask that they should reset
  Bool_t GetClearHandler() const { return fClearHandler; }   ///< Used to indicate to subtask that they should reset

  /**
   ** @brief Screenshot(s) generation: to be called by GUI element
   ** @param screenshotType   see CbmTsEveAnimationControl, depends on CBM/mCBM mode
   ** @param def_path         default path to screenshot, if empty user will be asked with GUI pop-up
   **/
  void MakeScreenshot(CbmTsEveAnimationControl::eScreenshotType screenshotType, TString def_path = "");

protected:
  virtual void LoadXMLSettings();
  void LoadXMLDetector(TGeoNode* node, FairXMLNode* xml, Int_t depth = 0);
  Int_t StringToColor(TString color) const;

private:
  static CbmTimesliceManager* gRinstanceTsMan;  //!

  FairRunAna* fRunAna           = nullptr;  //!
  FairRootManager* fRootManager = nullptr;  //!

  TString fXMLConfig             = "";
  std::map<int, int> fPDGToColor = {};
  void AddParticlesToPdgDataBase();
  void InitPdgColorMap();
  void SetTransparencyForLayer(TGeoNode* node, Int_t depth, Char_t transparency);

  Bool_t fClearHandler = kFALSE;  //!

  TGListTreeItem* fEvent   = nullptr;  //!
  Int_t fEntry             = 0;        //!
  TClonesArray* fCbmEvents = nullptr;  //!
  uint32_t fTimesliceIdx   = 0;        //!
  uint32_t fEventIdx       = 0;        //!
  double_t fTimeTimeslice  = -1;       //!
  double_t fTimeEvent      = -1;       //!

  TGLAnnotation* fTimesliceTimeText   = nullptr;  //!
  TGLAnnotation* fTimesliceNumberText = nullptr;  //!
  TGLAnnotation* fEventTimeText       = nullptr;  //!
  TGLAnnotation* fEventNumberText     = nullptr;  //!

  TGLViewer::ECameraType fRphiCam = TGLViewer::kCameraOrthoXOY;   //!
  TGLViewer::ECameraType fRhoCam  = TGLViewer::kCameraOrthoZOY;   //!
  TGLViewer::ECameraType fCamZY   = TGLViewer::kCameraOrthoZnOX;  //!
  TGLViewer::ECameraType fCamZX   = TGLViewer::kCameraOrthoZOY;   //!

  /// CBM views
  TEveViewer* fRPhiView                   = nullptr;  //!
  TEveViewer* fRhoZView                   = nullptr;  //!
  TEveViewer* fMultiView                  = nullptr;  //!
  TEveViewer* fMultiRPhiView              = nullptr;  //!
  TEveViewer* fMultiRhoZView              = nullptr;  //!
  TEveScene* fRPhiScene                   = nullptr;  //!
  TEveScene* fRhoZScene                   = nullptr;  //!
  TEveProjectionManager* fRPhiProjManager = nullptr;  //!
  TEveProjectionManager* fRhoZProjManager = nullptr;  //!
  TEveProjectionAxes* fAxesPhi            = nullptr;  //!
  TEveProjectionAxes* fAxesRho            = nullptr;  //!

  /// mCBM views
  bool fbMcbmViewersEna                 = false;    //!
  TEveViewer* fViewZY                   = nullptr;  //!
  TEveViewer* fViewZX                   = nullptr;  //!
  TEveViewer* fMcbmView                 = nullptr;  //!
  TEveViewer* fMcbmViewZY               = nullptr;  //!
  TEveViewer* fMcbmViewZX               = nullptr;  //!
  TEveScene* fSceneZY                   = nullptr;  //!
  TEveScene* fSceneZX                   = nullptr;  //!
  TEveProjectionManager* fProjManagerZY = nullptr;  //!
  TEveProjectionManager* fProjManagerZX = nullptr;  //!
  TEveProjectionAxes* fAxesZY           = nullptr;  //!
  TEveProjectionAxes* fAxesZX           = nullptr;  //!

  bool InitializeMainView(Int_t visopt, Int_t vislvl, Int_t maxvisnds);
  void InitializeViewsCbm();
  void SetViewers(TEveViewer* RPhi, TEveViewer* RhoZ);
  void InitializeViewsMcbm();
  void SetMcbmViewers(TEveViewer* ZY, TEveViewer* ZX);

  ClassDef(CbmTimesliceManager, 1);
};

#endif  // CbmTimesliceManager_H
