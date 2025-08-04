/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese[committer] */

#pragma once  // include this header only once per compilation unit

#include "CbmDigiEvent.h"
#include "DigiData.h"
#include "RecoResultsInputArchive.h"

#include <FairSource.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


/** @class CbmSourceDigiEvents
 ** @brief Source class for reading from files resulting from online processing (containing DigiEvents)
 ** @author Volker Friese
 ** @since 18 March 2024
 **
 ** The online process creates a std::vector of DigiEvents per timeslice. These are saved to file using the BOOST streamer.
 ** This class allows to read such files and get the DigiEvents into the ROOT tree for offline analysis. It creates a 
 ** branch DigiEvents containing the DigiEvent vector; one tree entry corresponds to one timelice.
 **/
class CbmSourceDigiEvents : public FairSource {

 public:
  /** @brief Constructor
   ** @param fileName  Name of (single) input file.
   **
   ** More input files can be added by the method AddInputFile.
   */
  CbmSourceDigiEvents(const char* fileName = "");

  /** @brief Copy constructor - not implemented **/
  CbmSourceDigiEvents(const CbmSourceDigiEvents&) = delete;

  /** @brief Assignment operator - not implemented **/
  CbmSourceDigiEvents& operator=(const CbmSourceDigiEvents&) = delete;

  /** @brief Destructor **/
  virtual ~CbmSourceDigiEvents() {}

  /** @brief Close source after end of run **/
  virtual void Close();

  /** @brief Source type
   ** @return FairSource::Source_Type
   **/
  virtual Source_Type GetSourceType() { return fSourceType; }

  /** @brief Initialisation **/
  virtual Bool_t Init();

  /** @brief Initialise unpackers (forced by base class, not relevant here) **/
  virtual Bool_t InitUnpackers() { return kTRUE; }

  /** @brief Read one time slice from file **/
  Int_t ReadEvent(UInt_t = 0);

  /** @brief Re-Initialise unpackers (forced by base class, not relevant here) **/
  virtual Bool_t ReInitUnpackers() { return kTRUE; }

  /** @brief Reset (forced by base class, not relevant here) **/
  virtual void Reset() {}

  /** @brief Set unpacker parameters (forced by base class, not relevant here) **/
  virtual void SetParUnpackers() {}

  /** @brief Set the Source Type 
   ** @param type Source type 
   **/
  void SetSourceType(Source_Type type) { fSourceType = type; }

  /** @brief Set Run ID (forced by base class, not relevant here) **/
  Bool_t SpecifyRunId() { return kTRUE; }


 private:
  /** Input file name **/
  std::string fInputFileName = {};

  /** Input archive **/
  std::unique_ptr<cbm::algo::RecoResultsInputArchive> fArchive = nullptr;

  /** Branch vector of DigiEvents **/
  std::vector<CbmDigiEvent>* fEvents = nullptr;

  /** @brief type of source that is currently used @remark currently we use kONLINE as default, since, kFILE skipps the first TS probably due to obsolete reasons (to be checked PR072021) */
  Source_Type fSourceType = Source_Type::kONLINE;

  /** Time-slice counter **/
  size_t fNumTs = 0;

  /** Event counter **/
  size_t fNumEvents = 0;


  ClassDef(CbmSourceDigiEvents, 1)
};
