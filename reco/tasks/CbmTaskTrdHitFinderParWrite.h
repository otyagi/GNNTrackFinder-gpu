/* Copyright (C) 2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Dominik Smith [committer] */

#ifndef CBMTRDHITFINDERPARWRITE_H
#define CBMTRDHITFINDERPARWRITE_H

#include "FairTask.h"
#include "trd/Clusterizer.h"
#include "trd/Clusterizer2D.h"
#include "trd/Hit.h"
#include "trd/HitFinder.h"
#include "trd/HitFinder2D.h"

#include <map>
#include <set>
#include <vector>

class CbmTrdParSetAsic;
class CbmTrdParSetGeo;
class CbmTrdParSetDigi;

/** CbmTaskTrdHitFinderParWrite.h
 *@author Dominik Smith <d.smith@gsi.de>
 **
 ** Task to create YAML files for TRD hitfinders
 **
 **/
class CbmTaskTrdHitFinderParWrite : public FairTask {

 public:
  /**
   * \brief Default constructor.
   */
  CbmTaskTrdHitFinderParWrite();

  /**
   * \brief Default destructor.
   */
  ~CbmTaskTrdHitFinderParWrite(){};

  /** Initialisation **/
  virtual InitStatus Init();
  virtual void SetParContainers();

  /** \brief Executed task **/
  virtual void Exec(Option_t*){};

  /** Finish task **/
  virtual void Finish(){};

 private:
  CbmTaskTrdHitFinderParWrite(const CbmTaskTrdHitFinderParWrite&);
  CbmTaskTrdHitFinderParWrite& operator=(const CbmTaskTrdHitFinderParWrite&);

  //==================================================================
  CbmTrdParSetAsic* fAsicPar = nullptr;  ///< parameter list for ASIC characterization
  CbmTrdParSetDigi* fDigiPar = nullptr;  ///< parameter list for read-out geometry
  CbmTrdParSetGeo* fGeoPar   = nullptr;  ///< parameter list for modules geometry

  ClassDef(CbmTaskTrdHitFinderParWrite, 1);
};
#endif
