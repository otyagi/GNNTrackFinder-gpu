/* Copyright (C) 2023-2024 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer], Sergei Zharko */

#ifndef CBM_SERVICES_HISTSERV_TESTER_APPLICATION_H
#define CBM_SERVICES_HISTSERV_TESTER_APPLICATION_H 1

#include "HistogramSender.h"
#include "ProgramOptions.h"

#include <memory>

namespace cbm::services::histserv_tester
{

  class Application {

  public:
    /** @brief Standard constructor, initialises the application
     ** @param opt  **/
    explicit Application(ProgramOptions const& opt);

    /** @brief Copy constructor forbidden **/
    Application(const Application&) = delete;

    /** @brief Move constructor forbidden **/
    Application(Application&&) = delete;

    /** @brief Copy assignment operator forbidden **/
    Application& operator=(const Application&) = delete;

    /** @brief Move assignment operator forbidden **/
    Application& operator=(Application&&) = delete;

    /** @brief Destructor **/
    ~Application() = default;

    /** @brief Run the application **/
    void Exec();

  private:
    ProgramOptions const& fOpt;  ///< Program options object

    /// Interface
    std::shared_ptr<cbm::algo::HistogramSender> fpSender = nullptr;
  };

}  // namespace cbm::services::histserv_tester

#endif /* CBM_SERVICES_HISTSERV_TESTER_APPLICATION_H */
