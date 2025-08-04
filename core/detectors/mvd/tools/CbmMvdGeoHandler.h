/* Copyright (C) 2014-2017 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Philipp Sitzmann [committer] */

/**
 * \file CbmMvdGeoHandler.h
 * \brief Helper class to extract information from the GeoManager. Addapted from TrdGeoHandler byFlorian Uhlig <f.uhlig@gsi.de>
 * \author Philipp Sitzmann <p.sitzmann@gsi.de>
 * \date 30/10/2014
 *
 */

#ifndef CbmMvdGeoHandler_H_
#define CbmMvdGeoHandler_H_ 1

#include <Rtypes.h>      // for ClassDef
#include <RtypesCore.h>  // for Int_t, Double_t, Bool_t, kFALSE, UInt_t
#include <TObject.h>     // for TObject
#include <TString.h>     // for TString

class CbmMvdDetector;
class CbmMvdStationPar;
class TBuffer;
class TClass;
class TGeoBBox;
class TGeoHMatrix;
class TGeoVolume;
class TMemberInspector;

enum class CbmMvdSensorTyp;

#include <map>  // for map

enum CbmMvdGeoTyp
{
  Default,
  beamtest,
  TwoStation,
  ThreeStation,
  FourStation,
  FourStationShift,
  MiniCbm,
  scripted
};


class CbmMvdGeoHandler : public TObject {
public:
  /**
   * \brief Constructor.
   **/
  CbmMvdGeoHandler();

  /**
   * \brief Destructor.
   **/
  ~CbmMvdGeoHandler();

  void Init(Bool_t isSimulation = kFALSE);

  /**
   * \brief Return module address calculated based on the current node in the TGeoManager.
   */
  Int_t GetSensorAddress();

  /**
   * \brief Navigate to node.
   */
  Int_t GetSensorAddress(const TString& path);

  Double_t GetSizeX(const TString& path);
  Double_t GetSizeY(const TString& path);
  Double_t GetSizeZ(const TString& path);
  Double_t GetX(const TString& path);
  Double_t GetY(const TString& path);
  Double_t GetZ(const TString& path);
  Int_t GetStation(const TString& path);
  void Fill();
  std::map<Int_t, Int_t> GetMap() { return fStationMap; };
  void PrintGeoParameter();
  Int_t GetIDfromPath(TString path);

  void SetSensorTyp(CbmMvdSensorTyp typ) { fSensorTyp = typ; };


private:
  void NavigateTo(const TString& sensor);
  void GetPipe();
  void GetGeometryTyp();
  void FillParameter();
  void FillDetector();
  void FillStationMap();


  CbmMvdSensorTyp fSensorTyp;

  CbmMvdDetector* fDetector;
  CbmMvdStationPar* fStationPar;

  std::map<Int_t, Int_t> fStationMap;

  Bool_t fIsSimulation;  //!

  UInt_t fGeoPathHash;         //!
  TGeoVolume* fCurrentVolume;  //!
  TGeoBBox* fVolumeShape;      //!
  Double_t fGlobal[3];         //! Global center of volume
  TGeoHMatrix* fGlobalMatrix;  //!
  Int_t fLayerId;              //!
  Int_t fModuleId;             //!
  Int_t fModuleType;           //!
  Int_t fStation;              //! StationTypeID, 1..3
  TString fMother;
  CbmMvdGeoTyp fGeoTyp;
  Int_t fVolId;
  Int_t fStationNumber;

  Double_t fWidth;
  Double_t fHeight;
  Double_t fZRadThickness;
  Double_t fBeamwidth;
  Double_t fBeamheight;
  Double_t fZThickness;
  Double_t fXres;
  Double_t fYres;

  TString fStationName;
  TString fDetectorName;
  TString fSectorName;
  TString fQuadrantName;
  TString fSensorHolding;
  TString fSensorName;
  TString fnodeName;

  CbmMvdGeoHandler(const CbmMvdGeoHandler&);
  CbmMvdGeoHandler operator=(const CbmMvdGeoHandler&);

  ClassDef(CbmMvdGeoHandler, 5)
};

#endif  //CbmMvdGeoHandler_H
