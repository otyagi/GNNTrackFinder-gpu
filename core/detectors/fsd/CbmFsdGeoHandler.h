/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Lukas Chlad [committer] */

#ifndef CBMFSDGEOHANDLER_H
#define CBMFSDGEOHANDLER_H

#include <RtypesCore.h>  // for Int_t, Bool_t
#include <TString.h>     // for TString

#include <iostream>  // for string
#include <map>       // for map
#include <vector>    // for vector

struct CbmFsdModuleSpecs;
struct CbmFsdUnitSpecs;
class TVirtualMC;
class TGeoManager;

class CbmFsdGeoHandler {

private:
  /** Default constructor **/
  CbmFsdGeoHandler();

  /** Default destructor **/
  ~CbmFsdGeoHandler() = default;

  /** @brief Helper function to extract copy number from geoPath using key word
   ** @return integer corresponding to copy number of the key word
   **/
  Int_t GetCopyNumberByKey(TString geoPath, TString key) const;

public:
  CbmFsdGeoHandler(const CbmFsdGeoHandler&) = delete;
  CbmFsdGeoHandler& operator=(const CbmFsdGeoHandler&) = delete;

  /**
	 * Return Instance of CbmFsdGeoHandler.
	 */
  static CbmFsdGeoHandler& GetInstance()
  {
    static CbmFsdGeoHandler fInstance;
    return fInstance;
  }

  /*
	 * \brief Return CbmFsdModuleSpecs by digi moduleId.
	 */
  CbmFsdModuleSpecs* GetModuleSpecsById(Int_t id);

  /*
	 * \brief Return CbmFsdUnitSpecs by digi unitId.
	 */
  CbmFsdUnitSpecs* GetUnitSpecsById(Int_t id);

  /*
	 * \brief Initialize maps.
	 */
  void InitMaps();

  /** @brief Get the unique address from geometry path string
   ** @return integer corresponding to CbmFsdAddress scheme
   **/
  int32_t GetAddress(TString geoPath) const;

  /** @brief Get the unique address from TVirtualMC
   ** @return integer corresponding to CbmFsdAddress scheme
   **/
  int32_t GetCurrentAddress(TVirtualMC* vmc) const;

  /** @brief Get the unique address from TGeoManager
   ** @return integer corresponding to CbmFsdAddress scheme
   **/
  int32_t GetCurrentAddress(TGeoManager* geoMan) const;

private:
  std::map<Int_t, CbmFsdModuleSpecs*> fModuleIdToSpecsMap;
  std::map<Int_t, CbmFsdUnitSpecs*> fUnitIdToSpecsMap;

  // these key TStrings must corresponds to what one introduce in create geo macros!!
  const TString fBranchStr    = "fsd_";
  const TString fUnitStr      = "unit";
  const TString fModuleStr    = "module";
  const TString fActiveMatStr = "scint";
};

#endif /* CBMFSDGEOHANDLER_H */
