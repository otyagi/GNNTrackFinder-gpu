/* Copyright (C) 2023 FIAS Frankfurt Institute for Advanced Studies, Frankfurt / Main
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Felix Weiglhofer [committer] */
#include "DigiData.h"

using namespace cbm::algo;

// Ctor / Dtor defined in .cxx file so we can use forward declarations for digi types in the header

DigiData::DigiData() {}

DigiData::~DigiData() {}

DigiData::DigiData(const CbmDigiData& storable)
  : fSts(ToPODVector(storable.fSts.fDigis))
  , fMuch(ToPODVector(storable.fMuch.fDigis))
  , fTof(ToPODVector(storable.fTof.fDigis))
  , fBmon(ToPODVector(storable.fBmon.fDigis))
  , fTrd(ToPODVector(storable.fTrd.fDigis))
  , fTrd2d(ToPODVector(storable.fTrd2d.fDigis))
  , fRich(ToPODVector(storable.fRich.fDigis))
  , fPsd(ToPODVector(storable.fPsd.fDigis))
  , fFsd(ToPODVector(storable.fFsd.fDigis))
{
}

size_t DigiData::Size(ECbmModuleId system) const
{
  switch (system) {
    case ECbmModuleId::kSts: return fSts.size();
    case ECbmModuleId::kMuch: return fMuch.size();
    case ECbmModuleId::kTof: return fTof.size();
    case ECbmModuleId::kBmon: return fBmon.size();
    case ECbmModuleId::kTrd: return fTrd.size();
    case ECbmModuleId::kTrd2d: return fTrd2d.size();
    case ECbmModuleId::kRich: return fRich.size();
    case ECbmModuleId::kPsd: return fPsd.size();
    case ECbmModuleId::kFsd: return fFsd.size();
    default: throw std::runtime_error("DigiData: Invalid system Id " + ::ToString(system));
  }
}

size_t DigiData::TotalSize() const
{
  return fSts.size() + fMuch.size() + fTof.size() + fBmon.size() + fTrd.size() + fTrd2d.size() + fRich.size()
         + fPsd.size() + fFsd.size();
}

size_t DigiData::TotalSizeBytes() const
{
  return sizeof(CbmStsDigi) * fSts.size() + sizeof(CbmMuchDigi) * fMuch.size() + sizeof(CbmTofDigi) * fTof.size()
         + sizeof(CbmBmonDigi) * fBmon.size() + sizeof(CbmTrdDigi) * fTrd.size() + sizeof(CbmTrdDigi) * fTrd2d.size()
         + sizeof(CbmRichDigi) * fRich.size() + sizeof(CbmPsdDigi) * fPsd.size() + sizeof(CbmFsdDigi) * fFsd.size();
}

CbmDigiData DigiData::ToStorable() const
{
  return CbmDigiData{
    .fBmon =
      {
        .fDigis = ToStdVector(fBmon),
      },
    .fSts =
      {
        .fDigis = ToStdVector(fSts),
      },
    .fMuch =
      {
        .fDigis = ToStdVector(fMuch),
      },
    .fRich =
      {
        .fDigis = ToStdVector(fRich),
      },
    .fTrd =
      {
        .fDigis = ToStdVector(fTrd),
      },
    .fTrd2d =
      {
        .fDigis = ToStdVector(fTrd2d),
      },
    .fTof =
      {
        .fDigis = ToStdVector(fTof),
      },
    .fPsd =
      {
        .fDigis = ToStdVector(fPsd),
      },
    .fFsd =
      {
        .fDigis = ToStdVector(fFsd),
      },
  };
}

std::vector<DigiEvent> DigiEvent::FromCbmDigiEvents(const std::vector<CbmDigiEvent>& events)
{
  std::vector<DigiEvent> result;
  result.reserve(events.size());
  for (const auto& event : events) {
    result.emplace_back(event);
  }
  return result;
}

std::vector<CbmDigiEvent> DigiEvent::ToCbmDigiEvents(const std::vector<DigiEvent>& events)
{
  std::vector<CbmDigiEvent> result;
  result.reserve(events.size());
  for (const auto& event : events) {
    result.emplace_back(event.ToStorable());
  }
  return result;
}

DigiEvent::DigiEvent(const CbmDigiEvent& storable)
  : DigiData(storable.fData)
  , fNumber(storable.fNumber)
  , fTime(storable.fTime)
  , fSelectionTriggers(storable.fSelectionTriggers)
{
}

CbmDigiEvent DigiEvent::ToStorable() const
{
  return CbmDigiEvent{
    .fData              = DigiData::ToStorable(),
    .fNumber            = fNumber,
    .fTime              = fTime,
    .fSelectionTriggers = fSelectionTriggers,
  };
}
