/* Copyright (C) 2006-2012 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexander Ayriyan, Gennadi Ososkov, Claudia Hoehne, Semen Lebedev, Denis Bertini [committer] */

/**
* \file CbmRichRingFitterTAU.h
*
* \brief Here the ring is fitted with the TAU algorithm from A. Ayriyan/ G. Ososkov.
*
* \author Alexander Ayriyan, Gennadi Ososkov, Claudia Hoehne, Semen Lebedev <s.lebedev@gsi.de>
* \date 2012
**/

#ifndef CBMRICHRINGFITTERTAU
#define CBMRICHRINGFITTERTAU 1

#include "CbmRichRingFitterBase.h"


/**
* \class CbmRichRingFitterTAU
*
* \brief Here the ring is fitted with the TAU algorithm from A. Ayriyan/ G. Ososkov.
*
* \author Alexander Ayriyan, Gennadi Ososkov, Claudia Hoehne, Semen Lebedev <s.lebedev@gsi.de>
* \date 2012
**/
class CbmRichRingFitterTAU : public CbmRichRingFitterBase {
 public:
  /**
    * \brief Default constructor.
    */
  CbmRichRingFitterTAU();

  /**
    * \brief Destructor.
    */
  virtual ~CbmRichRingFitterTAU();

  /**
    * \brief Inherited from CbmRichRingFitterBase.
    */
  virtual void DoFit(CbmRichRingLight* ring);

 private:
  int fRobust;
};

#endif
