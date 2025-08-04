/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmQaOnlineInterface.h
/// \date   28.02.2024
/// \brief  Set of tools for online->ROOT QA-objects conversions (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

// TODO: move to more suitable place (histoserv, for example)

#pragma once

#include "Histogram.h"
#include "Logger.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TProfile.h"
#include "TProfile2D.h"

//#include <log.hpp>
#include <type_traits>

namespace
{
  using cbm::algo::qa::H1D;
  using cbm::algo::qa::H2D;
  using cbm::algo::qa::Prof1D;
  using cbm::algo::qa::Prof2D;
}  // namespace

namespace cbm::qa
{
  /// \class RootHistogramAccessor
  /// \brief Helper class to access protected fields of TH1, TH2, TProfile and TProfile2D
  ///
  /// The ROOT TProfile classes do not alow to set a profile from outside, so we need to set all the
  /// fields directly. Also the access to TH1/TH2 protected fields is needed in order to set the
  /// total sums of w*x, w*x*x, w*y, w*x*y, and w*y*y.
  template<class RootHistogram>
  class RootHistogramAccessor : public RootHistogram {

   public:
    template<class... Args>
    RootHistogramAccessor(Args... args) : RootHistogram(args...)
    {
    }

    /// \brief Sets slice from the lower-dimension histogram
    /// \tparam Source histogram type
    /// \param src  Source histogram
    /// \param val  Target value on the additional axis
    template<class SourceQaHistogram>
    void AddSliceFromQaHistogram(const SourceQaHistogram& histo, double val);

    /// \brief Sets fields from qa histogram
    /// \param histo  Source histogram
    template<class QaHistogram>
    void SetFromQaHistogram(const QaHistogram& histo);
  };

  /// \class  OnlineInterface
  /// \brief  A collection of tools for online QA object conversions
  class OnlineInterface {
   public:
    // TODO: generalize
    /// \brief Fills a slice of a histogram of a higher dimension for a given value (....)
    /// \param src   1D source histogram
    /// \param value Slice coordinate
    /// \param dst   Destination 2D-histogram
    static void AddSlice(const H1D& src, double value, TH2D* dst);

    /// \brief Fills a slice of a profile of a higher dimension for a given value (....)
    /// \param src   1D source profile
    /// \param value Slice coordinate
    /// \param dst   Destination 2D-profile
    static void AddSlice(const Prof1D& src, double value, TProfile2D* dst);

    /// \brief Converts histogram H1D to ROOT histogram TH1D
    /// \param hist  1D-histogram
    /// \note  Allocates memory on heap for the ROOT histogram
    static TH1D* ROOTHistogram(const H1D& hist);

    /// \brief Converts histogram H2D to ROOT histogram TH2D
    /// \param hist  1D-histogram
    /// \note  Allocates memory on heap for the ROOT histogram
    static TH2D* ROOTHistogram(const H2D& hist);

    /// \brief Converts profile Prof1D to ROOT TProfile
    /// \param prof  1D-profile
    /// \note  Allocates memory on heap for the ROOT histogram
    static TProfile* ROOTHistogram(const Prof1D& prof);

    /// \brief Converts profile Prof2D to ROOT TProfile2D
    /// \param prof  2D-profile
    /// \note  Allocates memory on heap for the ROOT histogram
    static TProfile2D* ROOTHistogram(const Prof2D& prof);
  };
}  // namespace cbm::qa


namespace cbm::qa
{
  // -------------------------------------------------------------------------------------------------------------------
  //
  template<class RootHistogram>
  template<class QaHistogram>
  void RootHistogramAccessor<RootHistogram>::AddSliceFromQaHistogram(const QaHistogram& src, double val)
  {
    constexpr bool IsDstH2D    = std::is_same_v<RootHistogram, TH2D>;
    constexpr bool IsDstProf2D = std::is_same_v<RootHistogram, TProfile2D>;
    constexpr bool IsSrcH1D    = std::is_same_v<QaHistogram, H1D>;
    constexpr bool IsSrcProf1D = std::is_same_v<QaHistogram, Prof1D>;

    static_assert((IsDstH2D && IsSrcH1D) || (IsDstProf2D && IsSrcProf1D),
                  "function not implemented for a given (RootHistogram, QaHistogram) pair");

    auto iBinX = this->GetXaxis()->FindBin(val);
    for (int iBinY = 0; iBinY <= this->GetNbinsY() + 1; ++iBinY) {
      int iGlobBin = this->GetBin(iBinX, iBinY);
      auto binSrc  = src.GetBinAccumulator(iBinY);
      if constexpr (IsSrcH1D) {
        this->fArray[iGlobBin] += binSrc.value();
        if (this->fSumw2.fN) {
          this->fSumw2.fArray[iGlobBin] += binSrc.variance();
        }
      }
      else if constexpr (IsSrcProf1D) {
        this->fArray[iGlobBin] += binSrc.GetSumWV();
        this->fBinEntries.fArray[iGlobBin] += binSrc.GetSumW();
        this->fSumw2.fArray[iGlobBin] += binSrc.GetSumWV2();
        this->fBinSumw2.fArray[iGlobBin] += binSrc.GetSumW2();
        this->fTsumwz += binSrc.GetSumWV();
        this->fTsumwz2 += binSrc.GetSumWV2();
      }
    }
    this->fTsumw += src.GetTotSumW();
    this->fTsumw2 += src.GetTotSumW2();
    this->fTsumwx += src.GetTotSumWX();
    this->fTsumwx2 += src.GetTotSumWX2();
    this->SetEntries(this->GetEntries() + src.GetEntries());
  }

