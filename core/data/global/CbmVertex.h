/* Copyright (C) 2006-2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Florian Uhlig, Alex Bercuci, Denis Bertini [committer] */

// -------------------------------------------------------------------------
// -----                      CbmVertex header file                    -----
// -----                  Created 28/11/05  by V. Friese               -----
// -------------------------------------------------------------------------


/** CbmVertex.h
 *@author V.Friese <v.friese@gsi.de>
 **
 ** Data class for a vertex in CBM.
 ** Data level: RECO
 **/


#ifndef CBMVERTEX_H
#define CBMVERTEX_H 1

#include <Rtypes.h>          // for ClassDef
#include <RtypesCore.h>      // for Double32_t
#include <TMatrixFSymfwd.h>  // for TMatrixFSym
#include <TNamed.h>          // for TNamed
#include <TVector3.h>        // for TVector3

#include <cstdint>
#include <string>  // for string
#include <vector>  // for global track indices

class CbmVertex : public TNamed {

public:
  /** Default constructor  **/
  CbmVertex();


  /** Constructor with name and title **/
  CbmVertex(const char* name, const char* title);


  /** Constructor with all member variables 
   *@param name      Name of object
   *@param title     Title of object
   *@param x         x coordinate [cm]
   *@param y         y coordinate [cm]
   *@param z         z coordinate [cm]
   *@param chi2      chi square of vertex fit
   *@param ndf       Number of degrees of freedom of vertex fit
   *@param nTracks   Number of tracks used for vertex fit
   *@param covMat    Covariance Matrix (symmetric, 3x3)
   **/
  CbmVertex(const char* name, const char* title, double x, double y, double z, double chi2, int32_t ndf,
            int32_t nTracks, const TMatrixFSym& covMat);


  /** Destructor **/
  virtual ~CbmVertex();


  /** Ouput to screen **/
  virtual void Print(Option_t* opt = "") const;


  /** Accessors **/
  double GetX() const { return fX; };               // x position [cm]
  double GetY() const { return fY; };               // y position [cm]
  double GetZ() const { return fZ; };               // z posiiton [cm]
  double GetChi2() const { return fChi2; };         // chi2
  int32_t GetNDF() const { return fNDF; };          // nof degrees of freedom
  int32_t GetNTracks() const { return fNTracks; };  // nof tracks used
  void Position(TVector3& pos) const { pos.SetXYZ(fX, fY, fZ); };
  void CovMatrix(TMatrixFSym& covMat) const;
  double GetCovariance(int32_t i, int32_t j) const;

  /** \brief Accessors to the Global track array. Retrieve the tracks being actually used for vertex fit by entry in the indices array*/
  int32_t GetTrackIndex(int32_t iTrack) const;
  /** \brief Accessors to the Global track array. Check if track with global index iTrack was actually used for vertex fit*/
  bool FindTrackByIndex(uint32_t iTrack) const;
  bool SetTracks(std::vector<uint32_t>& indexVector);

  /** Reset the member variables **/
  void Reset();


  /** Set the member variables
   *@param x         x coordinate [cm]
   *@param y         y coordinate [cm]
   *@param z         z coordinate [cm]
   *@param chi2      chi square of vertex fit
   *@param ndf       Number of degrees of freedom of vertex fit
   *@param nTracks   Number of tracks used for vertex fit
   *@param covMat    Covariance Matrix (symmetric, 3x3)
   **/
  void SetVertex(double x, double y, double z, double chi2, int32_t ndf, int32_t nTracks, const TMatrixFSym& covMat);


  /** String output **/
  virtual std::string ToString() const;


private:
  /** Position coordinates  [cm] **/
  Double32_t fX, fY, fZ;

  /** Chi2 of vertex fit **/
  Double32_t fChi2;

  /** Number of degrees of freedom of vertex fit **/
  int32_t fNDF;

  /** Number of tracks used for the vertex fit **/
  int32_t fNTracks;

  /** Covariance matrix for x, y, and z stored in an array. The
   ** sequence is a[0,0], a[0,1], a[0,2], a[1,1], a[1,2], a[2,2]
   **/
  Double32_t fCovMatrix[6];

  /** indices of [global] tracks being used for the vertex fit AB 24.07.17 for mCBM 2024*/
  std::vector<uint32_t> fTrkIdx = {};

  ClassDef(CbmVertex, 2);
};


#endif
