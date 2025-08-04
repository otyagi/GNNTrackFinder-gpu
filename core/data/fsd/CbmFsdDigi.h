/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Pierre-Alain Loizeau, Lukas Chlad [committer] */

/** \file CbmFsdDigi.h
 ** \author Lukas Chlad <l.chlad@gsi.de>
 ** \date 15.06.2023
 **/

/** \class CbmFsdDigi
 ** \brief Data class for FSD digital information
 ** \version 1
 **
 ** Unique Address:        32 bits following CbmFsdAddress
 ** Time:                  64 bits double
 ** Energy deposition:     64 bits double
 **/

#ifndef CBMFSDDIGI_H
#define CBMFSDDIGI_H 1

#include "CbmDefs.h"        // for ECbmModuleId::kFsd
#include "CbmFsdAddress.h"  // for CbmFsdAddress

#ifndef NO_ROOT
#include <Rtypes.h>  // for ClassDefNV
#endif

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include <cstdint>
#include <string>  // for string


class CbmFsdDigi {

public:
  /**@brief Default constructor.
       **/
  CbmFsdDigi() {}


  /** @brief Constructor with assignment
       ** @param address Unique channel address
       ** @param time    Time [ns]
       ** @param edep    Energy deposition
       **/
  CbmFsdDigi(uint32_t address, double time, double edep) : fAddress(address), fTime(time), fEdep(edep) {}


  /** @brief Constructor with detailed assignment.
       ** @param unitId        Unit Identifier
       ** @param moduleId      Module Identifier
       ** @param photodetId    PhotoDetector Identifier
       ** @param time          Time [ns]
       ** @param edep          Energy deposition
       **/
  CbmFsdDigi(uint32_t unitId, uint32_t moduleId, uint32_t photodetId, double time, double edep)
    : fAddress(CbmFsdAddress::GetAddress(unitId, moduleId, photodetId))
    , fTime(time)
    , fEdep(edep)
  {
  }


  /** Destructor **/
  ~CbmFsdDigi() {}


  /** @brief Class name (static)
       ** @return CbmFsdDigi
       **/
  static const char* GetClassName() { return "CbmFsdDigi"; }


  /** @brief Address
       ** @return Unique channel address (see CbmFsdAddress)
       **/
  uint32_t GetAddress() const { return fAddress; };


  /** @brief Get the desired name of the branch for this obj in the cbm output tree  (static)
   ** @return "FsdDigi"
   **/
  static const char* GetBranchName() { return "FsdDigi"; }


  /** @brief Time
       ** @return Time [ns]
       **/
  double GetTime() const { return fTime; };


  /** @brief Charge
       ** @return Charge (energy deposition)
       **
       ** Alias for GetEdep(), for compatibility with template methods
       */
  double GetCharge() const { return fEdep; };


  /** @brief Energy deposit
       ** @return Energy deposit
       **/
  double GetEdep() const { return fEdep; };


  /** @brief Module Identifier
       ** @return Module number
       **/
  double GetModuleID() const
  {
    return CbmFsdAddress::GetElementId(GetAddress(), static_cast<int32_t>(CbmFsdAddress::Level::Module));
  }


  /** @brief Unit Identifier
       ** @return Unit number
       **/
  double GetUnitID() const
  {
    return CbmFsdAddress::GetElementId(GetAddress(), static_cast<int32_t>(CbmFsdAddress::Level::Unit));
  }


  /** @brief PhotoDet Identifier
       ** @return PhotoDet number
       **/
  double GetPhotoDetID() const
  {
    return CbmFsdAddress::GetElementId(GetAddress(), static_cast<int32_t>(CbmFsdAddress::Level::PhotoDet));
  }

  /** @brief System identifier
       ** @return System ID (ECbmModuleId)
       **/
  static ECbmModuleId GetSystem() { return ECbmModuleId::kFsd; }


  /** Modifiers **/
  void SetAddress(uint32_t address) { fAddress = address; };
  void SetAddress(uint32_t unitId, uint32_t moduleId, uint32_t photodetId);
  void SetTime(double time) { fTime = time; }
  void SetEdep(double edep) { fEdep = edep; }


  /** @brief String output
       ** @return Info
       **/
  std::string ToString() const;


private:
  uint32_t fAddress = 0;    /// Unique channel address
  double fTime      = -1.;  /// Time of measurement [ns]
  double fEdep      = 0.;   /// Energy deposition from FPGA [MeV]

  /// BOOST serialization interface
  friend class boost::serialization::access;
  template<class Archive>
  void serialize(Archive& ar, const unsigned int /*version*/)
  {
    ar& fAddress;
    ar& fTime;
    ar& fEdep;
  }

#ifndef NO_ROOT
  ClassDefNV(CbmFsdDigi, 1);
#endif
};

#endif  // CBMFSDDIGI_H
