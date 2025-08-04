/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Histogram.h
/// \date   27.02.2024
/// \brief  ROOT-free implementation of a histogram
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "Accumulators.h"

#include <boost/histogram.hpp>
//#include <boost/histogram/algorithm/sum.hpp>
#include <boost/histogram/serialization.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <type_traits>

#include <fmt/format.h>

namespace cbm::algo::qa
{
  namespace bh = boost::histogram;

  using RegularAxis_t = bh::axis::regular<>;

  using Axes1D_t = std::tuple<RegularAxis_t>;
  using Axes2D_t = std::tuple<RegularAxis_t, RegularAxis_t>;

  using HistStorage_t = bh::weight_storage;
  using ProfStorage_t = bh::RootStyleProfileStorage<double>;

  /// \enum  EHistAxis
  /// \brief Enumeration for histogram axes
  enum class EAxis : unsigned
  {
    X = 0,
    Y,
    Z
  };

  /// \enum  EHistFlag
  /// \brief Histogram control flags (bit masks)
  enum class EHistFlag : uint8_t
  {
    StoreVsTsId    = 0b00000001,  ///< Store the histogram vs timeslice index
    OmitIntegrated = 0b00000010,  ///< Omits storing integrated histogram
    SetMinimum     = 0b00000100   ///< Sets minimum to the histogram
  };

  /// \class  HistogramMetadata
  /// \brief  Metadata of the histogram
  /// \note   To be sent with a configuration to the histogram server
  class HistogramMetadata {
   public:
    using Flags_t = std::underlying_type_t<EHistFlag>;

    static constexpr std::string_view ksTsIdSuffix = "_ts_id";  ///< Suffix of additional histograms vs. TS index

    /// \brief Default constructor
    HistogramMetadata() = default;

    /// \brief Constructor from the metadata string representation
    /// \param msg  Metadata string representation
    explicit HistogramMetadata(const std::string& msg)
    {
      if (!msg.empty()) {
        fFlags = std::stoi(msg, nullptr, 16);
      }
    }

    /// \brief Destructor
    ~HistogramMetadata() = default;

    /// \brief Checks if the histogram flags configuration is valid
    /// \return Is valid
    bool CheckFlags() const
    {
      // The histogram must be plotted either vs TS, or over all TS
      return (GetFlag(EHistFlag::StoreVsTsId) || !GetFlag(EHistFlag::OmitIntegrated));
    }

    /// \brief Get flag
    /// \param key  Flag key from the EHistFlag enumeration
    bool GetFlag(EHistFlag key) const { return static_cast<bool>(fFlags & static_cast<Flags_t>(key)); }

    /// \brief Get flag
    /// \param key   Flag key from the EHistFlag enumeration
    /// \param flag  Flag value
    void SetFlag(EHistFlag key, bool flag = true)
    {
      flag ? (fFlags |= static_cast<Flags_t>(key)) : (fFlags &= ~static_cast<Flags_t>(key));
    }

    /// \brief Converts the metadata to a string
    ///
    /// Current implementation of the metadata string: <flags>
    /// Future implementation with additional fields: <flags>;<...>;<...>;...;<...>
    std::string ToString() const { return fmt::format("{0:02x}", fFlags); }

    /// \brief  Separates a name and metadata of histogram
    /// \return std::pair<std::string, std::string>: (name, metadata)
    static std::pair<std::string, std::string> SeparateNameAndMetadata(const std::string& msg)
    {
      size_t pos = msg.find_last_of('!');
      if (pos != msg.npos) {
        return std::make_pair(msg.substr(0, pos), msg.substr(pos + 1));
      }
      else {
        return std::make_pair(msg, "");
      }
    }

   private:
    friend class boost::serialization::access;
    template<class Archive>
    /// \brief Serialization rule
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fFlags;
    }

