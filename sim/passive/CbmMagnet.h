/* Copyright (C) 2019-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

#ifndef CBMMAGNET_H
#define CBMMAGNET_H

#include "FairModule.h"

class TGeoRotation;
class TGeoCombiTrans;

class CbmMagnet : public FairModule {
public:
  CbmMagnet(const char* name, const char* title = "CBM Magnet", Double_t px = 0., Double_t py = 0., Double_t pz = 0.,
            Double_t rx = 0., Double_t ry = 0., Double_t rz = 0.);
  CbmMagnet();
  virtual ~CbmMagnet();
  virtual void ConstructGeometry();

  virtual Bool_t IsSensitive(const std::string& name);
  Bool_t CheckIfSensitive(std::string name);

private:
  TGeoRotation* frot;       //!
  TGeoCombiTrans* fposrot;  //!

  CbmMagnet(const CbmMagnet&);
  CbmMagnet& operator=(const CbmMagnet&);

  ClassDef(CbmMagnet, 2)  //CBMMagnet
};

#endif  // CBMMAGNET_H
