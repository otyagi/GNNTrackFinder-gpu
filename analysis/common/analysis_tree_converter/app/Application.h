/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file Application.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 27.10.2023
 **/


#ifndef CBM_ATCONVERTER_APP_APPLICATION_H
#define CBM_ATCONVERTER_APP_APPLICATION 1

#include "ProgramOptions.h"
#include "Run.h"

namespace cbm::atconverter
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
    const std::vector<std::string>& TraFiles() const;
    const std::string& RawFile() const;
    const std::string& ParFile() const;
    const std::string& RecoFile() const;
    const std::string& SetupTag() const;
    const std::string& ConfigFile() const;


  private:
    ProgramOptions const& fOpt;  ///< Program options object
    Run fRun = {};               ///< Reconstruction run
  };

}  // namespace cbm::atconverter

#endif /* CBM_ATCONVERTER_APP_APPLICATION */
