/* Copyright (C) 2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Frederic Linz [committer], Volker Friese */

/** @file TaskFactory.h
 ** @author Frederic Linz <f.linz@gsi.de>
 ** @date 30.10.2023
 **/

#ifndef CBM_ATCONVERTER_STEER_TASKFACTORY_H
#define CBM_ATCONVERTER_STEER_TASKFACTORY_H 1

#include "Run.h"

namespace cbm::atconverter
{

  /** @class TaskFactory
   ** @brief Factory class for the instantiation of CBM analysis tree converter tasks
   ** @author Frederic Linz <f.linz@gsi.de>
   ** @since 30 October 2023
   **/
  class TaskFactory {
   public:
    /** @brief Constructor **/
    TaskFactory(Run* steer = nullptr);

    /** @brief Destructor **/
    virtual ~TaskFactory(){};

    /** @brief MC data manager for matching
     ** @param traFile  name of transport file
     **/
    void RegisterMCDataManager(const std::vector<TString>& traFiles);

    /** @brief AnalysisTree Converter Manager
     ** @param outputFile  name of AT file
     **/
    void RegisterConverterManager(const TString& outputFile);

    void RegisterCaTracking();     /// CA track finding
    void RegisterTrackMatching();  /// STS track matching
    void RegisterTrdPid();         /// PID with TRD

   private:  //members
    Run* fRun        = nullptr;
  };

}  // namespace cbm::atconverter

#endif /* CBM_ATCONVERTER_STEER_TASKFACTORY_H */
