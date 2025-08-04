/* Copyright (C) 2023 Facility for Antiproton and Ion Research in Europe, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin [committer] */

#include "TGeoManager.h"
#include "TGeoNavigator.h"

#include <time.h>

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TGeoManager* gman;
TGeoNode* tmp_node;
TGeoMedium* tmp_med;
TGeoMaterial* tmp_mat;
double tmp_den;

#define NODE_MAX 70000
#define SAMPLING 100000000

typedef struct node_list {
  char* NAME      = (char*) calloc(200, sizeof(char));  // zero 200 characters
  long int COUNT  = 0;
  int MATERIAL_ID = 0;  // Given by the TGeoManager
} node_list;

// find nodal number or return next free.
int find_node(char* strname, node_list* NODE_COUNT)
{
  int j = 0;

  while (true) {
    //printf("%d COMPARE %s to %s is  %d\n", j, NODE_COUNT[j].NAME, strname, strcmp(NODE_COUNT[j].NAME,strname));
    if (!*NODE_COUNT[j].NAME) goto not_allocated;
    if (!strcmp(NODE_COUNT[j].NAME, strname)) {
      return j;  // If not yet allocated.
    };
    j++;
    if (j == NODE_MAX) fprintf(stderr, "WARNING: NODE_MAX EXCEEDED\n");
  };
not_allocated:
  return j;
}
void total_mass(int output = 2)
{
  // output as output controll variable
  // output = 0 debuging
  // output = 1 (default) list out the global materials
  // output = 2 total list of individual nodes

  int NUM = SAMPLING;

  node_list NODE_COUNT[NODE_MAX];
  strcpy(NODE_COUNT[0].NAME, "/cave_1");

  char strname[200];
  char NODE_PATH[200];

  char ARRAY_PATH[20][40];

  double TOTAL_MASS = 0, SUM_DENSITY = 0;

  TFile file("test.geo.root", "READ");
  file.GetListOfKeys()->Print();
  gman = (TGeoManager*) file.Get("FAIRGeom");
  if (gman == nullptr) gman = gGeoManager;

  TGeoNavigator* navig = gman->AddNavigator();

  int i = 0, j = 0, k = 0;

  int NUMBER_OF_MATERIALS = gman->GetListOfMaterials()->Last()->GetUniqueID();
  long int MATERIAL_COUNT[100];

  for (i = 0; i < NUMBER_OF_MATERIALS; i++)
    MATERIAL_COUNT[i] = 0;

  srand(time(NULL));
  Double_t x, y, z;

  // Full jpsi volume
  /*	double XMIN=-700, XMAX=700;
	double YMIN=-570, YMAX=540;
	double ZMIN=-100, ZMAX=1882;*/
  //double ZMIN=-150, ZMAX=150;

  // rich_v22a test
  double XMIN = -265, XMAX = 265;
  double YMIN = -251, YMAX = 251;
  double ZMIN = 110, ZMAX = 335;

  if (output == 2)
    printf("RANDMAX IS %d\nResolution in X, Y, Z is %e %e %e\n", RAND_MAX, (XMAX - XMIN) / RAND_MAX,
           (YMAX - YMIN) / RAND_MAX, (ZMAX - ZMIN) / RAND_MAX);
  double TOTAL_VOLUME = (XMAX - XMIN) * (YMAX - YMIN) * (ZMAX - ZMIN);  // in cm3       for m3  times 10^-6
  if (output == 2) printf("Total volume is %f m3 \n", (double) TOTAL_VOLUME / 1000000);

  // I prevent rastoring bias when number of points is very large.
  // My range will be modified by the random resolution to avoid fixing the single grid.
  XMIN -= -1.0 * ((rand() / RAND_MAX) * ((XMAX - XMIN) / RAND_MAX));
  XMAX += ((rand() / RAND_MAX) * ((XMAX - XMIN) / RAND_MAX));

  YMIN -= -1.0 * ((rand() / RAND_MAX) * ((YMAX - YMIN) / RAND_MAX));
  YMAX += ((rand() / RAND_MAX) * ((YMAX - YMIN) / RAND_MAX));

  ZMIN -= -1.0 * ((rand() / RAND_MAX) * ((ZMAX - ZMIN) / RAND_MAX));
  ZMAX += ((rand() / RAND_MAX) * ((ZMAX - ZMIN) / RAND_MAX));

  // Position of STS  Tz=59.5
  // double XMIN=-162, XMAX=162;
  // double YMIN=-72, YMAX=72;
  // double ZMIN=-102+59.5, ZMAX=20+59.5;

  TOTAL_VOLUME = (XMAX - XMIN) * (YMAX - YMIN) * (ZMAX - ZMIN);  // in cm3       for m3  times 10^-6
  if (output == 2) printf("Modified total volume is %f m3 \n", (double) TOTAL_VOLUME / 1000000);

  for (i = 1; i <= NUM; i++) {

    x = (double) rand() / RAND_MAX * (XMAX - XMIN) + XMIN;
    y = (double) rand() / RAND_MAX * (YMAX - YMIN) + YMIN;
    z = (double) rand() / RAND_MAX * (ZMAX - ZMIN) + ZMIN;

    tmp_node = navig->FindNode(x, y, z);
    tmp_med  = tmp_node->GetMedium();
    tmp_mat  = tmp_med->GetMaterial();
    tmp_den  = tmp_mat->GetDensity();

    SUM_DENSITY += tmp_den;

    MATERIAL_COUNT[gman->GetMaterialIndex(tmp_mat->GetName())]++;

    j = 0;
    while (navig->GetMother(j) != nullptr) {
      strcpy(ARRAY_PATH[j], (navig->GetMother(j)->GetName()));
      j++;
    };

    strname[0] = '\0';
    while (j-- > 0) {
      strcat(strname, "/");
      strcat(strname, ARRAY_PATH[j]);
      ARRAY_PATH[j][0] = '\0';  // zeroing
    };

    //	printf("STRING WROTE: %s \n", strname);

    j = find_node(strname, NODE_COUNT);
    NODE_COUNT[j].COUNT++;
    strcpy(NODE_COUNT[j].NAME, strname);

    NODE_COUNT[j].MATERIAL_ID = gman->GetMaterialIndex(tmp_mat->GetName());

    j = 0;
  };

  // Ratio of materials 	         	    MATERIAL_COUNT/TOTAL_COUNT
  // VOLUME OF MATERIAL 			    [ABOVE]*TOTAL_VOLUME
  // TOTAL MASS OF MATERIAL		    [ABOVE]*DENSITY_OF_MATERIAL

  long int TOTAL_COUNT = NUM;
  double VC_RATIO      = (double) TOTAL_VOLUME / TOTAL_COUNT;

  printf("TOTAL COUNT: %ld \n", TOTAL_COUNT);
  printf("\n\n");
  printf("\nCOUNT OF MATERIALS: ");
  for (i = 0; i < NUMBER_OF_MATERIALS; i++)
    printf("%f%% ", (double) 100 * MATERIAL_COUNT[i] / TOTAL_COUNT);
  printf("\n");
  printf("\n\n");
  for (i = 0; i < NUMBER_OF_MATERIALS; i++)
    printf("VOLUME OF %s IS %f \n", gman->GetMaterial(i)->GetName(),
           (double) TOTAL_VOLUME * MATERIAL_COUNT[i] / TOTAL_COUNT);
  printf("\n\n");
  for (i = 0; i < NUMBER_OF_MATERIALS; i++)
    if (output == 2)
      printf("MASS OF %s IS %f kg\n", gman->GetMaterial(i)->GetName(),
             0.001 * (gman->GetMaterial(i)->GetDensity()) * TOTAL_VOLUME * MATERIAL_COUNT[i] / TOTAL_COUNT);


  printf("NODE \t COUNT \t VOLUME \t MATERIAL \t MASS \n");
  printf("---------- \t ---------- \t ---------- \t ---------- \t ---------- \n");

  for (int k = 0; k < 5000; k++) {
    if (NODE_COUNT[k].COUNT == 0) break;
    printf("%s \t %ld \t %f \t %s \t %f \n", NODE_COUNT[k].NAME, NODE_COUNT[k].COUNT,
           (float) NODE_COUNT[k].COUNT * VC_RATIO, gman->GetMaterial(NODE_COUNT[k].MATERIAL_ID)->GetName(),
           (float) NODE_COUNT[k].COUNT * VC_RATIO * (gman->GetMaterial(NODE_COUNT[k].MATERIAL_ID)->GetDensity()));
  }

  return;
}
