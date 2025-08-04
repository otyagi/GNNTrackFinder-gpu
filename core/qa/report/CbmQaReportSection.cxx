/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportSection.h
/// \brief  Base class for the report section (header)
/// \author S. Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#include "CbmQaReportSection.h"

using cbm::qa::report::CollapsibleElement;
using cbm::qa::report::Element;
using cbm::qa::report::Section;

// ---------------------------------------------------------------------------------------------------------------------
//
Section::Section(std::string_view label, std::string_view title)
  : CollapsibleElement(std::string("section") + std::string(label), title)
{
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Section::Add(std::shared_ptr<Element> pElement)
{
  // Add level to the section
  if (auto* pSubSection = dynamic_cast<Section*>(pElement.get())) {
    pSubSection->SetLevel(this->fLevel + 1);
  }
  this->CollapsibleElement::Add(pElement);
}
