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

#ifndef CBMFSDCONTFACT_H
#define CBMFSDCONTFACT_H

#include <FairContFact.h>  // for FairContFact

#include <Rtypes.h>  // for THashConsistencyHolder, ClassDef

class FairParIo;
class FairParSet;
class FairContainer;

class CbmFsdContFact : public FairContFact {
public:
  CbmFsdContFact();
  ~CbmFsdContFact() {}
  FairParSet* createContainer(FairContainer*);

private:
  void setAllContainers();

  ClassDef(CbmFsdContFact, 0)  // Factory for all Fsd parameter containers
};

#endif /* !CBMFSDCONTFACT_H */
