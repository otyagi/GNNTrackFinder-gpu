/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Denis Bertini [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                    CbmFieldContFact source file               -----
// -----                   Created 20/02/06  by V. Friese              -----
// -------------------------------------------------------------------------
#include "CbmFieldContFact.h"

#include "CbmFieldPar.h"  // for CbmFieldPar

#include <FairContFact.h>   // for FairContainer
#include <FairRuntimeDb.h>  // for FairRuntimeDb
#include <Logger.h>         // for LOG, Logger

#include <TList.h>    // for TList
#include <TString.h>  // for TString

#include <string.h>  // for strcmp

static CbmFieldContFact gCbmFieldContFact;


// -----   Constructor   ---------------------------------------------------
CbmFieldContFact::CbmFieldContFact()
{
  fName  = "CbmFieldContFact";
  fTitle = "Factory for field parameter containers";
  SetAllContainers();
  FairRuntimeDb::instance()->addContFactory(this);
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmFieldContFact::~CbmFieldContFact() {}
// -------------------------------------------------------------------------


// -----   Create containers   ---------------------------------------------
FairParSet* CbmFieldContFact::createContainer(FairContainer* container)
{


  const char* name = container->GetName();
  LOG(info) << "create CbmFieldPar container " << name;
  FairParSet* set = nullptr;
  if (strcmp(name, "CbmFieldPar") == 0)
    set = new CbmFieldPar(container->getConcatName().Data(), container->GetTitle(), container->getContext());
  return set;
}
// -------------------------------------------------------------------------


// -----   Set all containers (private)   ----------------------------------
void CbmFieldContFact::SetAllContainers()
{
  FairContainer* container = new FairContainer("CbmFieldPar", "Field parameter container", "Default field");
  containers->Add(container);
}
// -------------------------------------------------------------------------


ClassImp(CbmFieldContFact)
