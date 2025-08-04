/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportSection.h
/// \brief  Base class for the report section (header)
/// \author S. Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#pragma once

#include "CbmQaReportElement.h"
#include "CbmQaReportEngine.h"

#include <string>
#include <string_view>

namespace cbm::qa::report
{
  /// \class Section
  /// \brief Section of the report
  class Section : public CollapsibleElement {
   public:
    /// \brief Constructor
    /// \param label  Label of section
    /// \param title  Title
    Section(std::string_view label, std::string_view title);

    /// \brief Default constructor
    Section() = default;

    /// \brief Gets body of the element
    /// \param engine  A concrete implementation of the Engine to get the element body
    std::string GetBody(const Engine& engine) const override { return engine.SectionBody(*this); }

    /// \brief Destructor
    virtual ~Section() = default;

    /// \brief Adds daughter element
    void Add(std::shared_ptr<Element> pElement) override;

    /// \brief Gets level of the section
    int GetLevel() const { return fLevel; }

   protected:
    /// \brief Sets level of the section
    /// \param level  Level of the section (is it section, subsection etc.)
    /// \note  The level is assigned in the adding a new section
    void SetLevel(int level) { fLevel = level; }

   private:
    int fLevel          = 0;   ///< Level of the section
  };
}  // namespace cbm::qa::report
