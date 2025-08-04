/* Copyright (C) 2015-2024 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev, Martin Beyer, Adrian Amatus Weber, Andrey Lebedev [committer], Florian Uhlig */

/*
 * CbmRichDigiMapManager.cxx
 *
 *  Created on: Dec 17, 2015
 *      Author: slebedev
 */
#include "CbmRichDigiMapManager.h"

#include "CbmRichDetectorData.h"  // for CbmRichPmtData, CbmRichPixelData

#include <Logger.h>  // for LOG, Logger

#include <TGeoBBox.h>     // for TGeoBBox
#include <TGeoManager.h>  // for TGeoManager, gGeoManager
#include <TGeoMatrix.h>   // for TGeoMatrix
#include <TGeoNode.h>     // for TGeoIterator, TGeoNode
#include <TGeoVolume.h>   // for TGeoVolume
#include <TRandom.h>      // for TRandom, gRandom

#include <string>   // for operator<, stoul
#include <tuple>    // for tuple, make_tuple, get
#include <utility>  // for pair

#include <stddef.h>  // for size_t

using namespace std;

CbmRichDigiMapManager::CbmRichDigiMapManager()
  : fPixelPathToAddressMap()
  , fPixelAddressToDataMap()
  , fPixelAddresses()
  , fPmtPathToIdMap()
  , fPmtIdToDataMap()
  , fPmtIds()
{
  Init();
}

CbmRichDigiMapManager::~CbmRichDigiMapManager() {}

