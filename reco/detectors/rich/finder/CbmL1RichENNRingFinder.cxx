/* Copyright (C) 2006-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel, Sergey Gorbunov, Denis Bertini [committer], Semen Lebedev, Igor Kulakov */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction 
 *  
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de 
 *
 *====================================================================
 *
 *  Standalone RICH ring finder based on the Elastic Neural Net
 *
 *====================================================================
 */


// CBM includes
#include "CbmL1RichENNRingFinder.h"

#include "CbmEvent.h"
#include "CbmL1RichENNRingFinderParallel.h"
#include "CbmRichHit.h"
#include "CbmRichRing.h"
#include "TClonesArray.h"
#include "TStopwatch.h"
#include "assert.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;
using std::fabs;
using std::ios;
using std::sqrt;
using std::vector;


CbmL1RichENNRingFinder::CbmL1RichENNRingFinder(Int_t verbose)
  : finder(new CbmL1RichENNRingFinderParallel(verbose))
  , fRecoTime(0)
  , fNEvents(0)
{
}

CbmL1RichENNRingFinder::~CbmL1RichENNRingFinder() {}


void CbmL1RichENNRingFinder::Init() {}

Int_t CbmL1RichENNRingFinder::DoFind(CbmEvent* event, TClonesArray* HitArray, TClonesArray* ProjArray,
                                     TClonesArray* RingArray)
{
  return finder->DoFind(event, HitArray, ProjArray, RingArray);
}
