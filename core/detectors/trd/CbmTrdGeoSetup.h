/* Copyright (C) 2024 National Institute of Physics and Nuclear Engineering - Horia Hulubei, Bucharest
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Alexandru Bercuci [committer] */

/**
 * \file CbmTrdGeoSetup.h
 * \brief Describing class of the TRD setup geometry, FEE, gas.
 * \author Alexandru Bercuci <abercuci@niham.nipne.ro>
 * \date 20/10/2023
 *
 * This class describe the setup of the TRD detectors including :
 * - version : local versioning of the setup (within the TRD group)
 * - main-type of TRD version : 1/2 D version
 * - sub-type of TRD version : 1, 3, 5, etc
 * - rotation : no. of 90 deg 
 * - gas : gas type
 * 
 * Detector-wise description
 * - DAQ : Data AQuisition for each module (CROB/CRI config)
 * - FEE : Front-End Electronics description for each modue (ASIC config)
 */

#ifndef CBMTRDGEOSETUP_H_
#define CBMTRDGEOSETUP_H_

#include "CbmTrdDefs.h"  // for trd namespace

#include <FairParGenericSet.h>  // for FairParGenericSet
#include <FairTask.h>           // for FairTask

#include <TNamed.h>  // for the base class

#include <map>
#include <string>
#include <vector>

using namespace std;
using namespace cbm::trd;
class FairParamList;
class TGeoNode;
namespace cbm::trd::geo
{
  /** \class Setup
   * \brief Setup meta info for the TRD system to supplement the geometry
   */
  class Setup : public FairParGenericSet {
   public:
    friend class SetupManager;
    class Asic;
    class Module;

    /** \brief Setup descriptor for the TRD setup. The identification should be :       
     * \param[in] n identification of geometry setup to which it is linked.
     * \param[in] t responsible name, email and date of creation.
    **/
    Setup(const char* n, const char* t);
    virtual ~Setup();
    /** \brief Retrieve meta info for the setup according to label. \sa Setup().       
     * \param[in] label description of the meta info to be retrieved. In case of un-registered label return nullptr;
    **/
    virtual const char* GetInfo(const char* label) const;
    /** \brief Get module par by index
     * \param[in] i index of the module in the list */
    virtual int GetModuleId(int i) const;
    /** \brief Get module par by detector id
     * \param[in] detId detector id in the setup */
    virtual const Module* GetModulePar(int detId) const;
    /** \brief Retrieve no of modules in the setup*/
    virtual size_t GetNrOfModules() const { return fModule.size(); }
    /** \brief Retrieve full list of modules in the setup*/
    vector<Module*> GetModules() { return fModule; }
    virtual void addParam(Module* mod);
    /** \brief Write out the setup from object to FairParamList*/
    virtual void putParams(FairParamList*);
    /** \brief Read in the setup from FairParamList*/
    virtual bool getParams(FairParamList*);

   protected:
    int fVersion               = 1;              ///<
    eGas fGas                  = eGas::kNotSet;  ///<
    vector<Module*> fModule    = {};             ///< list of modules defined in the setup
    vector<string> fMetaFields = {};             ///< ordered list of meta info types
    map<string, string> fMeta  = {};             ///< meta info attached to this setup

   private:
    Setup(const Setup&);
    /** \brief Help message for user */
    void Help(const char* lab = nullptr) const;
    /** \brief Parse setup description for meta info */
    size_t Parse();

    ClassDef(cbm::trd::geo::Setup, 1)  // Setup description of the TRD system, accompanying the geometry
  };                                   // class cbm::trd::geo::Setup


  /**
   * \class Setup::Module
   * \brief Meta info for one TRD module
   */
  class Setup::Module : public TNamed {
   public:
    friend class Setup;
    friend class SetupManager;
    Module(const char* n = "", const char* t = "");

    int GetModuleId() const { return (fId == 0xffff ? -1 : 1); }
    int GetType() const { return fType; }
    int GetRotation() const { return fRot; }
    ePadPlane GetFamily() const { return fFamily; }
    eAsic GetFeeType() const { return fFee; }
    eWindow GetWindowType() const { return fWindow; }
    vector<int> GetDaq() const { return fDaq; }
    vector<Asic*> GetFEE() const { return fFEE; }

