/* Copyright (C) 2006-2012 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Sergey Gorbunov, Denis Bertini [committer] */

/** Abstract base class for hit is being used by the CbmKF Kalman Filter 
 *
 * @author  S.Gorbunov
 * @version 0.0
 * @since   02.12.05
 * 
 * Description will be here 
 *
 */

#ifndef CBMKFHIT_H
#define CBMKFHIT_H 1

#include "CbmKFTrackInterface.h"

class CbmKFHit : public TObject {

 public:
  CbmKFHit() : MaterialIndex(0){};
  virtual ~CbmKFHit() {}

  Int_t MaterialIndex;

  virtual Int_t Filter(CbmKFTrackInterface& track, Bool_t downstream, Double_t& QP0) = 0;

  ClassDef(CbmKFHit, 1)
};

#endif /* !CBMKFHIT_H */
