/* Copyright (C) 2017 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer] */

// -------------------------------------------------------------------------
// -----                     CbmMvdMimosis source file                 -----
// -----                                                               -----
// -------------------------------------------------------------------------

#include "CbmMvdMimosis.h"

#include <RtypesCore.h>  // for kTRUE
#include <TMath.h>       // for Power
#include <TString.h>     // for TString

// -----   Default constructor   -------------------------------------------
CbmMvdMimosis::CbmMvdMimosis()
{

  fMimosaName  = "Mimosis";              // Clear name of the simulated sensor
  fPixelPitchX = 0.003024;               // Pixel pitch in x of this sensor in cm
  fPixelPitchY = 0.002688;               // Pixel pitch in y of this sensor in cm
  fNPixelsX    = 1024;                   // Number of pixels in row
  fNPixelsY    = 504;                    // Number of pixels in col
  fNPixels     = fNPixelsX * fNPixelsY;  // Number of pixels in sensor
  fPixelSignX  = 1;                      // Direction of the pixel count X, if true, Pixel x=0 is at
                                         // left corner, else at right corner
  fPixelSignY = 1;                       // Direction of the pixel count Y, if true, Pixel x=0 is at
                                         // the lower corner, else at upper corner
  fShutterSign     = 1;
  fIntegrationTime = 5.0e3;  // Integration time of the pixels in ns
  fEpiTh           = 25e-4;  // Thickness of the epitaxial layer

  fChargeThreshold = 1.;
  fAnalogThreshold = 90;  //Threshold in electrons, must not be below 90e

  /** Description of the sensor for the digitizer //mesaured by melisa for Mimosa26AHR**/
  fNoise       = 13;
  fLorentzPar0 = 520.;
  fLorentzPar1 = 0.34;
  fLorentzPar2 = -1.2;
  fLandauMPV   = 877.4;
  fLandauSigma = 204.93;
  fLandauGain  = 3.58;

  /** ADC description **/

  fAdcDynamic     = 150;
  fAdcOffset      = 0;
  fAdcBits        = 1;
  fAdcSteps       = TMath::Power(2, fAdcBits);
  fAdcStepSize    = fAdcDynamic / fAdcSteps;
  fStatesPerBank  = 6;
  fStatesPerLine  = 9;
  fStatesPerFrame = 570;
  fPixelsPerBank  = 64;

  fSignalRiseTime            = 100;  // to be updated.
  fSignalFallTime            = 3e6;  // to be updated.
  fShaperNormalisationFactor = 1.;   // to be updated

  /** Self-organizsation **/

  fValidData = kTRUE;
}


Double_t CbmMvdMimosis::ComputeHitDeadTime(Float_t charge)
{
  return 2519.5443 * std::pow(charge, 0.035720252);
}  //Fast settings, AC -1V

Double_t CbmMvdMimosis::ComputeHitDelay(Float_t charge)
{

  Double_t delay      = GetDelay(charge);
  Double_t delaySigma = GetDelaySigma(charge);
  return delay + fRandom->Gaus(delay, delaySigma);  //models the pixel-to-pixel variation in terms of delay
}

Double_t CbmMvdMimosis::GetDelay(Float_t charge) { return 47591.8471 * std::pow(charge, -0.9990384691); };
Double_t CbmMvdMimosis::GetDelaySigma(Float_t charge) { return 11909.5799315438 * std::pow(charge, -1.0784955428); };

Double_t CbmMvdMimosis::ComputeHitJitter(Float_t charge)
{

  Double_t jitter = GetJitter(charge);  // Full width of jitter as function of charge
  return fRandom->Uniform(-jitter, jitter);
}

Double_t CbmMvdMimosis::GetJitter(Float_t charge) { return 194945.6385 * std::pow(charge, -1.6138338012) / 2; }


// -----   Destructor   ----------------------------------------------------
CbmMvdMimosis::~CbmMvdMimosis() {}
// -------------------------------------------------------------------------

ClassImp(CbmMvdMimosis)
