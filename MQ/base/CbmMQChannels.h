/* Copyright (C) 2019 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#ifndef CBMMQCHANNELS_H_
#define CBMMQCHANNELS_H_

#include "FairMQDevice.h"

#include <string>
#include <vector>


class CbmMQChannels {
public:
  CbmMQChannels(std::vector<std::string>);

  bool IsChannelNameAllowed(std::string channelName);
  bool CheckChannels(FairMQDevice* device);

  std::vector<int> GetComponentsToSend() { return fComponentsToSend; }
  std::vector<std::vector<std::string>> GetChannelsToSend() { return fChannelsToSend; }

private:
  std::vector<std::string> fAllowedChannels {};
  std::vector<int> fComponentsToSend {};
  std::vector<std::vector<std::string>> fChannelsToSend {{}};
};

#endif
