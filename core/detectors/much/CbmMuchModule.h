/* Copyright (C) 2008-2020 St. Petersburg Polytechnic University, St. Petersburg
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Mikhail Ryzhinskiy [committer], Evgeny Kryshen, Florian Uhlig */

/** CbmMuchModule.h
 *
 *@author  M.Ryzhinskiy <m.ryzhinskiy@gsi.de>
 *@version 1.0
 *@since   11.02.08
 **
 ** This class holds geometry parameters of much modules
 **
 **/

#ifndef CBMMUCHMODULE_H
#define CBMMUCHMODULE_H 1

#include <Rtypes.h>      // for THashConsistencyHolder, ClassDef
#include <RtypesCore.h>  // for Int_t, Double_t, Bool_t, Color_t, kTRUE
#include <TPave.h>       // for TPave
#include <TVector3.h>    // for TVector3

#include <map>      // for multimap
#include <utility>  // for pair

class TClonesArray;

class CbmMuchModule : public TPave {
public:
  /** Default constructor **/
  CbmMuchModule();
  /** Standard constructor
   *@param iStation  Station index
   *@param iLayer    Layer index
   *@param iSide     Defines side of the layer (0 - Front, 1 - Back)
   *@param iModule   Module index
   *@param position  Location of the module center in global c.s. (all dimensions in [cm])
   *@param size      Size of the module (all dimensions in [cm])
   *@param cutRadius Radius of the cut (if any, otherwise = -1.) [cm]
   **/
  CbmMuchModule(Int_t iStation, Int_t iLayer, Bool_t iSide, Int_t iModule, TVector3 position, TVector3 size,
                Double_t cutRadius);
  /** Destructor **/
  virtual ~CbmMuchModule() {}

  /** Accessors */
  Int_t GetDetectorId() const { return fDetectorId; }
  Double_t GetCutRadius() const { return fCutRadius; }
  TVector3 GetSize() const { return fSize; }
  TVector3 GetPosition() const { return fPosition; }
  Int_t GetDetectorType() const { return fDetectorType; }
  TClonesArray* GetPoints() const { return fPoints; }
  TClonesArray* GetHits() const { return fHits; }
  TClonesArray* GetClusters() const { return fClusters; }

  virtual Bool_t InitModule() { return kTRUE; }
  virtual void DrawModule(Color_t) {}

  void SetPoints(TClonesArray* points) { fPoints = points; }
  void SetHits(TClonesArray* hits) { fHits = hits; }
  void SetClusters(TClonesArray* clusters) { fClusters = clusters; }
  /** */
  void AddDigi(Double_t time, Int_t id) { fDigis.insert(std::pair<Double_t, Int_t>(time, id)); }
  /** */
  void ClearDigis() { fDigis.clear(); }
  /** */
  std::multimap<Double_t, Int_t> GetDigis() { return fDigis; }

protected:
  Int_t fDetectorId;        // Unique detector ID
  Int_t fDetectorType;      // Detector type
  Double_t fCutRadius;      // Radius of the cut (if any, otherwise = -1.) [cm]
  TVector3 fSize;           // Size vector of the module (all dimensions in [cm])
  TVector3 fPosition;       // Location vector of the module center in global c.s. (all dimensions in [cm])
  TClonesArray* fPoints;    //!
  TClonesArray* fHits;      //!
  TClonesArray* fClusters;  //!
  std::multimap<Double_t, Int_t> fDigis;  //!

private:
  CbmMuchModule(const CbmMuchModule&);
  CbmMuchModule& operator=(const CbmMuchModule&);

  ClassDef(CbmMuchModule, 2);
};
#endif
