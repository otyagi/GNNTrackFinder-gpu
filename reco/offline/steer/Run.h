/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file Run.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 12.05.2023
 **/

#ifndef CBM_RECO_OFFLINE_STEER_RUN_H
#define CBM_RECO_OFFLINE_STEER_RUN_H 1

#include "CbmDefs.h"
#include "Config.h"

#include <FairRunAna.h>

#include <TNamed.h>

#include <string>

class TGeoManager;
class CbmSetup;
class FairTask;
class FairFileSource;
class TTree;

namespace cbm::reco::offline
{

  class Run : public TNamed {

   public:
    /** @brief Constructor **/
    Run();


    /** @brief Destructor  **/
    virtual ~Run();


    /** @brief Add a task to the run **/
    void AddTask(FairTask* task);


    /** @brief Allow overwriting if output file already exists **/
    void AllowOverwrite() { fOverwrite = true; }


    /** @brief Run reconstruction **/
    void Exec();


    /** @brief Settings object **/
    const Config& GetConfig() const { return fConfig; }


    /** @brief Presence of input digi data
     ** @param detector Detector data to check
     ** @return true if data present, else false
     **/
    bool IsDataPresent(ECbmModuleId detector) const { return (fDataPresent.count(detector) == 0 ? false : true); }


    /** @brief Set configuration file name
     ** @param fileName  Configuration file name
     **
     ** Legacy interface for running from ROOT prompt. In the executable, the config is read in
     ** by the application.
     **/
    void LoadConfig(const char* fileName);


    /** @brief Set configuration
     ** @param fileName  Configuration object
     **/
    void SetConfig(const Config& config) { fConfig = config; }


    /** @brief Set geometry setup tag
     ** @param tag  Geometry setup tag
     **/
    void SetGeoSetupTag(const char* tag) { fSetupTag = tag; }


    /** @brief Set digitization (raw) file name
     ** @param fileName  Digi (raw) file name
     **/
    void SetRawFile(const char* fileName) { fRaw = fileName; }


    /** @brief Set number of timeslices to process
     ** @param numTs Number of timeslice to process
     **/
    void SetNumTs(int32_t num) { fNumTs = num; }


    /** @brief Set output file name
     ** @param fileName  Output file name
     **/
    void SetOutput(const char* fileName) { fOutput = fileName; }


    /** @brief Set parameter file name
     ** @param fileName  Parameter file name
     **/
    void SetParFile(const char* fileName) { fPar = fileName; }


   private:
    /** @brief Copy constructor forbidden **/
    Run(const Run&) = delete;


    /** @brief Assignment operator forbidden **/
    Run operator=(const Run&) = delete;


    /** @brief Check and mark presence of a digi branch
     ** @param detector ECbmModuleId
     ** @param tree Pointer to ROOT Tree
     **/
    void CheckDigiBranch(TTree* tree, ECbmModuleId detector);


    /** @brief Check existence of a file
     ** @param fileName  File name (absolute or relative to current directory)
     ** @return true if file exists
     **/
    bool CheckFile(const char* fileName);


    /** @brief Check the presence of digi input branches **/
    void CheckInputBranches(FairFileSource* source);


    /** @brief Create the reconstruction task topology (chain) **/
    void CreateTopology();


   private:
    FairRunAna fRun{};
    TString fOutput   = "";
    TString fRaw      = "";
    TString fPar      = "";
    TString fSetupTag = "";
    CbmSetup* fSetup  = nullptr;
    size_t fNumTs     = 0;
    bool fOverwrite   = false;

    Config fConfig                      = {};
    std::set<ECbmModuleId> fDataPresent = {};


    ClassDef(cbm::reco::offline::Run, 1);
  };

}  // namespace cbm::reco::offline

#endif /* CBM_RECO_OFFLINE_STEER_RUN_H */
