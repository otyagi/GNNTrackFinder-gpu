/* Copyright (C) 2023 PI-UHd, Heidelberg
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann [committer], Pierre-Alain Loizeau */

/** CbmTsPointSetArrayDraw
 *  @author N. Herrmann
 *  @since 05.11.23
 *  Task to display unused TOF hits (pointsets in array)
 *  Timeslice compatible version of CbmPointSetArrayDraw
 **
 **/

#ifndef CBMTSPOINTSETARRAYDRAW_H
#define CBMTSPOINTSETARRAYDRAW_H

#include <FairTask.h>  // for FairTask, InitStatus

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Double_t, Bool_t, kTRUE, Color_t, Opt...

class CbmPointSetArray;
class CbmTimesliceManager;
class TClonesArray;
class TObject;
class TVector3;

class CbmTsPointSetArrayDraw : public FairTask {

 public:
  /** Default constructor **/
  CbmTsPointSetArrayDraw() : FairTask("CbmTsPointSetArrayDraw", 0) {}

  /** Standard constructor
    *@param name        Name of task
    *@param colorMode   coloring of points
    *@param markerMode  how to mark points
    *@param iVerbose    Verbosity level
    **/
  CbmTsPointSetArrayDraw(const char* name, Int_t colorMode, Int_t markerMode, Int_t iVerbose = 1,
                         Bool_t render = kTRUE);

  /** Destructor **/
  virtual ~CbmTsPointSetArrayDraw();

  // Setters
  /** Set verbosity level. For this task and all of the subtasks. **/
  void SetVerbose(Int_t iVerbose) { fVerbose = iVerbose; }
  void SetColorMode(Int_t colorMode) { fColorMode = colorMode; }
  void SetMarkerMode(Int_t markerMode) { fMarkerMode = markerMode; }
  void SetRender(Bool_t render) { fRender = render; }
  // Accessors
  Int_t GetColorMode() const { return fColorMode; }
  Int_t GetMarkerMode() const { return fMarkerMode; }

  /** Executed task **/
  virtual void Exec(Option_t* option);
  /** Reset task **/
  void Reset();
  void GotoEvent(uint32_t uEventIdx);

 protected:
  TVector3 GetVector(TObject* obj);    //Get 3D Vector of Hit
  Double_t GetTime(TObject* obj);      //Get Time of Hit
  Int_t GetClusterSize(TObject* obj);  //Get ClusterSize of TofHit
  Double_t GetTot(TObject* obj);       //Get ToT of TofHit
  Int_t GetPointId(TObject* obj);      //Get RefId of Hit

  void DetermineTimeOffset();  //Determine TimeOffset and time of latest hit

  virtual void SetParContainers();
  /** Initialise taks **/
  virtual InitStatus Init();
  /** Action after each event **/
  virtual void Finish();

  Int_t fVerbose                  = 0;        // Verbosity level
  TClonesArray* fCbmEvents        = nullptr;  //!
  TString fTofHitArrayName        = "TofUHit";
  TClonesArray* fTsPointList      = nullptr;  // Array containing list of hits
  TClonesArray* fPointList        = nullptr;  // Array containing list of hits
  CbmTimesliceManager* fTsManager = nullptr;  // Pointer to Event Manager
  CbmPointSetArray* fl            = nullptr;  // Pointer to CbmPointSetArray
                                              // -> Cbm class for displaying array of Hit-sets -> TEvePointSetArray
  Color_t fColor       = kRed;                // Color of Hit-Markers
  Style_t fStyle       = 4;                   // Style of Hit-Markers
  Double_t fTimeOffset = 0;                   // Time Offset on Hits to scale first hit to 0
  Double_t fTimeMax    = 0;                   // Max Time of Hits in TofHit
  Int_t fColorMode     = 1;                   // Int determining how points get color-coded
  Int_t fMarkerMode    = 1;                   // Int determining how marker-size of points gets coded
  Bool_t fRender       = kTRUE;               // Boolean whether points shown on default

 private:
  CbmTsPointSetArrayDraw(const CbmTsPointSetArrayDraw&);
  CbmTsPointSetArrayDraw& operator=(const CbmTsPointSetArrayDraw&);

  ClassDef(CbmTsPointSetArrayDraw, 1);
};


#endif
