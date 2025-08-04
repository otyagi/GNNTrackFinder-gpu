/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportElementFactory.h
/// \brief  An abstract factory for the report elements
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#pragma once

#include <string>
#include <string_view>

namespace cbm::qa::report
{
  class Figure;
  class Header;
  class Section;
  class Table;
  class Tail;

  /// \class Engine
  /// \brief A base abstract class to provide an interface for element body (a visitor in the Visitor pattern)
  ///
  /// Each method of the class provides interface to create the body of the document element. The body
  /// is returned as a string.
  class Engine {
   public:
    /// \brief  Destructor
    virtual ~Engine() = default;

    /// \brief  Creates a body for figure
    /// \param  figure  Reference to figure
    /// \return Figure body
    virtual std::string FigureBody(const Figure& figure) const = 0;

    /// \brief  Creates a body for header
    /// \param  header  Reference to header
    /// \return Header body
    virtual std::string HeaderBody(const Header& header) const = 0;

    /// \brief  Creates a body for section
    /// \param  section  Reference to section
    /// \return Section body
    virtual std::string SectionBody(const Section& section) const = 0;

    /// \brief  Creates a body for table
    /// \param  table  Reference to table
    /// \return Table body
    virtual std::string TableBody(const Table& table) const = 0;

    /// \brief  Creates a body for tail
    /// \param  tail  Reference to tail
    /// \return Figure body
    virtual std::string TailBody(const Tail& tail) const = 0;

    /// \brief  Returns script extention
    virtual std::string ScriptExtention() const = 0;

    /// \brief  Returns engine name
    virtual std::string MyName() const = 0;

    /// \brief Defines the compilation rule (can be omitted)
    /// \param source  Path to the source
    virtual void Compile(const std::string& /*source*/) const {};
  };
}  // namespace cbm::qa::report
