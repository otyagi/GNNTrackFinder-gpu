/* Copyright (C) 2024 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */

#pragma once

#include <FairSource.h>

/**
 * @file CbmSourceDummy.h
 * @brief Dummy input source
**/

/**
 * @class CbmSourceDummy
 * @brief Dummy input FAIR source. Doesn't create any branches or data.
 * @note This is used to work around a bug in FairRunAna, which can trigger into a crash in
 * TGeoManager if no input source is set.
**/
class CbmSourceDummy : public FairSource {

 public:
  CbmSourceDummy() = default;

  /** @brief Destructor **/
  virtual ~CbmSourceDummy() = default;


  /** @brief Demanded by base class **/
  virtual void Close() {}


  /** @brief Demanded by base class **/
  virtual Source_Type GetSourceType() { return kFILE; }


  /** @brief Initialisation **/
  virtual Bool_t Init() { return true; }


  /** @brief Demanded by base class **/
  virtual Bool_t InitUnpackers() { return kTRUE; }


  /** @brief Demanded by base class **/
  virtual Int_t ReadEvent(UInt_t = 0) { return 0; }


  /** @brief Demanded by base class **/
  virtual Bool_t ReInitUnpackers() { return kTRUE; }


  /** @brief Demanded by base class **/
  virtual void Reset() {}


  /** @brief Demanded by base class **/
  virtual void SetParUnpackers() {}


  /** @brief Demanded by base class **/
  virtual Bool_t SpecifyRunId() { return kTRUE; }
};
