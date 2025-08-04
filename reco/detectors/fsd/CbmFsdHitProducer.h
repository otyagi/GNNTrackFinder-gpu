/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Lukas Chlad [committer] */

/**
 * @file CbmFsdHitProducer.h
 * @author Lukas Chlad (l.chlad@gsi.de)
 * @brief Hit Producer for FSD
 * @date 2023-08-09
 * 
 * The ideal hit producer produces hits of type CbmFsdHit as full energy deposited in 1 module
 * 
 */

#ifndef CBMFSDHITPRODUCER_H
#define CBMFSDHITPRODUCER_H

#include "CbmDigiManager.h"

#include <FairTask.h>

#include <RtypesCore.h>  // for Int_t, etc.

#include <utility>

class TVector3;
class TClonesArray;
class CbmEvent;

class CbmFsdHitProducer : public FairTask {

 public:
  /** \brief Default constructor **/
  CbmFsdHitProducer();


  /** \brief Destructor **/
  ~CbmFsdHitProducer();

  /** \brief Copy constructor **/
  CbmFsdHitProducer(const CbmFsdHitProducer&) = delete;

  /** \brief Assignment operator **/
  CbmFsdHitProducer operator=(const CbmFsdHitProducer&) = delete;


  /** \brief Virtual method Init **/
  virtual InitStatus Init();


  /** \brief Virtual method Exec **/
  virtual void Exec(Option_t* opt);

  /** \brief Virtual method Finish **/
  virtual void Finish();

 private:
  Int_t fNHits = 0;

  /** Output array of CbmFsdHit **/
  TClonesArray* fHitArray = nullptr;

  /** Digi Manager for input **/
  CbmDigiManager* fDigiMan = nullptr;  //!

  /** Event array **/
  TClonesArray* fEvents = nullptr;  //!

  /**
   * Processblock of data either event-by-event or CbmEvent
   */
  std::pair<Int_t, Int_t> ProcessData(CbmEvent* event);

  /** Counters **/
  ULong64_t fNofTs     = 0;
  ULong64_t fNofEvents = 0;
  ULong64_t fNofDigis  = 0;
  ULong64_t fNofHits   = 0;
  Double_t fTimeTot    = 0.;

  void Reset();

  ClassDef(CbmFsdHitProducer, 1);
};

#endif  // CBMFSDHITPRODUCER_H
