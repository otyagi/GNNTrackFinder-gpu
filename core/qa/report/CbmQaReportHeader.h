/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportHeader.h
/// \brief  Base class for the report header (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#pragma once

#include "CbmQaReportEngine.h"

#include <string>
#include <string_view>

namespace cbm::qa::report
{
  /// \class  Header
  /// \brief  Header of the report
  class Header {
   public:
    /// \brief Destructor
    virtual ~Header() = default;

    /// \brief Add tag
    void AddTag(std::string_view tag) { fvsTags.emplace_back(tag); }

    /// \brief Gets body of the element
    /// \param engine  A concrete implementation of the Engine to get the element body
    std::string GetBody(const Engine& engine) const { return engine.HeaderBody(*this); }

    /// \brief Gets author
    const std::string& GetAuthor() const { return fsAuthor; }

    /// \brief Gets page header
    const std::string& GetPageHeader() const { return fsPageHeader; }

    /// \brief Gets setup
    const std::string& GetSetup() const { return fsSetup; }

    /// \brief Gets subtitle
    const std::string& GetSubtitle() const { return fsSubtitle; }

    /// \brief Gets tags
    const std::vector<std::string>& GetTags() const { return fvsTags; }

    /// \brief Gets title
    const std::string& GetTitle() const { return fsTitle; }

    /// \brief Sets author
    void SetAuthor(std::string_view author) { fsAuthor = author; }

    /// \brief Sets page header
    void SetPageHeader(std::string_view pageHeader) { fsPageHeader = pageHeader; }

    /// \brief Sets setup
    void SetSetup(std::string_view setup) { fsSetup = setup; }

    /// \brief Sets subtitle
    void SetSubtitle(std::string_view subtitle) { fsSubtitle = subtitle; }

    /// \brief Sets title
    void SetTitle(std::string_view title) { fsTitle = title; }

   private:
    std::vector<std::string> fvsTags{};  ///< Different Tags

    std::string fsAuthor     = "";
    std::string fsSetup      = "";
    std::string fsSubtitle   = "";
    std::string fsTitle      = "";
    std::string fsPageHeader = "";  ///< Placed on top/bottom of the page
  };
}  // namespace cbm::qa::report
