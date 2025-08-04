/* Copyright (C) 2013 Institut fuer Kernphysik, Westfaelische Wilhelms-Universitaet Muenster, Muenster
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Cyrano Bergmann [committer] */

#ifndef CBMTRDCLUSTERIZERFASTQA_H
#define CBMTRDCLUSTERIZERFASTQA_H 1

#include "FairTask.h"

class TClonesArray;
class TH1F;
class TH2F;

class CbmTrdClusterizerFastQa : public FairTask {
 public:
  /* Defaul constructor */
  CbmTrdClusterizerFastQa();

  /* Standard constructor */
  CbmTrdClusterizerFastQa(const char* name, const char* title = "FairTask");

  /* Destructor */
  virtual ~CbmTrdClusterizerFastQa();

  /* Initialisation */
  InitStatus Init();

  /* Execution */
  virtual void Exec(Option_t* option);

  /* Finish at the end of each event */
  virtual void Finish();


 private:
  /* Prepare the QA histograms */
  void PrepareHistograms();


  /* Write the QA histograms to file*/
  void WriteHistograms();

  CbmTrdClusterizerFastQa(const CbmTrdClusterizerFastQa&);
  CbmTrdClusterizerFastQa& operator=(const CbmTrdClusterizerFastQa&);

  ClassDef(CbmTrdClusterizerFastQa, 1)
};

#endif
