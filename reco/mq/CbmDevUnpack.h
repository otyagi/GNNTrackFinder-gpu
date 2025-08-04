/* Copyright (C) 2022 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Dominik Smith [committer] */

/**
 * CbmDevUnpack.h
 *
 * @since 2020-05-04
 * @author P.-A. Loizeau
 */

#ifndef CBMDEVUNPACK_H
#define CBMDEVUNPACK_H

#include "CbmMqTMessage.h"
#include "FairMQDevice.h"
#include "FairParGenericSet.h"
#include "Rtypes.h"
#include "TObjArray.h"
#include "Timeslice.hpp"
#include "much/MuchReadoutConfig.h"
#include "much/UnpackMuch.h"
#include "sts/StsReadoutConfigLegacy.h"
#include "sts/UnpackSts.h"

#include <chrono>
#include <map>
#include <vector>

class TimesliceMetaData;
class CbmDigiTimeslice;

class CbmDevUnpack : public FairMQDevice {
 public:
  CbmDevUnpack();
  virtual ~CbmDevUnpack(){};

 private:
  std::map<uint16_t, cbm::algo::UnpackSts> fAlgoSts = {};
  cbm::algo::StsReadoutConfigLegacy fStsConfig{};

  std::map<uint16_t, cbm::algo::UnpackMuch> fAlgoMuch = {};
  cbm::algo::MuchReadoutConfig fMuchConfig{};

  /// message queues
  std::string fsChannelNameDataInput  = "ts-request";
  std::string fsChannelNameDataOutput = "unpts_0";
  std::string fsChannelNameCommands   = "commands";

  /// Statistics & first TS rejection
  size_t fNumMessages = 0;
  size_t fNumTs       = 0;

  /** @brief Read command line parameters for MQ device */
  virtual void InitTask();

  /** @brief Called by run loop, does init steps on first TS */
  bool ConditionalRun();

  /** @brief Initialize runtime parameters for UnpackSts algos */
  bool InitAlgos();

  /**
   * @brief Unpack a single timeslice
   * @param ts Input FLES timeslice
  */
  CbmDigiTimeslice DoUnpack(const fles::Timeslice& ts);

  /** @brief Serialize unpacked digi timeslice and send to output channel */
  bool SendData(const CbmDigiTimeslice& timeslice, const TimesliceMetaData& TsMetaData);
};

#endif /* CBMDEVUNPACK_H */
