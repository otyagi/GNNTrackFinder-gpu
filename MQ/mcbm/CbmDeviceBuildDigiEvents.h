/* Copyright (C) 2021 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */

/**
 * CbmDeviceBuildRawEvents.h
 *
 * @since 2021-11-18
 * @author P.-A. Loizeau
 */

#ifndef CBMDEVICEBUILDDIGIEVENTS_H_
#define CBMDEVICEBUILDDIGIEVENTS_H_

/// CBM headers
#include "CbmAlgoBuildRawEvents.h"
#include "CbmBmonDigi.h"
#include "CbmMuchDigi.h"
#include "CbmPsdDigi.h"
#include "CbmRichDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"
#include "CbmTrdDigi.h"

/// FAIRROOT headers
#include "FairMQDevice.h"

/// FAIRSOFT headers (geant, boost, ...)
#include "Rtypes.h"
#include "TObjArray.h"

/// C/C++ headers
#include <chrono>
#include <map>
#include <vector>

class TList;
class TClonesArray;
class FairRunOnline;
class CbmTsEventHeader;

class CbmDeviceBuildDigiEvents : public FairMQDevice {
public:
  CbmDeviceBuildDigiEvents();
  virtual ~CbmDeviceBuildDigiEvents();

protected:
  virtual void InitTask();
  bool HandleData(FairMQParts&, int);
  bool HandleCommand(FairMQMessagePtr&, int);

private:
  /// Constants

  /// Control flags
  Bool_t fbIgnoreTsOverlap = kFALSE;  //! Ignore data in Overlap part of the TS
  Bool_t fbFillHistos      = kTRUE;   //! Switch ON/OFF filling of histograms

  /// User settings parameters
  /// Algo enum settings
  std::string fsEvtOverMode                      = "NoOverlap";
  std::string fsRefDet                           = "kBmon";
  std::vector<std::string> fvsAddDet             = {};
  std::vector<std::string> fvsDelDet             = {};
  std::vector<std::string> fvsSetTrigWin         = {};
  std::vector<std::string> fvsSetTrigMinNb       = {};
  std::vector<std::string> fvsSetTrigMaxNb       = {};
  std::vector<std::string> fvsSetTrigMinLayersNb = {};
  std::vector<std::string> fvsSetHistMaxDigiNb   = {};
  /// I/O control
  bool fbDoNotSend       = false;
  bool fbDigiEventOutput = false;
  /// message queues
  std::string fsChannelNameDataInput   = "unpts_0";
  std::string fsChannelNameDataOutput  = "events";
  std::string fsChannelNameCommands    = "commands";
  std::string fsChannelNameHistosInput = "histogram-in";
  /// Histograms management
  uint32_t fuPublishFreqTs  = 100;
  double_t fdMinPublishTime = 0.5;
  double_t fdMaxPublishTime = 5.0;

  /// List of MQ channels names
  std::vector<std::string> fsAllowedChannels = {fsChannelNameDataInput};

  /// Statistics & first TS rejection
  uint64_t fulNumMessages                                = 0;
  uint64_t fulTsCounter                                  = 0;
  std::chrono::system_clock::time_point fLastPublishTime = std::chrono::system_clock::now();

  /// Processing algos
  CbmAlgoBuildRawEvents* fpAlgo = nullptr;

  /// TS MetaData stable values storage
  size_t fuNbCoreMsPerTs    = 0;        //!
  size_t fuNbOverMsPerTs    = 0;        //!
  Double_t fdMsSizeInNs     = 1280000;  //! Size of a single MS, [nanoseconds]
  Double_t fdTsCoreSizeInNs = -1.0;     //! Total size of the core MS in a TS, [nanoseconds]
  Double_t fdTsOverSizeInNs = -1.0;     //! Total size of the overlap MS in a TS, [nanoseconds]
  Double_t fdTsFullSizeInNs = -1.0;     //! Total size of all MS in a TS, [nanoseconds]

  /// Data reception
  /// TS information in header
  CbmTsEventHeader* fCbmTsEventHeader = nullptr;
  /// Digis storage
  std::vector<CbmBmonDigi>* fvDigiBmon = nullptr;
  std::vector<CbmStsDigi>* fvDigiSts   = nullptr;
  std::vector<CbmMuchDigi>* fvDigiMuch = nullptr;
  std::vector<CbmTrdDigi>* fvDigiTrd   = nullptr;
  std::vector<CbmTofDigi>* fvDigiTof   = nullptr;
  std::vector<CbmRichDigi>* fvDigiRich = nullptr;
  std::vector<CbmPsdDigi>* fvDigiPsd   = nullptr;
  /// TS MetaData storage
  TClonesArray* fTimeSliceMetaDataArray = nullptr;  //!
  TimesliceMetaData* fTsMetaData        = nullptr;

  /// Array of histograms to send to the histogram server
  TObjArray fArrayHisto = {};
  /// Vector of string pairs with ( HistoName, FolderPath ) to send to the histogram server
  std::vector<std::pair<std::string, std::string>> fvpsHistosFolder = {};
  /// Vector of string pairs with ( CanvasName, CanvasConfig ) to send to the histogram server
  /// Format of Can config is "NbPadX(U);NbPadY(U);ConfigPad1(s);....;ConfigPadXY(s)"
  /// Format of Pad config is "GrixX(b),GridY(b),LogX(b),LogY(b),LogZ(b),HistoName(s),DrawOptions(s)"
  std::vector<std::pair<std::string, std::string>> fvpsCanvasConfig = {};
  /// Flag indicating whether the histograms and canvases configurations were already published
  bool fbConfigSent = false;

  /// Check wether an MQ channel name is among those expected for this device
  bool IsChannelNameAllowed(std::string channelName);
  /// Get detector event builder config from string containing name
  RawEventBuilderDetector GetDetectorBuilderCfg(std::string detName);
  /// Get detector type from string containing name
  ECbmModuleId GetDetectorId(std::string detName);

  bool InitHistograms();
  void Finish();
  bool SendEvents(FairMQParts& partsIn);
  bool SendDigiEvents(FairMQParts& partsIn);
  bool SendHistoConfAndData();
  bool SendHistograms();
};

#endif /* CBMDEVICEBUILDDIGIEVENTS_H_ */
