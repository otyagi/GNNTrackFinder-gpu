/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Data.cxx
/// \date   12.02.2024
/// \brief  A unified data-structure to handle QA objects for the online reconstruction
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "AlgoFairloggerCompat.h"
#include "base/HistogramSender.h"
#include "qa/CanvasConfig.h"
#include "qa/HistogramContainer.h"
#include "qa/TaskProperties.h"

#include <boost/serialization/forward_list.hpp>

#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

namespace cbm::algo::qa
{
  /// \class Data
  /// \brief Class to handle QA-objects in the online reconstruction
  class Data {
   public:
    /// \brief Default constructor
    Data() = default;

    /// \brief Constructor
    /// \param name  Name of the QA module (appears as the directory name in the output)
    Data(std::string_view name) { RegisterNewTask(name); }

    /// \brief Copy constructor
    Data(const Data&) = default;

    /// \brief Move constructor
    Data(Data&&) = default;

    /// \brief Copy assignment operator
    Data& operator=(const Data&) = default;

    /// \brief Move assignment operator
    Data& operator=(Data&&) = default;

    /// \brief Destructor
    ~Data() = default;

    /// \brief Adds a canvas to the canvas config list
    /// \param canvas  A CanvasConfig object
    void AddCanvasConfig(const CanvasConfig& canvas) { fvsCanvCfgs.push_back(canvas.ToString()); }

    /// \brief Sends QA initialization information to the HistogramSender
    /// \param histoSender  A pointer to the histogram sender
    void Init(std::shared_ptr<HistogramSender> histoSender);

    /// \brief  Creates a QA-object and returns the pointer to it
    /// \tparam Obj      A type of the histogram (H1D, H2D, Prof1D, Prof2D)
    /// \tparam Args...  A signature of the histogram constructor
    /// \param  args     Parameters, passed to a histogram constructor
    template<class Obj, typename... Args>
    Obj* MakeObj(Args... args);

    /// \brief Resets the histograms
    void Reset() { fHistograms.Reset(); }

    /// \brief Sends QA data to the HistogramSender
    /// \param histoSender  A pointer to the histogram sender
    /// \note  Calls this->Reset() after sending the message to the histogram server
    void Send(std::shared_ptr<HistogramSender> histoSender);

    /// \brief Updates the timeslice index
    /// \param timesliceId  Timeslice index
    void SetTimesliceId(uint64_t timesliceId) { fHistograms.fTimesliceId = timesliceId; }

    /// \brief Registers a new QA task
    /// \param name  Name of the task
    void RegisterNewTask(std::string_view name);

   private:
    qa::HistogramContainer fHistograms;                ///< A container of histograms, which forms a zmq message
    std::string fsTaskNames;                           ///< A string containing names of tasks
    std::vector<qa::TaskProperties> fvTaskProperties;  ///< A vector to store properties for multiple QA-tasks
    std::vector<std::string> fvsCanvCfgs = {};         ///< Vector of canvas configs

    uint32_t fNofH1{0};     ///< Number of 1D-histograms
    uint32_t fNofH2{0};     ///< Number of 2D-histograms
    uint32_t fNofP1{0};     ///< Number of 1D-profiles
    uint32_t fNofP2{0};     ///< Number of 2D-profiles
    bool fbNotEmpty{true};  ///< false: if no histograms were provided, do not perform initialization and sending
  };

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<class Obj, typename... Args>
  Obj* Data::MakeObj(Args... args)
  {
    if constexpr (std::is_same_v<Obj, cbm::algo::qa::H1D>) {
      Obj* res = &(fHistograms.fvH1.emplace_front(args...));
      ++fNofH1;
      fvTaskProperties.back().fRangeH1.first = fHistograms.fvH1.begin();
      return res;
    }
    else if constexpr (std::is_same_v<Obj, cbm::algo::qa::H2D>) {
      Obj* res = &(fHistograms.fvH2.emplace_front(args...));
      ++fNofH2;
      fvTaskProperties.back().fRangeH2.first = fHistograms.fvH2.begin();
      return res;
    }
    else if constexpr (std::is_same_v<Obj, cbm::algo::qa::Prof1D>) {
      Obj* res = &(fHistograms.fvP1.emplace_front(args...));
      ++fNofP1;
      fvTaskProperties.back().fRangeP1.first = fHistograms.fvP1.begin();
      return res;
    }
    else if constexpr (std::is_same_v<Obj, cbm::algo::qa::Prof2D>) {
      Obj* res = &(fHistograms.fvP2.emplace_front(args...));
      ++fNofP2;
      fvTaskProperties.back().fRangeP2.first = fHistograms.fvP2.begin();
      return res;
    }
    return nullptr;
  }
}  // namespace cbm::algo