  // -------------------------------------------------------------------------------------------------------------------
  //
  template<class RootHistogram>
  template<class QaHistogram>
  void RootHistogramAccessor<RootHistogram>::SetFromQaHistogram(const QaHistogram& hist)
  {
    // Set individual bin properties
    if constexpr (std::is_same_v<RootHistogram, TProfile> || std::is_same_v<RootHistogram, TProfile2D>) {
      this->Sumw2();
      auto SetBin = [&](int iBinRoot, const auto& binQa) {
        this->fArray[iBinRoot]             = binQa.GetSumWV();
        this->fBinEntries.fArray[iBinRoot] = binQa.GetSumW();
        this->fSumw2.fArray[iBinRoot]      = binQa.GetSumWV2();
        this->fBinSumw2.fArray[iBinRoot]   = binQa.GetSumW2();
      };
      if constexpr (std::is_same_v<RootHistogram, TProfile>) {
        for (int iBinX = 0; iBinX <= this->GetNbinsX() + 1; ++iBinX) {
          int iBin = this->GetBin(iBinX);
          auto bin = hist.GetBinAccumulator(iBinX);
          SetBin(iBin, bin);
          this->fTsumwy += bin.GetSumWV();
          this->fTsumwy2 += bin.GetSumWV2();
        }
      }
      else if constexpr (std::is_same_v<RootHistogram, TProfile2D>) {
        for (int iBinX = 0; iBinX <= this->GetNbinsX() + 1; ++iBinX) {
          for (int iBinY = 0; iBinY <= this->GetNbinsY() + 1; ++iBinY) {
            int iBin = this->GetBin(iBinX, iBinY);
            auto bin = hist.GetBinAccumulator(iBinX, iBinY);
            SetBin(iBin, bin);
            this->fTsumwz += bin.GetSumWV();
            this->fTsumwz2 += bin.GetSumWV2();
          }
        }
      }
    }
    else if constexpr (std::is_same_v<RootHistogram, TH1D> || std::is_same_v<RootHistogram, TH2D>) {
      if (hist.GetTotSumW() != hist.GetTotSumW2()) {  // some of weights were not equal to 1.
        if (!this->fSumw2.fN) {
          this->Sumw2();
        }
      }
      auto SetBin = [&](int iBinRoot, const auto& binQa) {
        this->fArray[iBinRoot] = binQa.value();
        if (this->fSumw2.fN) {
          this->fSumw2.fArray[iBinRoot] = binQa.variance();
        }
      };
      if constexpr (std::is_same_v<RootHistogram, TH1D>) {
        for (int iBinX = 0; iBinX <= this->GetNbinsX() + 1; ++iBinX) {
          SetBin(this->GetBin(iBinX), hist.GetBinAccumulator(iBinX));
        }
      }
      else if constexpr (std::is_same_v<RootHistogram, TH2D>) {
        for (int iBinX = 0; iBinX <= this->GetNbinsX() + 1; ++iBinX) {
          for (int iBinY = 0; iBinY <= this->GetNbinsY() + 1; ++iBinY) {
            SetBin(this->GetBin(iBinX, iBinY), hist.GetBinAccumulator(iBinX, iBinY));
          }
        }
      }
    }

    // Set total sums
    if constexpr (std::is_base_of_v<TH2D, RootHistogram>) {
      this->fTsumwy  = hist.GetTotSumWY();
      this->fTsumwy2 = hist.GetTotSumWY2();
      this->fTsumwxy = hist.GetTotSumWXY();
    }
    this->fTsumw   = hist.GetTotSumW();
    this->fTsumw2  = hist.GetTotSumW2();
    this->fTsumwx  = hist.GetTotSumWX();
    this->fTsumwx2 = hist.GetTotSumWX2();
    this->SetEntries(hist.GetEntries());
  }
}  // namespace cbm::qa
