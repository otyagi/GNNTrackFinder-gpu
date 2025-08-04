/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Andrey Lebedev, Lukas Chlad [committer] */

/** @file CbmFsdAddress.h
 ** @author Lukas Chlad <l.chlad@gsi.de>
 ** @date 16.06.2023
 **
 ** Inspired by CbmStsAddress and only accomodate physical properties of expected FSD detector
 **
 **/

#ifndef CBMFSDADDRESS_H
#define CBMFSDADDRESS_H 1

#include "CbmDefs.h"  // for ECbmModuleId

#include <cassert>  // for assert
#include <cstdint>  // for uint32_t
#include <sstream>  // for string

/** Namespace CbmFsdAddress
 ** @brief Functions to encode or decode the address field of FSD data.
 ** @author Lukas Chlad <l.chlad@gsi.de>
 **
 ** Inspired by the structure used by STS.
 ** The current definition (version 0) of the address bit field for the FSD is:
 **
 ** Level           Bits  Max. Elements  Bit Position
 ** System (kFSD)     4         16          0 -  3
 ** Unit              4         16          4 -  7
 ** Module           15      32768          8 - 22
 ** PhotoDetector     2          4         23 - 24
 ** Unused            3                    25 - 27
 ** Version           4         16         28 - 31
 **/
namespace CbmFsdAddress
{
  /** Enumerator for the hierarchy levels of the FSD setup **/
  enum class Level
  {
    System,
    Unit,
    Module,
    PhotoDet,
    NumLevels
  };

  inline constexpr uint32_t kCurrentVersion = 0;

  // --- These values are not to be changed if backward compatibility
  // --- shall be maintained.
  inline constexpr int32_t kVersionSize  = 4;   // Bits for version number
  inline constexpr int32_t kVersionShift = 28;  // First bit for version number
  inline constexpr int32_t kVersionMask  = (1 << kVersionSize) - 1;

  namespace Detail
  {
    // clang-format off
    // -----    Definition of address bit field   ------------------------------
    inline constexpr uint16_t kBits[kCurrentVersion + 1][static_cast<uint32_t>(CbmFsdAddress::Level::NumLevels)] = {

      // Version 0
      {
        4,  // system
        4,  // unit 
       15,  // module
        2   // photodet
      }

    };
    // -------------------------------------------------------------------------

    // -----    Bit shifts -----------------------------------------------------
    inline constexpr int32_t kShift[kCurrentVersion + 1][static_cast<uint32_t>(CbmFsdAddress::Level::NumLevels)] = {
      {0, kShift[0][0] + kBits[0][0], kShift[0][1] + kBits[0][1], kShift[0][2] + kBits[0][2]}
    };
    // -------------------------------------------------------------------------

    // -----    Bit masks  -----------------------------------------------------
    inline constexpr int32_t kMask[kCurrentVersion + 1][static_cast<uint32_t>(CbmFsdAddress::Level::NumLevels)] = {
      {(1 << kBits[0][0]) - 1, (1 << kBits[0][1]) - 1, (1 << kBits[0][2]) - 1, (1 << kBits[0][3]) - 1}
    };
    // -------------------------------------------------------------------------
    // clang-format on
  }  // Namespace Detail


  /** @brief Construct address
   ** @param unit         Unit index
   ** @param module       Module index in unit
   ** @param photodet     PhotoDetector index in module
   ** @return Unique element address
   **/
  int32_t GetAddress(uint32_t Unit = 0, uint32_t Module = 0, uint32_t PhotoDet = 0, uint32_t Version = kCurrentVersion);


  /** @brief Construct the address of an element from the address of a
   ** descendant element.
   ** @param address Address of descendant element
   ** @param level   Desired hierarchy level
   ** @return Address of element at desired hierarchy level
   **
   ** This strips of the address information of all hierarchy levels
   ** below the desired one.
   **/
  int32_t GetMotherAddress(int32_t address, int32_t level);


  /** @brief Get the index of an element
   ** @param address Unique element address
   ** @param level Hierarchy level
   ** @return Element index
   **/
  uint32_t GetElementId(int32_t address, int32_t level);


  /** @brief Get system Id (should be integer value of ECbmModuleId::kFsd)
   ** @param address Unique element address
   **/
  uint32_t GetSystemId(int32_t address);


  /** @brief Extract version number
   ** @param address Unique element address
   ** @value Version number
   **
   ** The version is encoded in the last 4 bits (28 to 31).
   ** The maximal number of versions is 16.
   **/
  uint32_t GetVersion(int32_t address);


  /** @brief Set the index of an element, leaving the other element levels
   ** untouched
   ** @param address Unique element address
   ** @param level   Hierarchy level
   ** @param newId   New element index
   ** @return New address
   **/
  int32_t SetElementId(int32_t address, int32_t level, uint32_t newId);

  /** @brief String output
   ** @param address Unique element address
   **/
  std::string ToString(int32_t address);

}  // Namespace CbmFsdAddress


#endif  // CBMFSDADDRESS_H
