/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

#ifndef CBM_SERVICES_HISTSERV_APP_PROGRAMOPTIONS_H
#define CBM_SERVICES_HISTSERV_APP_PROGRAMOPTIONS_H 1

#include <cstdint>
#include <string>

namespace cbm::services::histserv
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

    /** @brief Get receive High-Water Mark for interface channel **/
    [[nodiscard]] const int32_t& ComChanZmqRcvHwm() const { return fiHistosInZmqHwm; }

    /** @brief Get receive timeout for interface channel **/
    [[nodiscard]] const int32_t& ComChanZmqRcvTo() const { return fiHistosInZmqRcvToMs; }

    /** @brief Get histo server http port **/
    [[nodiscard]] const uint32_t& HttpPort() const { return fuHttpServerPort; }

    /** @brief Get output file name (.root format) **/
    [[nodiscard]] const std::string& HistoFile() const { return fsHistoFileName; }

    /** @brief Get overwrite option **/
    [[nodiscard]] bool Overwrite() const { return fOverwrite; }

    /** @brief Get overwrite option **/
    [[nodiscard]] bool HideGuiCommands() const { return fHideGuiCommands; }

    /** @brief Get compressed input option **/
    [[nodiscard]] bool CompressedInput() const { return fCompressedInput; }

    // /** @brief Get configuration file name (YAML format) **/
    // [[nodiscard]] const std::string& ConfigFile() const { return fConfig; }

   private:
    /** @brief Parse command line arguments using boost program_options **/
    void ParseOptions(int argc, char* argv[]);


   private:  // members
    std::string fsChanHistosIn   = "tcp://127.0.0.1:56800";
    int32_t fiHistosInZmqHwm     = 1;                   ///< High-Water Mark, default keep only 1 update in buffer
    int32_t fiHistosInZmqRcvToMs = 10;                  ///< Timeout in ms: -1 = block, 0 = instant, val = nb ms
    uint32_t fuHttpServerPort    = 8080;                ///< HTTP port of the ROOT web server
    std::string fsHistoFileName  = "histos_dump.root";  ///< Output file name (ROOT format)
    bool fOverwrite              = false;               ///< Enable overwriting of existing output file
    bool fHideGuiCommands        = false;               ///< Hides (disables) the GUI commands for Reset/Save/Close
    bool fCompressedInput        = false;               ///< Enables ZSTD stream decompression is available
    //std::string fConfig        = "";         ///< Configuration file name (YAML format)
  };

}  // namespace cbm::services::histserv

#endif /* CBM_SERVICES_HISTSERV_APP_PROGRAMOPTIONS_H */
