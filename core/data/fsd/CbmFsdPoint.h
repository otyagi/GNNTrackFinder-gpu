/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Florian Uhlig, Lukas Chlad [committer] */

/** @class CbmFsdPoint
 ** @author L. Chlad
 **
 ** @brief Interception of MC track with the plane representing the FSD.
 **/


#ifndef CBMFSDPOINT_H
#define CBMFSDPOINT_H 1

#include <FairMCPoint.h>  // for FairMCPoint

#include <Rtypes.h>    // for ClassDef
#include <TVector3.h>  // for TVector3

#include <cstdint>
#include <string>  // for string

class CbmFsdPoint : public FairMCPoint {

public:
  /** Default constructor **/
  CbmFsdPoint();


  /** Constructor with arguments
   *@param trackID  Index of MCTrack
   *@param detID    Detector ID
   *@param pos      Coordinates at entrance to active volume [cm]
   *@param mom      Momentum of track at entrance [GeV]
   *@param tof      Time since event start [ns]
   *@param length   Track length since creation [cm]
   *@param eLoss    Energy deposit [GeV]
   **/
  CbmFsdPoint(int32_t trackID, int32_t detID, TVector3 pos, TVector3 mom, double tof, double length, double eLoss);


  /** Destructor **/
  virtual ~CbmFsdPoint();


  /** Output to screen **/
  virtual void Print(const Option_t* opt) const;

  std::string ToString() const;

  ClassDef(CbmFsdPoint, 1)
};


#endif
