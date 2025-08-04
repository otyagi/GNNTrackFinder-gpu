/* Copyright (C) 2006-2009 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Denis Bertini [committer], Mohammad Al-Turany */

#ifndef CBMPASSIVECONTFACT_H
#define CBMPASSIVECONTFACT_H

#include "FairContFact.h"

class FairContainer;

class CbmPassiveContFact : public FairContFact {
private:
  void setAllContainers();

public:
  CbmPassiveContFact();
  ~CbmPassiveContFact() {}
  FairParSet* createContainer(FairContainer*);
  ClassDef(CbmPassiveContFact,
           0)  // Factory for all Passive parameter containers
};

#endif /* !CBMPASSIVECONTFACT_H */
