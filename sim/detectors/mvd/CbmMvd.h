/* Copyright (C) 2006-2019 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Philipp Sitzmann */

// -------------------------------------------------------------------------
// -----                        CbmMvd header file                     -----
// -----                  Created 26/07/04  by V. Friese               -----
// -------------------------------------------------------------------------

/**  CbmMvd.h
 *@author V.Friese <v.friese@gsi.de>
 **
 ** Defines the active detector MVD. Constructs the geometry and
 ** registeres MCPoints.
 **/


#ifndef CBMMVD_H
#define CBMMVD_H 1

#include <FairDetector.h>  // for FairDetector

#include <Rtypes.h>          // for ClassDef
#include <RtypesCore.h>      // for Int_t, Bool_t, Double32_t, Double_t, Opt...
#include <TLorentzVector.h>  // for TLorentzVector
#include <TVector3.h>        // for TVector3

#include <map>     // for map
#include <string>  // for string

class CbmMvdGeoHandler;
class CbmMvdPoint;
class FairVolume;
class TBuffer;
class TClass;
class TClonesArray;
class TGeoMatrix;
class TList;
class TMemberInspector;

class CbmMvd : public FairDetector {

public:
  /** Default constructor **/
  CbmMvd();


  /** Standard constructor.
   *@param name    detetcor name
   *@param active  sensitivity flag
   **/
  CbmMvd(const char* name, Bool_t active);


  /** Destructor **/
  virtual ~CbmMvd();


  /** Virtual method ProcessHits
   **
   ** Defines the action to be taken when a step is inside the
   ** active volume. Creates a CbmMvdPoint and adds it to the
   ** collection.
   *@param vol  Pointer to the active volume
   **/
  virtual Bool_t ProcessHits(FairVolume* vol = 0);


  /** Virtual method BeginEvent **/
  virtual void BeginEvent();


  /** Virtual method EndOfEvent
   **
   ** If verbosity level is set, print hit collection at the
   ** end of the event and resets it afterwards.
   **/
  virtual void EndOfEvent();


  /** Virtual method Register
   **
   ** Registers the hit collection in the ROOT manager.
   **/
  virtual void Register();


  /** Accessor to the hit collection **/
  virtual TClonesArray* GetCollection(Int_t iColl) const;


  /** Virtual method Print
   **
   ** Screen output of hit collection.
   **/
  virtual void Print(Option_t* = "") const;


  /** Virtual method Reset
   **
   ** Clears the hit collection
   **/
  virtual void Reset();


  /** Virtual method CopyClones
   **
   ** Copies the hit collection with a given track index offset
   *@param cl1     Origin
   *@param cl2     Target
   *@param offset  Index offset
   **/
  virtual void CopyClones(TClonesArray* cl1, TClonesArray* cl2, Int_t offset);


  /** Virtual method Construct geometry
   **
   ** Constructs the MVD geometry
   **/
  virtual void ConstructGeometry();

  virtual void ConstructAsciiGeometry();

  virtual void ConstructRootGeometry(TGeoMatrix* shift = nullptr);

  virtual Bool_t IsSensitive(const std::string& name);
  virtual Bool_t CheckIfSensitive(std::string name);

  virtual std::map<Int_t, Int_t> GetMap() { return fStationMap; };

private:
  /** Track information to be stored until the track leaves the
	active volume. **/
  Int_t fTrackID;                  //!  track index
  Int_t fPdg;                      //!  track particle type
  Int_t fVolumeID;                 //!  volume id
  TLorentzVector fPosIn, fPosOut;  //!  position
  TLorentzVector fMomIn, fMomOut;  //!  momentum
  Double32_t fTime;                //!  time
  Double32_t fLength;              //!  length
  Double32_t fELoss;               //!  energy loss

  Int_t fPosIndex;                     //!
  TClonesArray* fCollection;           //!  The hit collection
  Bool_t kGeoSaved;                    //!
  TList* fGeoPar;                      //!  List of geometry parameters
  std::map<Int_t, Int_t> fStationMap;  //! Map from MC volume ID to station number

  CbmMvdGeoHandler* fmvdHandler;

  /** Private method AddHit
     **

    ** Adds a MvdPoint to the HitCollection
     **/
  CbmMvdPoint* AddHit(Int_t trackID, Int_t pdg, Int_t sensorNr, TVector3 posIn, TVector3 pos_out, TVector3 momIn,
                      TVector3 momOut, Double_t time, Double_t length, Double_t eLoss);


  /** Private method ResetParameters
     **
     ** Resets the private members for the track parameters
     **/
  void ResetParameters();

  CbmMvd(const CbmMvd&);
  CbmMvd& operator=(const CbmMvd&);


  ClassDef(CbmMvd, 1);
};


inline void CbmMvd::ResetParameters()
{
  fTrackID = fVolumeID = 0;
  fPosIn.SetXYZM(0.0, 0.0, 0.0, 0.0);
  fPosOut.SetXYZM(0.0, 0.0, 0.0, 0.0);
  fMomIn.SetXYZM(0.0, 0.0, 0.0, 0.0);
  fMomOut.SetXYZM(0.0, 0.0, 0.0, 0.0);
  fTime = fLength = fELoss = 0;
  fPosIndex                = 0;
};


#endif
