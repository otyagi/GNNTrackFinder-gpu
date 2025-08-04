/* Copyright (C) 2016 Justus-Liebig-Universitaet Giessen, Giessen
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Jordan Bendarouach [committer] */

#ifndef CBMRICHMIRROR_H
#define CBMRICHMIRROR_H


#include "CbmRichRingLight.h"
#include "FairTask.h"
#include "TVector3.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

class TClonesArray;
class TH1D;
class TH2D;


class CbmRichMirror {
 public:
  /*
	 * Constructor.
	 */
  CbmRichMirror() : fMirrorId(""), fMomentum(), fProjHit(), fExtrapHit(), fRotAngles(), fRingL() {}

  /*
	 * Destructor.
	 */
  virtual ~CbmRichMirror() {}

  // Modifiers:
  void setMirrorId(string s) { fMirrorId = s; }
  void setMomentum(TVector3 v) { fMomentum = v; }
  void setProjHit(Double_t xX, Double_t yY) { fProjHit.push_back(xX), fProjHit.push_back(yY); }
  void setExtrapHit(Double_t xX, Double_t yY) { fExtrapHit.push_back(xX), fExtrapHit.push_back(yY); }
  void setRotAngles(Double_t xX, Double_t yY) { fRotAngles.push_back(xX), fRotAngles.push_back(yY); }
  void setRingLight(CbmRichRingLight ringL) { fRingL = ringL; }

  // Accessors:
  string getMirrorId() { return fMirrorId; }
  TVector3 getMomentum() { return fMomentum; }
  vector<Double_t> getProjHit() { return fProjHit; }
  vector<Double_t> getExtrapHit() { return fExtrapHit; }
  vector<Double_t> getRotAngles() { return fRotAngles; }
  CbmRichRingLight getRingLight() { return fRingL; }


 private:
  string fMirrorId;
  TVector3 fMomentum;
  vector<Double_t> fProjHit;
  vector<Double_t> fExtrapHit;
  vector<Double_t> fRotAngles;
  CbmRichRingLight fRingL;

  CbmRichMirror(const CbmRichMirror&);
  CbmRichMirror operator=(const CbmRichMirror&);

  ClassDef(CbmRichMirror, 1);
};

#endif
