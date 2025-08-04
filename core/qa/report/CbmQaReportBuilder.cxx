/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaReportBuilder.h
/// \brief  Base class for the report builder (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>
/// \since  23.02.2024

#include "CbmQaReportBuilder.h"

#include "CbmQaReportFigure.h"
#include "Logger.h"
#include "TCanvas.h"

#include <fstream>


using cbm::qa::report::Builder;
using cbm::qa::report::Engine;
using cbm::qa::report::Figure;
using cbm::qa::report::Header;
using cbm::qa::report::Tail;

namespace fs = cbm::qa::report::fs;

// ---------------------------------------------------------------------------------------------------------------------
//
Builder::Builder(std::string_view title, fs::path outPath)
  : fpHeader(std::make_shared<Header>())
  , fpTail(std::make_shared<Tail>())
{
  fScriptsPath = fs::weakly_canonical(outPath);
  fFiguresPath = fs::weakly_canonical(fScriptsPath / "figures");
  fs::create_directories(fScriptsPath);
  fs::create_directories(fFiguresPath);

  LOG(info) << "Initializing report builder for a document " << title << ". The scripts path is "
            << fScriptsPath.string();
  fpHeader->SetTitle(title);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Builder::CreateScript(Engine& engine, bool bCompile)
{
  std::string outName = (fScriptsPath / (fmt::format("{}.{}", ksSourceName, engine.ScriptExtention()))).string();
  LOG(info) << "\nCreating source script \"" << outName << "\" using " << engine.MyName();

  std::ofstream script(outName);

  script << fpHeader->GetBody(engine);
  for (const auto& pSection : fvpSections) {
    script << pSection->GetBody(engine);
  }
  script << fpTail->GetBody(engine);
  script.close();

  if (bCompile) {
    engine.Compile(outName);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::string Builder::SaveCanvas(const TCanvas* canv, const std::string& relPath) const
{
  if (!canv) {
    LOG(fatal) << "cbm::qa::report::Builder::SaveCanvas(): a nullptr canvas is passed to the function. The provided "
               << "relative path is \"" << relPath << "\"";
  }
  fs::path absPath = this->AbsFigurePath(relPath + "." + fsFigureExtention);
  fs::create_directories(absPath.parent_path());
  canv->SaveAs(absPath.string().c_str());
  return this->PathInSource(absPath);
}
