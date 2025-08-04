/* Copyright (C) 2008-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Christina Dritsa [committer], Samir Amar-Youcef, Florian Uhlig, Philipp Sitzmann */

#include "CbmMvdPixelCharge.h"

#include <Logger.h>

// -------------------------------------------------------------------------
Bool_t CbmMvdPixelCharge::TestXY(Int_t channelNrX, Int_t channelNrY)
{

  if ((channelNrX == fChannelNrX) && (channelNrY == fChannelNrY)) {
    return 1;
  }
  else {
    return 0;
  };
}

// -----   Constructor with parameters   -----------------------------------
CbmMvdPixelCharge::CbmMvdPixelCharge(Float_t charge, Int_t channelNrX, Int_t channelNrY)
  : TObject()
  //, fCharge(charge)
  , fMaxChargeContribution(charge)
  , fChannelNrX(channelNrX)
  , fChannelNrY(channelNrY)
  , fTrackCharge(charge)
{
}

// ------- DigestCharge ----------------------------------------------------#

// Sums up the charge stored in track charge (assuming this is the summed charge from
// all segments of a track). Checks if a new track contributed charge to the pixel
// Checks if the new track is dominant

void CbmMvdPixelCharge::DigestCharge(Float_t pointX, Float_t pointY, Double_t time, Int_t pointId, Int_t trackId,
                                     Int_t inputNr, Int_t eventNr)
{
  Float_t chargeContr = fTrackCharge;

  //   for (auto charge : fPointWeight) {
  //     chargeContr -= charge;
  //   }

  if (chargeContr > 0.) {
    fCharge.push_back(fTrackCharge);
    fTrackId.push_back(trackId);
    fPointId.push_back(pointId);
    fInputNr.push_back(inputNr);
    fEventNr.push_back(eventNr);

    fPointWeight.push_back(chargeContr);
    fPointX.push_back(pointX);
    fPointY.push_back(pointY);
    fTime.push_back(time);
    fContributors++;

    if (chargeContr > fMaxChargeContribution) {
      fDominatorIndex        = fPointWeight.size();
      fMaxChargeContribution = chargeContr;
    }
    fTrackCharge = 0.;
  }
}

ClassImp(CbmMvdPixelCharge)
