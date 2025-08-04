/* Copyright (C) 2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer], Florian Uhlig */

/** CbmEventStore.cxx
 ** @author V.Friese <v.friese@gsi.de>
 ** @since 11.03.2020
 ** @version 1.0
 **/
#include "CbmEventStore.h"

#include "CbmLink.h"        // for CbmLink
#include "CbmMatch.h"       // for CbmMatch
#include "CbmModuleList.h"  // for CbmModuleList

#include <iostream>  // for operator<<, basic_ostream, stringstream


// -----   Constructor   ---------------------------------------------------
CbmEventStore::CbmEventStore(uint32_t eventId, bool hasMatches) : fEventId(eventId), fHasMatches(hasMatches)
{
  //  fDigis = new TObjArray(ToIntegralType(ECbmModuleId::kNofSystems));
}
// -------------------------------------------------------------------------


// -----   Copy constructor   ----------------------------------------------
CbmEventStore::CbmEventStore(const CbmEventStore& other) : TObject(other)
{
  fEventId    = other.fEventId;
  fHasMatches = other.fHasMatches;
  // TODO: Create new map with a copy of the original map
  // Don't know how to do that yet
  //  fDigis = (TObjArray*)other.fDigis->Clone();
}
// -------------------------------------------------------------------------


// -----   Destructor   ----------------------------------------------------
CbmEventStore::~CbmEventStore()
{
  for (auto system : fDigis) {
    delete system.second;
  }
  //  fDigis->Delete();
}
// -------------------------------------------------------------------------


// -----   Test for being empty   ------------------------------------------
bool CbmEventStore::IsEmpty() const
{
  uint32_t nDigis = 0;
  for (auto system : fDigis) {
    auto* digis = dynamic_cast<CbmDigiContainer*>(system.second);
    nDigis += digis->GetNofDigis();
  }

  /*
for ( int32_t system = 0; system < fDigis->GetEntriesFast(); system++) {
    auto* digis = dynamic_cast<CbmDigiContainer*>(fDigis->At(system));
    if ( digis ) nDigis += digis->GetNofDigis();
  }
*/
  return (nDigis > 0 ? false : true);
}
// -------------------------------------------------------------------------


// -----   Get number of data for a given system   -------------------------
uint32_t CbmEventStore::GetNofDigis(ECbmModuleId system) const
{
  if (system >= ECbmModuleId::kNofSystems) return 0;
  auto* digis = dynamic_cast<CbmDigiContainer*>(fDigis.at(system));
  if (!digis) return 0;
  return digis->GetNofDigis();
}
// -------------------------------------------------------------------------


// -----   Match to MC event   ---------------------------------------------
void CbmEventStore::MatchToMC(CbmMatch& result) const
{
  result.ClearLinks();
  if (!fHasMatches) return;
  for (auto system : fDigis) {
    auto* digis = dynamic_cast<CbmDigiContainer*>(system.second);
    if (!digis) continue;
    for (uint32_t index = 0; index < digis->GetNofDigis(); index++) {
      const CbmMatch* match = digis->GetDigiMatch(index);
      assert(match);
      for (int32_t iLink = 0; iLink < match->GetNofLinks(); iLink++) {
        const CbmLink& link = match->GetLink(iLink);
        result.AddLink(link.GetWeight(), 0, link.GetEntry(), link.GetFile());
      }  //# Links in match
    }    //# Matches in system
  }      //# Systems


  /*
  for (int32_t system = 0; system < fDigis->GetEntriesFast(); system++) {
    auto* digis = dynamic_cast<CbmDigiContainer*>(fDigis->At(system));
    if ( ! digis ) continue;
    for (uint32_t index = 0; index < digis->GetNofDigis(); index++) {
      const CbmMatch* match = digis->GetDigiMatch(index);
      assert(match);
      for (int32_t iLink = 0; iLink < match->GetNofLinks(); iLink++) {
        const CbmLink& link = match->GetLink(iLink);
        result.AddLink(link.GetWeight(), 0, link.GetEntry(), link.GetFile());
      } //# Links in match
    } //# Matches in system
  } //# Systems
*/
}
// -------------------------------------------------------------------------


// -----   String output   -------------------------------------------------
std::string CbmEventStore::ToString() const
{
  std::stringstream ss;
  ss << "Event " << fEventId;
  if (IsEmpty()) ss << " empty";
  else {
    ss << " Data: ";
    for (auto system : fDigis) {
      auto* vec = dynamic_cast<CbmDigiContainer*>(system.second);
      //     for ( int32_t system = 0; system < fDigis->GetEntriesFast(); system++) {
      //       if ( fDigis->At(system) ) {
      //         auto vec = static_cast<CbmDigiContainer*>(fDigis->At(system));
      assert(vec);
      ss << CbmModuleList::GetModuleNameCaps(system.first) << " " << vec->GetNofDigis() << " ";
      //       } //? Digi vector present
    }  //# Systems
  }    //? Not empty

  if (fHasMatches) ss << ", matches present";
  return ss.str();
}
// -------------------------------------------------------------------------


ClassImp(CbmEventStore)
