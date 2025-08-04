/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file TaskFactory.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 06.06.2023
 **/

#ifndef CBM_RECO_OFFLINE_STEER_TASKFACTORY_H
#define CBM_RECO_OFFLINE_STEER_TASKFACTORY_H 1

#include "Run.h"

namespace cbm::reco::offline
{

  /** @class TaskFactory
   ** @brief Factory class for the instantiation of CBM reconstruction tasks
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 6 June 2023
   **/
  class TaskFactory {
   public:
    /** @brief Constructor **/
    TaskFactory(Run* steer = nullptr);

    /** @brief Destructor **/
    virtual ~TaskFactory(){};

    void RegisterCaTracking();         /// CA track finding
    void RegisterDigiEventBuilder();   /// Event building from digis
    void RegisterGlobalTracking();     /// Global track finding
    void RegisterMuchReco();           /// Local reconstruction for MUCH
    void RegisterMvdReco();            /// Local reconstruction for MVD
    void RegisterPvFinder();           /// Primary vertex finding
    void RegisterRichHitFinder();      /// Hit finding in RICH
    void RegisterRichReco();           /// Local reconstruction for RICH
    void RegisterStsReco();            /// Local reconstruction for STS
    void RegisterTofReco();            /// Local reconstruction for TOF
    void RegisterPsdReco();            /// Local reconstruction for PSD
    void RegisterFsdReco();            /// Local reconstruction for FSD
    void RegisterTrackEventBuilder();  /// Event building from tracks
    void RegisterTrdReco();            /// Local reconstruction for TRD
    void RegisterTrdPid();             /// PID with TRD
    void RegisterBmonReco();           /// Reconstruction of Bmon

   private:  //members
    Run* fRun = nullptr;
  };

}  // namespace cbm::reco::offline

#endif /* CBM_RECO_OFFLINE_STEER_TASKFACTORY_H */
