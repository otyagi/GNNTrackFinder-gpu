/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Jan de Cuveland */

/** @file ProgramOptions.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 09.06.2023
 **
 ** Code taken from reco/app/cbmreco_fairrun/ProgramOptions.h (J. de Cuveland)
 **/

#ifndef CBM_RECO_OFFLINE_APP_PROGRAMOPTIONS_H
#define CBM_RECO_OFFLINE_APP_PROGRAMOPTIONS_H 1

#define DEFAULT_CONFIG "reco/offline/config/RecoConfig_event_ideal.yaml"
#define DEFAULT_SETUP "sis100_electron"


#include <string>

namespace cbm::reco::offline
{

  /** @class ProgramOptions
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 9 June 2023
   **
   ** Program option class for the application cbmreco_offline
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

    /** @brief Get digitization (raw) file name **/
    [[nodiscard]] const std::string& RawFile() const { return fRaw; }

    /** @brief Get output file name (.root format) **/
    [[nodiscard]] const std::string& OutputFile() const { return fOutput; }

    /** @brief Get configuration file name (YAML format) **/
    [[nodiscard]] const std::string& ConfigFile() const { return fConfig; }

    /** @brief Get overwrite option **/
    [[nodiscard]] bool Overwrite() const { return fOverwrite; }

    /** @brief Get parameter file name **/
    [[nodiscard]] const std::string& ParFile() const { return fPar; }

    /** @brief Get geometry setup tag **/
    [[nodiscard]] const std::string& SetupTag() const { return fSetup; }


   private:
    /** @brief Parse command line arguments using boost program_options **/
    void ParseOptions(int argc, char* argv[]);


   private:                       // members
    std::string fRaw    = "";     ///< Input raw (digi) file name (ROOT format)
    std::string fOutput = "";     ///< Output file name (ROOT format)
    std::string fPar    = "";     ///< Parameter file name (ROOT format)
    std::string fConfig = "";     ///< Configuration file name (YAML format)
    std::string fSetup  = "";     ///< Geometry setup tag
    bool fOverwrite     = false;  ///< Enable overwriting of existing output file
  };

}  // namespace cbm::reco::offline

#endif /* CBM_RECO_OFFLINE_APP_PROGRAMOPTIONS_H */
