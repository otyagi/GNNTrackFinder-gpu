/* Copyright (C) 2013-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Florian Uhlig [committer], Norbert Herrmann, Sergei Zharko */

/** @file CbmTofAddress.h
 ** @author Pierre-Alain Loizeau <loizeau@physi.uni-heidelberg.de>
 ** @date 07.06.2013
 **/

/** @class CbmTofAddress
 ** @brief CBM ToF interface class to the unique address
 ** @author Pierre-Alain Loizeau <loizeau@physi.uni-heidelberg.de>
 ** @version 1.0
 **
 ** CbmTofAddress is the class for the concrete interfaces to the
 ** unique address, which is encoded in a 32-bit field (int32_t), for the
 ** ToF detector elements.
 ** Difference to CbmTofDetectorId is that this class is adapted to
 ** real data instead of simulated data => no Gap info but instead info
 ** on strip side.
 ** Conversion functions are provided for now, but version dependent!
 **
 ** Bit table since transition to v21a
 **                                   3         2         1         0 Shift Bits Values
 ** Current definition:              10987654321098765432109876543210
 ** System ID (kTof=6) on bits  0- 3 00000000000000000000000000001111  << 0    4     15
 ** Super Module (SM)  on bits  4-10 00000000000000000000011111110000  << 4    7    128
 ** SM Type            on bits 11-14 00000000000000000111100000000000  <<11    4     15
 ** RPC ID             on bits 15-20 00000000000111111000000000000000  <<15    6     64
 ** Channel Side       on bits 21-21 00000000001000000000000000000000  <<21    1      2
 ** Channel ID         on bits 22-27 00001111110000000000000000000000  <<22    6     64
 ** RPC Type           on bits 28-31 11110000000000000000000000000000  <<28    4     16
 **
 ** Changing the number of bits of a fields automatically shift all others
 ** => to adapt field length, just change its size and the size of one of the other fields
 **/

#ifndef CBMTOFADDRESS_H
#define CBMTOFADDRESS_H 1

#include "CbmAddress.h"             // for CbmAddress, CbmAddress::fgkSystem...
#include "CbmDefs.h"                // for kTof
#include "CbmTofDetectorId.h"       // for CbmTofDetectorInfo
#include "CbmTofDetectorId_v12b.h"  // for CbmTofDetectorId_v12b

#include <cstdint>
#include <string>

class CbmTofAddress : public CbmAddress {
public:
  /** Constructor  **/
  CbmTofAddress() {};

  /** Destructor  **/
  virtual ~CbmTofAddress() {};

  /** Component bitmasks **/
  /** Gets a bitmask for the System bit field
   ** @return a bitmask with the bits set only for the System bit field
   **/
  static int32_t GetSystemIdBitmask() { return fgkSystemIdBitmask; }
  /** Gets a bitmask for the Super Module ID bit field
   ** @return a bitmask with the bits set only for the Super Module ID bit field
   **/
  static int32_t GetSmIdBitmask() { return fgkSmIdBitmask; }
  /** Gets a bitmask for the Super Module Type bit field
   ** @return a bitmask with the bits set only for the Super Module Type bit field
   **/
  static int32_t GetSmTypeBitmask() { return fgkSmTypeBitmask; }
  /** Gets a bitmask for the RPC ID bit field
   ** @return a bitmask with the bits set only for the RPC ID bit field
   **/
  static int32_t GetRpcIdBitmask() { return fgkRpcIdBitmask; }
  /** Gets a bitmask for the Channel Side bit field
   ** @return a bitmask with the bits set only for the Channel Side bit field
   **/
  static int32_t GetChannelSideBitmask() { return fgkChannelSideBitmask; }
  /** Gets a bitmask for the Channel ID bit field
   ** @return a bitmask with the bits set only for the Channel ID bit field
   **/
  static int32_t GetChannelIdBitmask() { return fgkChannelIdBitmask; }
  /** Gets a bitmask for the RPC Type bit field
   ** @return a bitmask with the bits set only for the RPC Type bit field
   **/
  static int32_t GetRpcTypeBitmask() { return fgkRpcTypeBitmask; }


  /** Field size accessors **/
  /** Number of bits for Super Module Id in the address field
   ** @return Number of bits
   **/
  static int32_t GetNofSmIdBits() { return fgkSmIdBits; };
  /** Number of bits for Super Module Type in the address field
   ** @return Number of bits
   **/
  static int32_t GetNofSmTypeBits() { return fgkSmTypeBits; };
  /** Number of bits for Rpc Id in the address field
   ** @return Number of bits
   **/
  static int32_t GetNofRpcIdBits() { return fgkRpcIdBits; };
  /** Number of bits for Channel Id in the address field
   ** @return Number of bits
   **/
  static int32_t GetNofChannelIdBits() { return fgkChannelIdBits; };
  /** Number of bits for Channel Side in the address field
   ** @return Number of bits
   **/
  static int32_t GetNofChSideBits() { return fgkChannelSideBits; };


