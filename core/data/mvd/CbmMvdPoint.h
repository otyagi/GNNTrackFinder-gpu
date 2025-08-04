/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

// -------------------------------------------------------------------------
// -----                      CbmMvdPoint header file                  -----
// -----                  Created 06/11/06  by V. Friese               -----
// -------------------------------------------------------------------------

/**  CbmMvdPoint.h
 *@author V.Friese <v.friese@gsi.de>
 *
 * Data class for interception of MC track with a MVD detetcor station. 
 * Holds in addition to the base class the coordinates and momentum at the 
 * exit from the active volume.
 *
 * Data level MC
 **/


#ifndef CBMMVDPOINT_H
#define CBMMVDPOINT_H 1

#include "CbmMvdDetectorId.h"  // for CbmMvdDetectorId

#include <FairMCPoint.h>  // for FairMCPoint

#include <Rtypes.h>      // for ClassDef
#include <RtypesCore.h>  // for Double32_t
#include <TVector3.h>    // for TVector3

#include <cstdint>

class CbmMvdPoint : public FairMCPoint, CbmMvdDetectorId {

public:
  /** Default constructor **/
  CbmMvdPoint();


  /** Constructor with arguments
   *@param trackId    Index of MCTrack
   *@param pdgCode    Particle type (PDG code)
   *@param stationNr  Station number
   *@param posIn      Coordinates at entrance to active volume [cm]
   *@param posOut     Coordinates at exit of active volume [cm]
   *@param momIn      Momentum of track at entrance [GeV]
   *@param momOut     Momentum of track at exit [GeV]
   *@param tof        Time since event start [ns]
   *@param length     Track length since creation [cm]
   *@param eLoss      Energy deposit [GeV]
   *@param frame      Number of frame this point is registered in
   **/
  CbmMvdPoint(int32_t trackId, int32_t pdgCode, int32_t detId, TVector3 posIn, TVector3 posOut, TVector3 momIn,
              TVector3 momOut, double tof, double length, double eLoss, int32_t frame = 0);


  /** Copy constructor **/
  //  CbmMvdPoint(const CbmMvdPoint& point) { *this = point; };


  /** Destructor **/
  virtual ~CbmMvdPoint();


  /** Accessors **/
  double GetXOut() const { return fX_out; }
  double GetYOut() const { return fY_out; }
  double GetZOut() const { return fZ_out; }
  double GetPxOut() const { return fPx_out; }
  double GetPyOut() const { return fPy_out; }
  double GetPzOut() const { return fPz_out; }
  int32_t GetPdgCode() const { return fPdgCode; }
  int32_t GetSystemId() const { return SystemId(fDetectorID); }
  int32_t GetStationNr() const { return StationNr(fDetectorID); }
  int32_t GetPointId() const
  {
    return fPointId;
  }  // Returns index of this object in its TClonesArray.
     // By default not filled. Used internally in the MvdDigitizer.
  void PositionOut(TVector3& pos) { pos.SetXYZ(fX_out, fY_out, fZ_out); }
  void MomentumOut(TVector3& mom) { mom.SetXYZ(fPx_out, fPy_out, fPz_out); }
  int32_t GetFrame() const { return fFrame; }
  int32_t GetAbsTime();

  /** Modifiers **/
  void SetPositionOut(TVector3 pos);
  void SetMomentumOut(TVector3 mom);
  void SetPdgCode(int32_t pdg) { fPdgCode = pdg; }
  void SetPointId(int32_t myId) { fPointId = myId; }
  void SetFrameNr(int32_t frame) { fFrame = frame; }


  /** Output to screen **/
  virtual void Print(const Option_t* opt) const;


protected:
  Double32_t fX_out, fY_out, fZ_out;
  Double32_t fPx_out, fPy_out, fPz_out;
  int32_t fPdgCode;  // index of the object in its TClonesArray. By default not filled => -1.
  int32_t fPointId;  // index of the object in its TClonesArray. By default not filled => -1.
  int32_t fFrame;
  double fStartTime;

  ClassDef(CbmMvdPoint, 1)
};


inline void CbmMvdPoint::SetPositionOut(TVector3 pos)
{
  fX_out = pos.X();
  fY_out = pos.Y();
  fZ_out = pos.Z();
}


inline void CbmMvdPoint::SetMomentumOut(TVector3 mom)
{
  fPx_out = mom.Px();
  fPy_out = mom.Py();
  fPz_out = mom.Pz();
}


#endif
