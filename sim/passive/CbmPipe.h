/* Copyright (C) 2006-2017 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: David Emschermann, Denis Bertini [committer], Florian Uhlig */

#ifndef CBMPIPE_H
#define CBMPIPE_H 1

#include "FairModule.h"

#include <string>

class TGeoCombiTrans;

class CbmPipe : public FairModule {
public:
  CbmPipe();
  CbmPipe(const char* name, const char* Title = "CBM Pipe");
  CbmPipe(const CbmPipe&) = delete;
  CbmPipe& operator=(const CbmPipe&) = delete;
  virtual ~CbmPipe();

  virtual void ConstructGeometry();

private:
  TGeoCombiTrans* fCombiTrans;  //! Transformation matrix for geometry positioning
  std::string fVolumeName;

  ClassDef(CbmPipe, 1)  //CBMPIPE
};

#endif  //CBMPIPE_H