  /** Bit field offsets **/
  /** Gets and offset for the System bit field
   ** @return and offset with the bits set only for the System bit field
   **/
  static int32_t GetSmIdOffset() { return fgkSmIdOffset; }
  /** Gets and offset for the Super Module Type bit field
   ** @return and offset with the bits set only for the Super Module Type bit field
   **/
  static int32_t GetSmTypeOffset() { return fgkSmTypeOffset; }
  /** Gets and offset for the RPC ID bit field
   ** @return and offset with the bits set only for the RPC ID bit field
   **/
  static int32_t GetRpcIdOffset() { return fgkRpcIdOffset; }
  /** Gets and offset for the Channel Side bit field
   ** @return and offset with the bits set only for the Channel Side bit field
   **/
  static int32_t GetChannelSideOffset() { return fgkChannelSideOffset; }
  /** Gets and offset for the Channel ID bit field
   ** @return and offset with the bits set only for the Channel ID bit field
   **/
  static int32_t GetChannelIdOffset() { return fgkChannelIdOffset; }
  /** Gets and offset for the RPC Type bit field
   ** @return and offset with the bits set only for the RPC Type bit field
   **/
  static int32_t GetRpcTypeOffset() { return fgkRpcTypeOffset; }

  /** Maskers **/
  /** Get the Super Module Id from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetSmId(uint32_t address) { return ((address >> fgkSmIdOffset) & ((1 << fgkSmIdBits) - 1)); };
  /** Get the Super Module Type from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetSmType(uint32_t address) { return ((address >> fgkSmTypeOffset) & ((1 << fgkSmTypeBits) - 1)); };
  /** Get the Rpc Id from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetRpcId(uint32_t address) { return ((address >> fgkRpcIdOffset) & ((1 << fgkRpcIdBits) - 1)); };
  /** Get the Rpc Type from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetRpcType(uint32_t address) { return ((address >> fgkRpcTypeOffset) & ((1 << fgkRpcTypeBits) - 1)); };
  /** Get the Channel Id from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetChannelId(uint32_t address)
  {
    return ((address >> fgkChannelIdOffset) & ((1 << fgkChannelIdBits) - 1));
  };
  /** Get the Channel Side from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetChannelSide(uint32_t address)
  {
    return ((address >> fgkChannelSideOffset) & ((1 << fgkChannelSideBits) - 1));
  };
  /** Get the module Full Id from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetModFullId(uint32_t address) { return (address & fgkiModFullIdMask); };
  /** Get the detector Full Id (module ID without RPC type) from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetRpcFullId(uint32_t address) { return (address & fgkiRpcFullIdMask); };
  /** Get the strip Full Id from the address
   ** @param address  Unique address
   ** @return  systemId
   **/
  static int32_t GetStripFullId(uint32_t address) { return (address & fgkiStripFullIdMask); };

  /** Builder **/
  /** Get the unique address from all parameters
   ** @param[in] Sm      Super Module Id.
   ** @param[in] Rpc     Rpc Id.
   ** @param[in] Channel Channel Id.
   ** @param[in] Side    Channel Side (optional, used for strips).
   ** @param[in] Sm Type Super Module Type (optional).
   ** @return  address
   **/
  static uint32_t GetUniqueAddress(uint32_t Sm, uint32_t Rpc, uint32_t Channel, uint32_t Side = 0, uint32_t SmType = 0,
                                   uint32_t RpcType = 0)
  {
    return (uint32_t)(((ToIntegralType(ECbmModuleId::kTof) & ((1 << fgkSystemBits) - 1)))
                      + ((Sm & ((1 << fgkSmIdBits) - 1)) << fgkSmIdOffset)
                      + ((SmType & ((1 << fgkSmTypeBits) - 1)) << fgkSmTypeOffset)
                      + ((Side & ((1 << fgkChannelSideBits) - 1)) << fgkChannelSideOffset)
                      + ((Rpc & ((1 << fgkRpcIdBits) - 1)) << fgkRpcIdOffset)
                      + ((Channel & ((1 << fgkChannelIdBits) - 1)) << fgkChannelIdOffset)
                      + ((RpcType & ((1 << fgkRpcTypeBits) - 1)) << fgkRpcTypeOffset));
  };

