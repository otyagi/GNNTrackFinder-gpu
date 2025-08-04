/* Copyright (C) 2021-2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */


#ifndef CBMDIGIDATA_H
#define CBMDIGIDATA_H 1

#include "CbmBmonDigiData.h"
#include "CbmFsdDigiData.h"
#include "CbmMuchDigiData.h"
#include "CbmPsdDigiData.h"
#include "CbmRichDigiData.h"
#include "CbmStsDigiData.h"
#include "CbmTofDigiData.h"
#include "CbmTrdDigiData.h"

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#ifndef NO_ROOT
#include <Rtypes.h>  // for ClassDef
#endif


/** @class CbmDigiData
 ** @brief Collection of digis from all detector systems
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 7.12.2022
 ** @version 2.0
 **/
class CbmDigiData {

 public:
  CbmBmonDigiData fBmon;  ///< Beam monitor data
  CbmStsDigiData fSts;    ///< STS data
  CbmMuchDigiData fMuch;  ///< MUCH data
  CbmRichDigiData fRich;  ///< RICH data
  CbmTrdDigiData fTrd;    ///< TRD data
  CbmTrdDigiData fTrd2d;  ///< TRD2D data
  CbmTofDigiData fTof;    ///< TOF data
  CbmPsdDigiData fPsd;    ///< PSD data
  CbmFsdDigiData fFsd;    ///< FSD data

  friend class boost::serialization::access;
  /** @brief BOOST serializer**/
  template<class Archive>
  void serialize(Archive& ar, const unsigned int /*version*/)
  {
    // note, version is always the latest when saving
    ar& fBmon;
    ar& fSts;
    ar& fMuch;
    ar& fTrd;
    ar& fTrd2d;
    ar& fTof;
    ar& fPsd;
    ar& fFsd;
    ar& fRich;
  }

  // --- ROOT serializer
#ifndef NO_ROOT
  ClassDefNV(CbmDigiData, 5);
#endif

  /** @brief Clear content **/
  void Clear()
  {
    fBmon.Clear();
    fSts.Clear();
    fMuch.Clear();
    fTrd.Clear();
    fTrd2d.Clear();
    fTof.Clear();
    fPsd.Clear();
    fFsd.Clear();
    fRich.Clear();
  }

  /** @brief Size of detector data **/
  size_t Size(ECbmModuleId system) const
  {
    switch (system) {
      case ECbmModuleId::kBmon: return fBmon.Size(); break;
      case ECbmModuleId::kSts: return fSts.Size(); break;
      case ECbmModuleId::kMuch: return fMuch.Size(); break;
      case ECbmModuleId::kTrd: return fTrd.Size(); break;
      case ECbmModuleId::kTrd2d: return fTrd2d.Size(); break;
      case ECbmModuleId::kTof: return fTof.Size(); break;
      case ECbmModuleId::kPsd: return fPsd.Size(); break;
      case ECbmModuleId::kRich: return fRich.Size(); break;
      case ECbmModuleId::kFsd: return fFsd.Size(); break;
      default: return 0; break;
    }
  }

  /** @brief Return total size in bytes */
  size_t SizeBytes() const
  {
    size_t size = 0;
    size += fBmon.Size() * sizeof(CbmBmonDigi);
    size += fSts.Size() * sizeof(CbmStsDigi);
    size += fMuch.Size() * sizeof(CbmMuchDigi);
    size += fTrd.Size() * sizeof(CbmTrdDigi);
    size += fTrd2d.Size() * sizeof(CbmTrdDigi);
    size += fTof.Size() * sizeof(CbmTofDigi);
    size += fPsd.Size() * sizeof(CbmPsdDigi);
    size += fFsd.Size() * sizeof(CbmFsdDigi);
    size += fRich.Size() * sizeof(CbmRichDigi);
    return size;
  }
};

BOOST_CLASS_VERSION(CbmDigiData, 5)
#endif /* CBMDIGIDATA_H */
