/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaParametersHandler.h
/// \brief  Handles an instance of the CA-parameters as a shared pointer (header)
/// \since  24.10.2024
/// \author Sergei Zharko <s.zharko@gsi.de>
/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaParametersHandler.h
/// \brief  Handles an instance of the CA-parameters as a shared pointer (header)
/// \since  24.10.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CaParameters.h"
#include "CbmL1DetectorID.h"

#include <memory>


namespace cbm::ca
{
  /// \brief Handles an shared pointer of CA parameters
  class ParametersHandler {
    using ParametersPtr_t = std::shared_ptr<cbm::algo::ca::Parameters<float>>;

   public:
    /// \brief Instance access
    static ParametersHandler* Instance();

    /// \brief Returns an shared pointer to the parameters instance
    /// \param filename  A name of the file with the parameters
    /// \note  If the parameters instance was read from different file, an exception will be thrown
    /// \throw std::logic_err  If there is an attempt to read parameters from two different sources
    const ParametersPtr_t Get(const std::string& filename);

    // Disable copy and move
    ParametersHandler(const ParametersHandler&) = delete;
    ParametersHandler(ParametersHandler&&)      = delete;
    ParametersHandler& operator=(const ParametersHandler&) = delete;
    ParametersHandler& operator=(ParametersHandler&&) = delete;

   private:
    /// \brief Default constructor
    ParametersHandler() = default;

    /// \brief Destructor
    ~ParametersHandler() = default;

    inline static ParametersHandler* fpInstance{nullptr};
    inline static std::mutex fMutex{};
    std::string fsInputName{""};            ///< Name of the input ca.par file
    ParametersPtr_t fpParameters{nullptr};  ///< ca::Parameters instance (double precision)
  };
}  // namespace cbm::ca
