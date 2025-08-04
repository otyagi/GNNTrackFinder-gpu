/* Copyright (C) 2012-2020 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Grigory Kozlov, Andrey Lebedev [committer], Florian Uhlig */

/*
 * CbmMuchClustering.h
 *
 *  Created on: Mar 5, 2012
 *      Author: kozlov
 */

#ifndef CBMMUCHCLUSTERING_H_
#define CBMMUCHCLUSTERING_H_

#include "CbmClusteringA1.h"
#include "CbmClusteringGeometry.h"
#include "CbmClusteringSL.h"
#include "CbmClusteringWard.h"
#include "CbmMuchGeoScheme.h"
#include "CbmMuchModuleGem.h"
#include "FairTask.h"
#include "TH1F.h"

#include <map>
#include <vector>

class TClonesArray;
class CbmDigiManager;

class CbmMuchClustering : public FairTask {
 public:
  CbmMuchClustering(const char* digiFileName);
  virtual ~CbmMuchClustering();
  virtual InitStatus Init();
  virtual void Exec(Option_t* opt);
  virtual void Finish();

  void SetAlgorithmVersion(Int_t AlgorithmVersion) { fAlgorithmVersion = AlgorithmVersion; }

  void CreateModulesGeometryArray();
  void SetDigiCharges();
  void ClearDigiCharges();
  void ClusteringMainFunction();

 private:
  void ReadDataBranches();

  /* Clustering algorithms
    * 1 - Developed algorithm, using all neighbors;
    * 2 - Developed algorithm, do not using diagonal neighbors;
    * 3 - Simple Single Linkage method, using all neighbors;
    * 4 - Simple Single Linkage method, do not using diagonal neighbors;
    * 5 - Ward's method (!) not tested
    */
  Int_t fAlgorithmVersion;
  Int_t fNofModules;   // Number of modules in detector
  Int_t fNofClusters;  // Number of clusters for event

  CbmMuchGeoScheme* fScheme;  // MuCh geometry scheme
  TString fDigiFile;          // Digitization file

  std::vector<CbmClusteringGeometry*> fModulesGeometryArray;
  std::map<Int_t, Int_t> fModulesByDetId;

  CbmDigiManager* fDigiMan = nullptr;  //! Interface to digi data
  TClonesArray* fCluster;              // Output array of CbmMuchCluster
  TClonesArray* fHit;                  // Output array of CbmMuchHit
  Int_t fNofEvents;                    // Event counter

  void ClusteringA1(CbmClusteringGeometry* m1, CbmMuchModuleGem* m2, Int_t Ver);
  void ClusteringSL(CbmClusteringGeometry* m1, CbmMuchModuleGem* m2, Int_t Ver);
  void ClusteringWard(CbmClusteringGeometry* m1, CbmMuchModuleGem* m2 /*, Int_t Ver*/);

  CbmMuchClustering(const CbmMuchClustering&);
  CbmMuchClustering& operator=(const CbmMuchClustering&);

  ClassDef(CbmMuchClustering, 1);
};

#endif
