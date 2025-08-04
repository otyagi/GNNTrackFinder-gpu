/* Copyright (C) 2022-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Sergei Zharko [committer] */

/// \file   CaDataManager.h
/// \brief  Input-output data manager for L1 tracking algorithm
/// \since  05.08.2022
/// \author S.Zharko <s.zharko@gsi.de>

#pragma once  // include this header only once per compilation unit

#include "CaDefs.h"
#include "CaInputData.h"

namespace cbm::algo::ca
{
  /// \class cbm::algo::ca::DataManager
  /// \brief A manager for the input-output data of the CA tracking algorithm
  ///
  class alignas(constants::misc::Alignment) DataManager {
   public:
    /// \brief Default constructor
    DataManager() = default;

    /// \brief Destructor
    ~DataManager() = default;

    /// \brief Copy constructor
    DataManager(const DataManager& other) = delete;

    /// \brief Move constructor
    DataManager(DataManager&& other) = delete;

    /// \brief Copy assignment operator
    DataManager& operator=(const DataManager& other) = delete;

    /// \brief Move assignment operator
    DataManager& operator=(DataManager&& other) = delete;

    /// \brief Gets number of hits stored
    /// \return  Number of hits
    int GetNofHits() { return fInputData.fvHits.size(); }

    /// \brief  Reads input data object from boost-serialized binary file
    /// \param  fileName  Name of input file
    void ReadInputData(const std::string& fileName);

    /// \brief  Reserve number of hits
    /// \param  nHits  Number of hits to be stored
    /// \note   If one does not call this method, the underlying vector of hits will be filled with the time penalty
    void ReserveNhits(HitIndex_t nHits) { fInputData.fvHits.reserve(nHits); }

    /// \brief  Resets the input data block
    /// \param  nHits  Number of hits to reserve
    void ResetInputData(HitIndex_t nHits = 0) noexcept;

    /// \brief  Pushes back a hit (with a data stream info)
    /// \param  hit       An ca::Hit object
    /// \param  streamId  Index of a data stream
    void PushBackHit(const Hit& hit, int64_t streamId)
    {
      if (fInputData.fvStreamStartIndices.size() == 0 || fLastStreamId != streamId) {  // new data stream
        fLastStreamId = streamId;
        fInputData.fvStreamStartIndices.push_back(fInputData.fvHits.size());
        // for a case.. it is fixed later in InitData()
        fInputData.fvStreamStopIndices.push_back(fInputData.fvHits.size());
      }
      fInputData.fvHits.push_back(hit);
    }

    /// \brief  Pushes back a hit
    /// \param  hit       An ca::Hit object
    void PushBackHit(const Hit& hit) { fInputData.fvHits.push_back(hit); }

    /// \brief  Sets the number of hit keys
    /// \param  nKeys  Number of hit keys
    void SetNhitKeys(int nKeys) { fInputData.fNhitKeys = nKeys; }

    /// \brief Takes (moves) the instance of the input data object
    InputData&& TakeInputData();

    /// \brief  Writes input data object to boost-serialized binary file
    /// \param  fileName  Name of input file
    void WriteInputData(const std::string& fileName) const;


   private:
    /// \brief Initializes data object
    ///
    /// Sorts hits by stations (complexity O(n)) and defines bordering hit index for station
    void InitData();

    /// Provides quick QA for input data
    /// \tparam  Level  The level of the checks. The values of the parameter:
    ///                 - 0: no checks will be done
    ///                 - 1: only number of hits and strips as well as validity of hits first and last indexes will be checked
    ///                 - 2: hits sorting is checked
    ///                 - 3: every hit is checked for consistency
    /// \note    The larger Level corresponds to more precise checks, but is followed by larger time penalty
    template<int Level>
    bool CheckInputData() const;


    // ***************************
    // ** Member variables list **
    // ***************************
    InputData fInputData{};     ///< Object of input data
    int64_t fLastStreamId{-1};  ///< data stream Id of the last hit added
  };


  // *************************************
  // ** Inline functions implementation **
  // *************************************

  // -------------------------------------------------------------------------------------------------------------------
  //

  // TODO: Complete this function
  template<int Level>
  inline bool DataManager::CheckInputData() const
  {
    if constexpr (Level == 0) {
      return true;
    }                                // Level = 0 -> do nothing
    else if constexpr (Level > 0) {  // Level = 1 and higher
      // ----- Check if the hits container is not empty ----------------------------------------------------------------
      if (fInputData.fvHits.size() == 0) {
        LOG(warn) << "DataManager [check input]: Sample contains empty hits, tracking will not be executed";
        return false;
      }

      // ----- Checks if the number of hit keys is valid ---------------------------------------------------------------
      if (fInputData.fNhitKeys < 1) {
        LOG(error) << "DataManager [check input]: Incorrect number of keys passed (" << fInputData.fNhitKeys
                   << "), tracking will not be executed";
        return false;
      }

      // ----- Checks the indexes of first and last hits in stations
      // TODO: Add one of the two following checks for fvStartHitIn

      if constexpr (Level > 1) {  // Level = 2 and higher
        // ----- Checks for hits sorting -------------------------------------------------------------------------------
        // TODO...
        if constexpr (Level > 2) {  // Level = 3 and higher
          // ----- Checks for consistency of the particular hit --------------------------------------------------------
          // TODO...
        }
      }
      return true;
    }
    return true;
  }
}  // namespace cbm::algo::ca
