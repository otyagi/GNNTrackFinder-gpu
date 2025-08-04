/* Copyright (C) 2013-2020 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev [committer], Florian Uhlig */

/**
 * \file CbmLink.cxx
 * \author Andrey Lebedev <andrey.lebedev@gsi.de>
 * \date 2013
 **/
#include "CbmLink.h"

#include <TObject.h>  // for TObject

#include <sstream>  // for operator<<, basic_ostream, stringstream
#include <string>   // for char_traits

using std::stringstream;

CbmLink::CbmLink() : TObject(), fFile(-1), fEntry(-1), fIndex(-1), fWeight(-1.) {}

CbmLink::CbmLink(float weight, int32_t index, int32_t entry, int32_t file)
  : TObject()
  , fFile(file)
  , fEntry(entry)
  , fIndex(index)
  , fWeight(weight)
{
}

CbmLink::~CbmLink() {}

std::string CbmLink::ToString() const
{
  stringstream ss;
  ss << "CbmLink: weight=" << fWeight << " index=" << fIndex << " entry=" << fEntry << " file=" << fFile << "\n";
  return ss.str();
}

ClassImp(CbmLink)
