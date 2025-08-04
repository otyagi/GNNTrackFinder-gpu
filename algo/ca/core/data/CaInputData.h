/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaInputData.h
/// \brief  Structure for input data to the L1 tracking algorithm (declaration)
/// \since  08.08.2022
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaHit.h"
#include "CaVector.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/array.hpp>

namespace cbm::algo::ca
{
  /// Class InputData represents a block of the input data to the L1 tracking algorithm per event or time slice.
  /// Filling of the InputData is carried out with L1IODataManager class
  ///
  class alignas(constants::misc::Alignment) InputData {
   public:
    friend class DataManager;  ///< Class which fills the InputData object for each event or time slice
    friend class boost::serialization::access;

    /// \brief Default constructor
    InputData();

    /// \brief Destructor
    ~InputData() = default;

    /// \brief Copy constructor
    InputData(const InputData& other);

    /// \brief Move constructor
    InputData(InputData&&) noexcept;

    /// \brief Copy assignment operator
    InputData& operator=(const InputData& other);

    /// \brief Move assignment operator
    InputData& operator=(InputData&& other) noexcept;

    /// \brief Gets hits sample size
    HitIndex_t GetSampleSize() const { return fvHits.size(); }

    /// \brief Gets number of data streams
    int GetNdataStreams() const { return fvStreamStartIndices.size(); }

    /// \brief  Gets reference to hit by its index
    /// \param  index  Index of hit in the hits sample
    const Hit& GetHit(HitIndex_t index) const { return fvHits[index]; }

    /// \brief Gets reference to hits vector
    const Vector<Hit>& GetHits() const { return fvHits; }

    /// \brief Gets number of hits in the hits vector
    HitIndex_t GetNhits() const { return fvHits.size(); }

    /// \brief Gets total number of stored keys
    int GetNhitKeys() const { return fNhitKeys; }

    /// \brief Gets index of the first hit in the sorted hits vector
    /// \param iStream  Index of the data stream
    HitIndex_t GetStreamStartIndex(int iStream) const { return fvStreamStartIndices[iStream]; }

    /// \brief Gets index of (the last + 1) hit in the sorted hits vector
    /// \param iStream  Index of the data stream
    HitIndex_t GetStreamStopIndex(int iStream) const { return fvStreamStopIndices[iStream]; }

    /// \brief Gets n hits for the data stream
    /// \param iStream  Index of the data stream
    HitIndex_t GetStreamNhits(int iStream) const
    {
      return fvStreamStopIndices[iStream] - fvStreamStartIndices[iStream];
    }


   private:
    /// \brief Swap method
    void Swap(InputData& other) noexcept;

    /// \brief Data serialization method
    template<class Archive>
    void serialize(Archive& ar, const unsigned int /*versino*/)
    {
      ar& fvHits;
      ar& fvStreamStartIndices;
      ar& fvStreamStopIndices;
      ar& fNhitKeys;
    }

    // ***************************
    // ** Member variables list **
    // ***************************

    /// \brief Sample of input hits
    Vector<Hit> fvHits{"InputData::fHits"};

    /// \brief Index of the first hit in the sorted hits vector for a given data stream
    Vector<HitIndex_t> fvStreamStartIndices{"InputData::fStreamStartIndices"};
    Vector<HitIndex_t> fvStreamStopIndices{"InputData::fStreamStopIndices"};

    /// \brief Number of hit keys used for rejecting fake STS hits
    int fNhitKeys = -1;
  };


  // ********************************************
  // ** Inline member functions initialization **
  // *********************************************

  // -------------------------------------------------------------------------------------------------------------------
  //
  [[gnu::always_inline]] inline void InputData::Swap(InputData& other) noexcept
  {
    std::swap(fvHits, other.fvHits);
    std::swap(fvStreamStartIndices, other.fvStreamStartIndices);
    std::swap(fvStreamStopIndices, other.fvStreamStopIndices);
    std::swap(fNhitKeys, other.fNhitKeys);
  }

}  // namespace cbm::algo::ca
