/* Copyright (C) 2012-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alla Maevskaya, Volker Friese [committer], Florian Uhlig */

/** CbmPsdHit.cxx
 **@author Alla Maevskaya <alla@inr.ru>
 **@since 3.08.20212
 **
 ** Modified to simplify fEdep[49] -> fEdep (S. Seddiki)
 **/
#include "CbmPsdHit.h"

#include <Logger.h>  // for Logger, LOG

#include <TObject.h>  // for TObject

// -----   Default constructor   -------------------------------------------
CbmPsdHit::CbmPsdHit() : TObject(), fModuleID(-1), fEdep(-1)  // SELIM: simplification vector [49] -> simple double
{

  //for (int32_t j=0; j<49; j++)     // SELIM: simplification vector [49] -> simple double
  //  fEdep[j]=0;
}
CbmPsdHit::CbmPsdHit(int32_t module, double edep)
  : TObject()
  , fModuleID(module)
  , fEdep(edep)  // SELIM: simplification vector [49] -> simple double
{
  //for (int32_t j=0; j<49; j++)     // SELIM: simplification vector [49] -> simple double
  //fEdep[j] = edep;
}


// -----   Destructor   ----------------------------------------------------
CbmPsdHit::~CbmPsdHit() {}
// -------------------------------------------------------------------------

void CbmPsdHit::Print(Option_t*) const { LOG(info) << "module : " << fModuleID << " ELoss " << fEdep; }

ClassImp(CbmPsdHit)
