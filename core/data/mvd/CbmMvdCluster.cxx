/* Copyright (C) 2008-2017 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann, Christina Dritsa [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                CbmMvdCluster source file                  -----

// -------------------------------------------------------------------------

#include "CbmMvdCluster.h"

using std::map;
using std::pair;

// -----   Default constructor   -------------------------------------------
CbmMvdCluster::CbmMvdCluster() : CbmCluster(), fPixelMap(), fRefId(-1), fClusterCharge(0) { fEarliestFrameNumber = -1; }
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
CbmMvdCluster::CbmMvdCluster(const CbmMvdCluster& rhs) : CbmCluster(), fPixelMap(), fRefId(-1), fClusterCharge(0)
{
  fPixelMap            = rhs.fPixelMap;
  fRefId               = rhs.fRefId;
  fClusterCharge       = rhs.fClusterCharge;
  fEarliestFrameNumber = rhs.fEarliestFrameNumber;
}
// -------------------------------------------------------------------------

// -----   Destructor   ----------------------------------------------------
CbmMvdCluster::~CbmMvdCluster() {}
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
void CbmMvdCluster::SetPixelMap(map<pair<int32_t, int32_t>, int32_t> PixelMap)
{
  fPixelMap = PixelMap;
  for (map<pair<int32_t, int32_t>, int32_t>::iterator iter = fPixelMap.begin(); iter != fPixelMap.end(); iter++)
    fClusterCharge += iter->second;
}

ClassImp(CbmMvdCluster)
