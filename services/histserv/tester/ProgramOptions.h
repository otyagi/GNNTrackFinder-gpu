/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#ifndef CBM_SERVICES_HISTSERV_TESTER_PROGRAMOPTIONS_H
#define CBM_SERVICES_HISTSERV_TESTER_PROGRAMOPTIONS_H 1

#include <string>

namespace cbm::services::histserv_tester
{

  /** @class ProgramOptions
   ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
   ** @since 26 June 2023
   **
   ** Program option class for the application histserv_nofairmq
   **/
  class ProgramOptions {
  public:
    /** @brief Standard constructor with command line arguments **/
    ProgramOptions(int argc, char* argv[]) { ParseOptions(argc, argv); }

    /** @brief Copy constructor forbidden **/
    ProgramOptions(const ProgramOptions&) = default;

    /** @brief Assignment operator forbidden **/
    ProgramOptions& operator=(const ProgramOptions&) = default;

    /** @brief Destructor **/
    ~ProgramOptions() = default;

    /** @brief Get interface channel name or hostname + port or whatever or ????? (FIXME: replacement of FairMQ) **/
    [[nodiscard]] const std::string& ComChan() const { return fsChanHistosIn; }

    /** @brief Get run duration **/
    [[nodiscard]] const int64_t& Runtime() const { return fRunTime; }

    /** @brief Get histos publication interval **/
    [[nodiscard]] const int64_t& PubInterval() const { return fPubInterval; }

    // /** @brief Get configuration file name (YAML format) **/
    // [[nodiscard]] const std::string& ConfigFile() const { return fConfig; }

  private:
    /** @brief Parse command line arguments using boost program_options **/
    void ParseOptions(int argc, char* argv[]);


  private:  // members
    std::string fsChanHistosIn = "histogram-in";
    int64_t fRunTime           = 90;
    int64_t fPubInterval       = 5;
  };

}  // namespace cbm::services::histserv_tester

#endif /* CBM_SERVICES_HISTSERV_TESTER_PROGRAMOPTIONS_H */
