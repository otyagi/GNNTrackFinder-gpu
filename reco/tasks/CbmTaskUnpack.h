/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */


#ifndef CBMTASKUNPACK_H
#define CBMTASKUNPACK_H 1

#include "CbmDefs.h"
#include "CbmDigiTimeslice.h"

// Hide the Monitor class from ROOT to not confuse it
#if !defined(__CLING__) && !defined(__ROOTCLING__)
#include "Monitor.hpp"
#else
namespace cbm
{
  class Monitor;
}
#endif

#include "AlgoTraits.h"
#include "EventBuilder.h"
#include "ParFiles.h"
#include "bmon/Unpack.h"
#include "much/Unpack.h"
#include "rich/Unpack.h"
#include "sts/Unpack.h"
#include "tof/Unpack.h"
#include "trd/Unpack.h"
#include "trd2d/Unpack.h"

#include <FairTask.h>

#include <boost/filesystem.hpp>

#include <sstream>
#include <vector>

class CbmDigiManager;
class CbmSourceTs;
class CbmTimeSlice;
class CbmTsEventHeader;
class FairRootManager;

namespace fs = boost::filesystem;

/** @class CbmTaskUnpack
 ** @brief Task class for associating digis to events
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 15.11.2021
 **
 ** Creates objects of class CbmDigiEvent and fills them with digi objects,
 ** using the algorithm EventBuilder.
 **
 ** TOFO: The current implementation is for STS only and with a dummy trigger list
 ** just to establish the framework integration of algorithm and data interfaces.
 **/
class CbmTaskUnpack : public FairTask {

 public:
  struct Config {
    bool dumpSetup = false;
  };

  struct Monitor {
    size_t numCompUsed = 0;
    size_t numMs       = 0;
    size_t numBytes    = 0;
    size_t numDigis    = 0;
  };

  /** @brief Constructor **/
  CbmTaskUnpack();


  /** @brief Constructor **/
  CbmTaskUnpack(fs::path paramsDir, uint32_t runId);


  /** @brief Copy constructor (disabled) **/
  CbmTaskUnpack(const CbmTaskUnpack&) = delete;


  /** @brief Destructor **/
  virtual ~CbmTaskUnpack();


  /** @brief Task execution **/
  virtual void Exec(Option_t* opt);


  /** @brief Finish timeslice **/
  virtual void Finish();


  /** @brief Assignment operator (disabled) **/
  CbmTaskUnpack& operator=(const CbmTaskUnpack&) = delete;


  /** @brief Set the monitor object **/
  void SetMonitor(cbm::Monitor* monitor) { fMonitor = monitor; }


  /** @brief Set the output file in "as if cbmroot digi file" mode (default is "as if rra") **/
  void SetOutputModeCbmrootLike(bool flag = true) { fCbmrootFormatOutput = flag; }


 private:  // methods
  /** @brief Task initialisation **/
  virtual InitStatus Init();

 private:  // members
  CbmSourceTs* fSource   = nullptr;
  cbm::Monitor* fMonitor = nullptr;
  std::string fHostname;

  /* Unpacker algorithms */
  std::unique_ptr<cbm::algo::bmon::Unpack> fBmonUnpack;
  std::unique_ptr<cbm::algo::much::Unpack> fMuchUnpack;
  std::unique_ptr<cbm::algo::rich::Unpack> fRichUnpack;
  std::unique_ptr<cbm::algo::sts::Unpack> fStsUnpack;
  std::unique_ptr<cbm::algo::tof::Unpack> fTofUnpack;
  std::unique_ptr<cbm::algo::trd::Unpack> fTrdUnpack;
  std::unique_ptr<cbm::algo::trd2d::Unpack> fTrd2dUnpack;

  size_t fNumTs                    = 0;
  size_t fNumMs                    = 0;
  size_t fNumBytes                 = 0;
  size_t fNumDigis                 = 0;
  double fTime                     = 0.;
  CbmDigiTimeslice* fDigiTimeslice = nullptr;  ///< Output data if writing root files "as if rra"

  /// Output data if writing root file "as if cbmroot digi file"
  /// Flag controlling the output mode
  bool fCbmrootFormatOutput = false;
  /// => Time-slice header (old version, class about to be deprecated? one should use only CbmTsEventHeader soon?)
  CbmTimeSlice* fTimeslice = nullptr;
  /// Time-slice event header
  CbmTsEventHeader* fTsEventHeader = nullptr;
  /// => Branch vectors of Digis
  std::vector<CbmBmonDigi>* fBmonDigis = nullptr;
  std::vector<CbmStsDigi>* fStsDigis   = nullptr;
  std::vector<CbmMuchDigi>* fMuchDigis = nullptr;
  std::vector<CbmTrdDigi>* fTrdDigis   = nullptr;
  std::vector<CbmTofDigi>* fTofDigis   = nullptr;
  std::vector<CbmRichDigi>* fRichDigis = nullptr;

  template<class Unpacker>
  auto RunUnpacker(const std::unique_ptr<Unpacker>& unpacker, const fles::Timeslice& ts, Monitor& monitor)
    -> cbm::algo::algo_traits::Output_t<Unpacker>;

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

  ClassDef(CbmTaskUnpack, 4);
};

#endif /* CBMTASKUNPACK_H */