void CbmRichDigiMapManager::Init()
{
  // temporary map storing local x and y coordinates of pixels in its PMT volume
  std::map<Int_t, tuple<Double_t, Double_t>> localPixelCoord;

  fPixelPathToAddressMap.clear();
  fPixelAddressToDataMap.clear();
  fPixelAddresses.clear();

  fPmtPathToIdMap.clear();
  fPmtIdToDataMap.clear();
  fPmtIds.clear();

  Int_t currentPixelAddress = 1;
  Int_t currentPmtId        = 1;

  // Get MAPMT dimensions
  Double_t pmtHeight    = 5.2;
  Double_t pmtWidth     = 5.2;
  TGeoVolume* pmtVolume = gGeoManager->FindVolumeFast("pmt");
  if (pmtVolume == nullptr) pmtVolume = gGeoManager->FindVolumeFast("pmt_vol_0");
  if (pmtVolume != nullptr) {
    const TGeoBBox* shape = (const TGeoBBox*) (pmtVolume->GetShape());
    if (shape != nullptr) {
      pmtHeight = 2. * shape->GetDY();
      pmtWidth  = 2. * shape->GetDX();
    }
  }

  TGeoIterator geoIterator(gGeoManager->GetTopNode()->GetVolume());
  geoIterator.SetTopName("/cave_1");
  TGeoNode* curNode;
  // PMT plane position\rotation
  TString pixelNameStr("pmt_pixel");
  geoIterator.Reset();
  while ((curNode = geoIterator())) {
    TString nodeName(curNode->GetName());
    TString nodePath;
    if (TString(curNode->GetVolume()->GetName()).Contains(pixelNameStr)) {
      geoIterator.GetPath(nodePath);

      int detSetup = getDetectorSetup(nodePath);
      switch (detSetup) {
        case 0: {
          // RICH detector
          //std::cout<<"RICH"<<std::endl;
          const TGeoMatrix* curMatrix   = geoIterator.GetCurrentMatrix();
          const Double_t* curNodeTr     = curMatrix->GetTranslation();
          const TGeoMatrix* localMatrix = curNode->GetMatrix();  // local transformation of pixel in MAPMT volume
          string path                   = string(nodePath.Data());

          size_t pmtInd = path.find_last_of("/");
          if (string::npos == pmtInd) continue;
          string pmtPath = path.substr(0, pmtInd + 1);

          fPixelPathToAddressMap.insert(pair<string, Int_t>(path, currentPixelAddress));
          CbmRichPixelData* pixelData = new CbmRichPixelData();
          pixelData->fX               = curNodeTr[0];
          pixelData->fY               = curNodeTr[1];
          pixelData->fZ               = curNodeTr[2];
          pixelData->fAddress         = currentPixelAddress;
          localPixelCoord[currentPixelAddress] =
            make_tuple(localMatrix->GetTranslation()[0], localMatrix->GetTranslation()[1]);
          fPixelAddressToDataMap.insert(pair<Int_t, CbmRichPixelData*>(pixelData->fAddress, pixelData));
          fPixelAddresses.push_back(currentPixelAddress);
          currentPixelAddress++;

          if (fPmtPathToIdMap.count(pmtPath) == 0) {
            fPmtPathToIdMap.insert(pair<string, Int_t>(pmtPath, currentPmtId));

            CbmRichPmtData* pmtData = new CbmRichPmtData();
            pmtData->fId            = currentPmtId;
            pmtData->fPixelAddresses.push_back(pixelData->fAddress);
            pmtData->fHeight = pmtHeight;
            pmtData->fWidth  = pmtWidth;
            fPmtIdToDataMap.insert(pair<Int_t, CbmRichPmtData*>(pmtData->fId, pmtData));
            pixelData->fPmtId = pmtData->fId;

            fPmtIds.push_back(pmtData->fId);

            currentPmtId++;
          }
          else {
            //cout << "pmtPath old:" << pmtPath << endl;
            Int_t pmtId             = fPmtPathToIdMap[pmtPath];
            CbmRichPmtData* pmtData = fPmtIdToDataMap[pmtId];
            if (pmtData == nullptr || pmtId != pmtData->fId) {
              LOG(error) << "(pmtData == nullptr || pmtId != pmtData->fId) ";
            }
            pmtData->fPixelAddresses.push_back(pixelData->fAddress);
            pixelData->fPmtId = pmtData->fId;
            if (pmtData->fPixelAddresses.size() > 64) {
              LOG(info) << "size:" << pmtData->fPixelAddresses.size() << " pmtData->fId:" << pmtData->fId
                        << " pmtPath:" << pmtPath << endl
                        << " path:" << path;
            }
          }
        } break;

        case 1: {
          //std::cout<<"mRICH"<<std::endl;
          const TGeoMatrix* curMatrix   = geoIterator.GetCurrentMatrix();
          const Double_t* curNodeTr     = curMatrix->GetTranslation();
          const TGeoMatrix* localMatrix = curNode->GetMatrix();  // local transformation of pixel in MAPMT volume
          string path                   = string(nodePath.Data());

          size_t pmtInd = path.find_last_of("/");
          if (string::npos == pmtInd) continue;
          string pmtPath = path.substr(0, pmtInd + 1);

          Int_t channel  = std::stoul(path.substr(pmtInd + 11));  // cut away "pmt_pixel_"
          Int_t posAtPmt = channel / 100;
          channel        = channel % 100;

          size_t pmtVolInd = path.rfind("/", pmtInd - 1);
          if (string::npos == pmtVolInd) continue;
          Int_t pmtPosBP = std::stoul(path.substr(pmtVolInd + 11,
                                                  pmtInd - pmtVolInd - 11));  // cut away "/pmt_vol_*_" ; position on BP

          size_t bpInd = path.rfind("/", pmtVolInd - 1);
          if (string::npos == bpInd) continue;
          Int_t posBP = std::stoul(path.substr(bpInd + 14,
                                               pmtVolInd - bpInd - 14));  // cut away "/pmt_cont_vol_" ; position on BP

          Int_t x = (posBP / 10) + ((((pmtPosBP - 1) / 3) + 1) % 2);
          Int_t y = (posBP % 10) + (2 - ((pmtPosBP - 1) % 3));

          Int_t DiRICH_Add = ((7 & 0xF) << 12) + ((x & 0xF) << 8) + ((y & 0xF) << 4) + (posAtPmt & 0xF);
          Int_t pixelUID   = ((DiRICH_Add << 16) | (channel & 0x00FF));
          Int_t pmtUID     = ((x & 0xF) << 4) + (y & 0xF);
          //std::cout<<"Addr: 0x"<< std::hex << DiRICH_Add << std::dec << "\t" << channel <<"\t"<< std::hex << pixelUID << std::dec << std::endl;

          fPixelPathToAddressMap.insert(pair<string, Int_t>(path, pixelUID));
          CbmRichPixelData* pixelData = new CbmRichPixelData();
          pixelData->fX               = curNodeTr[0];
          pixelData->fY               = curNodeTr[1];
          pixelData->fZ               = curNodeTr[2];
          pixelData->fAddress         = pixelUID;
          localPixelCoord[pixelUID]   = make_tuple(localMatrix->GetTranslation()[0], localMatrix->GetTranslation()[1]);
          fPixelAddressToDataMap.insert(pair<Int_t, CbmRichPixelData*>(pixelData->fAddress, pixelData));
          fPixelAddresses.push_back(pixelUID);
          //currentPixelAddress++;

          if (fPmtPathToIdMap.count(pmtPath) == 0) {
            fPmtPathToIdMap.insert(pair<string, Int_t>(pmtPath, pmtUID));

            CbmRichPmtData* pmtData = new CbmRichPmtData();
            pmtData->fId            = pmtUID;
            pmtData->fPixelAddresses.push_back(pixelData->fAddress);
            pmtData->fHeight = pmtHeight;
            pmtData->fWidth  = pmtWidth;
            fPmtIdToDataMap.insert(pair<Int_t, CbmRichPmtData*>(pmtData->fId, pmtData));
            pixelData->fPmtId = pmtData->fId;

            fPmtIds.push_back(pmtData->fId);
          }
          else {
            //cout << "pmtPath old:" << pmtPath << endl;
            Int_t pmtId             = fPmtPathToIdMap[pmtPath];
            CbmRichPmtData* pmtData = fPmtIdToDataMap[pmtId];
            if (pmtData == nullptr || pmtId != pmtData->fId) {
              LOG(error) << "(pmtData == nullptr || pmtId != pmtData->fId) ";
            }
            pmtData->fPixelAddresses.push_back(pixelData->fAddress);
            pixelData->fPmtId = pmtData->fId;
            if (pmtData->fPixelAddresses.size() > 64) {
              LOG(info) << "size:" << pmtData->fPixelAddresses.size() << " pmtData->fId:" << pmtData->fId
                        << " pmtPath:" << pmtPath << endl
                        << " path:" << path;
            }
          }

        } break;

        default: LOG(error) << "ERROR: Could not identify Detector setup!"; break;
      }
    }
  }

  // calculate Pmt center as center of gravity of 64 pixels
  for (auto const& pmt : fPmtIdToDataMap) {
    //	    Int_t pmtId = pmt.first;
    CbmRichPmtData* pmtData = pmt.second;
    pmtData->fX             = 0.;
    pmtData->fY             = 0.;
    pmtData->fZ             = 0.;
    for (int pixelId : pmtData->fPixelAddresses) {
      CbmRichPixelData* pixelData = fPixelAddressToDataMap[pixelId];
      if (pixelData == nullptr) continue;
      pmtData->fX += pixelData->fX;
      pmtData->fY += pixelData->fY;
      pmtData->fZ += pixelData->fZ;
    }
    pmtData->fX /= pmtData->fPixelAddresses.size();
    pmtData->fY /= pmtData->fPixelAddresses.size();
    pmtData->fZ /= pmtData->fPixelAddresses.size();
  }

  // calculate fPixelId for each pixel by sorting x and y translation inside PMT volume
  for (auto const& pmt : fPmtIdToDataMap) {
    CbmRichPmtData* pmtData = pmt.second;
    std::vector<tuple<CbmRichPixelData*, Double_t, Double_t>> pixelsInPmt;
    for (int pixelAddress : pmtData->fPixelAddresses) {
      CbmRichPixelData* pixelData = fPixelAddressToDataMap[pixelAddress];
      if (pixelData == nullptr) continue;
      pixelsInPmt.push_back(
        make_tuple(pixelData, get<0>(localPixelCoord[pixelAddress]), get<1>(localPixelCoord[pixelAddress])));
    }

    std::sort(
      pixelsInPmt.begin(), pixelsInPmt.end(),
      [](tuple<CbmRichPixelData*, Double_t, Double_t> a, tuple<CbmRichPixelData*, Double_t, Double_t> b) {
        return (get<2>(a) > get<2>(b)) || ((abs(get<2>(a) - get<2>(b)) <= 0.3) && (get<1>(a) < get<1>(b)));
        // first sort by y (up to down) coordinate, then by x (left to right) coordinate
        // stop sorting by y when y coordinate difference is less than 0.3 (to start sorting by x when y is the "same")
        // needed because edge/corner pixels are slightly larger
      });

    if (pixelsInPmt.size() != 64) {
      LOG(error) << "ERROR: Calculating local pixel indices failed, number of pixels in PMT is not 64. "
                 << pmtData->ToString();
    }
    for (unsigned int i = 0; i < pixelsInPmt.size(); i++) {
      get<0>(pixelsInPmt[i])->fPixelId = i;
    }
  }

  localPixelCoord.clear();

  LOG(info) << "CbmRichDigiMapManager is initialized";
  LOG(info) << "fPixelPathToAddressMap.size() = " << fPixelPathToAddressMap.size();
  LOG(info) << "fPixelAddressToDataMap.size() = " << fPixelAddressToDataMap.size();

  LOG(info) << "fPmtPathToIdMap.size() = " << fPmtPathToIdMap.size();
  LOG(info) << "fPmtIdToDataMap.size() = " << fPmtIdToDataMap.size();

  //    for (auto const& pmt : fPmtIdToDataMap) {
  //       // cout << pmt.first << endl;
  //        cout << pmt.second->ToString() << endl;
  //    }
}

