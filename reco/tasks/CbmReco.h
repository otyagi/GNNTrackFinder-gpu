/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBMRECO_H
#define CBMRECO_H 1

#include "CbmDefs.h"

// Hide the Monitor class from ROOT to not confuse it.
// A similar issue with ROOT and gcc9 was already discovered by ALICE O2:
// https://github.com/AliceO2Group/AliceO2/pull/4858
#if !defined(__CLING__) && !defined(__ROOTCLING__)
#include "Monitor.hpp"
#else
namespace cbm
{
  class Monitor;
}
#endif

#include "evbuild/Config.h"

#include <TString.h>

#include <map>
#include <string>
#include <utility>


/** @class CbmReco
 ** @brief Main steering class of reconstruction in CBM
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 14 March 2022
 **
 ** The class instantiates the processing steps as FairTasks, and configure and executes the
 ** run as FairRunOnline.
 **
 ** Currently included processing steps:
 ** - unpacking (STS only)
 ** - trigger on STS digis
 ** - event building (STS only)
 **/
class CbmReco {

 public:
  // Configuration structure
  // TODO: move more settings here
  struct Config {
    bool dumpSetup = false;
  };

  /** @brief Default constructor **/
  //CbmReco() {};


  /** @brief Standard constructor for a single source
   ** @param source  Name of input file or stream
   ** @param outFile Name of output file
   ** @param numTs   Number of timeslices to process. If negative, all available will be used.
   ** @param config  Configuration
   ** @param port    Port number for the http server. If 0, server will not be activated.
   **/
  CbmReco(Config config, std::string source, std::string outFile, std::string configFile,
          int32_t numTs = std::numeric_limits<int32_t>::max(), uint16_t port = 0, cbm::Monitor* monitor = nullptr);


  /** @brief Standard constructor for list of sources
   ** @param source  Vector of names of input files or input sources
   ** @param outFile Name of output file
   ** @param numTs   Number of timeslices to process. If negative, all available will be used.
   ** @param config  Configuration
   ** @param port    Port number for the http server
   **/
  CbmReco(Config config, std::vector<std::string> sources, std::string outFile, std::string configFile,
          int32_t numTs = std::numeric_limits<int32_t>::max(), uint16_t port = 0, cbm::Monitor* monitor = nullptr);


  /** @brief Destructor **/
  virtual ~CbmReco(){};


  /** @brief List all entries in the input vector
   ** @return String concatenating the sources names
   **/
  std::string ListSources() const;


  /** @brief Configure and execute run
   ** @return Number of processed timeslices. -1 if error encountered.
   **/
  int32_t Run();


 private:
  Config fConfig                        = {};  ///< Configuration
  std::vector<std::string> fSourceNames = {};  ///< Sources (input files or stream)
  std::string fOutputFileName           = "";  ///< Output file (ROOT)
  std::string fConfigFileName           = "";  ///< Configuration file (YAML)
  int32_t fNumTs                        = 0;   ///< Number of timeslices to process
  uint16_t fHttpServerPort              = 0;
  cbm::Monitor* fMonitor                = nullptr;

  ClassDef(CbmReco, 1);
};

#endif /* CBMRECO_H */
