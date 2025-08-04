/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   QaBase.h
/// \date   03.03.2024
/// \brief  Base class for digi QA (header)
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "Definitions.h"
#include "PODVector.h"
#include "QaData.h"

#include <string>

namespace cbm::algo::sts
{
  /// \class  cbm::algo::sts::QaBase
  /// \brief  QA module for STS raw digis
  /// \tparam Digi          A digi class for a given detector subsystem
  /// \tparam ReadoutSetup  A read-out config for a given detector subsystem
  /// \tparam AuxData       Auxilary information on digis, stored for each micro timeslice
  template<class Digi, class AuxData, class ReadoutSetup>
  class QaBase {
   public:
    /// \brief Constructor
    /// \param pSender  Pointer to histogram sender
    /// \param dirname
    QaBase(std::shared_ptr<HistogramSender> pSender, const std::string& dirname) : fQaData(dirname), fpSender(pSender)
    {
    }

    /// \brief Default constructor
    QaBase() = delete;

    /// \brief Copy constructor
    QaBase(const QaBase&) = delete;

    /// \brief Move constructor
    QaBase(QaBase&&) = delete;

    /// \brief Copy assignment operator
    QaBase& operator=(const QaBase&) = delete;

    /// \brief Move assignment operator
    QaBase& operator=(QaBase&&) = delete;

    /// \brief Checks, if the histogram sender is defined
    bool IsSenderDefined() const { return static_cast<bool>(fpSender.get()); }

    /// \brief Register digi-qa data
    void RegisterDigiData(const PODVector<Digi>* pvDigis) { fpvDigis = pvDigis; }

    /// \brief Register auxiliary digi data
    void RegisterAuxDigiData(const AuxData* pAuxDigis)
    {
      if (fbAux) {
        fpAuxDigis = pAuxDigis;
      }
      else {
        fpAuxDigis = nullptr;
      }
    }

    /// \brief Register read-out setup config
    void RegisterReadoutSetup(const ReadoutSetup& setup) { fpReadoutSetup = std::make_shared<ReadoutSetup>(setup); }

    /// \brief Sets usage of auxiliary data
    void SetUseAuxData(bool bAux = true) { fbAux = bAux; }

    /// \brief Sets timeslice index
    void SetTimesliceIndex(uint64_t tsIndex) { fQaData.SetTimesliceId(tsIndex); }

   protected:
    qa::Data fQaData;                                        ///< QA data
    std::shared_ptr<HistogramSender> fpSender    = nullptr;  ///< Histogram sender
    std::shared_ptr<ReadoutSetup> fpReadoutSetup = nullptr;  ///< Readout config instance
    const PODVector<Digi>* fpvDigis              = nullptr;  ///< Digis input
    const AuxData* fpAuxDigis                    = nullptr;  ///< Aux information on digis
    bool fbAux = false;  ///< Extra distributions (if the auxiliary data should be used)
  };
}  // namespace cbm::algo::sts
