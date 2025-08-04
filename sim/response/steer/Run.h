/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file Run.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 09.01.2024
 **/

#ifndef CBMSIM_DIGITIZATION_STEER_RUN_H
#define CBMSIM_DIGITIZATION_STEER_RUN_H 1

#include "Config.h"

#include <TNamed.h>
#include <TString.h>

#include <string>
#include <vector>

class CbmSetup;
class FairTask;
class CbmDigitization;
class TTree;

namespace cbm::sim::digitization
{

  class Run : public TNamed {

   public:
    /** @brief Constructor **/
    Run();


    /** @brief Destructor  **/
    virtual ~Run();


    /** @brief Allow overwriting if output file already exists **/
    void AllowOverwrite() { fOverwrite = true; }


    /** @brief Run digitization **/
    void Exec();


    /** @brief Settings object **/
    const Config& GetConfig() const { return fConfig; }


    /** @brief Get file name without ending
     ** @param file  full file name including ending **/
    std::string GetFileName(const TString file);


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


    /** @brief Set transport file name
     ** @param files  Transport input sources
     **/
    void SetTraFiles(const std::vector<std::string> files);


    /** @brief Set parameter file name
     ** @param fileName  Parameter file name
     **/
    void SetParFile(const char* fileName) { fPar = fileName; }


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


   private:
    CbmDigitization fRun{};
    TString fOutput = "";
    std::vector<TString> fTra;
    TString fPar      = "";
    TString fSetupTag = "";
    CbmSetup* fSetup  = nullptr;
    bool fOverwrite   = false;

   public:
    Config fConfig = {};


    ClassDef(cbm::sim::digitization::Run, 1);
  };

}  // namespace cbm::sim::digitization

#endif /* CBMSIM_DIGITIZATION_STEER_RUN_H */
