/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   KfFramework.h
/// @brief  The Kalman-filter framework main class (header)
/// @since  28.03.2024
/// @author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "KfDefs.h"
#include "KfSetup.h"

namespace cbm::algo::kf
{
  /// \class Framework
  /// \brief Main class of the KfCore library
  /// \tparam T  Underlying floating point data-type
  template<typename T>
  class Framework {
   public:
    /// \brief Default constructor
    Framework() = default;

    /// \brief Copy constructor
    Framework(const Framework&) = delete;

    /// \brief Move constructor
    Framework(Framework&&) = delete;

    /// \brief Destructor
    ~Framework() = default;

    /// \brief Copy assignment operator
    Framework& operator=(const Framework&) = delete;

    /// \brief Move assignment operator
    Framework& operator=(Framework&&) = delete;

    /// \brief Setup access (mutable)
    kf::Setup<T>& Setup() { return fSetup; }

    /// \brief Setup access
    const kf::Setup<T>& Setup() const { return fSetup; }

   private:
    kf::Setup<T> fSetup;  ///< KF setup
  };
}  // namespace cbm::algo::kf
