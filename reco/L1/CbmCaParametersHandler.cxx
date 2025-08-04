/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmCaParametersHandler.cxx
/// \brief  Handles an instance of the CA-parameters as a shared pointer (implementation)
/// \since  24.10.2024
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CbmCaParametersHandler.h"

#include "CaInitManager.h"
#include "CbmKfTrackingSetupBuilder.h"

#include <sstream>

namespace cbm::ca
{
  // -------------------------------------------------------------------------------------------------------------------
  //
  ParametersHandler* ParametersHandler::Instance()
  {
    if (fpInstance == nullptr) {
      std::lock_guard<std::mutex> lock(fMutex);
      fpInstance = new ParametersHandler{};
    }
    return fpInstance;
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  const ParametersHandler::ParametersPtr_t ParametersHandler::Get(const std::string& name)
  {
    if (fsInputName.empty()) {
      fsInputName = name;
    }
    else if (fsInputName != name) {
      // TODO: More accurate check on the files (the path can be different, but files can be the same)
      std::stringstream msg;
      msg << "ParametersHandler::Get: an attempt to re-define an instance of CA-parameters from "
          << "a different path: \"" << name << "\". The parameters were provided previously from \"" << fsInputName
          << "\"";
      throw std::logic_error(msg.str());
    }

    if (!fpParameters.get()) {
      using cbm::algo::kf::EFieldMode;
      using cbm::kf::TrackingSetupBuilder;
      // TODO: Probably, make a template from the floating-point data-type?
      //       Point: having setup of double, defined in the parameters.
      //       But, if the kf-setup will not be a part of the ca::Parameters, it would not be needed;
      ca::InitManager manager;
      manager.ReadParametersObject(fsInputName.c_str());
      manager.SetGeometrySetup(TrackingSetupBuilder::Instance()->MakeSetup<ca::fvec>(EFieldMode::Orig));
      fpParameters = std::make_shared<ca::Parameters<float>>(ca::Parameters<float>(manager.TakeParameters()));
    }

    return fpParameters;
  }
}  // namespace cbm::ca
