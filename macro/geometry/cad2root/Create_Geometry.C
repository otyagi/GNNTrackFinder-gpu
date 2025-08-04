/* Copyright (C) 2023 Facility for AntiProton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

/* Generic macro which creates root geometry from CAD  */

#include "CbmDigitize.h"  // For CbmBaselibrary
#include "CbmGeometryUtils.h"

#include "FairGeoInterface.h"
#include "FairGeoLoader.h"

#include "TFile.h"
#include "TGeoCompositeShape.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMatrix.h"
#include "TGeoMedium.h"
#include "TGeoPgon.h"
#include "TGeoSystemOfUnits.h"
#include "TGeoTube.h"
#include "TGeoVolume.h"
#include "TList.h"
#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"

#include <iostream>
#include <sstream>

#include <stdbool.h>
#include <string.h>

/* Generic macro to create root geometry */
void Create_Geometry(const char* tag = "bmon_v23a", double Tz = -287.2075)
{
  using namespace Cbm::GeometryUtils;

  // gGeoManager with full list of materials and media.
  TGeoManager* gM = (TGeoManager*) pop_TGeoManager(tag);

  // Create a top volume (assembly)
  TGeoVolume* top = new TGeoVolumeAssembly(tag);
  gM->SetTopVolume(top);

  int inum = 0;

// Importing of geometries from CAD, via stl and gdml
#include "PARTS.C"

// Everything else
#include "EXTRAS.C"

  // Writing to disk
  char fileName[100];
  strcpy(fileName, tag);
  strcat(fileName, ".geo.root");
  TFile* output = TFile::Open(fileName, "RECREATE");
  top->Write();  // Write to fileName
  TGeoTranslation* mat = new TGeoTranslation(0, 0, Tz);
  mat->Write();  // Write Translation
  output->Close();

  // End
  printf("Successfully completed. \n");
  printf("******************************************************\n");
}
