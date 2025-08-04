/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Lukas Chlad [committer] */

#ifndef CBMFSDDIGIDATA_H
#define CBMFSDDIGIDATA_H 1


#include "CbmFsdDigi.h"

#ifndef NO_ROOT
#include <Rtypes.h>  // for ClassDef
#endif

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include <vector>

/** @class CbmFsdDigiData
 ** @brief Container class for CbmFsdDigi objects
 ** @author Volker Friese <v.friese@gsi.de>
 ** @since 7.12.2022
 ** @version 1.0
 **
 ** Container class for transporting CbmFsdDigi objects.
 ** The current implementation is the simplest one: a std::vector.
 **/
class CbmFsdDigiData {

public:
  std::vector<CbmFsdDigi> fDigis = {};  ///< Data vector

  friend class boost::serialization::access;

  /** @brief BOOST serializer**/
  template<class Archive>
  void serialize(Archive& ar, const unsigned int /*version*/)
  {
    ar& fDigis;
  }

  /** @brief Clear content **/
  void Clear() { fDigis.clear(); }

  /** @brief Size */
  size_t Size() const { return fDigis.size(); }


  // --- ROOT serializer
#ifndef NO_ROOT
  ClassDefNV(CbmFsdDigiData, 1);
#endif
};

#endif