  static bool SameModule(uint32_t addressA, uint32_t addressB)
  {
    return (GetModFullId(addressA) == GetModFullId(addressB)) ? true : false;
  };
  /**
   ** @brief Sets the channel ID to the address 
   **/
  static uint32_t ConvertCbmTofDetectorInfo(CbmTofDetectorInfo infoInput)
  {
    // For now assume that the system ID will always be correct
    // Otherwise would need including CbmDetectorList.h
    //   if( kTof != infoInput.fDetectorSystem)
    //      return 0;

    return GetUniqueAddress(infoInput.fSModule, infoInput.fCounter, infoInput.fCell, 0, infoInput.fSMtype,
                            infoInput.fCounterType);
  };
  static uint32_t ConvertCbmTofDetectorId(int32_t detIdInput)
  {
    // For now assume that the system ID will always be correct
    // Otherwise would need including CbmDetectorList.h
    //         if( kTof != CbmTofDetectorId::GetSystemId( detIdInput ) )
    //            return 0;
    CbmTofDetectorId_v12b detId;
    return GetUniqueAddress(detId.GetSModule(detIdInput), detId.GetCounter(detIdInput), detId.GetCell(detIdInput), 0,
                            detId.GetSMType(detIdInput));
  };
  /** String representation of the address
   ** @param address  Unique address
   ** @return String representation of the address
   **/
  static std::string ToString(int32_t address);

 private:
  // FIXME: SZh 7.2.2025: Make these fields constexpr
  /**
   ** To adapt the address sub-fields repartition in size,
   ** you just need to change number of bits of the two sub-fields changing length.
   **/

  /** Sub-fields sizes in bits   **/

  // v14a
  /*
  // Number of bits for Super Module Id in the address field
  static const int32_t fgkSmIdBits = 8;
  // Number of bits for Super Module Type in the address field
  static const int32_t fgkSmTypeBits = 4;
  // Number of bits for Rpc Id in the address field
  static const int32_t fgkRpcIdBits = 7;
  // Number of bits for Channel Side in the address field
  static const int32_t fgkChannelSideBits = 1;
  // Number of bits for Channel Id in the address field
  static const int32_t fgkChannelIdBits = 8;
  // Number of bits for Rpc Type in the address field
  static const int32_t fgkRpcTypeBits = 0;
  */
  // v21a
  // Number of bits for Super Module Id in the address field
  static const int32_t fgkSmIdBits = 7;
  // Number of bits for Super Module Type in the address field
  static const int32_t fgkSmTypeBits = 4;
  // Number of bits for Rpc Id in the address field
  static const int32_t fgkRpcIdBits = 6;
  // Number of bits for Channel Side in the address field
  static const int32_t fgkChannelSideBits = 1;
  // Number of bits for Channel Id in the address field
  static const int32_t fgkChannelIdBits = 6;
  // Number of bits for Rpc Type in the address field
  static const int32_t fgkRpcTypeBits = 4;
  /**
   ** To adapt the address sub-fields repartition in order,
   ** you just need to change the way the offset are calculated.
   **/

  /** Sub-fields offsets in bits **/
  /** Offset in bits for Super Module Id in the address field  **/
  static const int32_t fgkSmIdOffset;
  /** Offset in bits for Super Module Type in the address field  **/
  static const int32_t fgkSmTypeOffset;
  /** Offset in bits for Channel Side Id in the address field  **/
  static const int32_t fgkChannelSideOffset;
  /** Offset in bits for Rpc Id in the address field  **/
  static const int32_t fgkRpcIdOffset;
  /** Offset in bits for Channel Id in the address field  **/
  static const int32_t fgkChannelIdOffset;
  /** Offset in bits for Channel Id in the address field  **/
  static const int32_t fgkRpcTypeOffset;

  /**
   ** For the module Full Id determination
   **/
  static const int32_t fgkiModFullIdMask;

  /**
   ** For the RPC Full Id determination (ignore RPC type and side)
   **/
  static const int32_t fgkiRpcFullIdMask;

  /**
   ** For the detector strip Id determination (ignore RPC type and side)
   **/
  static const int32_t fgkiStripFullIdMask;


  /** Individual bitmasks for each bit field **/
  static const int32_t fgkSystemIdBitmask;     //<  A bitmask for System ID
  static const int32_t fgkSmIdBitmask;         //<  A bitmask for SM
  static const int32_t fgkSmTypeBitmask;       //<  A bitmask for SM Type
  static const int32_t fgkRpcIdBitmask;        //<  A bitmask for RPC ID
  static const int32_t fgkChannelSideBitmask;  //<  A bitmask for Channel Side
  static const int32_t fgkChannelIdBitmask;    //<  A bitmask for Channel ID
  static const int32_t fgkRpcTypeBitmask;      //<  A bitmask for RPC Type
};

#endif  // CBMTOFADDRESS_H
