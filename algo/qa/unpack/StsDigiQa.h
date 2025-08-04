/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   StsDigiQa.h
/// \date   03.03.2024
/// \brief  QA module for STS raw digis (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "CbmStsDigi.h"
#include "Definitions.h"
#include "QaBase.h"
#include "sts/Unpack.h"

#include <unordered_map>
#include <vector>

namespace cbm::algo::sts
{
  using DigiQaBase = QaBase<CbmStsDigi, UnpackAux<sts::UnpackAuxData>, ReadoutSetup>;

  /// \class cbm::algo::sts::DigiQa
  /// \brief QA module for STS raw digis
  class DigiQa : public DigiQaBase {
   public:
    /// \brief Constructor
    DigiQa(std::shared_ptr<HistogramSender> pSender) : DigiQaBase(pSender, "RawDigi/STS") {}

    /// \brief Default constructor
    DigiQa() = delete;

    /// \brief Copy constructor
    DigiQa(const DigiQa&) = delete;

    /// \brief Move constructor
    DigiQa(DigiQa&&) = delete;

    /// \brief Copy assignment operator
    DigiQa& operator=(const DigiQa&) = delete;

    /// \brief Move assignment operator
    DigiQa& operator=(DigiQa&&) = delete;

    /// \brief Executes QA (filling histograms)
    void Exec();

    /// \brief Initializes QA (initialization of histograms and canvases)
    void Init();

   private:
    std::unordered_map<int32_t, int> fmAddressMap;  ///< Map of address to histogram index

    // ---- Histograms
    std::vector<qa::H1D*> fvphAddressChannel;        ///< hist: digi channel in different sensors
    std::vector<qa::H1D*> fvphAddressCharge;         ///< hist: digi charge in different sensors
    std::vector<qa::H2D*> fvphAddressChannelCharge;  ///< hist: digi channel vs. charge in different sensors
    std::vector<qa::H2D*> fvphAddressChannelElink;   ///< hist: digi channel (vs. eling (AUX)

    std::vector<qa::Prof1D*> fvppAddressChannelMissedEvt;  ///< prof: missed event ratio vs. channel (AUX)
    std::vector<qa::Prof1D*> fvppAddressTimeMissedEvt;     ///< prof: missed event ratio vs. time (AUX)

    qa::H2D* fvphFebAsic = nullptr;  ///< hist: digi FEB vs ASIC
  };
}  // namespace cbm::algo::sts
