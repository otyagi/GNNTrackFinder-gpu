/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   BmonHitfindQa.h
/// \brief  A BMON hitfinder QA
/// \since  07.02.2025
/// \author Sergei Zharko <s.zharko@gsi.de>

#pragma once

#include "PODVector.h"
#include "PartitionedVector.h"
#include "qa/QaTaskHeader.h"
#include "qa/hitfind/BmonHitfindQaParameters.h"

class CbmBmonDigi;

namespace cbm::algo
{
  namespace qa
  {
    class H1D;
    class H2D;
  }  // namespace qa

  namespace bmon
  {
    class Hit;
  }
}  // namespace cbm::algo

namespace cbm::algo::bmon
{
  /// \class HitfindQa
  /// \brief A QA module for the BMON hit-finder
  class HitfindQa : public qa::TaskHeader {
   public:
    /// \brief Constructor
    /// \param pManager  Pointer to the QA manager
    /// \param name      Name of the QA (directory)
    HitfindQa(const std::unique_ptr<qa::Manager>& pManager, std::string_view name) : qa::TaskHeader(pManager, name) {}

    /// \brief Constructor from the configuration object
    HitfindQa() = default;

    /// \brief Copy constructor
    HitfindQa(const HitfindQa&) = delete;

    /// \brief Move constructor
    HitfindQa(HitfindQa&&) = delete;

    /// \brief Destructor
    ~HitfindQa() = default;

    /// \brief Copy assignment operator
    HitfindQa& operator=(const HitfindQa&) = delete;

    /// \brief Move assignment operator
    HitfindQa& operator=(HitfindQa&&) = delete;

    /// \brief Executes the task, fills the histograms
    void Exec();

    /// \brief Initialized the task
    void Init();

    /// \brief Initialisation of the parameters
    void InitParameters(const CalibrateSetup& calSetup, const HitfindSetup& hitSetup)
    {
      fParameters = std::move(HitfindQaParameters(calSetup, hitSetup));
    }

    /// \brief Registers a sample of digis
    /// \param pDigis  A pointer to a vector of digis
    void RegisterDigis(const std::vector<CbmBmonDigi>* pDigis) { fpDigis = pDigis; }

    /// \brief Registers a sample of hits
    /// \param pHits  A pointer to a vector of hits
    void RegisterHits(const PartitionedVector<bmon::Hit>* pHits) { fpHits = pHits; }

    /// \brief Registers a sample of digi indices, used by hits
    /// \param pDigiIndices  A pointer to a vector of digi indices
    void RegisterDigiIndices(const PODVector<int32_t>* pDigiIndices) { fpDigiIndices = pDigiIndices; }

   private:
    //* Constants
    static constexpr int kChrgB     = 150;   ///< charge scale: number of bins
    static constexpr double kChrgL  = 0.;    ///< charge scale: lower bound
    static constexpr double kChrgU  = 150.;  ///< charge scale: upper bound
    static constexpr int kDtimeB    = 40;    ///< digi time difference: number of bins
    static constexpr double kDtimeL = 0.;    ///< digi time difference: lower bound [ns]
    static constexpr double kDtimeU = 2.0;   ///< digi time difference: upper bound [ns]

    //* Parameters
    HitfindQaParameters fParameters;  ///< Parameters of the hit finder QA

    //* Data samples
    const std::vector<CbmBmonDigi>* fpDigis{nullptr};     ///< Pointer to BMON digi sample
    const PartitionedVector<bmon::Hit>* fpHits{nullptr};  ///< Pointer to BMON hit sample
    const PODVector<int32_t>* fpDigiIndices{nullptr};     ///< Pointer to BMON digi indices, used by hits

    //* Histograms
    std::vector<qa::H1D*> fvphDigiOccupVsChan;   ///< Digi occupancy vs. channel [diamond]
    std::vector<qa::H2D*> fvphDigiChargeVsChan;  ///< Digi charge vs channel [diamond]
    std::vector<qa::H1D*> fvphHitNofChan;        ///< Hit  number of channels [diamond]
    std::vector<qa::H1D*> fvphHitTimeDiff;       ///< Time difference of two digis in a hit [diamond]
  };
}  // namespace cbm::algo::bmon
