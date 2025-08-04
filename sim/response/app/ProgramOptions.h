/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese, Jan de Cuveland */

/** @file ProgramOptions.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 09.01.2024
 **
 ** Code taken from reco/app/cbmreco_fairrun/ProgramOptions.h (J. de Cuveland)
 **/

#ifndef CBMSIM_DIGITIZATION_APP_PROGRAMOPTIONS_H
#define CBMSIM_DIGITIZATION_APP_PROGRAMOPTIONS_H 1

#define DEFAULT_CONFIG "sim/response/config/DigiConfig_event_ideal.yaml"
#define DEFAULT_SETUP "sis100_electron"


#include <string>
#include <vector>

namespace cbm::sim::digitization
{

  /** @class ProgramOptions
   ** @author Frederic Linz <f.linz@gsi.de>
   ** @since 09 January 2024
   **
   ** Program option class for the application cbmatc
   **/
  class ProgramOptions {
   public:
    /** @brief Standard constructor with command line arguments **/
    ProgramOptions(int argc, char* argv[]) { ParseOptions(argc, argv); }

    /** @brief Copy constructor forbidden **/
    ProgramOptions(const ProgramOptions&) = delete;

    /** @brief Assignment operator forbidden **/
    ProgramOptions& operator=(const ProgramOptions&) = delete;

    /** @brief Destructor **/
    ~ProgramOptions() = default;

    /** @brief Get output file name (.root format) **/
    [[nodiscard]] const std::string& OutputFile() const { return fOutput; }

    /** @brief Get transport file name **/
    [[nodiscard]] const std::vector<std::string>& TraFiles() const { return fTra; }

    /** @brief Get parameter file name **/
    [[nodiscard]] const std::string& ParFile() const { return fPar; }

    /** @brief Get configuration file name (YAML format) **/
    [[nodiscard]] const std::string& ConfigFile() const { return fConfig; }

    /** @brief Get geometry setup tag **/
    [[nodiscard]] const std::string& SetupTag() const { return fSetup; }

    /** @brief Get overwrite option **/
    [[nodiscard]] bool Overwrite() const { return fOverwrite; }


   private:
    /** @brief Parse command line arguments using boost program_options **/
    void ParseOptions(int argc, char* argv[]);


   private:                         // members
    std::string fOutput = "";       ///< Output file name (ROOT format)
    std::vector<std::string> fTra;  ///< List of transport input sources (ROOT format)
    std::string fPar    = "";       ///< Parameter file name (ROOT format)
    std::string fConfig = "";       ///< Configuration file name (YAML format)
    std::string fSetup  = "";       ///< Geometry setup tag
    bool fOverwrite     = false;    ///< Enable overwriting of existing output file
  };

}  // namespace cbm::sim::digitization

#endif /* CBMSIM_DIGITIZATION_APP_PROGRAMOPTIONS_H */
