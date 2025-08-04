/* Copyright (C) 2015-2024 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev, Martin Beyer, Andrey Lebedev [committer], Florian Uhlig */

/*
 * CbmRichDigiMap.h
 *
 *  Created on: Dec 17, 2015
 *      Author: slebedev
 */

#ifndef RICH_DETECTOR_CBMRICHDIGIMAPMANAGER_H_
#define RICH_DETECTOR_CBMRICHDIGIMAPMANAGER_H_

#include <RtypesCore.h>  // for Int_t
#include <TString.h>     // for TString

#include <iostream>  // for string
#include <map>       // for map
#include <vector>    // for vector

class CbmRichPixelData;
class CbmRichPmtData;

class CbmRichDigiMapManager {
 private:
  CbmRichDigiMapManager();

 public:
  /**
   * \brief Return Instance of CbmRichGeoManager.
   */
  static CbmRichDigiMapManager& GetInstance()
  {
    static CbmRichDigiMapManager fInstance;
    return fInstance;
  }

  /**
   * \brief Return digi address by path to node.
   */
  Int_t GetPixelAddressByPath(const std::string& path);

  /**
   * \brief Return CbmRichDataPixel by digi address.
   */
  CbmRichPixelData* GetPixelDataByAddress(Int_t address);

  /**
   * \brief Return random address. Needed for noise digi.
   */
  Int_t GetRandomPixelAddress();

  /**
   * \brief Return addresses of all pixels
   */
  std::vector<Int_t> GetPixelAddresses();

  /**
   * \brief Return ids for all pmts
   */
  std::vector<Int_t> GetPmtIds();

  /**
   * \brief Return CbmRichDataPmt by id.
   */
  CbmRichPmtData* GetPmtDataById(Int_t id);

  /**
   * \brief Return the addresses of the neighbour pixels.
   * \param address Pixel address
   * \param n Size of the grid (2n+1)*(2n+1)
   * \param horizontal return horizontal neighbours
   * \param vertical return vertical neighbours
   * \param diagonal return diagonal neighbours
   */
  std::vector<Int_t> GetNeighbourPixels(Int_t address, Int_t N, Bool_t horizontal = true, Bool_t vertical = true,
                                        Bool_t diagonal = true);

  /**
   * \brief Return the addresses of the direct neighbour pixels.
   * \param address Pixel address
   */
  std::vector<Int_t> GetDirectNeighbourPixels(Int_t address, Bool_t horizontal = true, Bool_t vertical = true)
  {
    return GetNeighbourPixels(address, 1, horizontal, vertical, false);
  }

  /**
   * \brief Return the addresses of the diagonal neighbour pixels.
   * \param address Pixel address
   */
  std::vector<Int_t> GetDiagonalNeighbourPixels(Int_t address)
  {
    return GetNeighbourPixels(address, 1, false, false, true);
  }

  /**
   * \brief Return the addresses of pixels in a (2n+1)*(2n+1) grid,
   * with the address pixel in the center of the grid.
   * Addresses are limited to the same MAPMT as the input address.
   * Needed for noise digis caused by charged particles.
   * \param address Pixel address
   * \param n Size of the grid (2n+1)*(2n+1)
   */
  std::vector<Int_t> GetNxNNeighbourPixels(Int_t address, Int_t n)
  {
    return GetNeighbourPixels(address, n, true, true, true);
  }

 public:
  virtual ~CbmRichDigiMapManager();

 private:
  std::map<std::string, Int_t> fPixelPathToAddressMap;
  std::map<Int_t, CbmRichPixelData*> fPixelAddressToDataMap;
  std::vector<Int_t> fPixelAddresses;  // vector of all  pixel addresses

  std::map<std::string, Int_t> fPmtPathToIdMap;
  std::map<Int_t, CbmRichPmtData*> fPmtIdToDataMap;
  std::vector<Int_t> fPmtIds;

  int getDetectorSetup(TString const nodePath);

  /**
   * \brief Initialize maps.
   */
  void Init();

  /**
   * \brief Copy constructor.
   */
  CbmRichDigiMapManager(const CbmRichDigiMapManager&);

  /**
   * \brief Assignment operator.
   */
  CbmRichDigiMapManager& operator=(const CbmRichDigiMapManager&);
};

#endif /* RICH_DETECTOR_CBMRICHDIGIMAPMANAGER_H_ */
