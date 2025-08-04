/* Copyright (C) 2023 Physikalisches Institut Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Yuri Kharlov, Volker Friese, Lukas Chlad [committer] */

/** @file CbmFsdMC.h
 ** @class CbmFsdMC
 ** @brief Class for the MC transport of the CBM-FSD
 ** @since 14.07.2023
 ** @author Lukas Chlad <l.chlad@gsi.de>
 ** @version 1.0
 **
 ** The CbmFsdMC defines the behaviour of the FSD system during
 ** transport simulation. It constructs the FSD transport geometry
 ** and creates objects of type CbmFsdPoints.
 **/


#ifndef CBMFSDMC_H
#define CBMFSDMC_H 1

#include "CbmDefs.h"  // for ECbmModuleId

#include <FairDetector.h>
#include <FairRootManager.h>

#include <TClonesArray.h>
#include <TLorentzVector.h>
#include <TString.h>

class FairVolume;

class CbmFsdMC : public FairDetector {

public:
  /** @brief Constructor
   ** @param active   If set true, ProcessHits will be called
   ** @param name     Name of detector object
   **/
  CbmFsdMC(Bool_t active = kTRUE, const char* name = "FSDMC")
    : FairDetector(name, active, ToIntegralType(ECbmModuleId::kFsd)) {};


  /** Destructor **/
  virtual ~CbmFsdMC();

  /** Prevent copy constructor and assignment operator **/
  CbmFsdMC(const CbmFsdMC&) = delete;
  CbmFsdMC operator=(const CbmFsdMC&) = delete;


  /** @brief Check whether a volume is sensitive.
   ** @param(name)  Volume name
   ** @value        kTRUE if volume is sensitive, else kFALSE
   **
   ** The decision is based on the volume name (has to contain "scint").
   ** Virtual from FairModule.
   **/
  virtual Bool_t IsSensitive(const std::string& name)
  {
    return (TString(name).Contains("scint", TString::kIgnoreCase) ? kTRUE : kFALSE);
  }

  /** @brief Check whether a volume is sensitive.
   ** @param(name)  Volume name
   ** @value        kTRUE if volume is sensitive, else kFALSE
   **
   ** The decision is based on the volume name (has to contain "scint").
   ** Virtual from FairModule.
   **/
  virtual Bool_t CheckIfSensitive(std::string name) { return IsSensitive(name); }


  /** @brief Construct the FSD geometry in the TGeoManager.
   **
   ** Only ROOT geometries are supported. The file must contain a top
   ** volume the name of which starts with "fsd" and a TGeoMatrix for
   ** the placement of the top fsd volume in the cave.
   ** Virtual from FairModule.
   **/
  virtual void ConstructGeometry();


  /** @brief Action at end of event
   **
   ** Short status log and Reset().
   ** Virtual from FairDetector.
   **/
  virtual void EndOfEvent();


  /**  @brief Initialisation
	 **
	 ** The output array is created. Then, the base
	 ** class method FairDetector::Initialize() is called.
	 ** Virtual from FairDetector.
	 **/
  virtual void Initialize();


  /** Accessor to the hit collection **/
  /** @brief Get output array of CbmFsdPoints
   ** @param iColl Number of collection. Must be zero, since there is only one.
   ** @value Pointer to TClonesArray with CbmFsdPoints
   **/
  virtual TClonesArray* GetCollection(Int_t iColl) const { return (iColl ? nullptr : fFsdPoints); }


  /** @brief Screen log
   ** Prints current number of StsPoints in array.
   ** Virtual from TObject.
   **/
  virtual void Print(Option_t* opt = "") const;


  /** @brief Stepping action
   ** @param volume  Pointer to the current volume
   ** @value Always kTRUE
   **
   ** Defines the action to be taken when a step is inside the
   ** active volume. Creates CbmFsdPoints and adds them to the
   ** collection.
   ** Abstract from FairDetector.
   **/
  virtual Bool_t ProcessHits(FairVolume* volume = 0);


  /** @brief Register the output array
   **
   ** Abstract from FairDetector.
   **/
  virtual void Register() { FairRootManager::Instance()->Register("FsdPoint", GetName(), fFsdPoints, kTRUE); }


  /** @brief Clear output array
   **
   ** Abstract from FairDetector.
   **/
  virtual void Reset() { fFsdPoints->Delete(); };

private:
  TClonesArray* fFsdPoints = nullptr;  //! Output array

  /** Track information to be temporarily stored **/
  Int_t fTrackID = -1;     //!  track index
  Int_t fAddress = -1;     //!  address (module and layer)
  TLorentzVector fPos {};  //!  position
  TLorentzVector fMom {};  //!  momentum
  Double_t fTime   = -1.;  //!  time
  Double_t fLength = -1.;  //!  length
  Double_t fEloss  = -1.;  //!  energy loss

  /** @brief Register all sensitive volumes
   ** @param node Pointer to start node
   **
   ** Starting from the specified node, the entire node tree is expanded
   ** and all volumes which satisfy the CheckIfSensitive() criterion
   ** are added to the list of sensitive volumes.
   */
  void RegisterSensitiveVolumes(TGeoNode* node);

  ClassDef(CbmFsdMC, 1)
};


#endif  //? CBMFSDMC_H
