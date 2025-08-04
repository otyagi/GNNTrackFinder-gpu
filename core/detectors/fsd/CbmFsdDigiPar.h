/* Copyright (C) 2023 Physikalisches Institut, Eberhard Karls Universitaet Tuebingen, Tuebingen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig, Lukas Chlad [committer] */

#ifndef CBMFSDDIGIPAR_H
#define CBMFSDDIGIPAR_H

#include <FairParGenericSet.h>  // for FairParGenericSet

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Double_t
#include <TArrayD.h>     // for TArrayD

class FairParamList;

class CbmFsdDigiPar : public FairParGenericSet {
public:
  CbmFsdDigiPar(
    const char* name = "CbmFsdDigiPar", const char* title = "Digitization parameters for the FSD detector",
    const char* context = "Needed parameters to adjust FsdDigitizer according to the geometry and read-out propetries");

  CbmFsdDigiPar(const CbmFsdDigiPar&) = delete;
  CbmFsdDigiPar& operator=(const CbmFsdDigiPar&) = delete;

  ~CbmFsdDigiPar(void);

  void clear(void);
  void putParams(FairParamList*);
  Bool_t getParams(FairParamList*);

  Int_t GetNumPhotoDets() const { return fNumPhotoDets; }
  Int_t GetNumUnits() const { return fNumUnits; }
  Double_t GetTimeResolution(Int_t iUnitId) const;
  Double_t GetEnergyResolution(Int_t iUnitId) const;
  Double_t GetDeadTime(Int_t iUnitId) const;

private:
  Int_t fNumPhotoDets;        // number of photo detectors per module
  Int_t fNumUnits;            // number of units within given FSD geo version
  TArrayD fTimeResolution;    // value to smear the timing via gaussian
  TArrayD fEnergyResolution;  // value to smear the energy measurement via gaussian
  TArrayD fDeadTime;          // value to separate digis in time-based

  ClassDef(CbmFsdDigiPar, 1)
};

#endif
