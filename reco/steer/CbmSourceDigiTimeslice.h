/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#pragma once  // include this header only once per compilation unit

#include "DigiData.h"
#include "RecoResultsInputArchive.h"

#include <FairSource.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CbmTimeSlice;
class CbmTsEventHeader;
class FairRootManager;

/** @class CbmSourceDigiTimeslice
 ** @brief Source class for reading from files resulting from online processing (containing raw Digis)
 ** @author Pierre-Alain loizeau <p.-a.loizeau
 ** @since 21 March 2024
 **
 ** The online process creates std::vector of Digis for each detector per timeslice.
 ** These are saved to file using the BOOST streamer.
 ** This class allows to read such files and get the Digis into the ROOT tree for offline analysis. It creates a
 ** branches containing the Digis vector with names following established conventions; one tree entry corresponds to one
 ** timelice.
 **/
class CbmSourceDigiTimeslice : public FairSource {

 public:
  /** @brief Constructor
   ** @param fileName  Name of (single) input file.
   **
   ** More input files can be added by the method AddInputFile.
   */
  CbmSourceDigiTimeslice(const char* fileName = "");

  /** @brief Copy constructor - not implemented **/
  CbmSourceDigiTimeslice(const CbmSourceDigiTimeslice&) = delete;

  /** @brief Assignment operator - not implemented **/
  CbmSourceDigiTimeslice& operator=(const CbmSourceDigiTimeslice&) = delete;

  /** @brief Destructor **/
  virtual ~CbmSourceDigiTimeslice() {}

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

  /** Branch vectors of Digis **/
  std::vector<CbmBmonDigi>* fBmonDigis = nullptr;
  std::vector<CbmStsDigi>* fStsDigis   = nullptr;
  std::vector<CbmMuchDigi>* fMuchDigis = nullptr;
  std::vector<CbmTrdDigi>* fTrdDigis   = nullptr;
  std::vector<CbmTofDigi>* fTofDigis   = nullptr;
  std::vector<CbmRichDigi>* fRichDigis = nullptr;

  /** @brief type of source that is currently used
      @remark currently we use kONLINE as default, since, kFILE skipps the first TS probably due to obsolete reasons
              (to be checked PR072021)
   **/
  Source_Type fSourceType = Source_Type::kONLINE;

  /** Time-slice header (old version)
   *  @remark The CbmTimeSlice class is about to be deprecated, one should use the CbmTsEventHeader class instead.
   **/
  CbmTimeSlice* fTimeslice = nullptr;

  /** Time-slice event header **/
  CbmTsEventHeader* fTsEventHeader = nullptr;


  /** Time-slice counter **/
  size_t fNumTs = 0;

  template<typename TVecobj>
  Bool_t RegisterVector(FairRootManager* ioman, std::vector<TVecobj>*& vec);

  template<typename TVecobj>
  typename std::enable_if<std::is_member_function_pointer<decltype(&TVecobj::GetTime)>::value, void>::type
  Timesort(std::vector<TVecobj>* vec = nullptr)
  {
    if (vec == nullptr) return;
    std::sort(vec->begin(), vec->end(),
              [](const TVecobj& a, const TVecobj& b) -> bool { return a.GetTime() < b.GetTime(); });
  }


  ClassDef(CbmSourceDigiTimeslice, 1)
};
