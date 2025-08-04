/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file Application.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 09.01.2024
 **/


#ifndef CBMSIM_DIGITIZATION_APP_APPLICATION_H
#define CBMSIM_DIGITIZATION_APP_APPLICATION 1

#include "ProgramOptions.h"
#include "Run.h"

namespace cbm::sim::digitization
{

  class Application {


   public:
    /** @brief Standard constructor, initialises the application
     ** @param opt  **/
    explicit Application(ProgramOptions const& opt);

    /** @brief Copy constructor forbidden **/
    Application(const Application&) = delete;

    /** @brief Assignment operator forbidden **/
    void operator=(const Application&) = delete;

    /** @brief Destructor **/
    ~Application() = default;

    /** @brief Run the application **/
    void Exec();

   private:
    const std::string& OutputFile() const;
    const std::vector<std::string>& TraFile() const;
    const std::string& ParFile() const;
    const std::string& SetupTag() const;
    const std::string& ConfigFile() const;


   private:
    ProgramOptions const& fOpt;  ///< Program options object
    Run fRun = {};               ///< Reconstruction run
  };

}  // namespace cbm::sim::digitization

#endif /* CBMSIM_DIGITIZATION_APP_APPLICATION */
