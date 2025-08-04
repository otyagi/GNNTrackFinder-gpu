/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#pragma once


#include "CbmDefs.h"
#include "CbmDigiEvent.h"

#include <FairTask.h>

#include <vector>


/** @class CbmTaskInspectDigiEvents
 ** @brief Demonstrator class to look at digi events in the ROOT tree
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 19.03.2024
 **
 ** This is just a demonstrator how to look at DigiEvents in the ROOT tree. It logs some information to the console.
 **/
class CbmTaskInspectDigiEvents : public FairTask {

 public:
  /** @brief Constructor **/
  CbmTaskInspectDigiEvents();


  /** @brief Copy constructor (disabled) **/
  CbmTaskInspectDigiEvents(const CbmTaskInspectDigiEvents&) = delete;


  /** @brief Destructor **/
  virtual ~CbmTaskInspectDigiEvents();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmTaskInspectDigiEvents& operator=(const CbmTaskInspectDigiEvents&) = delete;


 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();


 private:                                              // members
  const std::vector<CbmDigiEvent>* fEvents = nullptr;  //! Input data (events)
  size_t fNumTs                            = 0;        ///< Number of processed timeslices
  size_t fNumEvents                        = 0;        ///< Number of analysed events


  ClassDef(CbmTaskInspectDigiEvents, 1);
};
