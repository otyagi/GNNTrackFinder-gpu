/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Application.h
/// \brief  The application class for the run_info service
/// \since  24.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace cbm::services::run_info
{
  /// \enum  Information type required
  enum class EInfo
  {
    GeoTag,  //< Returns geometry setup tag
    //StsTag,
    //MvdTag,
    //......
    //CollisionSystem,
    //CollisionEnergyCm,
    //......
    Print,  //< Prints run information into stdout
    END
  };

  /// \class Application
  /// \brief Main class for the run_info service
  class Application {
   public:
    /// \brief Constructor from parameters
    Application() = default;

    /// \brief Copy constructor
    Application(const Application&) = default;

    /// \brief Move constructor
    Application(Application&&) = default;

    /// \brief Copy assignment operator
    Application& operator=(const Application&) = default;

    /// \brief Move assignment operator
    Application& operator=(Application&&) = default;

    /// \brief Destructor
    ~Application() = default;

    /// \brief  Parse command line arguments
    /// \return Selected info
    /// \throw  std::logic_error, if the option list is invalid
    std::optional<EInfo> ParseOptions(int argc, char* argv[]);

    /// \brief  Gets and prints information to the stdout
    /// \param  info  Requested information
    void Print(EInfo info) const;

    /// \brief  Prints all info
    std::string GetRunInfo() const;

   private:
    uint32_t fRunId{0};  ///< Run identifier
  };
}  // namespace cbm::services::run_info
