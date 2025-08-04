/* Copyright (C) 2019 PI-UHd, GSI
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Norbert Herrmann [committer] */

/**
 * CbmDeviceTriggerHandlerStar2019.h
 *
 * @since 2019-11-15
 * @author N. Herrmann
 */

#ifndef CBMDEVICETRIGGERHANDLERETOF_H_
#define CBMDEVICETRIGGERHANDLERETOF_H_

#include "CbmMqTMessage.h"
#include "CbmTofStarData2019.h"

#include "MicrosliceDescriptor.hpp"
#include "Timeslice.hpp"

#include "FairMQDevice.h"

#include "Rtypes.h"

#include <map>
#include <vector>

// Relevant TOF classes

extern "C" int star_rhicf_write(unsigned int trg_word, void* dta, int bytes);

// ROOT Classes and includes
class TString;

// C++ Classes and includes
#include <list>
#include <map>
#include <vector>

class CbmDeviceTriggerHandlerEtof : public FairMQDevice {
public:
  CbmDeviceTriggerHandlerEtof();
  virtual ~CbmDeviceTriggerHandlerEtof();

protected:
  virtual void InitTask();
  bool HandleData(FairMQParts&, int);
  bool HandleMessage(FairMQMessagePtr&, int);

private:
  // Variables used for histo filling

  Bool_t IsChannelNameAllowed(std::string channelName);

  Bool_t InitWorkspace();
  Bool_t InitContainers();

  Bool_t ReInitContainers();

  uint64_t fNumMessages;
  std::vector<std::string> fAllowedChannels = {"tofcomponent", "parameters", "etofevts", "tofhits", "syscmd"};

  // Input variables

  // Output variables

  // Constants or setting parameters
  Int_t fiMsgCnt;
  /// Control flags
  Bool_t fbMonitorMode;       //! Switch ON the filling of a minimal set of histograms
  Bool_t fbDebugMonitorMode;  //! Switch ON the filling of a additional set of histograms
  Bool_t fbSandboxMode;       //! Switch OFF the emission of data toward the STAR DAQ
  Bool_t fbEventDumpEna;      //! Switch ON the dumping of the events to a binary file

  Double_t fdEvent;

  // histograms
};

#endif /* CBMDEVICETRIGGERHANDLERETOF_H_ */
