/* Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// \file   TofHitfindQa.h
/// \brief  A TOF hitfinder QA
/// \since  04.03.2025
/// \author Sergei Zharko <s.zharko@gsi.de>


#pragma once

#include "PODVector.h"
#include "PartitionedVector.h"
#include "qa/QaTaskHeader.h"
#include "qa/hitfind/TofHitfindQaParameters.h"

namespace cbm::algo
{
  namespace qa
  {
    class H1D;
    class H2D;
  }  // namespace qa

  namespace tof
  {
    class Hit;
  }
}  // namespace cbm::algo

namespace cbm::algo::tof
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
    void InitParameters(const HitfindSetup& hitSetup) { fParameters = std::move(HitfindQaParameters(hitSetup)); }

    /// \brief Registers a sample of hits
    /// \param pHits  A pointer to a vector of hits
    void RegisterHits(const PartitionedVector<tof::Hit>* pHits) { fpHits = pHits; }

   private:
    //* Constants
    static constexpr int kHitOccupB{200};       ///< Hit occupancy: n bins
    static constexpr double kHitOccupL{-100.};  ///< Hit occupancy: lower bound [cm]
    static constexpr double kHitOccupU{+100.};  ///< Hit occupancy: upper bound [cm]

    //* Parameters
    HitfindQaParameters fParameters;  ///< Parameters of the hit finder QA

    //* Data samples
    const PartitionedVector<tof::Hit>* fpHits{nullptr};  ///< Pointer to TOF hit sample

    //* Histograms
    std::vector<qa::H1D*> fvphRpcHitOccupX;   ///< Hit occupancy in RPC vs. x
    std::vector<qa::H1D*> fvphRpcHitOccupY;   ///< Hit occupancy in RPC vs. y
    std::vector<qa::H1D*> fvphRpcHitOccupCh;  ///< Hit occupancy in RPC vs. channel
  };
}  // namespace cbm::algo::tof
