/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   QaTaskHeader.h
/// \date   10.02.2025
/// \brief  A header for a particular QA task, must be inherited by a given QA task
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "qa/QaManager.h"

#include <memory>

namespace cbm::algo::qa
{
  /// \class TaskHeader
  /// \brief An interface to the qa::Manager
  /// \note  Must be inherited by a QA task
  class TaskHeader {
   public:
    /// \brief Constructor
    /// \param pManager a QA-manager
    /// \param name A name of the task (histograms directory)
    TaskHeader(const std::unique_ptr<Manager>& pManager, std::string_view name)
      : fsName(name)
      , fpData(pManager != nullptr ? pManager->GetData() : nullptr)
    {
      if (fpData != nullptr) {
        fpData->RegisterNewTask(name);
      }
    }

    /// \brief Copy constructor
    TaskHeader(const TaskHeader&) = delete;

    /// \brief Move constructor
    TaskHeader(TaskHeader&&) = delete;

    /// \brief Destructor
    ~TaskHeader() = default;

    /// \brief Copy assignment operator
    TaskHeader& operator=(const TaskHeader&) = delete;

    /// \brief Move assignment operator
    TaskHeader& operator=(TaskHeader&&) = delete;

    /// \brief Checks, if the task is active
    ///
    /// The task can be inactive, if a nullptr qa::Manager was passed to the constructor. If it is the case,
    /// the fpData instance is not defined, and no actions on the task should be performed
    bool IsActive() const { return fpData.get(); }

    /// \brief Gets name of the task
    const std::string& GetTaskName() { return fsName; }

   protected:
    /// \brief Adds a canvas configuration
    /// \param canvas  A CanvasConfig object
    void AddCanvasConfig(const CanvasConfig& canvas) { fpData->AddCanvasConfig(canvas); }

    /// \brief  Creates a QA-object and returns the pointer to it
    /// \tparam Obj      A type of the histogram (H1D, H2D, Prof1D, Prof2D)
    /// \tparam Args...  A signature of the histogram constructor
    /// \param  args     Parameters, passed to a histogram constructor
    template<class Obj, typename... Args>
    Obj* MakeObj(Args... args)
    {
      return fpData->MakeObj<Obj>(args...);
    }

   private:
    std::string fsName{};                   ///< Name of the task
    std::shared_ptr<Data> fpData{nullptr};  ///< An instance of the QA data (shared between different tasks)
  };
}  // namespace cbm::algo::qa
