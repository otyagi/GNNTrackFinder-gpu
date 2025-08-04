/* Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: P.-A. Loizeau [committer] */

#ifndef ALGO_QA_RECOGENERALQA_H
#define ALGO_QA_RECOGENERALQA_H 1

#include "CbmDefs.h"
#include "HistogramSender.h"
#include "QaData.h"

#include <StorableTimeslice.hpp>

namespace cbm::algo::qa
{
  /** @class RecoQa
   ** @brief General QA for a Reco cycle on a single TS
   ** @author P.-A. Loizeau <p.-a.loizeau@gsi.de>
   ** @since 27 Mai 2024
   **/
  class RecoGeneralQa {
   public:
    /** @brief Constructor **/
    RecoGeneralQa(const uint64_t& runStartTimeNs, std::shared_ptr<HistogramSender> pSender);

    /// \brief Default constructor
    RecoGeneralQa() = delete;

    /// \brief Copy constructor
    RecoGeneralQa(const RecoGeneralQa&) = delete;

    /// \brief Move constructor
    RecoGeneralQa(RecoGeneralQa&&) = delete;

    /// \brief Copy assignment operator
    RecoGeneralQa& operator=(const RecoGeneralQa&) = delete;

    /// \brief Move assignment operator
    RecoGeneralQa& operator=(RecoGeneralQa&&) = delete;

    /** @brief Execution: fill histograms and emit them (FIXME: control emission frequency)
     ** @param Reference to current TS
     ** @return nothing
     **/
    void operator()(const fles::Timeslice& ts);

   private:  // methods
   private:  // members
    uint64_t fRunStartTimeNs;
    std::shared_ptr<HistogramSender> fpSender = nullptr;  ///< Histogram sender
    qa::Data fQaData{"Reco"};                             ///< QA data, with folder named Reco as hist destination
    bool fInitNotDone = true;

    // Constants
    static const int32_t kNbTsPerBinCount = 100;   // 100 TS duration per bin for raw counts
    static const int32_t kNbTsPerBinFrac  = 1000;  // 1000 TS duration per bin for fractions

    // ---- Histograms
    qa::H1D* fphTimeslicesCountEvo    = nullptr;  ///< hist: timeslices vs time in run in s, binned for 100 TS
    qa::H1D* fphTimeslicesFractionEco = nullptr;  ///< hist: fraction of al ts vs time in run in s, binned for 1000 TS
  };
}  // namespace cbm::algo::qa

#endif /* ALGO_QA_RECOGENERALQA_H */
