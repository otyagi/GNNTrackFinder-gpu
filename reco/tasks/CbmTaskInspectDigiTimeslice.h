/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#pragma once


#include "CbmBmonDigi.h"
#include "CbmDefs.h"
#include "CbmMuchDigi.h"
#include "CbmRichDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"
#include "CbmTrdDigi.h"

#include <FairTask.h>

#include <vector>

class FairRootManager;

/** @class CbmTaskInspectDigiTimeslice
 ** @brief Demonstrator class to save online unpacked digis in an output ROOT tree
 ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
 ** @since 21 March 2024
 **
 ** This is a demonstrator of how to convert online unpacked digis into an output ROOT tree.
 ** It also logs some information to the console as an example on how to look at these digis.
 **/
class CbmTaskInspectDigiTimeslice : public FairTask {

 public:
  /** @brief Constructor **/
  CbmTaskInspectDigiTimeslice();


  /** @brief Copy constructor (disabled) **/
  CbmTaskInspectDigiTimeslice(const CbmTaskInspectDigiTimeslice&) = delete;


  /** @brief Destructor **/
  virtual ~CbmTaskInspectDigiTimeslice();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmTaskInspectDigiTimeslice& operator=(const CbmTaskInspectDigiTimeslice&) = delete;


 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();

  template<typename TVecobj>
  const std::vector<TVecobj>* InitInput(FairRootManager* ioman);

 private:                                                // members
  const std::vector<CbmBmonDigi>* fBmonDigis = nullptr;  //! Input data (digis)
  const std::vector<CbmStsDigi>* fStsDigis   = nullptr;  //! Input data (digis)
  const std::vector<CbmMuchDigi>* fMuchDigis = nullptr;  //! Input data (digis)
  const std::vector<CbmTrdDigi>* fTrdDigis   = nullptr;  //! Input data (digis)
  const std::vector<CbmTofDigi>* fTofDigis   = nullptr;  //! Input data (digis)
  const std::vector<CbmRichDigi>* fRichDigis = nullptr;  //! Input data (digis)
  size_t fNumTs                              = 0;        ///< Number of processed timeslices


  ClassDef(CbmTaskInspectDigiTimeslice, 1);
};
