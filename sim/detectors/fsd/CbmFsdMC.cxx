/* Copyright (C) 2023 Physikalisches Institut Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alla Maevskaya, Florian Uhlig, Lukas Chlad [committer] */

/** @file CbmFsdMC.cxx
 ** @since 14.07.2023
 ** @author Lukas Chlad <l.chlad@gsi.de>
 **
 **/


#include "CbmFsdMC.h"

#include "CbmFsdGeoHandler.h"
#include "CbmFsdPoint.h"
#include "CbmGeometryUtils.h"
#include "CbmModuleList.h"
#include "CbmStack.h"

#include <FairVolume.h>

#include <TGeoNode.h>
#include <TGeoVolume.h>
#include <TVirtualMC.h>

#include <cassert>
#include <string>

// -----   Destructor   ----------------------------------------------------
CbmFsdMC::~CbmFsdMC()
{
  if (fFsdPoints) {
    fFsdPoints->Delete();
    delete fFsdPoints;
  }
}
// -------------------------------------------------------------------------


// -----   Construct the geometry from file   ------------------------------
void CbmFsdMC::ConstructGeometry()
{
  LOG(info) << "Importing FSD geometry from ROOT file " << fgeoName.Data();
  Cbm::GeometryUtils::ImportRootGeometry(fgeoName, this);
}
// -------------------------------------------------------------------------


// -----   End of event action   -------------------------------------------
void CbmFsdMC::EndOfEvent()
{
  Print();  // Status output
  fFsdPoints->Delete();
}
// -------------------------------------------------------------------------


// -----   Print   ---------------------------------------------------------
void CbmFsdMC::Print(Option_t*) const
{
  LOG(info) << fName << ": " << fFsdPoints->GetEntriesFast() << " points registered in this event.";
}
// -------------------------------------------------------------------------

// -----   Initialise   ----------------------------------------------------
void CbmFsdMC::Initialize()
{
  // --- Instantiate the output array
  fFsdPoints = new TClonesArray("CbmFsdPoint");

  // --- Call the Initialise method of the mother class
  FairDetector::Initialize();
}
// -------------------------------------------------------------------------


// -----   Public method ProcessHits  --------------------------------------
Bool_t CbmFsdMC::ProcessHits(FairVolume*)
{

  // No action for neutral particles
  if (TMath::Abs(gMC->TrackCharge()) <= 0) return kFALSE;

  // --- If this is the first step for the track in the volume:
  //     Reset energy loss and store track parameters
  if (gMC->IsTrackEntering()) {
    fTrackID = gMC->GetStack()->GetCurrentTrackNumber();

    fAddress = CbmFsdGeoHandler::GetInstance().GetCurrentAddress(gMC);
    gMC->TrackPosition(fPos);
    gMC->TrackMomentum(fMom);
    fTime   = gMC->TrackTime() * 1.0e09;
    fLength = gMC->TrackLength();
    fEloss  = 0.;
  }  //? track entering

  // --- For all steps within active volume: sum up differential energy loss
  fEloss += gMC->Edep();

  // --- If track is leaving: get track parameters and create CbmstsPoint
  if (gMC->IsTrackExiting() || gMC->IsTrackStop() || gMC->IsTrackDisappeared()) {

    // Create CbmFsdPoint
    Int_t size = fFsdPoints->GetEntriesFast();
    new ((*fFsdPoints)[size]) CbmFsdPoint(fTrackID, fAddress, fPos.Vect(), fMom.Vect(), fTime, fLength, fEloss);

    // --- Increment number of FsdPoints for this track in the stack
    CbmStack* stack = dynamic_cast<CbmStack*>(gMC->GetStack());
    assert(stack);
    stack->AddPoint(ECbmModuleId::kFsd);

  }  //? track is exiting or stopped

  return kTRUE;
}
// -------------------------------------------------------------------------


// -----   Register the sensitive volumes   --------------------------------
void CbmFsdMC::RegisterSensitiveVolumes(TGeoNode* node)
{

  TObjArray* daughters = node->GetVolume()->GetNodes();
  for (Int_t iDaughter = 0; iDaughter < daughters->GetEntriesFast(); iDaughter++) {
    TGeoNode* daughter = dynamic_cast<TGeoNode*>(daughters->At(iDaughter));
    assert(daughter);
    if (daughter->GetNdaughters() > 0) RegisterSensitiveVolumes(daughter);
    TGeoVolume* daughterVolume = daughter->GetVolume();
    if (CheckIfSensitive(daughterVolume->GetName())) { AddSensitiveVolume(daughterVolume); }  //? Sensitive volume
  }                                                                                           //# Daughter nodes
}
// -------------------------------------------------------------------------


ClassImp(CbmFsdMC)
