/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Hitfind.h
/// \brief  A BMON hit class
/// \since  07.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CbmBmonDigi.h"

#include <boost/serialization/access.hpp>

#include <cstdint>
#include <vector>

namespace cbm::algo::bmon
{
  /// \class  Hit
  /// \brief  A BMON hit
  class Hit {
   public:
    /// \brief Constructor from a single digi
    /// \param address  Address of the diamond
    /// \param digi     A digi
    Hit(uint32_t address, const CbmBmonDigi& digi) : fAddress(address), fNofChannels(1)
    {
      fTime      = digi.GetTime();
      fTimeError = 0.;  // FIXME: provide a rule to set an error
    }

    /// \brief Constructor from two digis
    /// \param address  Address of the diamond
    /// \param digiL    First digi
    /// \param digiR    Second digi
    Hit(uint32_t address, const CbmBmonDigi& digiL, const CbmBmonDigi& digiR) : fAddress(address), fNofChannels(2)
    {
      double weightSum = digiL.GetCharge() + digiR.GetCharge();
      fTime            = (digiL.GetTime() * digiL.GetCharge() + digiR.GetTime() * digiR.GetCharge()) / weightSum;
      fTimeError       = 0.;  // FIXME: provide a rule to set an error
    }

    /// \brief Constructor
    /// \param address   Address of diamond (the channel is not stored)
    /// \param time      Time of the hit [ns]
    /// \param timeError Time error of the hit [ns]
    /// \param nChannels Number of channels used (either one or two)
    Hit(uint32_t address, double time, double timeError, uint8_t nChannels)
      : fTime(time)
      , fTimeError(timeError)
      , fAddress(address)
      , fNofChannels(nChannels)
    {
    }

    /// \brief Gets hardware address
    uint32_t GetAddress() const { return fAddress; }

    /// \brief Gets number of channels
    uint8_t GetNofChannels() const { return fNofChannels; }

    /// \brief Gets time [ns]
    double GetTime() const { return fTime; }

    /// \brief Gets time error [ns]
    double GetTimeError() const { return fTimeError; }

    /// \brief Sets address
    /// \param address Hardware address
    void SetAddress(uint32_t address) { fAddress = address; }

    /// \brief Sets number of channels
    /// \param nofChannels Number of channels (digis), used to create a hit
    void SetNofChannels(uint8_t nofChannels) { fNofChannels = nofChannels; }

    /// \brief Sets time
    /// \param  time  Hit time [ns]
    void SetTime(double time) { fTime = time; }

    /// \brief Sets time error
    /// \param  timeError  Hit time error [ns]
    void SetTimeError(double timeError) { fTimeError = timeError; }

   private:
    double fTime{0.};         ///< Time [ns]
    double fTimeError{0.};    ///< Time error [ns]
    uint32_t fAddress{0};     ///< Assigned hit address
    uint8_t fNofChannels{0};  ///< Number of channels used

    /// \brief Boost serialization function
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, unsigned int /*version*/)
    {
      ar& fTime;
      ar& fTimeError;
      ar& fAddress;
      ar& fNofChannels;
    }
  };
}  // namespace cbm::algo::bmon
