/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig, Lukas Chlad [committer] */

/** CbmFsdContFact.h
 *
 * @author  F. Uhlig <f.uhlig@gsi.de>
 * @version 1
 * @since   25.01.21
 *
 *  Factory for the parameter containers of the FSD detector
 *
 */
#include "CbmFsdContFact.h"

#include "CbmFsdDigiPar.h"

#include <FairContFact.h>   // for FairContainer
#include <FairRuntimeDb.h>  // for FairRuntimeDb
#include <Logger.h>         // for LOG

#include <TList.h>    // for TList
#include <TString.h>  // for TString

#include <string.h>  // for strcmp

ClassImp(CbmFsdContFact)

  static CbmFsdContFact gCbmFsdContFact;

CbmFsdContFact::CbmFsdContFact()
{
  // Constructor (called when the library is loaded)
  fName  = "CbmFsdContFact";
  fTitle = "Factory for parameter containers in libFsdBase";
  setAllContainers();
  FairRuntimeDb::instance()->addContFactory(this);
}

void CbmFsdContFact::setAllContainers()
{
  /** Creates the Container objects with all accepted contexts and adds them to
   *  the list of containers for the FsdBase library.*/

  FairContainer* p1 =
    new FairContainer("CbmFsdDigiPar", "Digitization parameters for the FSD detector",
                      "Needed parameters to adjust FsdDigitizer according to the geometry and read-out propetries");
  p1->addContext("Needed parameters to adjust FsdDigitizer according to the geometry and read-out propetries");

  containers->Add(p1);
}

FairParSet* CbmFsdContFact::createContainer(FairContainer* c)
{
  /** Calls the constructor of the corresponding parameter container.
   * For an actual context, which is not an empty string and not the default context
   * of this container, the name is concatinated with the context. */
  const char* name = c->GetName();
  LOG(info) << " -I container name " << name;
  FairParSet* p = nullptr;

  if (strcmp(name, "CbmFsdDigiPar") == 0) {
    p = new CbmFsdDigiPar(c->getConcatName().Data(), c->GetTitle(), c->getContext());
  }

  return p;
}
