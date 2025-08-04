/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   Data.cxx
/// \date   12.02.2024
/// \brief  A unified data-structure to handle QA objects for the online reconstruction (implementation)
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "QaData.h"

#include <algorithm>

using cbm::algo::qa::Data;


// ---------------------------------------------------------------------------------------------------------------------
//
void Data::Init(std::shared_ptr<HistogramSender> histSender)
try {
  size_t nHistograms = 0;
  nHistograms += fNofH1;
  nHistograms += fNofH2;
  nHistograms += fNofP1;
  nHistograms += fNofP2;
  fbNotEmpty = static_cast<bool>(nHistograms);
  if (!fbNotEmpty) {
    L_(warn) << "no histograms were provided to a qa::Data instance (running in an idle mode)";
  }

  if (histSender.get() && fbNotEmpty) {

    // Check, if the tasks list was initialized properly: at least one task must be initialized
    if (fvTaskProperties.empty()) {
      std::stringstream msg;
      msg << "a qa::Data instance was not initialized properly: no task was registered. The list of the histograms:\n";
      auto ShowName = [&](const auto& h) { msg << " - " << h.GetName() << '\n'; };
      std::for_each(fHistograms.fvH1.begin(), fHistograms.fvH1.end(), ShowName);
      std::for_each(fHistograms.fvH2.begin(), fHistograms.fvH2.end(), ShowName);
      std::for_each(fHistograms.fvP1.begin(), fHistograms.fvP1.end(), ShowName);
      std::for_each(fHistograms.fvP2.begin(), fHistograms.fvP2.end(), ShowName);
      msg << "Please, insure that you either instantiate the qa::Data with the Data(std::string_view name) constructor";
      msg << ", or provide a task name explicitly with the function Data::RegisterNewTask(std::string_view name)";
      throw std::runtime_error(msg.str());
    }

    // Forming a histogram config message
    std::vector<std::pair<std::string, std::string>> vHistCfgs;
    // NOTE: Important to keep the order of filling the histograms: 1D -> 2D -> ..
    vHistCfgs.reserve(nHistograms);

    for (const auto& task : fvTaskProperties) {
      auto RegHist = [&](const auto& h) {
        if (!h.GetMetadata().CheckFlags()) {
          std::stringstream msg;
          msg << "attempt to pass a histogram " << h.GetName()
              << " with inconsistent flags (see HistogramMetadata::CheckFlags for detailes)";
          throw std::runtime_error(msg.str());
        }
        L_(info) << " - task: " << task.fsName << ", histogram: " << h.GetName();
        vHistCfgs.emplace_back(h.GetName() + "!" + h.GetMetadataString(), task.fsName);
      };
      fsTaskNames += fmt::format("{} ", task.fsName);
      std::for_each(task.fRangeH1.first, task.fRangeH1.second, RegHist);
      std::for_each(task.fRangeH2.first, task.fRangeH2.second, RegHist);
      std::for_each(task.fRangeP1.first, task.fRangeP1.second, RegHist);
      std::for_each(task.fRangeP2.first, task.fRangeP2.second, RegHist);
    }

    // Forming a canvas config message
    std::vector<std::pair<std::string, std::string>> vCanvCfgs;
    vCanvCfgs.reserve(fvsCanvCfgs.size());
    for (const auto& canv : fvsCanvCfgs) {
      vCanvCfgs.emplace_back(std::make_pair(canv.substr(0, canv.find_first_of(';')), canv));
    }

    histSender->PrepareAndSendMsg(std::pair<uint32_t, uint32_t>(vHistCfgs.size(), vCanvCfgs.size()),
                                  zmq::send_flags::sndmore);

    auto RegCfg = [&](const auto& cfg) { histSender->PrepareAndSendMsg(cfg, zmq::send_flags::sndmore); };

    std::for_each(vHistCfgs.begin(), vHistCfgs.end(), RegCfg);
    std::for_each(vCanvCfgs.begin(), vCanvCfgs.end(), RegCfg);

    // Histograms serialization and emission to close multi-part message
    histSender->PrepareAndSendMsg(qa::HistogramContainer{}, zmq::send_flags::none);
  }
}
catch (const std::exception& err) {
  L_(fatal) << "cbm::algo::qa::Data for " << fsTaskNames << " fatally aborted. Reason " << err.what();
  assert(false);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Data::RegisterNewTask(std::string_view name)
{
  auto itH1 = fHistograms.fvH1.begin();
  auto itH2 = fHistograms.fvH2.begin();
  auto itP1 = fHistograms.fvP1.begin();
  auto itP2 = fHistograms.fvP2.begin();
  fvTaskProperties.emplace_back(TaskProperties{.fsName   = {name.begin(), name.end()},
                                               .fRangeH1 = std::make_pair(itH1, itH1),
                                               .fRangeH2 = std::make_pair(itH2, itH2),
                                               .fRangeP1 = std::make_pair(itP1, itP1),
                                               .fRangeP2 = std::make_pair(itP2, itP2)});
}

// ---------------------------------------------------------------------------------------------------------------------
//
void Data::Send(std::shared_ptr<HistogramSender> histoSender)
{
  if (histoSender.get() && fbNotEmpty) {
    histoSender->PrepareAndSendMsg(fHistograms, zmq::send_flags::none);
    L_(info) << fsTaskNames << ": Published " << fNofH1 << " 1D- and " << fNofH2 << " 2D-histograms, " << fNofP1
             << " 1D- and " << fNofP2 << " 2D-profiles";
    this->Reset();
  }
}
