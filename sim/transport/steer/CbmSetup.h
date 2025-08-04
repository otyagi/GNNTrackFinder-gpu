/* Copyright (C) 2013-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Florian Uhlig [committer] */

/** @file CbmStsSetup.h
 ** @author Volker Friese <v.friese@gsi.de>
 ** @date 10.06.2013
 **/

#ifndef CBMSETUP_H
#define CBMSETUP_H 1


#include "CbmDefs.h"
#include "CbmGeoSetupDbProvider.h"
#include "CbmGeoSetupProvider.h"
#include "CbmGeoSetupRepoProvider.h"
#include "CbmModuleList.h"

#include <Logger.h>

#include "TNamed.h"
#include "TString.h"
#include "TVector3.h"

#include <map>

class FairModule;
class FairRunSim;
class CbmFieldMap;
class CbmSetupStorable;

enum ECbmSetupSource
{
  kRepo,
  kDb
};

// TODO: This class is a singleton, so it can be used when including
// a ROOT macro from another one. Since this is the only reason,
// the singleton nature shall be removed once not needed any longer.

class CbmSetup : public TNamed {
public:
  /** Destructor **/
  ~CbmSetup() {};


  /** Clear the setup
     **
     ** All settings are removed.
     **/
  virtual void Clear(Option_t* opt = "");

  /** Load a stored/exchanged copy of the setup
     **/
  void LoadStoredSetup(CbmSetupStorable* setupIn);

  /** Load setup modules, field and media.
     ** Afterward the parameters can be overriden over the provider
     ** See CbmGeoSetupProvider::GetSetup() for details
     ** Actual functionality is implemented in the CbmGeoSetupProvider
     **/
  void LoadSetup(const char* setupName) { fProvider->LoadSetup(setupName); }

  /** Register setup modules, field and media with FairRoot
     ** Actual functionality is implemented in the CbmGeoSetupProvider
     **/
  void RegisterSetup() { fProvider->RegisterSetup(); }

  /** Get materials (media) file path
     **/
  std::string GetMediaFilePath() { return fProvider->GetSetup().GetMedia().GetFilePath(); }

  /** Set materials (media) file path
     **/
  void SetMediaFilePath(std::string filePath) { fProvider->GetSetup().GetMedia().SetFilePath(filePath); }

  /** Create the field map using the given settings **/
  CbmFieldMap* CreateFieldMap();


  /** Get a geometry tag
     ** @param[in]  moduleId  Module identifier (type ESystemId or EPassiveId)
     ** @param[out] tag       Geometry tag for this module
     ** @return kTRUE if module is present in setup; else kFALSE
     **/
  Bool_t GetGeoTag(ECbmModuleId moduleId, TString& tag);


  /** Get a geometry file name
     ** @param[in]  moduleId  Module identifier (type ESystemId or EPassiveId)
     ** @param[out] fileName  Geometry file name for this module
     ** @return kTRUE if module is present in setup; else kFALSE
     **/
  Bool_t GetGeoFileName(ECbmModuleId moduleId, TString& fileName);


  /** Get a hash of the setup
     ** @return  A 64-bit integer, representing the hash value
     **
     ** The hash is formed from a string of pairs "module name -- module tag", separated by semicolon. Only
     ** active modules are considered.
     **/
  size_t GetHash();


  /** Get number of modules in the setup
     ** @value  Number of modules in setup
     **/
  Int_t GetNofModules() const { return fProvider->GetSetup().GetModuleMap().size(); }


  /** Get singleton instance of CbmSetup **/
  static CbmSetup* Instance();


  /** Get the activity flag of a detector
     ** @param moduleId  Module identifier (type ESystemId or EPassiveId)
     ** @return kTRUE if detector and active, else kFALSE
     **/
  Bool_t IsActive(ECbmModuleId moduleId);


  /** Check whether the setup is empty (contains no modules)
     ** @value kTRUE if the number of modules is null
     **/
  Bool_t IsEmpty() const { return (GetNofModules() == 0); }


  /** @brief Info to screen **/
  virtual void Print(Option_t* /*opt*/ = "") const { LOG(info) << ToString(); }


  /** Remove a module from the current setup
     ** @param moduleId  Module identifier (enum SystemId or kMagnet etc.)
     **/
  void RemoveModule(ECbmModuleId moduleId);


  /** Activate a module (detector)
     ** @param moduleId  Module identifier. SystemId for detectors, or kMagnet, kPipe, kTarget
     ** @param active    Activity tag for module (only in case of detectors)
     **
     ** Activate or deactivate a detector already present in the setup.
     ** The method will have no effect if called for a passive module (target,
     ** pipe, magnet). If a detector is flagged active,
     ** its ProcessHits method will be called during the transport simulation.
     **/
  void SetActive(ECbmModuleId moduleId, Bool_t active = kTRUE);


