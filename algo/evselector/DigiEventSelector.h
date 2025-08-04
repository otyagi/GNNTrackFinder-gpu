/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Shreya Roy [committer], Pierre-Alain Loizeau, Norbert Herrmann, Volker Friese, Dominik Smith */

#ifndef CBM_ALGO_DIGIEVENTSELECTOR_H
#define CBM_ALGO_DIGIEVENTSELECTOR_H 1

#include "DigiData.h"
#include "DigiEventSelectorConfig.h"
#include "TrackingSetup.h"

#include <cstdint>
#include <gsl/span>
#include <map>

namespace cbm::algo::evbuild
{

  /** @struct DigiEventSelectorParams
   ** @author Dominik Smith <d.smith@ gsi.de>
   ** @author Shreya Roy <s.roy@gsi.de>
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 26.01.2023
   */
  struct DigiEventSelectorParams {
    std::map<ECbmModuleId, size_t> fMinNumDigis  = {};
    std::map<ECbmModuleId, size_t> fMinNumLayers = {};
  };


  /** @class DigiEventSelector
   ** @author Dominik Smith <d.smith@ gsi.de>
   ** @author Shreya Roy <s.roy@gsi.de>
   ** @author Volker Friese <v.friese@gsi.de>
   ** @since 26.01.2023
   **
   ** @brief Algorithm to select CbmDigiEvents based on the number of digis and the number of
   ** activated layers in each detector system.
   **/
  class DigiEventSelector {

   public:
    /** @brief Constructor with configuration **/
    DigiEventSelector(DigiEventSelectorConfig config) : fConfig(config){};

    /** @brief Test one event for the selection criteria
     ** @param event DigiEvent
     ** @return true if event satisfies the criteria; else false
     **/
    bool operator()(const DigiEvent& event) const;

    /** @brief Registers tracking setup
     ** @param pSetup  The tracking setup instance
     **/
    void RegisterTrackingSetup(std::shared_ptr<TrackingSetup> pSetup) { fpTrackingSetup = pSetup; }

    /** @brief Info to string **/
    std::string ToString() const;


   private:  // methods
    /** @brief Test for the number of STS stations
     ** @param digis Vector of STS digis
     ** @param minNum Requested minimum of active STS stations
     ** @return True if the number of active STS layers is above the threshold
     **/
    bool CheckStsStations(gsl::span<const CbmStsDigi> digis, size_t minNum) const;

    /** @brief Test for the number of TOF layers
     ** @param digis Vector of TOF digis
     ** @param minNum Requested minimum of active TOF layers
     ** @return True if the number of active TOF layers is above the threshold
     **/
    bool CheckTofLayers(gsl::span<const CbmTofDigi> digis, size_t minNum) const;


   private:                           // members
    DigiEventSelectorConfig fConfig;  ///< Configuration / parameters
    std::shared_ptr<TrackingSetup> fpTrackingSetup = nullptr;  ///< Tracking setup (access to stations info)
  };

}  // namespace cbm::algo::evbuild

#endif /* CBM_ALGO_DIGIEVENTSELECTOR_H */
