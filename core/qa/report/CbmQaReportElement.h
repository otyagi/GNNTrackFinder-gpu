/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportElement.h
/// \brief  Base class for the report element (header)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  21.02.2024

#pragma once

#include "CbmQaReportDefines.h"

#include <memory>
#include <string>
#include <vector>

namespace cbm::qa::report
{
  class Engine;

  /// \class Element
  /// \brief Interface for the report element
  ///
  class Element {
    friend class CollapsibleElement;

   public:
    /// \brief Constructor
    /// \param label  Element label
    /// \param title  Element title
    Element(std::string_view label, std::string_view title) : fsLabel(label), fsTitle(title) {}

    /// \brief Default constructor
    Element() = default;

    /// \brief Destructor
    virtual ~Element() = default;

    /// \brief Gets body of the element
    /// \param engine  A concrete implementation of the Engine to get the element body
    virtual std::string GetBody(const Engine& engine) const = 0;

    /// \brief Gets label
    const std::string& GetLabel() const { return fsLabel; }

    /// \brief Gets name
    const std::string& GetTitle() const { return fsTitle; }

    /// \brief Sets label
    void SetLabel(std::string_view label) { fsLabel = label; }

    /// \brief Sets name
    void SetTitle(std::string_view title) { fsTitle = title; }

    /// \brief Gets mother element
    const Element* GetMother() const { return fpMother; }

   private:
    void AssignMother(const Element* pMother) { fpMother = pMother; }

    // TODO: Add check for multiple label definition
    const Element* fpMother = nullptr;  ///< Mother element
    std::string fsLabel     = "";       ///< Label for referencing
    std::string fsTitle     = "";       ///< Title of the element
  };

  /// \class CollapsibleElement
  /// \brief Interface to the element, which can contain daughter elements
  ///
  class CollapsibleElement : public Element {
   public:
    using Element::Element;

    /// \brief Destructor
    virtual ~CollapsibleElement() = default;

    /// \brief Adds element
    // TODO: Check for label
    virtual void Add(std::shared_ptr<Element> pElement)
    {
      pElement->AssignMother(this);
      fvDaughterElements.push_back(pElement);
    }

    /// \brief Get daughter elements
    const std::vector<std::shared_ptr<Element>> GetDaughterElements() const { return fvDaughterElements; }

   private:
    std::vector<std::shared_ptr<Element>> fvDaughterElements;  ///< Daughter elements
  };
}  // namespace cbm::qa::report
