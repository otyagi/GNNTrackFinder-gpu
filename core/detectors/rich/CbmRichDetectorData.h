/* Copyright (C) 2015-2023 GSI/JINR-LIT, Darmstadt/Dubna
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev, Andrey Lebedev [committer], Florian Uhlig */

/*detector
 * CbmRichData.h
 *
 *  Created on: Dec 17, 2015
 *      Author: slebedev
 */

#ifndef RICH_DETECTOR_CBMRICHDETECTORDATA_H_
#define RICH_DETECTOR_CBMRICHDETECTORDATA_H_

#include <RtypesCore.h>  // for Double_t, Int_t, Bool_t

#include <algorithm>  // for find
#include <string>     // for string, to_string
#include <vector>     // for vector

class CbmRichPixelData {
 public:
  Int_t fAddress;
  Double_t fX;     // global x coordinate
  Double_t fY;     // global y coordinate
  Double_t fZ;     // global z coordinate
  Int_t fPixelId;  // pixel index [0, 63] row major, view from z to -z
  Int_t fPmtId;
};

class CbmRichPmtData {
 public:
  Bool_t ContainsPixel(Int_t address)
  {
    return std::find(fPixelAddresses.begin(), fPixelAddresses.end(), address) != fPixelAddresses.end();
  }

  std::string ToString()
  {
    return "id:" + std::to_string(fId) + " nofPixels:" + std::to_string(fPixelAddresses.size())
           + " x:" + std::to_string(fX) + " y:" + std::to_string(fY) + " z:" + std::to_string(fZ)
           + " W:" + std::to_string(fWidth) + " H:" + std::to_string(fHeight);
  }
  Int_t fId;
  std::vector<Int_t> fPixelAddresses;
  Double_t fX;
  Double_t fY;
  Double_t fZ;
  Double_t fWidth;
  Double_t fHeight;
};

#endif /* RICH_DETECTOR_CBMRICHDETECTORDATA_H_ */
