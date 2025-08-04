/* Copyright (C) 2012-2020 Petersburg Nuclear Physics Institute named by B.P.Konstantinov of National Research Centre "Kurchatov Institute", Gatchina
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Evgeny Kryshen [committer], Florian Uhlig */

/** CbmMuchModuleGemRectangular.h
 *@author  E.Kryshen <e.kryshen@gsi.de>
 *@version 1.0
 *@since   03.08.12
 **/
#ifndef CBMMUCHMODULEGEMRECTANGULAR_H
#define CBMMUCHMODULEGEMRECTANGULAR_H 1

#include "CbmMuchModuleGem.h"  // for CbmMuchModuleGem

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Double_t, Bool_t
#include <TVector3.h>    // for TVector3

#include <vector>  // for vector

class CbmMuchPadRectangular;
class CbmMuchSectorRectangular;

class CbmMuchModuleGemRectangular : public CbmMuchModuleGem {
public:
  CbmMuchModuleGemRectangular();
  CbmMuchModuleGemRectangular(Int_t iStation, Int_t iLayer, Bool_t iSide, Int_t iModule, TVector3 position,
                              TVector3 size, Double_t cutRadius);
  virtual ~CbmMuchModuleGemRectangular() {}
  virtual Bool_t InitModule();
  CbmMuchSectorRectangular* GetSector(Double_t x, Double_t y);
  CbmMuchSectorRectangular* GetSector(Int_t ix, Int_t iy);
  CbmMuchPadRectangular* GetPad(Double_t x, Double_t y);
  std::vector<std::vector<Int_t>> GetGrid() { return fGrid; }
  Int_t GetGridNx() { return fGridNx; }
  Int_t GetGridNy() { return fGridNy; }
  Double_t GetGridDx() { return fGridDx; }
  Double_t GetGridDy() { return fGridDy; }

protected:
  Bool_t fUseModuleDesign;
  Int_t fGridNx;                          // Number of grid values in X direction
  Int_t fGridNy;                          // Number of grid values in Y direction
  Double_t fGridDx;                       // X-width of the grid cell
  Double_t fGridDy;                       // Y-width of the grid cell
  std::vector<std::vector<Int_t>> fGrid;  // 2D-vector of sector indices on the grid

  // Grid related private functions
  void InitNeighbourSectors();
  void InitNeighbourPads();
  Int_t GetGridIndexX(Double_t x);
  Int_t GetGridIndexY(Double_t y);
  Bool_t InitGrid(Bool_t useModuleDesign);

  ClassDef(CbmMuchModuleGemRectangular, 1);
};
#endif
