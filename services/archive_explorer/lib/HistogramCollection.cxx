/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "HistogramCollection.h"

#include <TH1.h>

#include <fmt/format.h>

using namespace cbm::explore;

HistogramCollection::~HistogramCollection() {}

void HistogramCollection::Reset()
{
  for (auto& histo : fHistos) {
    histo.histo->Reset();
  }
}

TH1* HistogramCollection::GetHisto(const std::string& name) const
{
  for (auto& histo : fHistos) {
    if (histo.histo->GetName() == name) { return histo.histo; }
  }
  return nullptr;
}

std::string HistogramCollection::GetHistoPath(const std::string& name) const
{
  for (auto& histo : fHistos) {
    if (histo.histo->GetName() == name) { return histo.path; }
  }
  return "";
}

void HistogramCollection::Div(const HistogramCollection& other)
{
  for (auto& histo : fHistos) {
    auto otherHisto = other.GetHisto(histo.histo->GetName());
    if (not otherHisto) {
      throw std::runtime_error(
        fmt::format("Cannot divide histograms, because they do not match: No histogram with name "
                    "{} found in other collection.",
                    histo.histo->GetName()));
    }
    histo.histo->Divide(otherHisto);
  }
}

void HistogramCollection::CreateFolder(const char* path, const char* name)
{
  fFolders.push_back({.path = path, .name = name});
}
