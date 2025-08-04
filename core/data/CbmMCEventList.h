/* Copyright (C) 2015-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

/** @file CbmMCEventList.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 24.11.2015
 **/

#ifndef CBMMCEVENTLIST_H
#define CBMMCEVENTLIST_H 1

#include "CbmLink.h"
#include "CbmMCEventInfo.h"  // for CbmMCEventInfo

#include <Rtypes.h>  // for THashConsistencyHolder, ClassDef
#include <TNamed.h>  // for TNamed

#include <cstddef>  // for size_t
#include <cstdint>
#include <string>  // for string
#include <vector>  // for vector, vector<>::iterator

/* Implementation note (VF/180618):
 * Both indexed access (for loops over all MC events in a time slice)
 * and random access (to get the event time for a given fileId and EventId)
 * are required. It was thus chosen to internally represent the event list
 * as a sortable vector of CbmMCEventInfo. I tried to use std::tuple instead
 * of CbmMCEventInfo, but ROOT seems not to be able to stream that.
 * Since it is assumed that the creation of the list is separate from the
 * access to the list, no sorting or checking for double occurrences of events
 * is done on insertion for performance reasons.
 * Sorting will be done on first access to the list. It includes checking of
 * double occurrences of (fileId, eventId).
 */

/** @class CbmMCEventList
 ** @brief Container class for MC events with number, file and start time
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 24.11.2015
 ** @version 18.06.2018
 **/
class CbmMCEventList : public TNamed {

public:
  /** @brief Standard constructor **/
  CbmMCEventList();


  /** @brief Destructor **/
  virtual ~CbmMCEventList();


  /** @brief Delete all event entries **/
  virtual void Clear(Option_t*) { fEvents.clear(); }


  /** @brief Event number by index
     ** @value Event number for event at given index in list
     **
     ** Returns -1 if the index is out of bounds.
     **/
  int32_t GetEventIdByIndex(uint32_t index);


  /** @brief Event start time
     ** @param event  MC event number
     ** @param file   MC input file number
     ** @value MC event start time [ns]
     **
     ** Returns -1. if the event is not present in the list.
     **/
  double GetEventTime(uint32_t event, uint32_t file);

  /** @brief Event start time
   * * @param link link to the MC event
   **/
  Double_t GetEventTime(const CbmLink& link) { return GetEventTime(link.GetEntry(), link.GetFile()); }

  /** @brief Event time by index
     ** @value Event time for event at given index in list
     **
     ** Returns -1. if the index is out of bounds.
     **/
  double GetEventTimeByIndex(uint32_t index);


  /** @brief File number by index
     ** @value File number for event at given index in list
     **
     ** Returns -1 if the index is out of bounds.
     **/
  int32_t GetFileIdByIndex(uint32_t index);

  /** @brief Event index
     ** @param event  MC event number
     ** @param file   MC input file number
     ** @value eventindex in list
     **
     ** Returns -1. if the event is not present in the list.
     **/
  Int_t GetEventIndex(UInt_t event, UInt_t file);

  /** @brief Event index
     **/
  Int_t GetEventIndex(const CbmLink& link) { return GetEventIndex(link.GetEntry(), link.GetFile()); }


  /** @brief Event file and event indices as CbmLink
     **/
  CbmLink GetEventLinkByIndex(uint32_t index);

  /** @brief Number of events in the list
     ** @value Number of events
     **/
  std::size_t GetNofEvents() const { return fEvents.size(); }


  /** Insert an event with its start time into the event list
     ** @param event  MC event number
     ** @param file   MC input file number
     ** @param time   MC event start time [ns]
     ** @value false if (file, event) is already in the list. Else true.
     **
     ** If the event from the given file is already in the list, the list
     ** will not be modified and false is returned.
     **
     ** Negative event times are not allowed. In that case, false is
     ** returned and the list will not be modified.
     */
  bool Insert(uint32_t event, uint32_t file, double time);


  /** Print to screen **/
  virtual void Print(Option_t* opt = "") const;


  /** @brief Sort the list
     **
     ** The sorting uses the comparison operator of std::tuple, i.e.
     ** the list is first sorted w.r.t. fileId, then w.r.t. the event Id.
     ** Double occurrences of a pair (fileId, eventId) will throw an
     ** exception.
     */
  void Sort();


  /** Status to string **/
  std::string ToString(const char* option = "") const;


private:
  /** Event container **/
  std::vector<CbmMCEventInfo> fEvents;

  /** Flag whether list has been sorted **/
  bool fIsSorted;


  /** @brief Check for double occurrences of events in list
     ** @value true is no double occurrences, else false
     **/
  bool Check();


  /** @brief Find an element in the list
     ** @param file  Input file ID
     ** @param event MC event number (index)
     **/
  std::vector<CbmMCEventInfo>::iterator Find(uint32_t file, uint32_t event);


  ClassDef(CbmMCEventList, 3);
};


#endif /* CBMMCEVENTLIST_H */
