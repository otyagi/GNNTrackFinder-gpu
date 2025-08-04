/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

#define MAX_MATERIALS 200

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <libgen.h>

#define NODE_DEPTH 25

/*  The materials in the transported TGeoManager will be compared to the materials in the subsystem binaries.
    The general idea pre-writing will be to run transport for a user specified setup generating the TGeoManager.
    The number of volumes for each material will be recorded in a standard map. Each subsystem will be extracted
    from the CBMSETUP, and the materials will be extraced from the respective binary and then substracted from
    the from the TGeoManager version original. After going through all subsystems, if there is no volumes in the
    original standard, the two TGeoManager and the material in the subsystem match, otherwise there is a material
    mismatch detected. Output to the screen will help diagonsis. 					       */

typedef struct MAT_VOL mat_vol_t;

struct MAT_VOL {
  int count;  // Number of volumes
  char name[200];
};

int REG_MATERIALS = 0;  // counter for the currently number of registered materials.

int get_register_count_material(const char* MATERIAL_NAME, mat_vol_t* material_array)
{
  int i = 0;

  while (i <= MAX_MATERIALS && i <= REG_MATERIALS) {
    // Register
    if (REG_MATERIALS == i) {
      strcpy((material_array[i]).name, MATERIAL_NAME);
      REG_MATERIALS++;
      material_array[i].count = 0;
      break;
    };
    // Found
    if (!strcmp((material_array[i]).name, MATERIAL_NAME)) { break; };
    i++;
  };
  material_array[i].count++;

  return 0;
}

int extract_mat_vol(char* fileName, mat_vol_t* material_array)
{
  TGeoNode* node[NODE_DEPTH];
  TGeoMedium* med;
  TGeoManager* gman = nullptr;
  TGeoVolume* top   = nullptr;

  TFile file(fileName, "READ");
  if (!file.IsOpen()) {
    std::cerr << "Was a root geo file supplied?" << endl;
    exit(1);
  };

  file.GetListOfKeys()->Print();
  file.GetSize();
  char geo_tag[300];
  strcpy(geo_tag, fileName);
  char* ptr = strstr(geo_tag, ".geo.root");
  *(ptr)    = '\0';

  gman = (TGeoManager*) file.Get("FAIRGeom");
  if (gman != nullptr) { top = gman->GetTopVolume(); };
  if (top == nullptr) top = (TGeoVolume*) file.Get(geo_tag);
  if (top == nullptr) top = (TGeoVolume*) file.Get("top");
  if (top == nullptr) top = (TGeoVolume*) file.Get("TOP");
  if (top == nullptr) top = (TGeoVolume*) file.Get("Top");
  if (top == nullptr) top = (TGeoVolume*) file.Get("geometryFromGDML");
  if (top == nullptr) top = (TGeoVolume*) file.Get(file.GetListOfKeys()->First()->GetName());
  if (top == nullptr) {
    std::cerr << "No Top Volume found. Is the TGeoManager or Top Volume unusally named" << std::endl;
    exit(1);
  };

  TObjArray* nodes = (TObjArray*) (top->GetNodes());

  int i_array[NODE_DEPTH], num_array[NODE_DEPTH], j;
  for (int i = 0; i < NODE_DEPTH; i++)
    i_array[i] = 0;
  for (int i = 0; i < NODE_DEPTH; i++)
    num_array[i] = 0;

  j            = 0;
  i_array[0]   = 0;
  num_array[0] = nodes->GetEntries();

  while (num_array[0] != 0) {
    if (j == 0) { node[0] = static_cast<TGeoNode*>(nodes->At(i_array[0])); }
    else {
      node[j] = static_cast<TGeoNode*>(node[j - 1]->GetDaughter(i_array[j]));
    };
    i_array[j]++;  // Update number.
    med              = node[j]->GetMedium();
    num_array[j + 1] = node[j]->GetNdaughters();

    get_register_count_material(med->GetMaterial()->GetName(), material_array);

    if (num_array[j + 1] > 0) {
      j++;
      num_array[j + 2] = 0;
    };

    while (i_array[j] == num_array[j]) {
      num_array[j] = 0;
      i_array[j]   = 0;  // Reset the counter before falling back.
      j--;
    };

    if (j >= NODE_DEPTH) {
      std::cerr << "Maximum depth of " << NODE_DEPTH << " exceeded. Increase NODE_DEPTH in header of macro," << endl;
      exit(NODE_DEPTH);
    };
  };

  file.Close();
  return 0;
}

