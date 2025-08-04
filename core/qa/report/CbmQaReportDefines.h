/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportDefines.h
/// \brief  Common definitions for cbm::qa::report
/// \author S. Zharko <s.zharko@gsi.de>
/// \since  25.02.2024

#pragma once

//#include <filesystem>
#include <boost/filesystem.hpp>

namespace cbm::qa::report
{
  // Filesystem
  //namespace fs = std::filesystem;
  namespace fs = boost::filesystem;
}  // namespace cbm::qa::report
