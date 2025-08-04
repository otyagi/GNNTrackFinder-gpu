/* Copyright (C) 2013-2025 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pierre-Alain Loizeau, Florian Uhlig [committer], Norbert Herrmann, Segei Zharko */

/** @file CbmTofAddress.cxx
 ** @author Pierre-Alain Loizeau <loizeau@physi.uni-heidelberg.de>
 ** @date 07.06.2013
 **
 ** Empty file, just there for the sake of the build system.
 **/

#include "CbmTofAddress.h"

#include <iomanip>
#include <sstream>

// It seems C++ standard force the initialization to be in cxx/cpp file (outside of class definition)
// When not trivial constant values => To check if it should apply also to simple values maybe?
/** Offset in bits for Super Module Id in the address field  **/
const int32_t CbmTofAddress::fgkSmIdOffset = CbmAddress::fgkSystemBits;
/** Offset in bits for Super Module Type in the address field  **/
const int32_t CbmTofAddress::fgkSmTypeOffset = CbmTofAddress::fgkSmIdBits + CbmTofAddress::fgkSmIdOffset;
/** Offset in bits for Rpc Id in the address field  **/
const int32_t CbmTofAddress::fgkRpcIdOffset = CbmTofAddress::fgkSmTypeBits + CbmTofAddress::fgkSmTypeOffset;
/** Offset in bits for Channel Side Id in the address field  **/
const int32_t CbmTofAddress::fgkChannelSideOffset = CbmTofAddress::fgkRpcIdBits + CbmTofAddress::fgkRpcIdOffset;
/** Offset in bits for Channel Id in the address field  **/
const int32_t CbmTofAddress::fgkChannelIdOffset =
  CbmTofAddress::fgkChannelSideBits + CbmTofAddress::fgkChannelSideOffset;
/** Offset in bits for Rpc Type in the address field  **/
const int32_t CbmTofAddress::fgkRpcTypeOffset = CbmTofAddress::fgkChannelIdBits + CbmTofAddress::fgkChannelIdOffset;

const int32_t CbmTofAddress::fgkiModFullIdMask =
  (((1 << fgkSystemBits) - 1)) + (((1 << fgkSmIdBits) - 1) << fgkSmIdOffset)
  + (((1 << fgkSmTypeBits) - 1) << fgkSmTypeOffset) + (((1 << fgkRpcIdBits) - 1) << fgkRpcIdOffset)
  + (((1 << fgkRpcTypeBits) - 1) << fgkRpcTypeOffset);

const int32_t CbmTofAddress::fgkiRpcFullIdMask =
  (((1 << fgkSystemBits) - 1)) + (((1 << fgkSmIdBits) - 1) << fgkSmIdOffset)
  + (((1 << fgkSmTypeBits) - 1) << fgkSmTypeOffset) + (((1 << fgkRpcIdBits) - 1) << fgkRpcIdOffset);

const int32_t CbmTofAddress::fgkiStripFullIdMask =
  (((1 << fgkSystemBits) - 1)) + (((1 << fgkSmIdBits) - 1) << fgkSmIdOffset)
  + (((1 << fgkSmTypeBits) - 1) << fgkSmTypeOffset) + (((1 << fgkRpcIdBits) - 1) << fgkRpcIdOffset)
  + (((1 << fgkChannelIdBits) - 1) << fgkChannelIdOffset);

const int32_t CbmTofAddress::fgkSystemIdBitmask    = (1 << CbmAddress::fgkSystemBits) - 1;
const int32_t CbmTofAddress::fgkSmIdBitmask        = ((1 << fgkSmIdBits) - 1) << fgkSmIdOffset;
const int32_t CbmTofAddress::fgkSmTypeBitmask      = ((1 << fgkSmTypeBits) - 1) << fgkSmTypeOffset;
const int32_t CbmTofAddress::fgkRpcIdBitmask       = ((1 << fgkRpcIdBits) - 1) << fgkRpcIdOffset;
const int32_t CbmTofAddress::fgkChannelSideBitmask = ((1 << fgkChannelSideBits) - 1) << fgkChannelSideOffset;
const int32_t CbmTofAddress::fgkChannelIdBitmask   = ((1 << fgkChannelIdBits) - 1) << fgkChannelIdOffset;
const int32_t CbmTofAddress::fgkRpcTypeBitmask     = ((1 << fgkRpcTypeBits) - 1) << fgkRpcTypeOffset;

std::string CbmTofAddress::ToString(int32_t address)
{
  using std::setfill;
  using std::setw;
  std::stringstream msg;
  msg << std::hex << "0x" << setw(8) << setfill('0') << address << std::dec << setfill(' ');
  msg << ": SmType=" << setw(3) << CbmTofAddress::GetSmType(address);
  msg << ", Sm=" << setw(2) << CbmTofAddress::GetSmId(address);
  msg << ", Rpc=" << setw(2) << CbmTofAddress::GetRpcId(address);
  msg << ", Ch=" << setw(2) << CbmTofAddress::GetChannelId(address);
  msg << ", Side=" << setw(1) << CbmTofAddress::GetChannelSide(address);
  msg << ", RpcType=" << setw(2) << CbmTofAddress::GetRpcType(address);
  return msg.str();
}
