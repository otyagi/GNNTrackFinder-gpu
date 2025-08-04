/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergei Zharko [committer] */

/// @file   CbmCaIdealHitProducerDetBase.h
/// @brief  A FairTask to run ideal hit producers for CA tracking purposes (header)
/// @author S.Zharko<s.zharko@gsi.de>
/// @since  01.06.2023

#ifndef CbmCaIdealHitProducer_h
#define CbmCaIdealHitProducer_h 1

#include "CbmCaIdealHitProducerDet.h"
#include "CbmL1DetectorID.h"
#include "FairTask.h"

#include <string>

namespace cbm::ca
{
  /// @brief Ideal hit producer task for CA tracking
  class IdealHitProducer : public FairTask {
   public:
    /// @brief Constructor
    /// @param name     Name of the task
    /// @param verbose  Verbosity level
    IdealHitProducer(const char* name, int verbose) : FairTask(name, verbose) {}

    /// @brief Destructor
    ~IdealHitProducer() = default;

    /// @brief Copy constructor
    IdealHitProducer(const IdealHitProducer&) = delete;

    /// @brief Move constructor
    IdealHitProducer(IdealHitProducer&&) = delete;

    /// @brief Copy assignment operator
    IdealHitProducer& operator=(const IdealHitProducer&) = delete;

    /// @brief Move assignment operator
    IdealHitProducer& operator=(IdealHitProducer&&) = delete;

    /// @brief Initialization of the task
    InitStatus Init();

    /// @brief Re-initialization of the task
    InitStatus ReInit() { return Init(); }

    /// @brief Execution of the task
    void Exec(Option_t* option);

    /// @brief Sets YAML configuration file with defined smearing parameters
    /// @param name  Name of the configuration file
    void SetConfigName(const char* name);

    ClassDef(IdealHitProducer, 1);

   private:
    IdealHitProducerDet<ca::EDetectorID::kMvd> fHitProducerMvd;    ///< Instance of hit producer for MVD
    IdealHitProducerDet<ca::EDetectorID::kSts> fHitProducerSts;    ///< Instance of hit producer for STS
    IdealHitProducerDet<ca::EDetectorID::kMuch> fHitProducerMuch;  ///< Instance of hit producer for MuCh
    IdealHitProducerDet<ca::EDetectorID::kTrd> fHitProducerTrd;    ///< Instance of hit producer for TRD
    IdealHitProducerDet<ca::EDetectorID::kTof> fHitProducerTof;    ///< Instance of hit producer for TOF

    DetIdArr_t<bool> fbUseDet = {{false}};  ///< Usage flag of different detectors
  };

}  // namespace cbm::ca


#endif  // CbmCaIdealhitProducer_h
