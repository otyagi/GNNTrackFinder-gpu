/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */


#ifndef CBMRECOBMON_H
#define CBMRECOBMON_H 1

#include "CbmBmonDigi.h"
#include "FairTask.h"

#include <vector>

class TClonesArray;


/** @struct CbmRecoT0MoniData
 ** @brief Monitor data for Bmon reconstruction
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 10.11.2022
 **/
struct CbmRecoT0MoniData {
  size_t fNumTs        = 0;
  size_t fNumEvents    = 0;
  size_t fNumEvtsBmon0 = 0;
  size_t fNumEvtsBmon1 = 0;
  size_t fNumEvtsBmonn = 0;
  double fExecTime     = 0.;

  CbmRecoT0MoniData& operator+=(const CbmRecoT0MoniData& other)
  {
    fNumTs += other.fNumTs;
    fNumEvents += other.fNumEvents;
    fNumEvtsBmon0 += other.fNumEvtsBmon0;
    fNumEvtsBmon1 += other.fNumEvtsBmon1;
    fNumEvtsBmonn += other.fNumEvtsBmonn;
    fExecTime += other.fExecTime;
    return *this;
  }
};

/** @class CbmRecoT0
 ** @brief Task class for reconstruction of the event t0
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 10.11.2022
 **
 ** The current implementation reads the t0 information from the TdzDigi object. t0 is set to -1. if
 ** no such object is in the event, and to -2. if there are several.
 **/
class CbmRecoT0 : public FairTask {

 public:
  /** @brief Constructor
   **
   ** @param name    Name of task
   ** @param title   Title of task
   **/
  CbmRecoT0(const char* name = "RecoBmon");


  /** @brief Destructor **/
  virtual ~CbmRecoT0();


  /** @brief Initialisation **/
  virtual InitStatus Init();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief End-of-timeslice action **/
  virtual void Finish();


 private:
  // --- Data
  const std::vector<CbmBmonDigi>* fBmonDigis = nullptr;  ///< BMON digis
  TClonesArray* fEvents                      = nullptr;  ///< CbmEvent

  // --- Monitor
  CbmRecoT0MoniData fMonitor = {};  ///< Monitor data


  ClassDef(CbmRecoT0, 1);
};

#endif
