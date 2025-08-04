/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau[committer] */

#ifndef CbmTimeslicePixelHitSetDraw_H_
#define CbmTimeslicePixelHitSetDraw_H_

#include <CbmDefs.h>  // For ECbmDataType

#include <FairPointSetDraw.h>  // for FairPointSetDraw

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Color_t, Int_t, Style_t

class TClonesArray;
class TObject;
class TVector3;

/** @class CbmTimesliceRecoTracks
 ** @author Pierre-Alain Loizeau <p.-a.loizeau@gsi.de>
 ** @brief Interface class to add Cbm Hits drawing (derived from PixelHit) to CbmTimesliceManager. Cannot be used alone!
 **/
class CbmTimeslicePixelHitSetDraw : public FairPointSetDraw {
public:
  /**
   ** @brief Constructor
   ** @param Name of the container/bramhc, ROOT color of the displayed points, ROOT style of the displayed points
   **/
  CbmTimeslicePixelHitSetDraw(const char* name, Color_t color, Style_t mstyle, Int_t iVerbose = 1);
  virtual ~CbmTimeslicePixelHitSetDraw() = default;

  virtual InitStatus Init();
  virtual void Exec(Option_t* option);
  void Reset();

  /**
   ** @brief Load hits from selected event in timeslice. RESERVED FOR GUI CALLS!
   **/
  void GotoEvent(uint32_t uEventIdx);

protected:
  TVector3 GetVector(TObject* obj);

private:
  FairDataSourceI* fLocalDataSourcePtr = nullptr;                 //!
  TClonesArray* fCbmEvents             = nullptr;                 //!
  ECbmDataType fDataType               = ECbmDataType::kUnknown;  //!
  uint32_t fEventIdx                   = 0;                       //!

  ClassDef(CbmTimeslicePixelHitSetDraw, 1);
};

#endif /* CbmTimeslicePixelHitSetDraw_H_ */
