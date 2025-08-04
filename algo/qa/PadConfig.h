/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   PadConfig.h
/// \date   12.02.2024
/// \brief  A class representing a pad config in the message for the Histogram server
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once

#include "Histogram.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cbm::algo::qa
{
  /// \class PadConfig
  /// \brief A pad configuration for the histogram server
  ///
  /// The class represents a configuration of the pad, which can be converted in the part of the
  /// canvas initialization message for the histogram server.
  class PadConfig {
   public:
    /// \brief Constructor
    PadConfig() = default;

    /// \brief Constructor from parameters
    /// \param gridX
    /// \param gridY
    /// \param logX
    /// \param logY
    /// \param logZ
    PadConfig(bool gridX, bool gridY, bool logX, bool logY, bool logZ)
      : fbGridX(gridX)
      , fbGridY(gridY)
      , fbLogX(logX)
      , fbLogY(logY)
      , fbLogZ(logZ)
    {
    }

    /// \brief  Constructor from a single histogram
    /// \tparam Hist  Histogram class
    /// \param  hist  Histogram object
    /// \param  opt   Draw options for the histogram
    template<class Hist>
    PadConfig(const Hist* hist, std::string_view opt)
    {
      RegisterHistogram(hist, opt);
    }

    /// \brief Copy constructor
    PadConfig(const PadConfig&) = default;

    /// \brief Move constructor
    PadConfig(PadConfig&&) = default;

    /// \brief Copy assignment operator
    PadConfig& operator=(const PadConfig&) = default;

    /// \brief Move assignment operator
    PadConfig& operator=(PadConfig&&) = default;

    /// \brief Destructor
    ~PadConfig() = default;

    /// \brief  Set grid flags
    /// \param  gridX  Flag for x-axis
    /// \param  gridY  Flag for y-axis
    void SetGrid(bool gridX, bool gridY = false);

    /// \brief  Sets logarithm axis
    /// \param  logX  Logarithm flag for x-axis
    /// \param  logY  Logarithm flag for y-axis
    /// \param  logZ  Logarithm flag for z-axis
    void SetLog(bool logX, bool logY = false, bool logZ = false);

    /// \brief  Registers an object in the pad
    /// \param  name  Name of the object
    /// \param  opt   Draw options for the object
    void RegisterObject(std::string_view name, std::string_view opt)
    {
      fvObjectList.emplace_back(std::make_pair(name, opt));
    }

    /// \brief  Registers a histogram in the pad
    /// \tparam Hist  Histogram class
    /// \param  hist  Histogram object
    /// \param  opt   Draw options for the histogram
    ///
    /// If the histogram has EHistFlag::StoreVsTsId, then the TS dependendent histogram
    /// will be plotted only. If both integrated and vs. TS histograms are needed, please
    /// explicitly provide the histogram names to two separate pads using the RegisterObject
    /// function.
    template<class Hist>
    void RegisterHistogram(const Hist* hist, std::string_view opt)
    {
      // NOTE: SZh 21.06.2024:
      const auto& metadata = hist->GetMetadata();
      if (metadata.GetFlag(EHistFlag::StoreVsTsId)) {
        RegisterObject(hist->GetName() + std::string(HistogramMetadata::ksTsIdSuffix), opt);
      }
      else if (!metadata.GetFlag(EHistFlag::OmitIntegrated)) {
        RegisterObject(hist->GetName(), opt);
      }
    }

    /// \brief  Returns message config
    std::string ToString() const;

   private:
    bool fbGridX{false};  ///< Grid flag for x-axis
    bool fbGridY{false};  ///< Grid flag for y-axis
    bool fbLogX{false};   ///< Log flag for x-axis
    bool fbLogY{false};   ///< Log flag for y-axis
    bool fbLogZ{false};   ///< Log flag for z-axis

    std::vector<std::pair<std::string, std::string>> fvObjectList;  ///< List of objects on the pad
  };


}  // namespace cbm::algo
