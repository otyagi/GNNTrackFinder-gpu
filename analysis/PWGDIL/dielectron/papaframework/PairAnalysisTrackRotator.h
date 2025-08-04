/* Copyright (C) 2015-2020 Justus-Liebig-Universitaet Giessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Julian Book [committer] */

#ifndef DILEPTONTRACKROTATOR_H
#define DILEPTONTRACKROTATOR_H

//#############################################################
//#                                                           #
//#         Class PairAnalysisTrackRotator                    #
//#                                                           #
//#  Authors:                                                 #
//#   Julian    Book,     Uni Ffm / Julian.Book@cern.ch       #
//#                                                           #
//#############################################################

#include <TMath.h>
#include <TNamed.h>
#include <TRandom3.h>

class PairAnalysisTrackRotator : public TNamed {
public:
  enum class ERotationType
  {
    kRotatePositive,
    kRotateNegative,
    kRotateBothRandom
  };

  PairAnalysisTrackRotator();
  PairAnalysisTrackRotator(const char* name, const char* title);

  virtual ~PairAnalysisTrackRotator();

  //Setters
  void SetIterations(UInt_t niter) { fIterations = niter; }
  void SetRotationType(ERotationType type) { fRotationType = type; }
  void SetStartAnglePhi(Double_t phi) { fStartAnglePhi = phi; }
  void SetConeAnglePhi(Double_t phi) { fConeAnglePhi = phi; }

  //Getters
  Int_t GetIterations() const { return fIterations; }
  ERotationType GetRotationType() const { return fRotationType; }
  Double_t GetStartAnglePhi() const { return fStartAnglePhi; }
  Double_t GetConeAnglePhi() const { return fConeAnglePhi; }

  Double_t GetAngle() const { return fStartAnglePhi + (2 * gRandom->Rndm() - 1) * fConeAnglePhi; }
  Double_t GetCharge() const { return TMath::Nint(gRandom->Rndm()); }

private:
  UInt_t fIterations          = 1;                                 // number of iterations
  ERotationType fRotationType = ERotationType::kRotateBothRandom;  // which track to rotate
  Double_t fStartAnglePhi     = TMath::Pi();                       // starting angle for rotation
  Double_t fConeAnglePhi      = TMath::Pi();                       // opening angle in phi for multiple rotation

  PairAnalysisTrackRotator(const PairAnalysisTrackRotator& c);
  PairAnalysisTrackRotator& operator=(const PairAnalysisTrackRotator& c);

  ClassDef(PairAnalysisTrackRotator, 1)  // Track rotation configuration
};


#endif
