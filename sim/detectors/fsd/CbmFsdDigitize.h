/* Copyright (C) 2023 Physikalisches Institut Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Morozov, Volker Friese, Lukas Chlad [committer] */

/** @class CbmFsdDigitize
 ** @date  14.07.2023
 ** @author Lukas Chlad <l.chlad@gsi.de>
 ** @brief Class for the digitization of the CBM-FSD
 **
 ** The digitizer produces digits of type CbmFsdDigi as sum of Edep of Points and adds smearing in energy and time according to parameters
 **/


#ifndef CBMFSDDIGITIZE_H
#define CBMFSDDIGITIZE_H 1


#include "CbmDefs.h"
#include "CbmDigitize.h"
#include "CbmFsdDigi.h"

#include <TArrayD.h>

class TClonesArray;
class CbmFsdDigiPar;

class CbmFsdDigitize : public CbmDigitize<CbmFsdDigi> {

public:
  /** Default constructor **/
  CbmFsdDigitize() : CbmDigitize<CbmFsdDigi>("FsdDigitize") {};

  /** Destructor **/
  virtual ~CbmFsdDigitize() = default;

  CbmFsdDigitize(const CbmFsdDigitize&) = delete;
  CbmFsdDigitize operator=(const CbmFsdDigitize&) = delete;

  ECbmModuleId GetSystemId() const { return ECbmModuleId::kFsd; }


  /**
   ** @brief Inherited from FairTask.
   **/
  virtual InitStatus Init();

  /**
   ** @brief Inherited from FairTask.
   **/
  virtual void SetParContainers();

  /** Virtual method Exec **/
  virtual void Exec(Option_t* opt);


  /** @brief End-of-run action **/
  virtual void Finish();


private:
  CbmFsdDigiPar* fDigiPar = nullptr;

  Int_t fNumPhotoDets = -1;
  Int_t fNumUnits     = -1;
  TArrayD fTimeResolution {};
  TArrayD fEnergyResolution {};
  TArrayD fDeadTime {};

  Int_t fNumEvents    = 0;
  Double_t fNumPoints = 0.;
  Double_t fNumDigis  = 0.;
  Double_t fTimeTot   = 0.;

  /** Input array of CbmFsdPoints **/
  TClonesArray* fPointArray = nullptr;

  // Temporary storage for digis, key is DetectorID from FsdPoint
  std::map<int32_t, std::pair<CbmFsdDigi*, CbmMatch*>> fDigiBuffer;

  /** @brief Initialise the parameters **/
  void InitParams();

  /** @brief release digi from local buffer to CbmDaq
   ** TIME BASED 
   **  - at the beginning of MCEvent loop over digi buffer 
   **    and send those digis that are too far from start of current event to be possibly edited
   **    free the location of sent digis
   **  - at the end of whole run do the same only send everything, clear buffer
   **
   ** EVENT BASED
   **  - at the beginning of MCEvent loop send everything, clear buffer
   **/
  void ReleaseBuffer(Bool_t sendEverything);


  ClassDef(CbmFsdDigitize, 1);
};

#endif
