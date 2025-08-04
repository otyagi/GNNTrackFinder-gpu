/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#include "Histo1D.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


namespace cbm::algo
{

  // -----   Standard constructor
  Histo1D::Histo1D(uint32_t numBins, double minValue, double maxValue, const std::string& name,
                   const std::string& title)
    : fName(name)
    , fTitle(title)
    , fNumBins(numBins)
    , fMinValue(minValue)
    , fMaxValue(maxValue)
  {
    if (numBins == 0 || maxValue <= minValue)
      throw std::runtime_error("Histo1D: Invalid specifications " + std::to_string(numBins) + " "
                               + std::to_string(minValue) + " " + std::to_string(maxValue));
    fContent.resize(numBins);
  }

  // -----   Add entry
  void Histo1D::Add(double value, double weight)
  {
    if (value < fMinValue)
      fUnderflow += weight;
    else if (!(value < fMaxValue))
      fOverflow += weight;
    else {
      uint32_t bin = uint32_t(double(fNumBins) * (value - fMinValue) / (fMaxValue - fMinValue));
      assert(bin < fNumBins);
      fContent[bin] += weight;
      fNumEntries++;
    }
  }

  // -----   Clear content
  void Histo1D::Clear()
  {
    fContent.assign(fNumBins, 0.);
    fUnderflow  = 0;
    fOverflow   = 0;
    fNumEntries = 0;
  }

  // -----   Content access
  double Histo1D::Content(uint32_t bin) const
  {
    if (fNumBins < bin)
      return 0.;
    else
      return fContent[bin];
  }


  // -----   First moment of distribution
  double Histo1D::Mean() const
  {
    double sum1    = 0.;
    double sum2    = 0.;
    double binsize = (fMaxValue - fMinValue) / double(fNumBins);
    for (uint32_t bin = 0; bin < fNumBins; bin++) {
      double x = fMinValue + (bin + 0.5) * binsize;
      sum1 += x * fContent[bin];
      sum2 += fContent[bin];
    }
    return sum1 / sum2;
  }


  // -----   Operator +=
  // TODO: Comparison of floating point numbers; probably fishy.
  Histo1D& Histo1D::operator+=(const Histo1D& other)
  {
    if (other.fNumBins == fNumBins && other.fMinValue == fMinValue && other.fMaxValue == fMaxValue) {
      fUnderflow += other.fUnderflow;
      fOverflow += other.fOverflow;
      for (uint32_t bin = 0; bin < fNumBins; bin++)
        fContent[bin] += other.fContent[bin];
      fNumEntries += other.fNumEntries;
    }
    else
      throw std::runtime_error("Histo1D: Trying to add incompatible histograms");
    return *this;
  }


  // -----   Second moment of distribution
  double Histo1D::Stddev() const
  {
    double sum1    = 0.;
    double sum2    = 0.;
    double binsize = (fMaxValue - fMinValue) / double(fNumBins);
    for (uint32_t bin = 0; bin < fNumBins; bin++) {
      double x = fMinValue + (bin + 0.5) * binsize;
      sum1 += x * x * fContent[bin];
      sum2 += fContent[bin];
    }
    double mean = Mean();
    return sqrt(sum1 / sum2 - mean * mean);
  }


  // -----   Properties to string
  std::string Histo1D::ToString() const
  {
    std::stringstream ss;
    ss << fName << ": bins " << fNumBins << " range " << fMinValue << " to " << fMaxValue << " entries " << fNumEntries
       << " mean " << Mean() << " stddev " << Stddev() << " out of range " << fUnderflow << " , " << fOverflow
       << " title <" << fTitle << ">";
    return ss.str();
  }

} /* namespace cbm::algo */
