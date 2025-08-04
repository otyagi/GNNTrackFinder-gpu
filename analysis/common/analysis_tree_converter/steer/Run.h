/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file Run.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 27.10.2023
 **/

#ifndef CBM_ATCONVERTER_STEER_RUN_H
#define CBM_ATCONVERTER_STEER_RUN_H 1

#include "CbmDefs.h"
#include "Config.h"

#include <FairRunAna.h>

#include <TNamed.h>
#include <TString.h>

#include <string>

class CbmSetup;
class FairTask;
class FairFileSource;
class TTree;

namespace cbm::atconverter
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


    /** @brief Set output file name
     ** @param fileName  Output file name
     **/
    void SetOutput(const char* fileName) { fOutput = fileName; }


    /** @brief Set transport input files
     ** @param files  Vector of transport input file names
     **/
    void SetTraFiles(const std::vector<std::string> files);


    /** @brief Set digitizazion (raw) file name
     ** @param fileName  Digitization file name
     **/
    void SetRawFile(const char* fileName) { fRaw = fileName; }


    /** @brief Set parameter file name
     ** @param fileName  Parameter file name
     **/
    void SetParFile(const char* fileName) { fPar = fileName; }


    /** @brief Set reconstruction file name
     ** @param fileName  Reconstruction file name
     **/
    void SetRecoFile(const char* fileName) { fReco = fileName; }


  private:
    /** @brief Copy constructor forbidden **/
    Run(const Run&) = delete;


    /** @brief Assignment operator forbidden **/
    Run operator=(const Run&) = delete;


    /** @brief Check existence of a file
     ** @param fileName  File name (absolute or relative to current directory)
     ** @return true if file exists
     **/
    bool CheckFile(const char* fileName);


    /** @brief Check and mark presence of reco branches
     ** @param tree Pointer to ROOT tree
     ** @param detector ECbmModuleId
     **/
    void CheckRecoBranch(TTree* tree, ECbmModuleId detector);


    /** @brief Check the presence of reco input branches **/
    void CheckInputBranches(FairFileSource* source);


    /** @brief Create the reconstruction task topology (chain) **/
    void CreateTopology();


  private:
    FairRunAna fRun {};
    TString fOutput   = "";
    std::vector<TString> fTra;
    TString fRaw      = "";
    TString fPar      = "";
    TString fReco     = "";
    TString fSetupTag = "";
    CbmSetup* fSetup  = nullptr;
    bool fOverwrite   = false;

  public:
    Config fConfig                      = {};
    std::set<ECbmModuleId> fDataPresent = {};


    ClassDef(cbm::atconverter::Run, 1);
  };

}  // namespace cbm::atconverter

#endif /* CBM_ATCONVERTER_STEER_RUN_H */
