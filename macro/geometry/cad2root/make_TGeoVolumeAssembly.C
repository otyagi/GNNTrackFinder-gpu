/* Copyright (C) 2023 Facility for AntiProton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

/* Macro converts a gdml script to root binary */

#include "TGeoVolume.h"
#include "TROOT.h"
#include "TSystem.h"
//#include "TGeoVolumeAssembly.h"

#include "TFile.h"
#include "TGeoCompositeShape.h"
#include "TGeoManager.h"
#include "TGeoMaterial.h"
#include "TGeoMatrix.h"
#include "TGeoMedium.h"
#include "TGeoPgon.h"
#include "TGeoSystemOfUnits.h"
#include "TGeoTube.h"
#include "TList.h"
#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"

#include <iostream>
#include <sstream>

#include <strings.h>

void make_TGeoVolumeAssembly(const char* gdml_file)
{

  char vol_file[200];
  char basename[200];

  strcpy(basename, gdml_file);
  char* c = strstr(basename, ".gdml");
  *c      = '\0';

  strcpy(vol_file, gdml_file);
  strcpy(strstr(vol_file, ".gdml"), ".root");

  TGeoManager::Import(gdml_file);

  TGeoVolumeAssembly* top = (TGeoVolumeAssembly*) gGeoManager->GetTopVolume();

  TFile* output_file = TFile::Open(vol_file, "RECREATE");

  top->Write();

  output_file->Close();

  printf("Successfully completed. \n");
}