  /** Set the magnetic field map
     ** @param tag   Field map tag
     ** @param scale Field scaling factor
     **
     ** The magnetic field map is automatically selected according to the
     ** magnet geometry version. The user can, however, override this by
     ** choosing a different field map. In this case, consistency between
     ** field map and magnet geometry is within the responsibility of the
     ** user.
     **/
  void SetField(const char* tag, Double_t scale = 1., Double_t xPos = 0., Double_t yPos = 0., Double_t zPos = 0.);


  /** Set the field scaling factor
     ** @param scale  Field scaling factor
     **
     ** The currently selected field map will be scaled by the specified
     ** factor.
     **/
  void SetFieldScale(Double_t scale) { fProvider->GetSetup().GetField().SetScale(scale); }


  /** Add a module to the setup
     ** @param moduleId  Module identifier. SystemId for detectors, or kMagnet, kPipe, kTarget
     ** @param geoTag    Geometry version for module
     ** @param active    Activity tag for module (only in case of detectors)
     **
     ** The module / detector will be added to the setup. If a detector is flagged active,
     ** its ProcessHits method will be called during the transport simulation.
     **/
  void SetModule(ECbmModuleId moduleId, const char* geoTag, Bool_t active = kTRUE);


  /** @brief Info to string **/
  std::string ToString() const;


  /** @brief Set the source the setup will be loaded from
     ** @param setupSource  enum value \ref ECbmSetupSource
     **/
  void SetSetupSource(ECbmSetupSource setupSource);


  /** @brief Get the geo setup provider **/
  CbmGeoSetupProvider* GetProvider() { return fProvider; };


  /** @brief Set the geo setup provider
     ** @param value provider
     ** This class takes the ownership of the provider
     **/
  void SetProvider(CbmGeoSetupProvider* value)
  {
    delete fProvider;
    fProvider = value;
  };

private:
  static CbmSetup* fgInstance;  ///< Pointer to static instance

  CbmGeoSetupProvider* fProvider {new CbmGeoSetupRepoProvider()};  ///! Setup provider


  /** Default constructor **/
  CbmSetup() : TNamed("CBM Setup", "") {};

  /** Copy constructor and assignment operator (not implemented ) **/
  CbmSetup(const CbmSetup&);
  CbmSetup operator=(const CbmSetup&);


  ClassDef(CbmSetup, 3);
};


class CbmSetupStorable : public TNamed {
public:
  /** Default constructor **/
  CbmSetupStorable() : TNamed("CBM Setup", "") {};

  /** Destructor **/
  ~CbmSetupStorable() {};

  /** Copy constructor and assignment operator (not implemented ) **/
  CbmSetupStorable(const CbmSetupStorable& rhs) : TNamed(rhs)
  {
    if (nullptr != rhs.fProviderRepo) {
      /// To avoid clang format one-lining
      fProviderRepo = new CbmGeoSetupRepoProvider(*(rhs.fProviderRepo));
    }
    else if (nullptr != fProviderDb) {
      /// To avoid clang format one-lining
      fProviderDb = new CbmGeoSetupDbProvider(*(rhs.fProviderDb));
    }
  }

  /** Constructor from CbmSetup object **/
  CbmSetupStorable(CbmSetup* rawSetup)
  {
    CbmGeoSetupProvider* ptrGenProv      = rawSetup->GetProvider();
    CbmGeoSetupRepoProvider* ptrRepoProv = dynamic_cast<CbmGeoSetupRepoProvider*>(ptrGenProv);
    if (nullptr == ptrRepoProv) {
      /// To avoid clang format one-lining
      CbmGeoSetupDbProvider* ptrDbProv = dynamic_cast<CbmGeoSetupDbProvider*>(ptrGenProv);
      if (nullptr != ptrDbProv) {
        /// To avoid clang format one-lining
        fProviderDb = new CbmGeoSetupDbProvider(*ptrDbProv);
      }
    }
    else {
      fProviderRepo = new CbmGeoSetupRepoProvider(*ptrRepoProv);
    }
  }

  CbmGeoSetupRepoProvider* GetRepoProvPtr() { return fProviderRepo; }
  CbmGeoSetupDbProvider* GetDbProvPtr() { return fProviderDb; }

private:
  CbmGeoSetupRepoProvider* fProviderRepo = nullptr;
  CbmGeoSetupDbProvider* fProviderDb     = nullptr;

  ClassDef(CbmSetupStorable, 1);
};
#endif /* CBMSETUP_H */
