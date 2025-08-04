/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaOnlineInterface.cxx
/// \date   28.02.2024
/// \brief  Set of tools for online->ROOT QA-objects conversions (source)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "CbmQaOnlineInterface.h"
//#include "Histogram.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TProfile.h"
#include "TProfile2D.h"

using cbm::algo::qa::H1D;
using cbm::algo::qa::H2D;
using cbm::algo::qa::Prof1D;
using cbm::algo::qa::Prof2D;
using cbm::qa::OnlineInterface;
using cbm::qa::RootHistogramAccessor;

// ---------------------------------------------------------------------------------------------------------------------
//
void OnlineInterface::AddSlice(const H1D& src, double value, TH2D* dst)
{
  auto* pDst = static_cast<RootHistogramAccessor<TH2D>*>(dst);
  pDst->AddSliceFromQaHistogram<H1D>(src, value);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void OnlineInterface::AddSlice(const Prof1D& src, double value, TProfile2D* dst)
{
  auto* pDst = static_cast<RootHistogramAccessor<TProfile2D>*>(dst);
  pDst->AddSliceFromQaHistogram<Prof1D>(src, value);
}

// ---------------------------------------------------------------------------------------------------------------------
//
TH1D* OnlineInterface::ROOTHistogram(const H1D& hist)
{
  int nBins   = hist.GetNbinsX();
  double xMin = hist.GetMinX();
  double xMax = hist.GetMaxX();
  bool add    = TH1::AddDirectoryStatus();
  TH1::AddDirectory(false);
  auto* pRes = new RootHistogramAccessor<TH1D>(hist.GetName().c_str(), hist.GetTitle().c_str(), nBins, xMin, xMax);
  TH1::AddDirectory(add);
  pRes->SetFromQaHistogram<H1D>(hist);
  return pRes;
}

// ---------------------------------------------------------------------------------------------------------------------
//
TH2D* OnlineInterface::ROOTHistogram(const H2D& hist)
{
  int nBinsX  = hist.GetNbinsX();
  double xMin = hist.GetMinX();
  double xMax = hist.GetMaxX();
  int nBinsY  = hist.GetNbinsY();
  double yMin = hist.GetMinY();
  double yMax = hist.GetMaxY();
  bool add    = TH1::AddDirectoryStatus();
  TH1::AddDirectory(false);
  auto* pRes = new RootHistogramAccessor<TH2D>(hist.GetName().c_str(), hist.GetTitle().c_str(), nBinsX, xMin, xMax,
                                               nBinsY, yMin, yMax);
  TH1::AddDirectory(add);
  pRes->SetFromQaHistogram<H2D>(hist);
  return pRes;
}

// ---------------------------------------------------------------------------------------------------------------------
//
TProfile* OnlineInterface::ROOTHistogram(const Prof1D& prof)
{
  const char* name = prof.GetName().c_str();
  const char* titl = prof.GetTitle().c_str();
  int nBinsX       = prof.GetNbinsX();
  double xMin      = prof.GetMinX();
  double xMax      = prof.GetMaxX();
  double yMin      = prof.GetMinY();
  double yMax      = prof.GetMaxY();
  bool add         = TH1::AddDirectoryStatus();
  TH1::AddDirectory(false);
  auto* pRes = new RootHistogramAccessor<TProfile>(name, titl, nBinsX, xMin, xMax, yMin, yMax);
  TH1::AddDirectory(add);
  pRes->SetFromQaHistogram<Prof1D>(prof);
  return pRes;
}

// ---------------------------------------------------------------------------------------------------------------------
//
TProfile2D* OnlineInterface::ROOTHistogram(const Prof2D& prof)
{
  const char* name = prof.GetName().c_str();
  const char* titl = prof.GetTitle().c_str();
  int nBinsX       = prof.GetNbinsX();
  double xMin      = prof.GetMinX();
  double xMax      = prof.GetMaxX();
  int nBinsY       = prof.GetNbinsY();
  double yMin      = prof.GetMinY();
  double yMax      = prof.GetMaxY();
  double zMin      = prof.GetMinZ();
  double zMax      = prof.GetMaxZ();
  bool add         = TH1::AddDirectoryStatus();
  TH1::AddDirectory(false);
  auto* pRes = new RootHistogramAccessor<TProfile2D>(name, titl, nBinsX, xMin, xMax, nBinsY, yMin, yMax, zMin, zMax);
  TH1::AddDirectory(add);
  pRes->SetFromQaHistogram<Prof2D>(prof);
  return pRes;
}