int examine_materials(const char* setup = "sis100_electron")
{
  TString srcDir    = gSystem->Getenv("VMCWORKDIR");
  ECbmEngine engine = kGeant3;

  CbmTransport run;
  run.SetEngine(engine);
  run.SetOutFileName(Form("data/examine_%s.tra.root", setup), kTRUE);
  run.SetParFileName(Form("data/examine_%s.par.root", setup));
  run.SetGeoFileName(Form("data/examine_%s.geo.root", setup));
  run.LoadSetup(setup);
  run.SetField(new CbmFieldConst());  // Avoid crash for setups without field as not real simu (COSY, mCBM, ...)
  // Shoot single 10 GeV proton straight through the pipe to minimize transport
  run.SetBeamPosition(0, 0, 0.1, 0.1);
  run.AddInput(new CbmBeamGenerator(1, 1, 1, 10., 0));
  run.SetRandomSeed(1234);  // I fix this number so everything is deterministic
  run.Run(1);

  mat_vol_t* transport_materials = (mat_vol_t*) malloc(sizeof(mat_vol_t) * MAX_MATERIALS);
  for (int i = 0; i < MAX_MATERIALS; i++) {
    transport_materials[i].count = 0;
    strcpy(transport_materials[i].name, "");
  };

  extract_mat_vol(Form("data/examine_%s.geo.root", setup), transport_materials);

  // Detector and passive subsystems
  for (int i = 0; i < 120; i++)
    printf("#");
  printf("\n");

  TString fileName;
  char fileNameC[300];
  CbmSetup* cs                  = run.GetSetup();
  mat_vol_t* detector_materials = (mat_vol_t*) malloc(sizeof(mat_vol_t) * 500);
  for (int i = 0; i < MAX_MATERIALS; i++) {
    detector_materials[i].count = 0;
    strcpy(detector_materials[i].name, "");
  };

  REG_MATERIALS = 0;
  for (ECbmModuleId subsystem = ECbmModuleId::kMvd; subsystem < ECbmModuleId::kCave; ++subsystem) {
    if (cs->IsActive(subsystem)) {

      cs->GetGeoFileName(subsystem, fileName);  // returns true


      // Handling double beampipe
      if ((subsystem == ECbmModuleId::kPipe) && strchr(fileName.Data(), ':')) {

        fileName = srcDir + "/geometry/" + fileName;  // Get the right directory
        printf("fileName is %s\n", fileName.Data());
        strcpy(fileNameC, fileName.Data());
        char* change = strchr(fileNameC, ':');
        *change      = '\0';
        printf("FIRST PIPE fileName now is %s\n", fileNameC);
        extract_mat_vol(fileNameC, detector_materials);
        strcpy(fileNameC, srcDir + "/geometry/" + (change + 1));
        printf("SECOND PIPE fileName now is %s\n", fileNameC);
        extract_mat_vol(fileNameC, detector_materials);

        // Other subystems
      }
      else {

        fileName = srcDir + "/geometry/" + fileName;  // Get the right directory
        printf("fileName is %s\n", fileName.Data());
        strcpy(fileNameC, fileName.Data());
        extract_mat_vol(fileNameC, detector_materials);
      };
    };
  }

  // Quick Sort Technique
  mat_vol_t temp_mat;
  for (int i = 0; i < (MAX_MATERIALS - 1); ++i) {
    for (int j = 0; j < (MAX_MATERIALS - 1 - i); ++j) {
      if ((detector_materials[j].count) < (detector_materials[j + 1].count)) {
        temp_mat                  = detector_materials[j + 1];
        detector_materials[j + 1] = detector_materials[j];
        detector_materials[j]     = temp_mat;
      };
    };
  };
  for (int i = 0; i < (MAX_MATERIALS - 1); ++i) {
    for (int j = 0; j < (MAX_MATERIALS - 1 - i); ++j) {
      if ((transport_materials[j].count) < (transport_materials[j + 1].count)) {
        temp_mat                   = transport_materials[j + 1];
        transport_materials[j + 1] = transport_materials[j];
        transport_materials[j]     = temp_mat;
      };
    };
  };
  int SUMT = 0, SUMD = 0;
  for (int j = 0; j < MAX_MATERIALS; j++) {
    if ((transport_materials + j)->count > 0) SUMT += (transport_materials + j)->count;
    if ((detector_materials + j)->count > 0) SUMD += (detector_materials + j)->count;
  };
  (void) printf("TOTAL NUMBER OF VOLUMES\tTRANSPORTED: %d\t DETECTORS: %d\n", SUMT, SUMD);

  printf("%-25s\t\tTransported\t\tSubsystems\t\tDifference \n", "Material");
  int FAIL = 0;  // Success
  int DIFF = 0;

  for (int i = 0; i < MAX_MATERIALS; i++) {
    if (0 < (transport_materials + i)->count) {
      printf("%-25s\t\t%9d", (transport_materials + i)->name, (transport_materials + i)->count);
      for (int j = 0; j < MAX_MATERIALS; j++) {
        if (!strcmp((transport_materials[i].name), (detector_materials[j].name)))
          printf("\t\t%9d", (detector_materials + i)->count);
      };
      DIFF = ((transport_materials + i)->count) - ((detector_materials + i)->count);
      printf("\t\t%9d\n", DIFF);
    };
    if ((strcmp(transport_materials[i].name, "dummy") != 0) && (strcmp(transport_materials[i].name, "air") != 0))
      FAIL += abs(DIFF);
  };

  if (0 == FAIL) { (void) printf("[SUCCESS] Excluding air and dummy materials, no material errors were detected. \n"); }
  else {
    (void) printf("[FAILED] Excluding air and dummy materials, %d issues with materials were detected. \n", FAIL);
  };

  return FAIL;  // Success (when FAIL=0)
}
