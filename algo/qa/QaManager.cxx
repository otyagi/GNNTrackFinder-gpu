/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   QaManager.cxx
/// \date   09.02.2025
/// \brief  QA manager for the online data reconstruction
/// \author Sergei Zharko <s.zharko@gsi.de>

#include "qa/QaManager.h"

using cbm::algo::HistogramSender;
using cbm::algo::qa::Data;
using cbm::algo::qa::Manager;

// ---------------------------------------------------------------------------------------------------------------------
//
Manager::Manager(std::shared_ptr<HistogramSender> histoSender) : fpSender(histoSender) {}

// ---------------------------------------------------------------------------------------------------------------------
//
void Manager::Init() { fpData->Init(fpSender); }

// ---------------------------------------------------------------------------------------------------------------------
//
void Manager::SendHistograms() { fpData->Send(fpSender); }
