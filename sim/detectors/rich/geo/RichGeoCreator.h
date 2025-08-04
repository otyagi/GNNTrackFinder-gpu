/* Copyright (C) 2022-2024 UGiessen/GSI, Giessen/Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Simon Neuhaus */

/* to generate geometry version v22a use version of this file and RichGeoCreator.cxx from commit bed4c932
   to generate geometry version v23a use version of this file and RichGeoCreator.cxx from commit c25124a0
   to generate geometry version v24a use version of this file and RichGeoCreator.cxx from commit current version (update at some point)
*/

#ifndef RICH_GEO_CREATOR
#define RICH_GEO_CREATOR

#include "TObject.h"

#include <string>

#include <cmath>

#include "RichGeoMaterials.h"

class TGeoVolumeAssembly;
class TGeoVolume;
class TGeoShape;
class TGeoMatrix;
class TGeoCompositeShape;
class TGeoRotation;

class RichGeoCreator : public TObject {
public:
  RichGeoCreator();

  virtual ~RichGeoCreator();

  void Create();

  void SetGeoName(const std::string& geoName) { fGeoName = geoName; }
  void SetAddShieldingBox(bool addShildingBox) { fAddShieldingBox = addShildingBox; }
  void SetVolumeColors(bool setVolumeColors) { fSetVolumeColors = setVolumeColors; }

private:
  RichGeoMaterials fMaterials;

  std::string fGeoName  = "rich_v24a";
  bool fAddShieldingBox = true;
  bool fSetVolumeColors = false;

  // General RICH parameters
  double fDegToRad              = 1.74532925199433E-02;
  double fRichOrigZ             = 180.;   //cm
  double fAccAngle              = 25.;    //deg
  double fRichLength            = 220.;   //cm
  double fRichHeight            = 501.7;  //cm
  double fRichWidth             = 600;    //cm
  double fRichCoveringThickness = 0.5;    //cm
  double fRichAddZ              = 20.;    //cm

  // Entrance & Exit parameters
  double fRichEntranceWidth     = 260.;   //cm
  double fRichEntranceHeight    = 175.;   //cm
  double fRichEntranceThickness = 0.025;  //cm
  double fRichExitWidth         = 569.;   //cm
  double fRichExitHeight        = 384.;   //cm
  double fRichExitThickness     = 1;      //cm

  // Pipe parameters
  double fPipeOuterRadiusEntrance = 16.4;  //cm
  double fPipeThicknessEntrance   = 0.3;   //cm //in CAD 2mm with overlap regions with thickness 4.04mm
  double fPipeOuterRadiusExit     = 29.9;  //cm
  double fPipeThicknessExit       = fPipeThicknessEntrance;  //cm
  double fPipeCylPartLen          = 10.0;                    //cm
  double fPipeLength              = fRichLength + 0.5;       //cm

  // Mirror parameters
  double fMirrorRadius     = 300.;    //cm
  double fMirrorThickness  = 0.6;     //cm
  double fMirrorPos        = 350.;    //cm
  double fMirrorRot        = 12.;     //deg
  double fMirrorPhiSize    = 8.425;   //deg
  double fMirrorPhiStep    = 8.5;     //deg
  double fMirrorThetaSize  = 8.088;   //deg
  double fMirrorThetaStep  = 8.1625;  //deg
  double fMirrorThetaGap   = 0.0745;  //deg
  double fMirrorSupportAdd = 4.5;     //cm
  double fMirrorGapY       = 0.1;     //cm

  // PMT parameters
  int fNofPixels           = 8;
  double fPixelSize        = 0.6;    //cm
  double fPixelSizeEdge    = 0.625;  //cm
  double fPixelThickness   = 0.05;   //cm
  double fPmtSize          = 5.2;    //cm
  double fPmtDepth         = 3.871;  //cm
  double fCameraRot        = 15.;    //deg
  double fCameraShiftY     = 2.5;    //cm
  double fCameraShiftZ     = 5.;     //cm
  double fCameraRadius     = 169.7;  //cm
  double fCameraGap        = 0.1;    //cm
  double fCameraTouchWidth = 15.75;  //cm

  // Sensitive plane parameters
  double fSensPlaneZ = -30.;  //70.;  //cm

  // Calculated parameters. These parameters are calculated in CalculateParams()
  double fCameraFullRotX    = 0.;
  double fCameraOriginY     = 0.;
  double fCameraOriginZ     = 0.;
  double fMirrorOriginY     = 0.;
  double fMirrorOriginZ     = 0.;
  double fAlpha             = 0.;
  double fCameraHeight      = 0.;
  double fCameraTouchRadius = 0.;
  double fCameraSegmentAng  = 0.;

  void CalculateParams();

  TGeoVolume* CreatePmt();
  TGeoVolume* CreateCameraModule();
  TGeoVolume* CreateCameraStrip();
  TGeoVolumeAssembly* CreateCameraContainer();
  TGeoVolumeAssembly* CreateMirror();
  TGeoVolumeAssembly* CreateMainFrame();
  TGeoVolume* CreateGas();
  TGeoVolume* CreateRichContainer();
  TGeoVolume* CreateSensitivePlane();
  TGeoVolume* CreateRichEntrance();
  TGeoVolume* CreateRichExit();
  TGeoVolume* CreatePipe();
  TGeoVolume* CreatePipeCyl();
  TGeoVolumeAssembly* CreateShieldingBox();
  TGeoVolumeAssembly* CreateBeltAssembly();
  TGeoVolumeAssembly* CreateMirrorSupportBelts();
  TGeoVolumeAssembly* CreateMirrorSupportPillars();
  TGeoVolume* CreateStuds(double zpos, double lenHalf, double angle);
  TGeoVolume* CreatePillarConnection();

  void CreateRich();

  double ToDeg(double rad) { return rad / fDegToRad; }
  double ToRad(double deg) { return deg * fDegToRad; }

  TGeoCompositeShape* MakeSubtraction(const std::string& name, TGeoShape* left, TGeoShape* right,
                                      TGeoMatrix* lmat = nullptr, TGeoMatrix* rmat = nullptr);
  TGeoCompositeShape* MakeUnion(const std::string& name, TGeoShape* left, TGeoShape* right, TGeoMatrix* lmat = nullptr,
                                TGeoMatrix* rmat = nullptr);
  TGeoCompositeShape* MakeIntersection(const std::string& name, TGeoShape* left, TGeoShape* right,
                                       TGeoMatrix* lmat = nullptr, TGeoMatrix* rmat = nullptr);

  TGeoCombiTrans* MakeCombiTrans(double dx, double dy, double dz, double rotX, double rotY, double rotZ);
  TGeoTranslation* MakeTrans(double dx, double dy, double dz);
  TGeoRotation* MakeRot(double rotX, double rotY, double rotZ);

  ClassDef(RichGeoCreator, 1)
};

#endif