Int_t CbmRichDigiMapManager::GetPixelAddressByPath(const string& path)
{
  std::map<string, Int_t>::iterator it;
  it = fPixelPathToAddressMap.find(path);
  if (it == fPixelPathToAddressMap.end()) return -1;
  return it->second;
}


CbmRichPixelData* CbmRichDigiMapManager::GetPixelDataByAddress(Int_t address)
{
  std::map<Int_t, CbmRichPixelData*>::iterator it;
  it = fPixelAddressToDataMap.find(address);
  if (it == fPixelAddressToDataMap.end()) return nullptr;
  return it->second;
}

Int_t CbmRichDigiMapManager::GetRandomPixelAddress()
{
  Int_t nofPixels = fPixelAddresses.size();
  Int_t index     = gRandom->Integer(nofPixels);
  return fPixelAddresses[index];
}

vector<Int_t> CbmRichDigiMapManager::GetPixelAddresses() { return fPixelAddresses; }


vector<Int_t> CbmRichDigiMapManager::GetPmtIds() { return fPmtIds; }


CbmRichPmtData* CbmRichDigiMapManager::GetPmtDataById(Int_t id)
{
  std::map<Int_t, CbmRichPmtData*>::iterator it;
  it = fPmtIdToDataMap.find(id);
  if (it == fPmtIdToDataMap.end()) return nullptr;
  return it->second;
}

