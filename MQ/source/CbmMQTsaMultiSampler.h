/* Copyright (C) 2017-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Pierre-Alain Loizeau */

/**
 * CbmMQTsaSampler.h
 *
 * @since 2017-11-17
 * @author F. Uhlig
 */

#ifndef CBMMQTSAMULTISAMPLER_H_
#define CBMMQTSAMULTISAMPLER_H_


#include "MicrosliceDescriptor.hpp"
#include "StorableTimeslice.hpp"
#include "Timeslice.hpp"
#include "TimesliceSource.hpp"

#include "FairMQDevice.h"

class TCanvas;
class TH1F;
class TH1I;
class TProfile;
#include <TObjArray.h>

#include <ctime>
#include <string>
#include <utility>
#include <vector>

class CbmMQTsaMultiSampler : public FairMQDevice {
public:
  CbmMQTsaMultiSampler();
  virtual ~CbmMQTsaMultiSampler();

protected:
  uint64_t fMaxTimeslices;

  std::string fFileName;
  std::string fDirName;

  std::vector<std::string> fInputFileList;  ///< List of input files
  uint64_t fFileCounter;
  std::string fHost;
  uint64_t fPort;
  uint64_t fHighWaterMark;

  bool fbNoSplitTs        = false;
  bool fbSendTsPerSysId   = false;
  bool fbSendTsPerChannel = false;

  std::string fsChannelNameHistosInput  = "histogram-in";
  std::string fsChannelNameHistosConfig = "histo-conf";
  std::string fsChannelNameCanvasConfig = "canvas-conf";
  uint32_t fuPublishFreqTs              = 0;
  double_t fdMinPublishTime             = 0.5;
  double_t fdMaxPublishTime             = 5;

  uint64_t fuPrevTsIndex = 0;
  uint64_t fTSCounter;
  uint64_t fMessageCounter;

  int fMaxMemory = 0;

  virtual void InitTask();
  virtual bool ConditionalRun();

private:
  bool InitHistograms();
  bool CheckTimeslice(const fles::Timeslice& ts);
  void PrintMicroSliceDescriptor(const fles::MicrosliceDescriptor& mdsc);
  void CalcRuntime();
  bool IsChannelNameAllowed(std::string);
  bool CreateAndSendComponent(const fles::Timeslice&, int);
  bool CreateAndCombineComponentsPerSysId(const fles::Timeslice&);
  bool CreateAndCombineComponentsPerChannel(const fles::Timeslice&);
  bool CreateAndSendFullTs(const fles::Timeslice&);
  bool SendData(const fles::StorableTimeslice&, int);
  bool SendData(const fles::StorableTimeslice&, std::string);
  bool SendMissedTsIdx(std::vector<uint64_t> vIndices);
  bool SendCommand(std::string sCommand);
  bool SendHistograms();
  bool ResetHistograms();

  fles::TimesliceSource* fSource;  //!
  std::chrono::steady_clock::time_point fTime;
  std::chrono::system_clock::time_point fLastPublishTime;


  // The vector fAllowedChannels contain the list of defined channel names
  // which are used for connecting the different devices. For the time
  // being the correct connection are done checking the names. A connection
  // using the name stscomponent will receive timeslices containing the
  // sts component only. The corresponding system ids are defined in the
  // vector fSysId. At startup it is checked which channels are defined
  // in the startup script such that later on only timeslices whith the
  // corresponding data are send to the correct channels.
  // TODO: Up to now we have three disconnected vectors which is very
  //       error prone. Find a better solution


  std::vector<std::string> fAllowedChannels = {"stscomponent", "richcomponent", "trdcomponent", "muchcomponent",
                                               "tofcomponent", "t0component",   "psdcomponent"};
  //    std::vector<int> fSysId = {16, 48, 64, 96, 144, 80};
  std::vector<int> fSysId = {0x10, 0x30, 0x40, 0x50, 0x60, 0x90, 0x80};

  std::vector<int> fComponentsToSend                    = {0, 0, 0, 0, 0, 0, 0};
  std::vector<std::vector<std::string>> fChannelsToSend = {{}, {}, {}, {}, {}, {}, {}};

  bool fbListCompPerSysIdReady                       = false;
  std::vector<std::vector<uint32_t>> fvvCompPerSysId = {{}, {}, {}, {}, {}, {}, {}};

  bool fbListCompPerChannelReady                       = false;
  std::vector<std::string> fvChannelsToSend            = {};
  std::vector<std::vector<uint32_t>> fvvCompPerChannel = {};

  std::string fsChannelNameMissedTs = "";
  std::string fsChannelNameCommands = "";

  /// Array of histograms to send to the histogram server
  TObjArray fArrayHisto = {};
  /// Vector of string pairs with ( HistoName, FolderPath ) to send to the histogram server
  std::vector<std::pair<std::string, std::string>> fvpsHistosFolder = {};
  /// Vector of string pairs with ( CanvasName, CanvasConfig ) to send to the histogram server
  /// Format of Can config is "NbPadX(U);NbPadY(U);ConfigPad1(s);....;ConfigPadXY(s)"
  /// Format of Pad config is "GrixX(b),GridY(b),LogX(b),LogY(b),LogZ(b),HistoName(s),DrawOptions(s)"
  std::vector<std::pair<std::string, std::string>> fvpsCanvasConfig = {};

  /// Histograms
  TH1I* fhTsRate          = nullptr;
  TH1I* fhTsSize          = nullptr;
  TProfile* fhTsSizeEvo   = nullptr;
  TH1F* fhTsMaxSizeEvo    = nullptr;
  TH1I* fhMissedTS        = nullptr;
  TProfile* fhMissedTSEvo = nullptr;
  TCanvas* fcSummary      = nullptr;
  uint64_t fuStartTime    = 0;
  double_t fdTimeToStart  = 0.;
  double_t fdLastMaxTime  = 0.;
  double_t fdTsMaxSize    = 0.;
};

#endif /* CBMMQTSASAMPLER_H_ */
