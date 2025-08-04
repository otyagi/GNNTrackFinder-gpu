/* Copyright (C) 2007-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Andrey Lebedev, Florian Uhlig [committer] */

/**  CbmMuchTrack.h
 *@author A.Lebedev <andrey.lebedev@gsi.de>
 *@since 2007
 **
 ** Base class for local tracks in the MUCH detector.
 ** Derives from CbmTrack.
 **/

#ifndef CBMMUCHTRACK_H_
#define CBMMUCHTRACK_H_ 1

#include "CbmTrack.h"  // for CbmTrack

#include <Rtypes.h>  // for ClassDef

class CbmMuchTrack : public CbmTrack {
public:
  /** Default constructor **/
  CbmMuchTrack();

  /** Destructor **/
  virtual ~CbmMuchTrack();


  /** Associate a MuchPixelHit to the track
    ** @param hitIndex  Index of the Much hit in TClonesArray
    **/
  void AddMuchHit(int32_t hitIndex) { AddHit(hitIndex, kMUCHPIXELHIT); }

  ClassDef(CbmMuchTrack, 3);
};

#endif
