/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include "Definitions.h"

#include <FairTask.h>

class CbmTrdParSetAsic;
class CbmTrdParSetDigi;

class CbmTaskTrdUnpackParWrite : public FairTask {

 public:
  struct Pars {
    CbmTrdParSetAsic* asic;  ///< parameter list for ASIC characterization
    CbmTrdParSetDigi* digi;  ///< parameter list for read-out geometry
    cbm::algo::Setup setup;  ///< Setup type
  };

  /**
   * \brief Default constructor.
   */
  CbmTaskTrdUnpackParWrite(Pars pars) : fPars(pars) {}

  /**
   * \brief Default destructor.
   */
  virtual ~CbmTaskTrdUnpackParWrite(){};

  /** Initialisation **/
  virtual InitStatus Init();
  virtual void SetParContainers(){};

  /** \brief Executed task **/
  virtual void Exec(Option_t* /*option*/){};

  /** Finish task **/
  virtual void Finish(){};

 private:
  Pars fPars;
};