vector<Int_t> CbmRichDigiMapManager::GetNeighbourPixels(Int_t address, Int_t n, Bool_t horizontal, Bool_t vertical,
                                                        Bool_t diagonal)
{
  vector<Int_t> neighbourPixels;
  if (n == 0) return neighbourPixels;
  CbmRichPixelData* addressPixelData = GetPixelDataByAddress(address);
  Int_t indX                         = addressPixelData->fPixelId % 8;
  Int_t indY                         = addressPixelData->fPixelId / 8;
  vector<Int_t> pmtPixelAddresses    = GetPmtDataById(addressPixelData->fPmtId)->fPixelAddresses;
  for (auto const& iAddr : pmtPixelAddresses) {
    if (iAddr == address) continue;
    CbmRichPixelData* iPixelData = GetPixelDataByAddress(iAddr);
    Int_t iIndX                  = iPixelData->fPixelId % 8;
    Int_t iIndY                  = iPixelData->fPixelId / 8;
    if (horizontal && !vertical && !diagonal && n == 1) {  // direct horizontal neighbours
      if (abs(iIndX - indX) == 1 && abs(iIndY - indY) == 0) neighbourPixels.push_back(iAddr);
    }
    else if (vertical && !horizontal && !diagonal && n == 1) {  // direct vertical neighbours
      if (abs(iIndX - indX) == 0 && abs(iIndY - indY) == 1) neighbourPixels.push_back(iAddr);
    }
    else if (!horizontal && !vertical && diagonal && n == 1) {  // diagonal neighbours
      if ((abs(iIndX - indX) == 1) && (abs(iIndY - indY) == 1)) neighbourPixels.push_back(iAddr);
    }
    else if (horizontal && vertical && !diagonal && n == 1) {  // direct horizontal and vertical neighbours
      if ((abs(iIndX - indX) + abs(iIndY - indY)) == 1) neighbourPixels.push_back(iAddr);
    }
    else if (horizontal && vertical && diagonal) {  // all neighbours in (2N+1)*(2N+1) grid
      if ((abs(iIndX - indX) <= n) && (abs(iIndY - indY) <= n)) neighbourPixels.push_back(iAddr);
    }
    else {
      LOG(error) << "ERROR: Unrecogniced option in CbmRichDigiMapManager::GetNeighbourPixels " << endl
                 << " n = " << n << " horizontal = " << horizontal << " vertical = " << vertical
                 << " diagonal = " << diagonal;
    }
  }
  return neighbourPixels;
}

int CbmRichDigiMapManager::getDetectorSetup(TString const nodePath)
{
  //identify Detector by nodePath:
  //  0: RICH
  //  1: mRICH
  bool check = nodePath.Contains("mcbm");
  if (check) {
    //mRICH
    return 1;
  }
  else {
    //RICH
    return 0;
  }

  return 0;
}