   protected:
    /** \brief Read info relevant for the module from the geometry*/
    virtual InitStatus init(TGeoNode*);
    /** \brief Write out the setup from object to FairParamList*/
    virtual void putParams(FairParamList*);
    /** \brief Read in the setup from FairParamList*/
    virtual bool getParams(FairParamList*);

    uint16_t fId       = 0xffff;              ///< TRD chamber id in the setup
    ePadPlane fFamily  = ePadPlane::kNotSet;  ///< Chamber family (1D / 2D)
    eAsic fFee         = eAsic::kNotSet;      ///< ASIC family (SPADIC/FASP)
    eWindow fWindow    = eWindow::kNotSet;    ///< Entrance window type
    eGas fGas          = eGas::kNotSet;       ///< gas type
    int fType          = 1;                   ///< TRD chamber sub-type (e.g. 1, 3, 5, 7 for TRD1D)
    int fFeeType       = 1;                   ///< FEB type for each FEE version
    int fRot           = 0;                   ///< rotation of chamber in steps of 90 deg
    vector<int> fDaq   = {};                  ///<
    vector<Asic*> fFEE = {};                  ///<
   private:
    Module(const Module&);

    ClassDef(cbm::trd::geo::Setup::Module, 1)  // Setup description of a TRD module
  };                                           // class cbm::trd::geo::Setup::Module


  /**
   * \class Setup::Asic
   * \brief Meta info for one TRD ASIC
   */
  class Setup::Asic : public TNamed {
   public:
    friend class Setup;
    friend class SetupManager;
    Asic(const char* n, const char* t);
    int GetMask() const { return fMask; }
    int GetId() const { return (fId == 0xff ? -1 : fId); }
    int GetUniqueId() const { return (fUnique == 0xffff ? -1 : fUnique); }
    vector<int> GetPads() const { return fPad; }

   private:
    Asic(const Asic&);

    uint8_t fId      = 0xff;    ///< ASIC id in the chamber
    uint16_t fUnique = 0xffff;  ///< ASIC id in the production
    uint16_t fMask   = 0xffff;  ///<
    vector<int> fPad = {};      ///<

    ClassDef(cbm::trd::geo::Setup::Asic, 1)  // Setup description of an ASIC
  };                                         // class cbm::trd::geo::Setup::Asic


  /**
   * \class SetupManager
   * \brief Generate setup meta info for the TRD system
   */
  class SetupManager : public FairTask {
   public:
    /** \brief Default constructor.*/
    SetupManager() : FairTask("TrdSetupManager") { ; }
    /** \brief Destructor.*/
    virtual ~SetupManager() { ; }
    /** \brief Inherited from FairTask.*/
    virtual InitStatus Init();
    /** \brief Inherited from FairTask.*/
    virtual void SetParContainers();
    /** \brief Inherited from FairTask.*/
    virtual void Exec(Option_t*) { ; }
    /** \brief Inherited from FairTask.*/
    virtual void Finish();

    void SetContact(const char* contact) { fContact = contact; }
    void SetDescription(const char* text) { fDescription = text; }

   private:
    SetupManager(const SetupManager&);
    SetupManager& operator=(const SetupManager&);
    /** \brief .*/
    void CreateModuleParameters(const TString& path);
    /** \brief .*/
    bool CreateParFilesFromGeometry(TString outDir = "");

    string fGeoTag      = "";       ///< the setup name
    string fContact     = "";       ///< contact info (usual email) of the responsible
    string fDescription = "";       ///< further description of the current setup
    Setup* fSetup       = nullptr;  ///< the setup object

    ClassDef(cbm::trd::geo::SetupManager, 1)  // Manages the creation of meta info for the TRD system setup
  };                                          // class cbm::trd::geo::SetupManager
}  // namespace cbm::trd::geo
#endif  // CBMTRDGEOSETUP_H_
