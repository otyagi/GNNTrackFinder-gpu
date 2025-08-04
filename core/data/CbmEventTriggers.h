/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   CbmEventTriggers.h
/// \brief  A structure to store different triggers in parallel to the CbmEvent
/// \since  25.01.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include <cstdint>
#include <string>

#if !defined(NO_ROOT) && !XPU_IS_HIP_CUDA
#include <Rtypes.h>  // for ClassDef
#endif

#include <boost/serialization/access.hpp>

/// \class CbmEventTriggers
/// \brief Class to store different triggers for a given event
class CbmEventTriggers {
 public:
  /// \enum   ETrigger
  /// \brief  Defines a trigger bitmask
  enum class ETrigger : uint32_t
  {
    Lambda = 0x00000001,  ///< Lambda-trigger
    Ks     = 0x00000002   ///< Ks-trigger
  };

  //using Trigger_t = std::underlying_type_t<ETrigger>;
  using Trigger_t = uint32_t;

  /// \brief Default constructor
  CbmEventTriggers() = default;

  /// \brief A constructor from integer
  /// \param bitmap  A bitmap of the triggers
  explicit CbmEventTriggers(uint32_t bitmap) { fBitmap = bitmap; }

  /// \brief Copy constructor
  CbmEventTriggers(const CbmEventTriggers&) = default;

  /// \brief Move constructor
  CbmEventTriggers(CbmEventTriggers&&) = default;

  /// \brief Destructor
  ~CbmEventTriggers() = default;

  /// \brief Copy assignment operator
  CbmEventTriggers& operator=(const CbmEventTriggers&) = default;

  /// \brief Move assignment operator
  CbmEventTriggers& operator=(CbmEventTriggers&&) = default;

  /// \brief Gets a bitmap
  /// \return bitmap  (integer)
  Trigger_t GetBitmap() const { return fBitmap; }

  /// \brief Sets a trigger
  /// \param key  Trigger key
  void Set(ETrigger key) { fBitmap |= static_cast<Trigger_t>(key); }

  /// \brief Resets a trigger
  /// \param key  Trigger key
  void Reset(ETrigger key) { fBitmap &= ~static_cast<Trigger_t>(key); }

  /// \brief Resets all the triggers
  void ResetAll() { fBitmap = 0; }

  /// \brief Tests a particular single trigger
  /// \param key  Trigger key
  bool Test(ETrigger key) const { return static_cast<bool>(fBitmap & static_cast<Trigger_t>(key)); }

  /// \brief Tests, if ALL the triggers in the bitmask are on
  /// \param bitmask  Trigger bitmask
  bool TestAll(Trigger_t bitmask) const { return bitmask == (fBitmap & bitmask); }

  /// \brief Tests, if ANY of the triggers in the bitmask are on
  /// \param bitmask  Trigger bitmask
  bool TestAny(Trigger_t bitmask) const { return static_cast<bool>(fBitmap | bitmask); }

  /// \brief String representation of the class content
  std::string ToString() const;

 private:
  Trigger_t fBitmap{0};  ///< bitmap storing the triggers according to ETrigger

  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int /*version*/)
  {
    ar& fBitmap;
  }

#if !defined(NO_ROOT) && !XPU_IS_HIP_CUDA
  ClassDefNV(CbmEventTriggers, 2);
#endif
};
