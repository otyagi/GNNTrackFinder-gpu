/* Copyright (C) 2022-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Volker Friese [committer], Sergei Zharko */


#ifndef CBMBMONDIGI_H
#define CBMBMONDIGI_H 1

#include "CbmDefs.h"
#include "CbmTofAddress.h"

#ifndef NO_ROOT
#include <Rtypes.h>  // for ClassDef
#endif

#include <boost/serialization/access.hpp>
#include <boost/serialization/base_object.hpp>

#include <cstdint>

#ifndef CBMTOFDIGI_H
class CbmTofDigi;  // For declaration of the conversion constructor without starting a cyclic dependency
#endif             // CBMTOFDIGI_H

/** @class CbmBmonDigi
 ** @brief Data class for a signal in the t-zero detector
 **
 ** The current implementation is a placeholder, storing just the event time. To be replaced
 ** with a real detector measurement model once available.
 **/
class CbmBmonDigi {

public:
  /** @brief Default Constructor
   **/
  CbmBmonDigi() = default;

  /** @brief Constructor
   ** @param time Measurement time [ns]
   ** @param charge Measured charge]
   ** @param address [32b CbmAddress]
   **/
  CbmBmonDigi(int32_t addr, double time, float charge) : fAddress(addr), fTime(time), fCharge(charge) {};


  /** @brief Constructor
   ** @param reference to CbmTofDigi (equivalent content)
   **/
  CbmBmonDigi(const CbmTofDigi& digi);


  /** @brief Constructor
   ** @param pointer to const CbmTofDigi object (equivalent content)
   **/
  CbmBmonDigi(const CbmTofDigi* digi);


  /** @brief Destructor **/
  ~CbmBmonDigi() {};

  /** @brief Get the desired name of the branch for this objects in the cbm output tree  (static)
   ** @return "BmonDigi"
   **/
  static const char* GetBranchName() { return "BmonDigi"; }


  /** @brief Class name (static)
   ** @return CbmBmonDigi
   **/
  static const char* GetClassName() { return "CbmBmonDigi"; }


  /** System ID (static)
  ** @return System identifier (EcbmModuleId)
  **/
  static ECbmModuleId GetSystem() { return ECbmModuleId::kBmon; }

  /** @brief Address
   ** @return Address
   **/
  int32_t GetAddress() const { return fAddress; }


  /** @brief Time
   ** @return Time of measurement [ns]
   **/
  double GetTime() const { return fTime; }

  /**
   **  @brief Gets channel ID
   **  @return Channel ID
   **/
  int32_t GetChannel() const { return CbmTofAddress::GetChannelId(fAddress); }

  /** @brief Charge
   ** @return Charge
   **/
  double GetCharge() const { return fCharge; }


  /** @brief Set the address
   ** @param address [32b CbmAddress]
   **/
  void SetAddress(int32_t addr) { fAddress = addr; }


  /** @brief Set the measurement time
   ** @param time Measurement time [ns]
   **/
  void SetTime(double time) { fTime = time; }


  /** @brief Set the measured charge
   ** @param charge Charge
   **/
  void SetCharge(float charge) { fCharge = charge; }


private:
 // FIXME: SZh 5.2.2025: change address type int32_t -> uint32_t
 int32_t fAddress = ToIntegralType<ECbmModuleId>(ECbmModuleId::kBmon);  ///< Unique CBM address
 double fTime     = -1.;                                                ///< Time of signal in BMON [ns]
 float fCharge    = -1.;                                                ///< Charge

 friend class boost::serialization::access;

 template<class Archive>
 void serialize(Archive& ar, const unsigned int /*version*/)
 {
   ar& fAddress;
   ar& fTime;
   ar& fCharge;
 }

#ifndef NO_ROOT
  ClassDefNV(CbmBmonDigi, 1);
#endif
};

#endif /* CBMBMONDIGI_H */
