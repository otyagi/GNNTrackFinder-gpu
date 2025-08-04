/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef ALGO_QA_HISTO1D_H
#define ALGO_QA_HISTO1D_H 1

#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

#include <cstdint>
#include <string>
#include <vector>


namespace cbm::algo
{

  /** @class Histo1D
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 15 June 2023
   **
   ** Lightweight one-dimensional histogram class
   **/
  class Histo1D {
   public:
    /** @brief Standard constructor
     ** @param numBins   Number of bins
     ** @param minValue  Lower edge of histogram range
     ** @param maxValue  Upper edge of histogram range
     ** @param name      Histogram name
     ** @param title     ROOT convention: Histogram title; Axis X title; Axis Y title
     **/
    Histo1D(uint32_t numBins, double minValue, double maxValue, const std::string& name = "",
            const std::string& title = "");


    /** @brief Copy constructor: needed for boost serialization of vector of Histo1D **/
    Histo1D(const Histo1D&) = default;


    /** @brief Destructor **/
    virtual ~Histo1D() = default;


    /** @brief Add an entry to the histogram
     ** @param value  Value to be histogrammed
     ** @param weight Weight for the entry
     **/
    void Add(double value, double weight = 1.);


    /** @brief Clear histogram contents **/
    void Clear();


    /** @brief Histogram content in a bin
     ** @param bin Bin index
     ** @return Histogram content. Zero if bin index is out of scope.
     **/
    double Content(uint32_t bin) const;


    /** @brief Number of entries **/
    double NumEntries() const { return fNumEntries; }


    /** @brief Upper edge **/
    double MaxValue() const { return fMaxValue; }


    /** @brief First moment of distribution **/
    double Mean() const;


    /** @brief Lower edge **/
    double MinValue() const { return fMinValue; }


    /** @brief Histogram name **/
    const std::string& Name() const { return fName; }


    /** @brief Histogram name **/
    const std::string& Title() const { return fTitle; }


    /** @brief Number of bins **/
    uint32_t NumBins() const { return fNumBins; }

    /** @brief Add another histogram to an existing one
     ** @param other Histogram object to be added
     **/
    Histo1D& operator+=(const Histo1D& other);


    /** @brief Overflow **/
    double Overflow() const { return fOverflow; }


    /** @brief Underflow **/
    double Underflow() const { return fUnderflow; }


    /** @brief Second moment of distribution **/
    double Stddev() const;


    /** @brief Properties to string **/
    std::string ToString() const;


   private:
    /** Properties (no need for const if no accessors, avoid really complicated boost serialization) **/
    std::string fName;
    std::string fTitle;
    uint32_t fNumBins;
    double fMinValue;
    double fMaxValue;

    /** Content **/
    std::vector<double> fContent = {};
    double fUnderflow            = 0;
    double fOverflow             = 0;
    size_t fNumEntries           = 0;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*version*/)
    {
      ar& fName;
      ar& fTitle;
      ar& fNumBins;
      ar& fMinValue;
      ar& fMaxValue;
      ar& fContent;
      ar& fUnderflow;
      ar& fOverflow;
      ar& fNumEntries;
    }

    /** @brief Default constructor: needed for boost serialization of vector of Histo1D, need copy ctor call after! **/
    Histo1D() : fName(""), fTitle(""), fNumBins(0), fMinValue(0), fMaxValue(0){};
  };


  /** @brief Adding two histograms
   ** @param h1,h2 Histograms to be added
   ** @return Sum histogram
   **/
  inline Histo1D operator+(Histo1D h1, const Histo1D& h2)
  {
    h1 += h2;
    return h1;
  }

} /* namespace cbm::algo */

BOOST_CLASS_VERSION(cbm::algo::Histo1D, 1)

#endif /* ALGO_QA_HISTO1D_H */