    Flags_t fFlags{};  ///< Flags collection for the histogram
  };

  /// \class  TotalSums1D
  /// \brief  Storage for total sums of weights, squared weights, weights over x, weights over squared x
  class TotalSums1D {
   public:
    /// \brief Gets total sum of weights
    double GetTotSumW() const { return fTotSumW; }

    /// \brief Gets total sum of squared weights
    double GetTotSumW2() const { return fTotSumW2; }

    /// \brief Gets total sum of weight over x products
    double GetTotSumWX() const { return fTotSumWX; }

    /// \brief Gets total sum of weight over squared x products
    double GetTotSumWX2() const { return fTotSumWX2; }

   protected:
    /// \brief Updates the sums
    /// \param x  X value
    /// \param w  weight
    void UpdateTotalSums(double x, double w)
    {
      fTotSumW += w;
      fTotSumW2 += w * w;
      fTotSumWX += w * x;
      fTotSumWX2 += w * x * x;
    }

    /// \brief Resets the sums
    void Reset()
    {
      fTotSumW   = 0;
      fTotSumW2  = 0;
      fTotSumWX  = 0;
      fTotSumWX2 = 0;
    }

    double fTotSumW   = 0.;  ///< Total sum (over all bins) of weights
    double fTotSumW2  = 0.;  ///< Total sum (over all bins) of squared weights
    double fTotSumWX  = 0.;  ///< Total sum (over all bins) of weight over x products
    double fTotSumWX2 = 0.;  ///< Total sum (over all bins) of weight over square x products

   private:
    /// \brief Serialization rule
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fTotSumW;
      ar& fTotSumW2;
      ar& fTotSumWX;
      ar& fTotSumWX2;
    }
  };

  /// \class  TotalSums2D
  /// \brief  TotalSums1D including storage for total sums of w*x*y, w*y, w*y*y products
  class TotalSums2D : public TotalSums1D {
   public:
    /// \brief Gets total sum of weight over squared y products
    double GetTotSumWXY() const { return fTotSumWXY; }

    /// \brief Gets total sum of weight over y products
    double GetTotSumWY() const { return fTotSumWY; }

    /// \brief Gets total sum of weight over squared y products
    double GetTotSumWY2() const { return fTotSumWY2; }

   protected:
    /// \brief Resets the sums
    void Reset()
    {
      TotalSums1D::Reset();
      fTotSumWXY = 0;
      fTotSumWY  = 0;
      fTotSumWY2 = 0;
    }

    /// \brief Updates the sums
    /// \param x  X value
    /// \param y  Y value
    /// \param w  weight
    void UpdateTotalSums(double x, double y, double w)
    {
      TotalSums1D::UpdateTotalSums(x, w);
      fTotSumWXY += w * x * y;
      fTotSumWY += w * y;
      fTotSumWY2 += w * y * y;
    }

    double fTotSumWY  = 0.;  ///< Total sum (over all bins) of weight over y products
    double fTotSumWXY = 0.;  ///< Total sum (over all bins) of weight over square x products
    double fTotSumWY2 = 0.;  ///< Total sum (over all bins) of weight over x over y products

   private:
    /// \brief Serialization rule
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<TotalSums1D>(*this);
      ar& fTotSumWXY;
      ar& fTotSumWY;
      ar& fTotSumWY2;
    }
  };


  /// \class  Histogram
  /// \brief  Interface to a histogram/profile
  /// \tparam Axes      The axes type
  /// \tparam Storage   Storage type for the histogram
  /// \tparam TotalSums Class for storing the total sums of weights for axes (essential to calculate mean and std.dev.)
  template<class Axes, class Storage, class TotalSums>
  class Histogram : public TotalSums {
   protected:
    using Hist_t                   = bh::histogram<Axes, Storage>;  // TODO: Test in future
    static constexpr unsigned Rank = std::tuple_size_v<Axes>;

   public:
    /// \brief Destructor
    ~Histogram() = default;

    /// \brief  Gets range lower bound of the selected axis
    /// \tparam AxisType  An axis
    template<EAxis A, unsigned IA = static_cast<unsigned>(A), std::enable_if_t<(IA < Rank), bool> = true>
    double GetAxisMin() const
    {
      return fHistogram.template axis<IA>().value(0.);
    }

    /// \brief  Gets range upper bound of the selected axis
    /// \tparam AxisType  An axis
    template<EAxis A, unsigned IA = static_cast<unsigned>(A), std::enable_if_t<(IA < Rank), bool> = true>
    double GetAxisMax() const
    {
      return fHistogram.template axis<IA>().value(static_cast<double>(GetAxisNbins<A>()));
    }

    /// \brief  Gets number of bins in axis
    /// \tparam AxisT  An axis
    /// \tparam D
    template<EAxis A, unsigned IA = static_cast<unsigned>(A), std::enable_if_t<(IA < Rank), bool> = true>
    uint32_t GetAxisNbins() const
    {
      return fHistogram.template axis<IA>().size();
    }

    /// \brief Gets number of entries
    // TODO: Gives different results, if weights are not 1 (investigate!)
    double GetEntries() const
    {
      // return bh::algorithm::sum(fHistogram); // -> effective entries (but different to the ROOT ones: if w != 1)
      return fEntries;
    }

    /// \brief Get flag
    /// \param key  Flag key from the EHistFlag enumeration
    bool GetFlag(EHistFlag key) const { return fMetadata.GetFlag(key); }

    /// \brief Gets name
    const std::string& GetName() const { return fName; }

    /// \brief Gets metadata instance
    const HistogramMetadata& GetMetadata() const { return fMetadata; }

    /// \brief Gets metadata string
    std::string GetMetadataString() const { return fMetadata.ToString(); }

    /// \brief Gets number of bins for x axis
    uint32_t GetNbinsX() const { return GetAxisNbins<EAxis::X>(); }

    /// \brief Gets x-axis lower bound
    double GetMinX() const { return GetAxisMin<EAxis::X>(); }

    /// \brief Gets x-axis lower bound
    double GetMaxX() const { return GetAxisMax<EAxis::X>(); }

    /// \brief Gets number of bins for y axis
    template<unsigned RankCheck = Rank, std::enable_if_t<(RankCheck > 1), bool> = true>
    uint32_t GetNbinsY() const
    {
      return GetAxisNbins<EAxis::Y>();
    }

    /// \brief Gets y-axis lower bound
    template<unsigned RankCheck = Rank, std::enable_if_t<(RankCheck > 1), bool> = true>
    double GetMinY() const
    {
      return GetAxisMin<EAxis::Y>();
    }

    /// \brief Gets y-axis lower bound
    template<unsigned RankCheck = Rank, std::enable_if_t<(RankCheck > 1), bool> = true>
    double GetMaxY() const
    {
      return GetAxisMax<EAxis::Y>();
    }

    /// \brief Gets maximum value
    double GetMaximum() const
    {
      return *std::max_element(bh::indexed(fHistogram).begin(), bh::indexed(fHistogram).end());
    }

    /// \brief Gets minimum value
    double GetMinimum() const
    {
      return *std::min_element(bh::indexed(fHistogram).begin(), bh::indexed(fHistogram).end());
    }

    /// \brief Gets title
    const std::string& GetTitle() const { return fTitle; }

    /// \brief Resets the histogram
    void Reset()
    {
      TotalSums::Reset();
      fHistogram.reset();
      fEntries = 0;
    }

    /// \brief Get flag
    /// \param key   Flag key from the EHistFlag enumeration
    /// \param flag  Flag value
    void SetFlag(EHistFlag key, bool flag = true) { fMetadata.SetFlag(key, flag); }

    /// \brief Sets name
    /// \param name  Histogram name
    void SetName(const std::string& name) { fName = name; }

    /// \brief Sets title
    /// \param title  Histogram title
    /// \note  ROOT axes convention is applicable: "<main title>;<x-axis title>;<y-axis title>"
    void SetTitle(const std::string& title) { fTitle = title; }

    /// \brief String representation of the histogram/profile
    //std::string ToString() const
    //{
    //  std::stringstream msg;
    //  msg <<
    //}

   protected:
    /// \brief Default constructor
    Histogram() = default;

    /// \brief Copy constructor
    explicit Histogram(const Histogram<Axes, Storage, TotalSums>& h) = default;

    //explicit Histogram(const Histogram<Axes, Storage>& h)
    //  : fHistogram(h.fHistogram)
    //  , fName(h.fName)
    //  , fTitle(h.fTitle)
    //  , fEntries(h.fEntries)
    //{
    //}

    /// \brief Constructor
    /// \param bhist  Boost histogram object
    /// \param name   Name of the histogram
    /// \param title  Title of the histogram
    Histogram(const Hist_t& bhist, const std::string& name, const std::string title)
      : fHistogram(bhist)
      , fName(name)
      , fTitle(title)
      , fEntries(0)
      , fMetadata(HistogramMetadata())
    {
    }

    /// \brief Gets bin index in underlying boost histogram
    /// \param  iBin  Bin in histogram
    /// \return Bin in boost histogram
    inline static int GetBinBH(uint32_t iBin) { return (iBin > 0) ? iBin - 1 : -1; }

    Hist_t fHistogram;            ///< Underlying boost histogram
    std::string fName  = "";      ///< Name of the histogram
    std::string fTitle = "";      ///< Title of the histogram
    int fEntries       = 0;       ///< Number of histogram entries
    HistogramMetadata fMetadata;  ///< Meta-data for histogram


   private:
    friend class boost::serialization::access;
    template<class Archive>
    /// \brief Serialization rule
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<TotalSums>(*this);
      ar& fHistogram;
      ar& fName;
      ar& fTitle;
      ar& fEntries;
      ar& fMetadata;
    }
  };

  using BaseH1D    = Histogram<Axes1D_t, HistStorage_t, TotalSums1D>;
  using BaseH2D    = Histogram<Axes2D_t, HistStorage_t, TotalSums2D>;
  using BaseProf1D = Histogram<Axes1D_t, ProfStorage_t, TotalSums1D>;
  using BaseProf2D = Histogram<Axes2D_t, ProfStorage_t, TotalSums2D>;

  /// \class  cbm::algo::qa::H1D
  /// \brief  1D-histogram
  class H1D : public BaseH1D {
   public:
    /// \brief Constructor for 1D-histogram
    /// \param name  Name of histogram
    /// \param title Title of histogram
    /// \param nBins  Number of bins
    /// \param xMin   Lower x bound
    /// \param xMax   Upper x bound
    H1D(const std::string& name, const std::string& title, uint32_t nBins, double xMin, double xMax)
      : BaseH1D(bh::make_weighted_histogram(RegularAxis_t(nBins, xMin, xMax)), name, title)
    {
    }

    /// \brief Default constructor
    H1D() = default;

    /// \brief Copy constructor
    H1D(const H1D&) = default;

    /// \brief Move constructor
    H1D(H1D&&) = default;

    /// \brief Copy assignment operator
    H1D& operator=(const H1D&) = default;

    /// \brief Move assignment operator
    H1D& operator=(H1D&&) = default;

    /// \brief  Fills histogram
    /// \param  x Value
    /// \param  w Weight
    /// \return Bin number  TODO: implement
    int Fill(double x, double w = 1.)
    {
      auto cellIt = fHistogram(bh::weight(w), x);
      ++fEntries;
      int iBin = cellIt - fHistogram.begin();
      if (iBin == 0 || iBin > static_cast<int>(GetNbinsX())) {
        return -1;  // ROOT TH1::Fill behaviour
      }
      // NOTE: In ROOT TH1 the total sums are updated only for non-overflow bins
      BaseH1D::UpdateTotalSums(x, w);
      return iBin;
    }

    /// \brief Gets underlying bin accumulator
    /// \param iBin  Bin index
    auto GetBinAccumulator(uint32_t iBin) const { return fHistogram.at(Histogram::GetBinBH(iBin)); }

    /// \brief Gets bin content
    /// \param iBin  Bin index
    double GetBinContent(uint32_t iBin) const { return fHistogram.at(Histogram::GetBinBH(iBin)).value(); }

    /// \brief Gets bin error
    /// \param iBin  Bin index
    double GetBinError(uint32_t iBin) const { return std::sqrt(fHistogram.at(Histogram::GetBinBH(iBin)).variance()); }

   private:
    /// \brief Serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<BaseH1D>(*this);
    }
  };

  /// \class  cbm::algo::qa::H2D
  /// \brief  2D-histogram
  class H2D : public BaseH2D {
   public:
    /// \brief Constructor for 2D-histogram
    /// \param name  Name of histogram
    /// \param title Title of histogram
    /// \param nBinsX  Number of x bins
    /// \param xMin    Lower x bound
    /// \param xMax    Upper x bound
    /// \param nBinsY  Number of y bins
    /// \param yMin    Lower y bound
    /// \param yMax    Upper y bound
    H2D(const std::string& name, const std::string& title, uint32_t nBinsX, double xMin, double xMax, uint32_t nBinsY,
        double yMin, double yMax)
      : BaseH2D(bh::make_weighted_histogram(RegularAxis_t(nBinsX, xMin, xMax), RegularAxis_t(nBinsY, yMin, yMax)), name,
                title)
    {
    }

    /// \brief Default constructor
    H2D() = default;

    /// \brief Copy constructor
    H2D(const H2D&) = default;

    /// \brief Move constructor
    H2D(H2D&&) = default;

    /// \brief Copy assignment operator
    H2D& operator=(const H2D&) = default;

    /// \brief Move assignment operator
    H2D& operator=(H2D&&) = default;

    /// \brief  Fills histogram
    /// \param  x Value along x-axis
    /// \param  y Value along y-axis
    /// \param  w Weight
    /// \return Bin number  TODO: implement
    int Fill(double x, double y, double w = 1.)
    {
      auto cellIt = fHistogram(x, y, bh::weight(w));
      ++fEntries;

      int iBin       = cellIt - fHistogram.begin();
      uint32_t iBinX = iBin % (GetNbinsX() + 2);
      if (iBinX == 0 || iBinX > GetNbinsX()) {
        return -1;
      }
      uint32_t iBinY = iBin / (GetNbinsX() + 2);
      if (iBinY == 0 || iBinY > GetNbinsY()) {
        return -1;
      }
      // NOTE: In ROOT TH2 the total sums are updated only for non-overflow bins
      BaseH2D::UpdateTotalSums(x, y, w);
      return iBin;
    }

    /// \brief Gets underlying bin accumulator
    /// \param iBinX  Bin index along x-axis
    /// \param iBinY  Bin index along y-axis
    auto GetBinAccumulator(uint32_t iBinX, uint32_t iBinY) const
    {
      return fHistogram.at(Histogram::GetBinBH(iBinX), Histogram::GetBinBH(iBinY));
    }

    /// \brief Gets bin content
    /// \param iBinX  Bin index in x-axis
    /// \param iBinY  Bin index in y-axis
    double GetBinContent(uint32_t iBinX, uint32_t iBinY) const
    {
      return fHistogram.at(Histogram::GetBinBH(iBinX), Histogram::GetBinBH(iBinY)).value();
    }

    /// \brief Gets bin error
    /// \param iBinX  Bin index in x-axis
    /// \param iBinY  Bin index in y-axis
    double GetBinError(uint32_t iBinX, uint32_t iBinY) const
    {
      return std::sqrt(fHistogram.at(Histogram::GetBinBH(iBinX), Histogram::GetBinBH(iBinY)).variance());
    }

   private:
    /// \brief Serialization rule
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<BaseH2D>(*this);
    }
  };

  // TODO: Boost::histogram by default uses a different error calculation approach as ROOT TProfile. TODO: investigate

  class Prof1D : public BaseProf1D {
   public:
    /// \brief Constructor for 2D-histogram
    /// \param name  Name of histogram
    /// \param title Title of histogram
    /// \param nBinsX  Number of x bins
    /// \param xMin    Lower x bound
    /// \param xMax    Upper x bound
    /// \param yMin    Lower y bound (optional)
    /// \param yMax    Upper y bound (optional)
    ///
    /// \note If the yMin and yMax are defined (yMin != yMax), all the passed values outside the [yMin, yMax] range
    ///       will be ignored.
    Prof1D(const std::string& name, const std::string& title, uint32_t nBinsX, double xMin, double xMax,
           double yMin = 0., double yMax = 0.)
      : BaseProf1D(bh::MakeRootStyleProfile(RegularAxis_t(nBinsX, xMin, xMax)), name, title)
      , fYmin(yMin)
      , fYmax(yMax)
    {
    }

    /// \brief Default constructor
    Prof1D() = default;

    /// \brief Copy constructor
    Prof1D(const Prof1D&) = default;

    /// \brief Move constructor
    Prof1D(Prof1D&&) = default;

    /// \brief Copy assignment operator
    Prof1D& operator=(const Prof1D&) = default;

    /// \brief Move assignment operator
    Prof1D& operator=(Prof1D&&) = default;

    /// \brief  Fills histogram
    /// \param  x Value along x-axis
    /// \param  y Value along y-axis
    /// \param  w Weight
    /// \return Bin number  TODO: implement
    int Fill(double x, double y, double w = 1.)
    {
      /// Skip measurement, if it goes beyond the defined [fYmin, fYmax] range
      if ((fYmin != fYmax) && (y < fYmin || y > fYmax || std::isnan(y))) {
        return -1;
      }

      auto cellIt = fHistogram(x, bh::sample(y), bh::weight(w));
      ++fEntries;
      int iBin = cellIt - fHistogram.begin();
      if (iBin == 0 || iBin > static_cast<int>(GetNbinsX())) {
        return -1;  // ROOT TH1::Fill behaviour
      }
      // NOTE: In ROOT TProfile the total sums are updated only for non-overflow bins
      BaseProf1D::UpdateTotalSums(x, w);
      return iBin;
    }

    /// \brief Gets underlying bin accumulator
    /// \param iBin  Bin index along x-axis
    auto GetBinAccumulator(uint32_t iBin) const { return fHistogram.at(Histogram::GetBinBH(iBin)); }

    /// \brief Gets bin content
    /// \param iBin  Bin index
    double GetBinContent(uint32_t iBin) const { return fHistogram.at(Histogram::GetBinBH(iBin)).GetMean(); }

    /// \brief Gets bin entries
    /// \param iBin  Bin index
    double GetBinCount(uint32_t iBin) const { return fHistogram.at(Histogram::GetBinBH(iBin)).GetEffCount(); }

    /// \brief Gets bin error
    /// \param iBin  Bin index
    double GetBinError(uint32_t iBin) const { return fHistogram.at(Histogram::GetBinBH(iBin)).GetSEM(); }

    /// \brief Gets y-axis lower bound
    double GetMinY() const { return fYmin; }

    /// \brief Gets y-axis lower bound
    double GetMaxY() const { return fYmax; }

   private:
    /// \brief Serialization rule
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<BaseProf1D>(*this);
      ar& fYmin;
      ar& fYmax;
    }

    double fYmin = 0.;  ///< Lower bound of the profile y-axis
    double fYmax = 0.;  ///< Upper bound of the profile y-axis
  };

  class Prof2D : public BaseProf2D {
   public:
    /// \brief Constructor for 2D-histogram
    /// \param name  Name of histogram
    /// \param title Title of histogram
    /// \param nBinsX  Number of x bins
    /// \param xMin    Lower x bound
    /// \param xMax    Upper x bound
    /// \param nBinsY  Number of y bins
    /// \param yMin    Lower y bound
    /// \param yMax    Upper y bound
    /// \param zMin    Lower z bound (optional)
    /// \param zMax    Upper z bound (optional)
    ///
    /// \note If the zMin and zMax are defined (zMin != zMax), all the passed values outside the [zMin, zMax] range
    ///       will be ignored.
    Prof2D(const std::string& name, const std::string& title, uint32_t nBinsX, double xMin, double xMax,
           uint32_t nBinsY, double yMin, double yMax, double zMin = 0., double zMax = 0.)
      : BaseProf2D(bh::MakeRootStyleProfile(RegularAxis_t(nBinsX, xMin, xMax), RegularAxis_t(nBinsY, yMin, yMax)), name,
                   title)
      , fZmin(zMin)
      , fZmax(zMax)
    {
    }

    /// \brief Default constructor
    Prof2D() = default;

    /// \brief Copy constructor
    Prof2D(const Prof2D&) = default;

    /// \brief Move constructor
    Prof2D(Prof2D&&) = default;

    /// \brief Copy assignment operator
    Prof2D& operator=(const Prof2D&) = default;

    /// \brief Move assignment operator
    Prof2D& operator=(Prof2D&&) = default;

    /// \brief  Fills histogram
    /// \param  x Value along x-axis
    /// \param  y Value along y-axis
    /// \param  z Value along z-axis
    /// \param  w Weight
    /// \return Bin number  TODO: implement
    int Fill(double x, double y, double z, double w = 1.)
    {
      /// Skip measurement, if it goes beyond the defined [fYmin, fYmax] range
      if ((fZmin != fZmax) && (z < fZmin || z > fZmax || std::isnan(z))) {
        return -1;
      }

      auto cellIt = fHistogram(x, y, bh::sample(z), bh::weight(w));
      ++fEntries;

      int iBin       = cellIt - fHistogram.begin();
      uint32_t iBinX = iBin % (GetNbinsX() + 2);
      if (iBinX == 0 || iBinX > GetNbinsX()) {
        return -1;
      }
      uint32_t iBinY = iBin / (GetNbinsX() + 2);
      if (iBinY == 0 || iBinY > GetNbinsY()) {
        return -1;
      }
      // NOTE: In ROOT TProfile2D the total sums are updated only for non-overflow bins
      BaseProf2D::UpdateTotalSums(x, y, w);
      return iBin;
    }

    /// \brief Gets underlying bin accumulator
    /// \param iBinX  Bin index along x-axis
    /// \param iBinY  Bin index along y-axis
    auto GetBinAccumulator(uint32_t iBinX, uint32_t iBinY) const
    {
      return fHistogram.at(Histogram::GetBinBH(iBinX), Histogram::GetBinBH(iBinY));
    }

    /// \brief Gets bin content
    /// \param iBinX  Bin index along x-axis
    /// \param iBinY  Bin index along y-axis
    double GetBinContent(uint32_t iBinX, uint32_t iBinY) const
    {
      return fHistogram.at(Histogram::GetBinBH(iBinX), Histogram::GetBinBH(iBinY)).GetMean();
    }

    /// \brief Gets bin entries
    /// \param iBinX  Bin index along x-axis
    /// \param iBinY  Bin index along y-axis
    double GetBinCount(uint32_t iBinX, uint32_t iBinY) const
    {
      return fHistogram.at(Histogram::GetBinBH(iBinX), Histogram::GetBinBH(iBinY)).GetEffCount();
    }

    /// \brief Gets bin error
    /// \param iBinX  Bin index along x-axis
    /// \param iBinY  Bin index along y-axis
    double GetBinError(uint32_t iBinX, uint32_t iBinY) const
    {
      return fHistogram.at(Histogram::GetBinBH(iBinX), Histogram::GetBinBH(iBinY)).GetSEM();
    }

    /// \brief Gets z-axis lower bound
    double GetMinZ() const { return fZmin; }

    /// \brief Gets z-axis lower bound
    double GetMaxZ() const { return fZmax; }

   private:
    /// \brief Serialization rule
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& boost::serialization::base_object<BaseProf2D>(*this);
      ar& fZmin;
      ar& fZmax;
    }

    double fZmin = 0.;  ///< Lower bound of the profile z-axis
    double fZmax = 0.;  ///< Upper bound of the profile z-axis
  };

}  // namespace cbm::algo::qa
