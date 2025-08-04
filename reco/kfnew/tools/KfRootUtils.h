/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   KfRootUtils.h
/// \brief  Different ROOT utility functions for the KF-framework (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \date   01.11.2024

#pragma once

namespace cbm::algo::kf
{
  class MaterialMap;
}

class TH2F;
class TString;

namespace kf::tools
{
  /// \struct  KfRootUtils
  /// \brief   A structure to keep all the utilities together
  struct RootUtils {
    /// \brief  Converts a material budget map into a TH2D histogram
    /// \param  material  A material map
    /// \param  name      Name for the histogram
    /// \param  title     Title for the histogram (note: axis title are set automatically)
    static TH2F* ToHistogram(const cbm::algo::kf::MaterialMap& material, const TString& name, const TString& title);

    // TODO: similar methods for fields etc. go here as well
    // static TH2D* ToHistogram(const FieldSlice<T>& fieldSlice);
  };
};  // namespace kf::tools
