/* Copyright (C) 2008-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Pascal Raisig */

/**
 * \file CbmTrdParManager.h
 * \brief Assign pad layout to TRD Modules.
 * \author Florian Uhlig <f.uhlig@gsi.de>
 * \date 06/06/2008
 *
 * Updated 20/05/2013 by Andrey Lebedev <andrey.lebedev@gsi.de>
 *
 * Create digitisation parameter map.
 * Read pad geometry from CbmTrdPads.h,
 * assign pad layout to sectors in TRD modules.
 */

#ifndef CBMTRDPARMANAGER_H
#define CBMTRDPARMANAGER_H

#include "CbmTrdHardwareSetupR.h"  //for CbmTrdHardwareSetupR
#include "CbmTrdParSet.h"          // for GetParSetList

#include "FairTask.h"  // for FairTask, InitStatus

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Bool_t, kFALSE, Int_t, Option_t, kTRUE
#include <TString.h>     // for TString

#include <memory>

class CbmTrdGeoHandler;
class CbmTrdParSetAsic;
class CbmTrdParSetDigi;
class CbmTrdParSetGain;
class CbmTrdParSetGas;
class CbmTrdParSetGeo;

/**
 * \class CbmTrdParManager
 * \brief Manipulate calibration parameters for the TRD detectors
 */
class CbmTrdParManager : public FairTask {
public:
  enum class ECbmTrdParSets : Int_t
  {
    kBegin            = 0,
    kCbmTrdParSetAsic = kBegin,
    kCbmTrdParSetDigi,
    kCbmTrdParSetGain,
    kCbmTrdParSetGas,
    kCbmTrdParSetGeo,
    kEnd = kCbmTrdParSetGeo
  };
  /**
   * @brief enumerator for organising the existing parameter sets of the trd
   *
   */

  /**
   * \brief Default constructor.
   * \param[in] fasp switch between ASICs. Default SPADIC (fasp == kFALSE)
   * \sa SetFASP(Bool_t)
   **/
  CbmTrdParManager(Bool_t fasp = kFALSE);

  /**
   * \brief Destructor.
   **/
  virtual ~CbmTrdParManager();

  /**
   * \brief Inherited from FairTask.
   **/
  virtual InitStatus Init();

  /**
   * \brief Inherited from FairTask.
   **/
  virtual void SetParContainers();

  /**
   * \brief Change the default ASIC to FASP
   **/
  virtual void SetFASP(Bool_t set = kTRUE) { fFASP = set; }

  /**
   * \brief Inherited from FairTask.
   **/
  virtual void Exec(Option_t* option);

  /**
   * \brief Inherited from FairTask.
   **/
  virtual void Finish();

  /**
   * \brief Create parameter files from geometry in gGeoManager
   * A run macro can be found in the trd cbm.gsi git repository
   **/
  bool CreateParFilesFromGeometry(bool createRootFileOutput, TString outDir = "");

  static void GetParSetList(std::vector<std::shared_ptr<CbmTrdParSet>>* parSetList);
  static void GetParFileExtensions(std::vector<std::string>* vec);

private:
  void CreateModuleParameters(const TString& path);
  bool CreateParFilesFromGeometry(TString outDir = "");

  Int_t fMaxSectors;  // Maximum number of sectors for all modules
  Bool_t fFASP;       ///< Switch to FASP ASIC

  //    // Map of Unique TRD Module Id to corresponding CbmTrdModuleSim
  //    std::map<Int_t, CbmTrdModuleSim*> fModuleMap;

  CbmTrdParSetAsic* fAsicPar;  ///< The set of ASIC characterization parameters
  CbmTrdParSetDigi* fDigiPar;  ///< The set of read-out description parameters
  CbmTrdParSetGas* fGasPar;    ///< The set of gas description parameters
  CbmTrdParSetGain* fGainPar;  ///< The set of gain conversion parameters
  CbmTrdParSetGeo* fGeoPar;    ///< The set of gain conversion parameters

  CbmTrdGeoHandler* fGeoHandler;

  TString fGeometryTag;
  CbmTrdHardwareSetupR fHardwareSetup;

  CbmTrdParManager(const CbmTrdParManager&);
  CbmTrdParManager& operator=(const CbmTrdParManager&);

  ClassDef(CbmTrdParManager, 3)
};
#endif  // CBMTRDPARMANAGER_H
