/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Alexandru Bercuci */

#ifndef CBMTRDPARMODGEO_H
#define CBMTRDPARMODGEO_H

#include "CbmTrdParMod.h"  // for CbmTrdParMod

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Double_t, Char_t

class TGeoPhysicalNode;

/** \brief Definition of geometry for one TRD module **/
class CbmTrdParModGeo : public CbmTrdParMod {
 public:
  CbmTrdParModGeo(const char* name = "CbmTrdParModGeo", const char* title = "TRD module geometry");
  virtual ~CbmTrdParModGeo();
  virtual Double_t GetDX() const;
  virtual Double_t GetDY() const;
  virtual Double_t GetDZ() const;
  const Char_t* GetPath() const { return GetTitle(); }
  virtual void GetXYZ(Double_t xyz[3]) const;
  virtual Double_t GetX() const
  {
    Double_t xyz[3];
    GetXYZ(xyz);
    return xyz[0];
  }
  virtual Double_t GetY() const
  {
    Double_t xyz[3];
    GetXYZ(xyz);
    return xyz[1];
  }
  virtual Double_t GetZ() const
  {
    Double_t xyz[3];
    GetXYZ(xyz);
    return xyz[2];
  }
  virtual void LocalToMaster(Double_t in[3], Double_t out[3]) const;
  bool SetNode();

  TGeoPhysicalNode* GetNode() { return fNode; }

 private:
  CbmTrdParModGeo(const CbmTrdParModGeo&);
  const CbmTrdParModGeo& operator=(const CbmTrdParModGeo&);

  TGeoPhysicalNode* fNode;  ///< geometrical data for the module

  ClassDef(CbmTrdParModGeo,
           1)  // Wrapper around the TGeoPhysicalNode containing definition of geometry parameters for one TRD module
};

#endif
