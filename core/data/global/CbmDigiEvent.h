/* Copyright (C) 2021-25 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

#ifndef CBMDIGIEVENT_H
#define CBMDIGIEVENT_H 1

#include "CbmDigiData.h"
#include "CbmEventTriggers.h"

#include <boost/serialization/access.hpp>


/** @class CbmDigiEvent
 ** @brief Collection of digis from all detector systems within one event
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 7.12.2022
 ** @version 3.0
 **/
class CbmDigiEvent {

public:
 CbmDigiData fData;                    ///< Event data
 uint64_t fNumber;                     ///< Event identifier
 double fTime;                         ///< Event trigger time [ns]
 CbmEventTriggers fSelectionTriggers;  ///< Event selection triggers

 friend class boost::serialization::access;
 /** @brief BOOST serializer**/
 template<class Archive>
 void serialize(Archive& ar, const unsigned int version)
 {
   ar& fData;
   ar& fNumber;
   ar& fTime;
   if (version >= 3) {
     ar& fSelectionTriggers;
   }
 }

  // --- ROOT serializer
#ifndef NO_ROOT
 ClassDefNV(CbmDigiEvent, 3);
#endif

  /** @brief Clear content **/
  void Clear()
  {
    fData.Clear();
    fNumber = 0;
    fTime   = 0.;
    fSelectionTriggers.ResetAll();
  }
};

#endif /* CBMDIGIEVENT_H */
