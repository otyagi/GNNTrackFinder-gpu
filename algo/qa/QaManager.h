/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   QaManager.h
/// \date   09.02.2025
/// \brief  QA manager for the online data reconstruction
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "base/HistogramSender.h"
#include "base/SubChain.h"
#include "qa/QaData.h"

namespace cbm::algo::qa
{
  /// \class Manager
  /// \brief A central class to manage the histogram storage and sending to the histogram server
  class Manager : public SubChain {
   public:
    /// \brief Constructor
    /// \param histoSender  A histogram sender instance
    Manager(std::shared_ptr<HistogramSender> histoSender = nullptr);

    /// \brief Copy constructor
    Manager(const Manager&) = delete;

    /// \brief Move constructor
    Manager(Manager&&) = delete;

    /// \brief Destructor
    ~Manager() = default;

    /// \brief Copy assignment operator
    Manager& operator=(const Manager&) = delete;

    /// \brief Move assignment operator
    Manager& operator=(Manager&&) = delete;

    /// \brief Gets an instance of QA data
    std::shared_ptr<Data> GetData() const { return fpData; }

    /// \brief Initializes the instance and sends the histogram and canvas configuration to the server
    void Init();

    /// \brief Sends a collection of histograms to the server
    /// \note  Resets the histograms after sending them
    void SendHistograms();

    /// \brief Sets a timeslice index
    /// \param timesliceId  A timeslice index
    void SetTimesliceId(uint64_t timesliceId) { fpData->SetTimesliceId(timesliceId); }

   private:
    std::shared_ptr<HistogramSender> fpSender{nullptr};      ///< Histogram sender
    std::shared_ptr<Data> fpData{std::make_shared<Data>()};  ///< Instance of QA Data
  };
}  // namespace cbm::algo::qa
