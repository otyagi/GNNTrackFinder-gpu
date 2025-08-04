/* Copyright (C) 2011-2016 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Michael Deveaux, Philipp Sitzmann [committer], Florian Uhlig */

// -------------------------------------------------------------------------
// -----                     CbmMvdSensorDataSheet source file                  -----
// -----                  Created 31/01/11  by M. Deveaux              -----
// -------------------------------------------------------------------------

#include "CbmMvdSensorDataSheet.h"

#include "TMath.h"

#include <Logger.h>

#include <ostream>  // for operator<<, stringstream, basic_ostream
using std::endl;


// -----   Default constructor   -------------------------------------------
CbmMvdSensorDataSheet::CbmMvdSensorDataSheet()
  : TNamed()
  , fMimosaName("DefaulSensor")
  , fPixelPitchX(18.4e-4)
  , fPixelPitchY(18.4e-4)
  , fNPixelsX(0)
  , fNPixelsY(0)
  , fNPixels(0)
  , fPixelSignX(1)
  , fPixelSignY(1)
  , fShutterSign(1)
  , fIntegrationTime(50e3)
  , fEpiTh(14e-4)
  , fShaperNormalisationFactor(1)
  , fChargeThreshold(1)
  , fSignalRiseTime(nan("NotSet"))
  , fSignalFallTime(nan("NotSet"))
  , fNoise(0)
  , fLandauMPV(8.62131e+02)
  , fLandauSigma(2.e+02)
  , fLandauGain(1.56)
  , fLorentzPar0(4.12073e+02)
  , fLorentzPar1(0.8e+00)
  , fLorentzPar2(0.)
  , fAdcDynamic(150)
  , fAdcOffset(0)
  , fAdcBits(1)
  , fAdcSteps(TMath::Power(2, fAdcBits))
  , fAdcStepSize(fAdcDynamic / fAdcSteps)
  , fStatesPerBank(0)
  , fStatesPerLine(0)
  , fStatesPerFrame(0)
  , fPixelsPerBank(0)
  , fAnalogThreshold(-1)
  , fValidData(kFALSE)


{
}

// -----   Public method Print   -------------------------------------------
void CbmMvdSensorDataSheet::Print(Option_t* /*opt*/) const { LOG(info) << ToString(); }

std::string CbmMvdSensorDataSheet::ToString() const
{
  std::stringstream ss;
  ss << " --- " << GetName() << "--------------------------------" << endl;
  ss << " Technical Data:" << endl;
  ss << "--------------------------" << endl;
  ss << "Name of the sensor: " << fMimosaName << endl;
  ss << "Pixel pitch (x/y) [µm]: " << fPixelPitchY * 1e4 << "/" << fPixelPitchY * 1e-4 << endl;
  ss << "N Pixels (rows/cols): " << fNPixelsX << "/" << fNPixelsY << endl;
  ss << "Thickness of active volume[µm]: " << fEpiTh * 1e4 << endl;
  ss << "Integration time [µs]: " << fIntegrationTime * 1e6 << endl;
  ss << "ADC resolution: " << fAdcBits << "bit ADC (1 = discriminator)" << endl;
  ss << "Default noise [e ENC]: " << fNoise << endl;
  ss << "Matrix size (x/y) [cm]: " << fPixelPitchX * fNPixelsX << " / " << fPixelPitchY * fNPixelsY << endl;
  ss << endl;
  ss << "Readout system: " << endl;
  ss << "----------------" << endl;
  ss << "PixelNmb X from left to right?: " << fPixelSignX << endl;
  ss << "PixelNmb Y from up to down?: " << fPixelSignY << endl;
  ss << "Rolling shutter from up to down?: " << fShutterSign << endl;
  ss << endl;
  ss << "ADC / SUZE -Settings: " << endl;
  ss << "----------------" << endl;
  ss << "ADC Range [e]: " << fAdcDynamic << endl;
  ss << "ADC offset [e]: " << fAdcOffset << endl;
  ss << "ADC stepsize [e]: " << fAdcStepSize << endl;
  ss << "SUZE states per banks: " << fStatesPerBank << endl;
  ss << "SUZE states per line : " << fStatesPerLine << endl;
  ss << "SUZE states per frame: " << fStatesPerFrame << endl;
  ss << "SUZE pixels per bank: " << fPixelsPerBank << endl;
  ss << endl;
  ss << "Data card properly initialized: " << fValidData << endl;
  return ss.str();
}

// ----------- Methode ComputeCCE ----------------------------------
// ----------- Computes the relative number of electrons being deposited at chargePointXYZ and collected by a diode at diodeXYZ
// ----------- Standard implementation for backward compatibility based on Lorentz-Model.
// ----------- See PhD Christina Dritsa for scientific details on this model.

Double_t CbmMvdSensorDataSheet::ComputeCCE(Float_t chargePointX, Float_t chargePointY, Float_t /*chargePointZ*/,
                                           Float_t diodeX, Float_t diodeY, Float_t /*diodeZ*/)
{

  return

    (0.5 * fLorentzPar0 * fLorentzPar1 / TMath::Pi()
       / TMath::Max(1.e-10,


                    ((chargePointX - diodeX) * (chargePointX - diodeX) / fPixelPitchX / fPixelPitchX)
                      + ((chargePointY - diodeY) * (chargePointY - diodeY) / fPixelPitchY / fPixelPitchY)
                      + 0.25 * fLorentzPar1 * fLorentzPar1)
     + fLorentzPar2);

  // Original Equation
  //      Float_t totCharge = (sPoint->charge * fLorentzNorm * (0.5 * fPar0 * fPar1 / TMath::Pi())
  //                             / TMath::Max(1.e-10, (((Current[0] - xCentre) * (Current[0] - xCentre))
  //                                                   + ((Current[1] - yCentre) * (Current[1] - yCentre)))
  //                                                      / fPixelSize / fPixelSize
  //                                                    + 0.25 * fPar1 * fPar1)
  //                           + fPar2);
}

Int_t CbmMvdSensorDataSheet::ComputeAdcCharge(Float_t charge)
{

  Int_t adcCharge;

  if (charge < fAdcOffset) {
    return 0;
  };

  adcCharge = int((charge - fAdcOffset) / fAdcStepSize);
  if (adcCharge > fAdcSteps - 1) {
    adcCharge = fAdcSteps - 1;
  }

  return adcCharge;
}

ClassImp(CbmMvdSensorDataSheet)
