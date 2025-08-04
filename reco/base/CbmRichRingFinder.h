/* Copyright (C) 2006-2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Supriya Das, Denis Bertini [committer] */

/******************************************************************************
*  $Id: CbmRichRingFinder.h,v 1.6 2006/04/17 22:11:04 sgorboun Exp $
*
*  Class  : CbmRichRingFinder
*  Description : Abstract base class for concrete RICH ring finding algorithm.
*                Each derived class must implement the method DoFind.
*
*  Author : Supriya Das
*  E-mail : S.Das@gsi.de
*
*******************************************************************************
*  $Log: CbmRichRingFinder.h,v $
*  Revision 1.6  2006/04/17 22:11:04  sgorboun
*  changes in L1 ENN Ring Finder
*
*  Revision 1.5  2006/01/26 09:48:34  hoehne
*  Array of projected tracks added for track-based ring finders
*
*  Revision 1.4  2006/01/19 11:12:48  hoehne
*  initial revision: new abstract base class for RingFinders
*
*******************************************************************************/

#ifndef CBM_RICH_RING_FINDER
#define CBM_RICH_RING_FINDER 1

#include "TObject.h"

class TClonesArray;
class CbmEvent;


class CbmRichRingFinder : public TObject {

 public:
  /** Default constructor **/
  CbmRichRingFinder(){};


  /** Destructor **/
  virtual ~CbmRichRingFinder(){};


  /** Virtual method Init. If needed, to be implemented in the
   ** concrete class. Else no action.
   **/
  virtual void Init(){};


  /** Abstract method DoFind. To be implemented in the concrete class.
   ** Task: Read the hit array and fill the ring array,
   ** pointers to which are given as arguments
   **
   *@param event        CbmEvent for time-based mode, if event==nullptr then event-by-event mode.
   *@param rHitArray    Array of RICH hits
   *@param rProjArray   Array of projected tracks (for track based finders)
   *@param rRingArray   Array of CbmRichRing
   *@value Number of rings created
   **/
  virtual Int_t DoFind(CbmEvent* event, TClonesArray* rHitArray, TClonesArray* rProjArray,
                       TClonesArray* rRingArray) = 0;

 private:
  CbmRichRingFinder(const CbmRichRingFinder&);
  CbmRichRingFinder& operator=(const CbmRichRingFinder&);

  ClassDef(CbmRichRingFinder, 1);
};

#endif
