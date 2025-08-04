/* Copyright (C) 2019-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Florian Uhlig [committer] */

/**
 * CbmDeviceMonitorBmon.h
 *
 * @since 2019-03-26
 * @author F. Uhlig
 */

#ifndef CBMDEVICEMONITORBmon_H_
#define CBMDEVICEMONITORBmon_H_

#include "CbmMqTMessage.h"

#include "Timeslice.hpp"

#include "FairMQDevice.h"

#include "Rtypes.h"
#include "TObjArray.h"

#include <chrono>
#include <map>
#include <vector>

class TList;
class CbmMcbm2018MonitorAlgoBmon;

class CbmDeviceMonitorBmon : public FairMQDevice {
public:
  CbmDeviceMonitorBmon();
  virtual ~CbmDeviceMonitorBmon();

protected:
  virtual void InitTask();
  bool HandleData(FairMQMessagePtr&, int);

private:
  /// Constants
  static const uint16_t kusSysId = 0x90;

  /// Control flags
  Bool_t fbIgnoreOverlapMs;  //! Ignore Overlap Ms: all fuOverlapMsNb MS at the end of timeslice
  Bool_t fbComponentsAddedToList = kFALSE;

  /// User settings parameters
  std::string fsChannelNameDataInput;
  std::string fsChannelNameHistosInput;
  uint32_t fuHistoryHistoSize;
  uint32_t fuMinTotPulser;
  uint32_t fuMaxTotPulser;
  uint32_t fuOffSpillCountLimit;
  uint32_t fuOffSpillCountLimitNonPulser;
  double fdSpillCheckInterval;
  std::vector<uint32_t> fvuChanMap;
  uint32_t fuPublishFreqTs;
  double_t fdMinPublishTime;
  double_t fdMaxPublishTime;

  /// List of MQ channels names
  std::vector<std::string> fsAllowedChannels;

  /// Parameters management
  TList* fParCList;

  /// Statistics & first TS rejection
  uint64_t fulNumMessages;
  uint64_t fulTsCounter;
  std::chrono::system_clock::time_point fLastPublishTime;

  /// Processing algo
  CbmMcbm2018MonitorAlgoBmon* fMonitorAlgo;

  /// Array of histograms to send to the histogram server
  TObjArray fArrayHisto;
  /// Vector of string pairs with ( HistoName, FolderPath ) to send to the histogram server
  std::vector<std::pair<std::string, std::string>> fvpsHistosFolder;
  /// Vector of string pairs with ( CanvasName, CanvasConfig ) to send to the histogram server
  /// Format of Can config is "NbPadX(U);NbPadY(U);ConfigPad1(s);....;ConfigPadXY(s)"
  /// Format of Pad config is "GrixX(b),GridY(b),LogX(b),LogY(b),LogZ(b),HistoName(s),DrawOptions(s)"
  std::vector<std::pair<std::string, std::string>> fvpsCanvasConfig;
  /// Flag indicating whether the histograms and canvases configurations were already published
  bool fbConfigSent = false;

  bool IsChannelNameAllowed(std::string channelName);
  bool InitContainers();
  bool InitHistograms();
  bool DoUnpack(const fles::Timeslice& ts, size_t component);
  void Finish();
  bool SendHistoConfAndData();
  bool SendHistograms();
};

#endif /* CBMDEVICEMONITORBmon_H_ */
