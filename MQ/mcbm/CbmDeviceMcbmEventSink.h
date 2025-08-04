/* Copyright (C) 2020-2021 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau [committer] */

/**
 * CbmDeviceMcbmEventSink.h
 *
 * @since 2020-05-04
 * @author P.-A. Loizeau
 */

#ifndef CBMDEVICEMCBMEVTSINK_H_
#define CBMDEVICEMCBMEVTSINK_H_

/// CBM headers
#include "CbmEvent.h"
#include "CbmMqTMessage.h"
#include "CbmMuchBeamTimeDigi.h"
#include "CbmPsdDigi.h"
#include "CbmRichDigi.h"
#include "CbmStsDigi.h"
#include "CbmTofDigi.h"
#include "CbmTrdDigi.h"

#include "TimesliceMetaData.h"

/// FAIRROOT headers
#include "FairMQDevice.h"

/// FAIRSOFT headers (geant, boost, ...)
#include "Rtypes.h"
#include "TClonesArray.h"
#include "TObjArray.h"

/// C/C++ headers
#include <chrono>
#include <map>
#include <vector>

class TFile;
class TList;
class TClonesArray;
//class TimesliceMetaData;
class FairRunOnline;
class FairRootManager;

class CbmUnpackedTimeslice {
  /// TODO: rename to CbmTsWithEvents
public:
  CbmUnpackedTimeslice(FairMQParts& parts);
  ~CbmUnpackedTimeslice();

  TimesliceMetaData fTsMetaData;
  std::vector<CbmTofDigi> fvDigiBmon;
  std::vector<CbmStsDigi> fvDigiSts;
  std::vector<CbmMuchBeamTimeDigi> fvDigiMuch;
  std::vector<CbmTrdDigi> fvDigiTrd;
  std::vector<CbmTofDigi> fvDigiTof;
  std::vector<CbmRichDigi> fvDigiRich;
  std::vector<CbmPsdDigi> fvDigiPsd;
  TClonesArray fEventsArray;
};

class CbmDeviceMcbmEventSink : public FairMQDevice {
public:
  CbmDeviceMcbmEventSink();
  virtual ~CbmDeviceMcbmEventSink();

protected:
  virtual void InitTask();
  bool HandleMissTsData(FairMQMessagePtr&, int);
  bool HandleData(FairMQParts&, int);
  bool HandleCommand(FairMQMessagePtr&, int);

private:
  /// Constants

  /// Control flags
  Bool_t fbFillHistos = kFALSE;  //! Switch ON/OFF filling of histograms
  Bool_t fbFinishDone = kFALSE;  //! Keep track of whether the Finish was already called

  /// User settings parameters
  /// Algo enum settings
  std::string fsOutputFileName = "mcbm_digis_events.root";
  /// message queues
  std::string fsChannelNameMissedTs     = "missedts";
  std::string fsChannelNameDataInput    = "events";
  std::string fsChannelNameCommands     = "commands";
  std::string fsChannelNameHistosInput  = "histogram-in";
  std::string fsChannelNameHistosConfig = "histo-conf";
  std::string fsChannelNameCanvasConfig = "canvas-conf";
  /// Histograms management
  uint32_t fuPublishFreqTs  = 100;
  double_t fdMinPublishTime = 0.5;
  double_t fdMaxPublishTime = 5.0;

  /// List of MQ channels names
  std::vector<std::string> fsAllowedChannels = {fsChannelNameDataInput};

  /// Parameters management
  //      TList* fParCList = nullptr;
  //      Bool_t InitParameters( TList* fParCList );

  /// Statistics & missed TS detection
  uint64_t fuPrevTsIndex                                 = 0;
  uint64_t fulNumMessages                                = 0;
  uint64_t fulTsCounter                                  = 0;
  uint64_t fulMissedTsCounter                            = 0;
  std::chrono::system_clock::time_point fLastPublishTime = std::chrono::system_clock::now();

  /// Control Commands reception
  bool fbReceivedEof      = false;
  uint64_t fuLastTsIndex  = 0;
  uint64_t fuTotalTsCount = 0;

  /// Data reception
  /// Event (TS) header
  //      FairEventHeader*        fEvtHeader;
  /// TS MetaData storage
  TClonesArray* fTimeSliceMetaDataArray = nullptr;  //!
  TimesliceMetaData* fTsMetaData        = nullptr;
  /// Digis storage
  std::vector<CbmTofDigi>* fvDigiBmon          = nullptr;
  std::vector<CbmStsDigi>* fvDigiSts           = nullptr;
  std::vector<CbmMuchBeamTimeDigi>* fvDigiMuch = nullptr;
  std::vector<CbmTrdDigi>* fvDigiTrd           = nullptr;
  std::vector<CbmTofDigi>* fvDigiTof           = nullptr;
  std::vector<CbmRichDigi>* fvDigiRich         = nullptr;
  std::vector<CbmPsdDigi>* fvDigiPsd           = nullptr;
  /// CbmEvents
  TClonesArray* fEventsArray = nullptr;  //! output container of CbmEvents
  //      std::vector< CbmEvent * > &        fEventVector;    //! vector with all created events

  /// Storage for re-ordering
  /// Missed TS vector
  std::vector<uint64_t> fvulMissedTsIndices = {};
  /// Buffered TS
  std::map<uint64_t, CbmUnpackedTimeslice> fmFullTsStorage = {};

  /// Data storage
  FairRunOnline* fpRun           = nullptr;
  FairRootManager* fpFairRootMgr = nullptr;

  /// Array of histograms to send to the histogram server
  TObjArray fArrayHisto = {};
  /// Vector of string pairs with ( HistoName, FolderPath ) to send to the histogram server
  std::vector<std::pair<std::string, std::string>> fvpsHistosFolder = {};
  /// Vector of string pairs with ( CanvasName, CanvasConfig ) to send to the histogram server
  /// Format of Can config is "NbPadX(U);NbPadY(U);ConfigPad1(s);....;ConfigPadXY(s)"
  /// Format of Pad config is "GrixX(b),GridY(b),LogX(b),LogY(b),LogZ(b),HistoName(s),DrawOptions(s)"
  std::vector<std::pair<std::string, std::string>> fvpsCanvasConfig = {};

  /// Internal methods
  bool IsChannelNameAllowed(std::string channelName);
  //      Bool_t InitContainers();
  void CheckTsQueues();
  void PrepareTreeEntry(CbmUnpackedTimeslice unpTs);
  void DumpTreeEntry();
  bool SendHistograms();
  void Finish();
};

#endif /* CBMDEVICEMCBMEVTSINK_H_ */
