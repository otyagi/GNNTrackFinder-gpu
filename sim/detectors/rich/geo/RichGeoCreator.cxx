/* Copyright (C) 2022-2024 UGiessen/GSI, Giessen/Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Semen Lebedev [committer], Eoin Clerkin, Simon Neuhaus*/


#include "RichGeoCreator.h"

#include "Logger.h"
#include "RichGeoMaterials.h"
#include "TGeoArb8.h"
#include "TGeoBBox.h"
#include "TGeoBoolNode.h"
#include "TGeoCompositeShape.h"
#include "TGeoCone.h"
#include "TGeoEltu.h"
#include "TGeoMatrix.h"
#include "TGeoSphere.h"
#include "TGeoTube.h"
#include "TGeoVolume.h"

#include <TMath.h>

#include <iostream>

using namespace std;

// using TMath::ACos;
using TMath::ASin;
using TMath::ATan;
using TMath::Cos;
using TMath::Sin;
using TMath::Tan;
const double pi = TMath::Pi();

RichGeoCreator::RichGeoCreator() {}

RichGeoCreator::~RichGeoCreator() {}

void RichGeoCreator::Create()
{
  LOG(info) << "Create geometry: " << fGeoName;
  fMaterials.Init();

  CalculateParams();

  //  TGeoVolume* vol = CreateMirrorSupportBelts();
  //  vol->Draw("ogl");
  //  vol->Export("rich_volume.root");

  CreateRich();
}

void RichGeoCreator::CalculateParams()
{
  double cosMirRot  = cos(ToRad(fMirrorRot));
  double sinMirRot  = sin(ToRad(fMirrorRot));
  double mirRadCos  = fMirrorRadius * cosMirRot;
  double E          = sin(ToRad(90. - fAccAngle - fMirrorRot)) / sin(ToRad(fAccAngle));
  double A          = (4. * E * E + 4. * E * sinMirRot + 1.) / (cosMirRot * cosMirRot);
  double B          = (2. * (2. * E + sinMirRot) / cosMirRot) * (1. - fMirrorPos / mirRadCos);
  double C          = fMirrorPos * (fMirrorPos - 2. * mirRadCos) / (mirRadCos * mirRadCos);
  double t1         = (-B + sqrt(B * B - 4. * A * C)) / (2. * A);
  fAlpha            = ToDeg(asin(t1));  //rad
  double sinAlpha   = sin(ToRad(fAlpha));
  double cosAlpha   = cos(ToRad(fAlpha));
  double psi        = ToDeg(atan((fMirrorRadius * (sinAlpha * cosMirRot - sinMirRot * (1. - cosAlpha))) / fMirrorPos));
  double mirRadHalf = fMirrorRadius / 2.;

  fCameraFullRotX = psi + 2. * fMirrorRot - fCameraRot;
  fCameraOriginY  = fMirrorPos * tan(ToRad(psi)) + mirRadHalf * sin(ToRad(psi + 2. * fMirrorRot))
                   + fCameraRadius * sin(ToRad(fCameraFullRotX));
  fCameraOriginZ =
    fMirrorPos - mirRadHalf * cos(ToRad((psi + 2. * fMirrorRot))) - fCameraRadius * cos(ToRad(fCameraFullRotX));
  fMirrorOriginY = fMirrorRadius * sin(ToRad(fMirrorRot + fAlpha));
  fMirrorOriginZ = fMirrorPos - mirRadCos;

  fCameraHeight      = 7. * (2. * fPmtSize + fCameraGap) + 6. * fCameraGap;
  double camRad1     = fCameraRadius - 1. - fPmtDepth;
  fCameraTouchRadius = std::sqrt(camRad1 * camRad1 + fCameraTouchWidth * fCameraTouchWidth / 4.);
  fCameraSegmentAng  = ToDeg(2. * std::asin((fCameraTouchWidth / 2.) / fCameraTouchRadius));
}

TGeoVolume* RichGeoCreator::CreatePmt()
{
  double pxSizeHalf      = fPixelSize / 2.;
  double pxThicknessHalf = fPixelThickness / 2.;
  double pxSizeEdgeHalf  = fPixelSizeEdge / 2.;
  double pmtSizeHalf     = fPmtSize / 2.;
  double pmtDepthHalf    = fPmtDepth / 2.;
  TGeoMedium* medCsI     = fMaterials.GetMedium("CsI");

  TGeoBBox* pixelPB       = new TGeoBBox("pmt_pixel_PB", pxSizeHalf, pxSizeHalf, pxThicknessHalf);
  TGeoBBox* pixelCornerPB = new TGeoBBox("pmt_pixel_corner_PB", pxSizeEdgeHalf, pxSizeEdgeHalf, pxThicknessHalf);
  TGeoBBox* pixelYPB      = new TGeoBBox("pmt_pixel_y_PB", pxSizeHalf, pxSizeEdgeHalf, pxThicknessHalf);
  TGeoBBox* pixelXPB      = new TGeoBBox("pmt_pixel_x_PB", pxSizeEdgeHalf, pxSizeHalf, pxThicknessHalf);
  TGeoBBox* pmtPB         = new TGeoBBox("pmt_PB", pmtSizeHalf, pmtSizeHalf, pmtDepthHalf);

  TGeoVolume* pixel = new TGeoVolume("pmt_pixel", pixelPB, medCsI);
  //if (fSetVolumeColors) pixel->SetLineColor(kRed - 9);
  TGeoVolume* pixelCorner = new TGeoVolume("pmt_pixel_corner", pixelCornerPB, medCsI);
  //if (fSetVolumeColors) pixelCorner->SetLineColor(kRed);
  TGeoVolume* pixelY = new TGeoVolume("pmt_pixel_y", pixelYPB, medCsI);
  //if (fSetVolumeColors) pixelY->SetLineColor(kRed - 5);
  TGeoVolume* pixelX = new TGeoVolume("pmt_pixel_x", pixelXPB, medCsI);
  //if (fSetVolumeColors) pixelX->SetLineColor(kRed + 3);
  TGeoVolume* pmt = new TGeoVolume("pmt", pmtPB, fMaterials.GetMedium("RICHgas_CO2_dis"));
  //if (fSetVolumeColors) pmt->SetLineColor(kRed + 1);

  double edgePxPos = 3. * fPixelSize + pxSizeEdgeHalf;
  double zC        = pmtDepthHalf - pxThicknessHalf;
  int copyNum      = 1;
  for (int x = 0; x < fNofPixels; x++) {
    double xC = (x - 3.5) * fPixelSize;
    if (x == 0) xC = -edgePxPos;
    if (x == 7) xC = edgePxPos;
    for (int y = 0; y < fNofPixels; y++) {
      double yC = (y - 3.5) * fPixelSize;
      if (y == 0) yC = edgePxPos;
      if (y == 7) yC = -edgePxPos;

      bool isXEdge            = (x == 0 || x == fNofPixels - 1);
      bool isYEdge            = (y == 0 || y == fNofPixels - 1);
      TGeoVolume* curPmtPixel = pixel;
      if (isXEdge) curPmtPixel = pixelX;
      if (isYEdge) curPmtPixel = pixelY;
      if (isXEdge && isYEdge) curPmtPixel = pixelCorner;

      pmt->AddNode(curPmtPixel, copyNum, MakeTrans(xC, yC, zC));
      copyNum++;
    }
  }

  TGeoBBox* pmtContPB = new TGeoBBox("pmt_cont_PB", pmtSizeHalf, pmtSizeHalf, pmtDepthHalf);
  TGeoVolume* pmtCont = new TGeoVolume("pmt_cont", pmtContPB, fMaterials.GetMedium("RICHgas_CO2_dis"));
  pmtCont->AddNode(pmt, 1, MakeTrans(0., 0., 0.));
  //if (fSetVolumeColors) pmtCont->SetLineColor(kRed - 4);

  return pmtCont;
}


TGeoVolume* RichGeoCreator::CreateCameraModule()
{
  double pmtSizeGap = fPmtSize + fCameraGap;

  TGeoVolume* pmt       = CreatePmt();
  TGeoBBox* camModulePB = new TGeoBBox("camera_module_PB", (3. * fPmtSize + 2. * fCameraGap) / 2.,
                                       (2. * fPmtSize + fCameraGap) / 2., fPmtDepth / 2.);

  TGeoVolume* camModule = new TGeoVolume("camera_module", camModulePB, fMaterials.GetMedium("RICHgas_CO2_dis"));
  //if (fSetVolumeColors) camModule->SetLineColor(kRed - 1);
  camModule->AddNode(pmt, 1, MakeTrans(-pmtSizeGap, pmtSizeGap / 2., 0.));
  camModule->AddNode(pmt, 2, MakeTrans(0., pmtSizeGap / 2., 0.));
  camModule->AddNode(pmt, 3, MakeTrans(pmtSizeGap, pmtSizeGap / 2., 0.));
  camModule->AddNode(pmt, 4, MakeTrans(-pmtSizeGap, -pmtSizeGap / 2., 0.));
  camModule->AddNode(pmt, 5, MakeTrans(0., -pmtSizeGap / 2., 0.));
  camModule->AddNode(pmt, 6, MakeTrans(pmtSizeGap, -pmtSizeGap / 2., 0.));

  return camModule;
}

TGeoVolume* RichGeoCreator::CreateCameraStrip()
{
  double pmtSizeGap    = fPmtSize + fCameraGap;
  double pmtDepthHalf  = fPmtDepth / 2.;
  double aluFrameDepth = 0.5;

  TGeoVolume* camModule = CreateCameraModule();

  double dY = (7. * (2. * fPmtSize + fCameraGap) + 6. * fCameraGap) / 2.;
  TGeoBBox* aluFramePB =
    new TGeoBBox("alu_frame_PB", fCameraTouchWidth / 2., (14. * fPmtSize + 13. * fCameraGap) / 2., aluFrameDepth);
  TGeoBBox* op0       = new TGeoBBox("camera_strip_PB_op0", (3. * fPmtSize + 2. * fCameraGap) / 2., dY, pmtDepthHalf);
  TGeoBBox* op1       = new TGeoBBox("camera_strip_PB_op1", fCameraTouchWidth / 2., dY, aluFrameDepth);
  TGeoTranslation* tr = MakeTrans(0., 0., -pmtDepthHalf - aluFrameDepth);
  TGeoCompositeShape* camStripPB = MakeUnion("camera_strip_PB", op0, op1, nullptr, tr);

  TGeoVolume* aluFrame = new TGeoVolume("alu_frame", aluFramePB, fMaterials.GetMedium("aluminium"));
  //if (fSetVolumeColors) aluFrame->SetLineColor(kBlue - 4);
  TGeoVolume* camStrip = new TGeoVolume("camera_strip", camStripPB, fMaterials.GetMedium("RICHgas_CO2_dis"));
  //if (fSetVolumeColors) camStrip->SetLineColor(kOrange + 9);
  int copyNum = 1;
  for (int i = -6; i <= 6; i += 2) {
    camStrip->AddNode(camModule, copyNum, MakeTrans(0., static_cast<double>(i) * pmtSizeGap, 0.));
    copyNum++;
  }
  camStrip->AddNode(aluFrame, 1, MakeTrans(0., 0., -pmtDepthHalf - aluFrameDepth));

  return camStrip;
}

TGeoVolumeAssembly* RichGeoCreator::CreateCameraContainer()
{
  TGeoVolume* camStrip = CreateCameraStrip();
  double len           = fCameraRadius - (fPmtDepth / 2.);

  TGeoVolumeAssembly* camCont = new TGeoVolumeAssembly("camera_cont");
  int copyNum                 = 1;
  for (int i = -7; i <= 6; i++) {
    double ang      = (static_cast<double>(i) + 0.5) * fCameraSegmentAng;
    TGeoMatrix* mat = MakeCombiTrans(len * sin(ToRad(ang)), 0., len * cos(ToRad(ang)), 0., ang, 0.);
    camCont->AddNode(camStrip, copyNum, mat);
    copyNum++;
  }
  return camCont;
}

TGeoVolumeAssembly* RichGeoCreator::CreateMirror()
{
  double mRadius       = fMirrorRadius;
  double mRadius2      = mRadius + fMirrorThickness;
  double dTheta        = fAlpha / 2. - fMirrorThetaGap * 3. / 4.;
  double phi1          = -fMirrorPhiSize / 2.;
  double phi2          = fMirrorPhiSize / 2.;
  double mRot90        = 90. + fMirrorRot;
  TGeoMedium* medGlass = fMaterials.GetMedium("RICHglass");

  // Mirror tile type 0
  double theta0    = mRot90 + fAlpha / 2. + fMirrorThetaGap * 3. / 4.;
  TGeoSphere* op00 = new TGeoSphere("mirror_tile_type0_PB_op0", mRadius, mRadius2, theta0, theta0 + dTheta, phi1, phi2);
  TGeoBBox* op01   = new TGeoBBox("mirror_tile_type0_PB_op1", 500., fRichWidth / 2., 10.);
  TGeoTranslation* tr0       = MakeTrans(0., 0., -fMirrorOriginY - 10.);
  TGeoCompositeShape* mType0 = MakeSubtraction("mirror_tile_type0_PB", op00, op01, nullptr, tr0);
  TGeoVolume* mVolType0      = new TGeoVolume("mirror_tile_type0", mType0, medGlass);
  //if (fSetVolumeColors) mVolType0->SetLineColor(kYellow + 1);

  // Mirror tile type 1
  double theta1      = mRot90 + fMirrorThetaGap / 2.;
  TGeoSphere* mType1 = new TGeoSphere("mirror_tile_type1_PB", mRadius, mRadius2, theta1, theta1 + dTheta, phi1, phi2);
  TGeoVolume* mVolType1 = new TGeoVolume("mirror_tile_type1", mType1, medGlass);
  //if (fSetVolumeColors) mVolType1->SetLineColor(kYellow + 2);

  // Mirror tile type 2
  double theta2      = mRot90 - fAlpha / 2. + fMirrorThetaGap / 4.;
  TGeoSphere* mType2 = new TGeoSphere("mirror_tile_type2_PB", mRadius, mRadius2, theta2, theta2 + dTheta, phi1, phi2);
  TGeoVolume* mVolType2 = new TGeoVolume("mirror_tile_type2", mType2, medGlass);
  //if (fSetVolumeColors) mVolType2->SetLineColor(kYellow + 3);

  // Mirror tile type 3
  double theta3      = mRot90 - fAlpha;
  TGeoSphere* mType3 = new TGeoSphere("mirror_tile_type3_PB", mRadius, mRadius2, theta3, theta3 + dTheta, phi1, phi2);
  TGeoVolume* mVolType3 = new TGeoVolume("mirror_tile_type3", mType3, medGlass);
  //if (fSetVolumeColors) mVolType3->SetLineColor(kYellow - 1);

  // Mirror tile type 4
  double theta4 = mRot90 + fAlpha / 2. + fMirrorThetaGap * 3. / 4.;
  TGeoSphere* op4_0 =
    new TGeoSphere("mirror_tile_type4_PB_op0", mRadius, mRadius2, theta4, theta4 + dTheta, phi1, phi2);
  TGeoBBox* op4_1 = new TGeoBBox("mirror_tile_type4_cutoff", 30, 40, 25);
  TGeoCombiTrans* tr4_1 =
    MakeCombiTrans(mRadius * Sin((theta4 + dTheta / 2) * pi / 180) * Cos((phi1 + phi2) / 2 * pi / 180),
                   mRadius * Sin((theta4 + dTheta / 2) * pi / 180) * Sin((phi1 + phi2) / 2 * pi / 180) + 15,
                   mRadius * Cos((theta4 + dTheta / 2) * pi / 180) - 22, 45, 0, 0);
  TGeoCompositeShape* mType4 = MakeSubtraction("mirror_tile_type4_PB", op4_0, op4_1, nullptr, tr4_1);
  TGeoVolume* mVolType4      = new TGeoVolume("mirror_tile_type4", mType4, medGlass);
  //if (fSetVolumeColors) mVolType4->SetLineColor(kYellow - 2);

  // Mirror tile type 5
  double theta5 = mRot90 + fAlpha / 2. + fMirrorThetaGap * 3. / 4.;
  TGeoSphere* op5_0 =
    new TGeoSphere("mirror_tile_type5_PB_op0", mRadius, mRadius2, theta5, theta5 + dTheta, phi1, phi2);
  TGeoBBox* op5_1 = new TGeoBBox("mirror_tile_type5_cutoff", 20, 30, 40);
  TGeoCombiTrans* tr5_1 =
    MakeCombiTrans(mRadius * Sin((theta5 + dTheta / 2) * pi / 180) * Cos((phi1 + phi2) / 2 * pi / 180),
                   mRadius * Sin((theta5 + dTheta / 2) * pi / 180) * Sin((phi1 + phi2) / 2 * pi / 180) - 20,
                   mRadius * Cos((theta5 + dTheta / 2) * pi / 180) - 22, 45, 0, 0);
  TGeoCompositeShape* mType5 = MakeSubtraction("mirror_tile_type5_PB", op5_0, op5_1, nullptr, tr5_1);
  TGeoVolume* mVolType5      = new TGeoVolume("mirror_tile_type5", mType5, medGlass);
  //if (fSetVolumeColors) mVolType5->SetLineColor(kYellow - 3);

  TGeoVolumeAssembly* mHalf = new TGeoVolumeAssembly("mirror_half");

  for (int t = 0; t < 4; t++) {
    TGeoVolume* mVol = nullptr;
    if (t == 1) mVol = mVolType1;
    if (t == 2) mVol = mVolType2;
    if (t == 3) mVol = mVolType3;
    for (int i = 0; i < 10; i++) {
      double rotZ = (i - 4.5) * fMirrorPhiStep;
      int copyNum = i + 1;
      if (t == 0) {
        mVol = mVolType0;
        if (i == 4 || i == 5) {  // special case for the 2 middle mirror tiles
          mVol    = (i == 4) ? mVolType5 : mVolType4;
          copyNum = 1;
        }
      }
      mHalf->AddNode(mVol, copyNum, MakeRot(0., 0., rotZ));
    }
  }

  return mHalf;
}

TGeoVolume* RichGeoCreator::CreateStuds(double ypos, double lenHalf, double angle)
{
  TGeoMedium* medGas = fMaterials.GetMedium("RICHgas_CO2_dis");
  TGeoMedium* medAl  = fMaterials.GetMedium("aluminium");

  double StudsSizeHalf    = 5.;
  double StudsSizeHalfGas = 4.8;
  double lenHalf1         = lenHalf + Tan(angle * pi / 180) * StudsSizeHalf / 2;
  double lenHalf2         = lenHalf - Tan(angle * pi / 180) * StudsSizeHalf / 2;
  double alpha1           = angle / 2;

  TGeoTrap* StudsGas      = new TGeoTrap("StudsGas_Trap", StudsSizeHalfGas, 0, 0, StudsSizeHalfGas, lenHalf1, lenHalf2,
                                    alpha1, StudsSizeHalfGas, lenHalf1, lenHalf2, alpha1);
  TGeoTrap* Studs         = new TGeoTrap("Studs_Trap", StudsSizeHalf, 0, 0, StudsSizeHalf, lenHalf1, lenHalf2, alpha1,
                                 StudsSizeHalf, lenHalf1, lenHalf2, alpha1);
  TGeoVolume* volStudsGas = new TGeoVolume("StudsGas", StudsGas, medGas);
  TGeoVolume* volStuds    = new TGeoVolume("Studs", Studs, medAl);

  volStuds->AddNode(volStudsGas, 1);
  TGeoVolume* returnVol = new TGeoVolumeAssembly("Stud");
  returnVol->AddNode(volStuds, 1, MakeCombiTrans(0, ypos, lenHalf, 0, 90, 0));
  return returnVol;
}

TGeoVolume* RichGeoCreator::CreatePillarConnection()
{
  TGeoMedium* medGas = fMaterials.GetMedium("RICHgas_CO2_dis");
  TGeoMedium* medAl  = fMaterials.GetMedium("aluminium");

  double lenHalf   = 79 / 2;
  double widthHalf = 3.0;
  double hightHalf = 1.25;
  double thickness = 0.2;
  double lendiff   = widthHalf * Tan(8.5 * pi / 180);

  TGeoTrap* PillarConnectionGas =
    new TGeoTrap("PillarConnectionGas_Trap", hightHalf - thickness, 0, 0, widthHalf - thickness, lenHalf + lendiff,
                 lenHalf - lendiff, 0, widthHalf - thickness, lenHalf + lendiff, lenHalf - lendiff, 0);
  TGeoTrap* PillarConnection = new TGeoTrap("PillarConnection_Trap", hightHalf, 0, 0, widthHalf, lenHalf + lendiff,
                                            lenHalf - lendiff, 0, widthHalf, lenHalf + lendiff, lenHalf - lendiff, 0);
  TGeoVolume* volPillarConnectionGas = new TGeoVolume("PillarConnectionGas", PillarConnectionGas, medGas);
  TGeoVolume* volPillarConnection    = new TGeoVolume("PillarConnection", PillarConnection, medAl);
  volPillarConnection->AddNode(volPillarConnectionGas, 1);
  TGeoVolume* returnVol = new TGeoVolumeAssembly("PillarConnection");
  returnVol->AddNode(volPillarConnection, 1, MakeCombiTrans(0, 0, 0, -90, 0, 0));
  return returnVol;
}

TGeoVolumeAssembly* RichGeoCreator::CreateMirrorSupportPillars()
{
  TGeoMedium* medGas = fMaterials.GetMedium("RICHgas_CO2_dis");
  TGeoMedium* medAl  = fMaterials.GetMedium("aluminium");

  TGeoBBox* pbGas = new TGeoBBox("mirror_support_pillar_gas_PB", 4.8, (fRichHeight / 2.) - 10.5, 4.8);
  TGeoBBox* pb    = new TGeoBBox("mirror_support_pillar_PB", 5., (fRichHeight / 2.) - 10.5, 5.);

  TGeoVolume* volGas = new TGeoVolume("mirror_support_pillar_gas", pbGas, medGas);
  TGeoVolume* vol    = new TGeoVolume("mirror_support_pillar", pb, medAl);
  //if (fSetVolumeColors) volGas->SetLineColor(kAzure + 5);
  //if (fSetVolumeColors) vol->SetLineColor(kAzure + 5);
  vol->AddNode(volGas, 1);

  TGeoVolumeAssembly* StudsHalf = new TGeoVolumeAssembly("Studs_Half");
  StudsHalf->AddNode(CreateStuds(16.4, 30.03 / 2., 24.2), 1);
  StudsHalf->AddNode(CreateStuds(57.8, 14.86 / 2., 15.9), 2);
  StudsHalf->AddNode(CreateStuds(101.05, 5.74 / 2., 7.7), 3);
  StudsHalf->AddNode(CreateStuds(145.4, 2.9 / 2., -0.5), 4);

  vol->AddNode(StudsHalf, 1, MakeCombiTrans(0, 0., -5, 0, 180, 0));
  vol->AddNode(StudsHalf, 2, MakeCombiTrans(0, 0., -5, 0, 180, 180));

  TGeoVolume* PillarConnection = CreatePillarConnection();

  double dZ                   = 41.;
  TGeoVolumeAssembly* pillars = new TGeoVolumeAssembly("mirror_support_pillars");
  pillars->AddNode(vol, 1, MakeCombiTrans(210.7759 - 2.035, 0., -29.5298 + dZ - 12.4346, 0., 42., 0.));
  pillars->AddNode(PillarConnection, 1, MakeCombiTrans(170.7323, 0., -15.9069 + dZ, 0., 33.6, 0.));
  pillars->AddNode(vol, 2, MakeCombiTrans(134.1222 - 1.3985, 0., 21.3989 + dZ - 11.2483, 0., 25.2, 0.));
  pillars->AddNode(PillarConnection, 2, MakeCombiTrans(89.097, 0., 23.74 + dZ, 0., 16.8, 0.));
  pillars->AddNode(vol, 3, MakeCombiTrans(46.015 - 0.545, 0., 48. + dZ - 10.6668, 0., 8.4, 0.));
  pillars->AddNode(PillarConnection, 3, MakeCombiTrans(0., 38.75, 37.34 + dZ, 0., 0., 0.));
  pillars->AddNode(PillarConnection, 4, MakeCombiTrans(0., -38.75, 37.34 + dZ, 0., 0., 0.));
  pillars->AddNode(vol, 4, MakeCombiTrans(-46.015 + 0.545, 0., 48. + dZ - 10.6668, 0., -8.4, 0.));
  pillars->AddNode(PillarConnection, 5, MakeCombiTrans(-89.097, 0., 23.74 + dZ, 0., -16.8, 0.));
  pillars->AddNode(vol, 5, MakeCombiTrans(-134.1222 + 1.3985, 0., 21.3989 + dZ - 11.2483, 0., -25.2, 0.));
  pillars->AddNode(PillarConnection, 6, MakeCombiTrans(-170.7323, 0., -15.9069 + dZ, 0., -33.6, 0.));
  pillars->AddNode(vol, 6, MakeCombiTrans(-210.7759 + 2.035, 0., -29.5298 + dZ - 12.4346, 0., -42., 0.));

  return pillars;
}

TGeoVolumeAssembly* RichGeoCreator::CreateBeltAssembly()
{
  TGeoMedium* medGas = fMaterials.GetMedium("RICHgas_CO2_dis");
  TGeoMedium* medAl  = fMaterials.GetMedium("aluminium");

  TGeoBBox* pbGas1    = new TGeoBBox("belt1_gas_PB", 1.25, 10.367, 1.167);
  TGeoBBox* pb1       = new TGeoBBox("belt1_PB", 1.5, 10.6, 1.4);
  TGeoVolume* volGas1 = new TGeoVolume("belt1_gas", pbGas1, medGas);
  TGeoVolume* vol1    = new TGeoVolume("belt1", pb1, medAl);
  //if (fSetVolumeColors) volGas1->SetLineColor(kViolet + 8);
  //if (fSetVolumeColors) vol1->SetLineColor(kViolet + 8);
  vol1->AddNode(volGas1, 1, MakeTrans(0., 0., 0.233));

  TGeoBBox* pbGas2    = new TGeoBBox("belt2_gas_PB", 4.7835, 1.25, 1.167);
  TGeoBBox* pb2       = new TGeoBBox("belt2_PB", 4.9, 1.5, 1.4);
  TGeoVolume* volGas2 = new TGeoVolume("belt2_gas", pbGas2, medGas);
  TGeoVolume* vol2    = new TGeoVolume("belt2", pb2, medAl);
  //if (fSetVolumeColors) volGas2->SetLineColor(kViolet + 8);
  //if (fSetVolumeColors) vol2->SetLineColor(kViolet + 8);
  vol2->AddNode(volGas2, 1, MakeTrans(0.1165, 0., 0.233));

  TGeoBBox* pbGas3    = new TGeoBBox("belt3_gas_PB", 11.6135, 1.25, 1.167);
  TGeoBBox* pb3       = new TGeoBBox("belt3_PB", 11.73, 1.5, 1.4);
  TGeoVolume* volGas3 = new TGeoVolume("belt3_gas", pbGas3, medGas);
  TGeoVolume* vol3    = new TGeoVolume("belt3", pb3, medAl);
  //if (fSetVolumeColors) volGas3->SetLineColor(kViolet + 8);
  //if (fSetVolumeColors) vol3->SetLineColor(kViolet + 8);
  vol3->AddNode(volGas3, 1, MakeTrans(0.1165, 0., 0.233));

  TGeoTrap* pbGas4    = new TGeoTrap("belt4_gas_PB", 0.7, 16.22, 90., 10.97, 1.3, 1.3, 0., 9.5, 1.3, 1.3, 0.);
  TGeoTrap* pb4       = new TGeoTrap("belt4_PB", 0.9, 16.22, 90., 11.378, 1.4, 1.4, 0., 9.5, 1.4, 1.4, 0.);
  TGeoVolume* volGas4 = new TGeoVolume("belt4_gas", pbGas4, medGas);
  TGeoVolume* vol4    = new TGeoVolume("belt4", pb4, medAl);
  //if (fSetVolumeColors) volGas4->SetLineColor(kViolet + 8);
  //if (fSetVolumeColors) vol4->SetLineColor(kViolet + 8);
  vol4->AddNode(volGas4, 1, MakeTrans(0.1, 0., 0.));

  TGeoVolumeAssembly* belt = new TGeoVolumeAssembly("belt");
  belt->AddNode(vol1, 1);
  belt->AddNode(vol2, 1, MakeTrans(-6.4, 9.1, 0.));
  belt->AddNode(vol3, 1, MakeTrans(-13.23, -9.1, 0.));
  belt->AddNode(vol4, 1, MakeCombiTrans(-17.567, 0.75, 0., -127., -90., -90.));

  return belt;
}

TGeoVolumeAssembly* RichGeoCreator::CreateMirrorSupportBelts()
{
  double mRadius2      = fMirrorRadius + fMirrorThickness;
  double mRadius3      = mRadius2 + fMirrorSupportAdd;
  double phi1          = -fMirrorPhiSize / 2.;
  double phi2          = fMirrorPhiSize / 2.;
  double dTheta        = fMirrorThetaSize;
  double mRot90        = 90. + fMirrorRot;
  double richWidthHalf = fRichWidth / 2.;
  TGeoMedium* medGas   = fMaterials.GetMedium("RICHgas_CO2_dis");

  TGeoVolumeAssembly* beltAssembly = CreateBeltAssembly();

  // Type 0
  double theta0       = mRot90 + fAlpha / 2. + fMirrorThetaGap * 3. / 4.;
  TGeoSphere* sphere0 = new TGeoSphere("belt_type0_PB_op0", mRadius2, mRadius3, theta0, theta0 + dTheta, phi1, phi2);
  TGeoBBox* box0      = new TGeoBBox("belt_type0_PB_op1", 500., richWidthHalf, 10.);
  TGeoCompositeShape* pbType0 =
    MakeSubtraction("belt_type0_PB", sphere0, box0, nullptr, MakeTrans(0., 0., -fMirrorOriginY - 10.));
  TGeoVolume* volType0 = new TGeoVolume("belt_type0", pbType0, medGas);
  volType0->AddNode(beltAssembly, 1, MakeCombiTrans(275.5, -13.23, -124.7, 65., 0., -90.));
  //if (fSetVolumeColors) volType0->SetLineColor(kGreen);

  // Type 1
  double theta1        = mRot90 + fMirrorThetaGap / 2.;
  TGeoSphere* pbType1  = new TGeoSphere("belt_type1_PB", mRadius2, mRadius3, theta1, theta1 + dTheta, phi1, phi2);
  TGeoVolume* volType1 = new TGeoVolume("belt_type1", pbType1, medGas);
  volType1->AddNode(beltAssembly, 1, MakeCombiTrans(290.4, -13.23, -84.0, 73., 0., -90.));
  //if (fSetVolumeColors) volType1->SetLineColor(kGreen - 1);

  // Type 2
  double theta2        = mRot90 - fAlpha / 2. + fMirrorThetaGap / 4.;
  TGeoSphere* pbType2  = new TGeoSphere("belt_type2_PB", mRadius2, mRadius3, theta2, theta2 + dTheta, phi1, phi2);
  TGeoVolume* volType2 = new TGeoVolume("belt_type2", pbType2, medGas);
  volType2->AddNode(beltAssembly, 1, MakeCombiTrans(299.4, -13.23, -41.5, 82., 0., -90.));
  //if (fSetVolumeColors) volType2->SetLineColor(kGreen - 2);

  // Type 3
  double theta3        = mRot90 - fAlpha;
  TGeoSphere* pbType3  = new TGeoSphere("belt_type3_PB", mRadius2, mRadius3, theta3, theta3 + dTheta, phi1, phi2);
  TGeoVolume* volType3 = new TGeoVolume("belt_type3", pbType3, medGas);
  volType3->AddNode(beltAssembly, 1, MakeCombiTrans(302.5, -13.23, 1.97, 91., 0., -90.));
  //if (fSetVolumeColors) volType3->SetLineColor(kGreen - 3);

  //// Type 4
  //double theta4       = mRot90 + fAlpha / 2. + fMirrorThetaGap * 3. / 4.;
  //TGeoSphere* sphere4 = new TGeoSphere("belt_type4_PB_op0", mRadius2, mRadius3, theta4, theta4 + dTheta, phi1, phi2);
  //TGeoTubeSeg* tube4  = new TGeoTubeSeg("belt_type4_PB_op1", 0., fPipeOuterRadiusExit, 400., 0., 360.);
  //TGeoCompositeShape* sub4_1 = MakeSubtraction("belt_type4_PB_op01", sphere4, tube4, nullptr, MakeCombiTrans(0., 0., -fMirrorOriginY, 90., 0., 90. + fMirrorPhiStep * 0.5));
  //TGeoBBox* box4 = new TGeoBBox("belt_type4_PB_op2", 500., richWidthHalf, 10.);
  //TGeoCompositeShape* pbType4 = MakeSubtraction("belt_type4_PB", sub4_1, box4, nullptr, MakeTrans(0., 0., -fMirrorOriginY - 10.));
  //TGeoVolume* volType4 = new TGeoVolume("belt_type4", pbType4, medGas);
  //volType4->AddNode(beltAssembly, 1, MakeCombiTrans(281.0, 0., -113.0, 0., -66., 0.));
  ////if (fSetVolumeColors) volType4->SetLineColor(kGreen + 1);
  //
  ////  Type 5
  //double theta5       = mRot90 + fAlpha / 2. + fMirrorThetaGap * 3. / 4.;
  //TGeoSphere* sphere5 = new TGeoSphere("belt_type5_PB_op0", mRadius2, mRadius3, theta5, theta5 + dTheta, phi1, phi2);
  //TGeoTubeSeg* tube5  = new TGeoTubeSeg("belt_type5_PB_op1", 0., fPipeOuterRadiusExit, 400., 0., 360.);
  //TGeoCompositeShape* sub5_1 = MakeSubtraction("belt_type5_PB_op01", sphere5, tube5, nullptr, MakeCombiTrans(0., 0., -fMirrorOriginY, 90., 0., 90. - fMirrorPhiStep * 0.5));
  //TGeoBBox* box5 = new TGeoBBox("belt_type5_PB_op2", 500., richWidthHalf, 10.);
  //TGeoCompositeShape* pbType5 = MakeSubtraction("belt_type5_PB", sub5_1, box5, nullptr, MakeTrans(0., 0., -fMirrorOriginY - 10.));
  //TGeoVolume* volType5 = new TGeoVolume("belt_type5", pbType5, medGas);
  //volType5->AddNode(beltAssembly, 1, MakeCombiTrans(281., 0., -113., 0., -114., 180.));
  ////if (fSetVolumeColors) volType5->SetLineColor(kGreen + 2);

  TGeoVolumeAssembly* contHalf = new TGeoVolumeAssembly("mirror_support_belts_half");
  for (int t = 0; t < 4; t++) {
    TGeoVolume* mVol = nullptr;
    if (t == 1) mVol = volType1;
    if (t == 2) mVol = volType2;
    if (t == 3) mVol = volType3;
    for (int i = 0; i < 10; i++) {
      double rotZ = (i - 4.5) * fMirrorPhiStep;
      int copyNum = i + 1;
      if (t == 0) {
        mVol = volType0;
        if (i == 4 || i == 5) {  // special case for the 2 middle belts
          continue;              //mVol    = (i == 4) ? volType5 : volType4;
          copyNum = 1;
        }
      }
      contHalf->AddNode(mVol, copyNum, MakeRot(0., 0., rotZ));
    }
  }
  return contHalf;
}

TGeoVolumeAssembly* RichGeoCreator::CreateMainFrame()
{
  TGeoMedium* medGas  = fMaterials.GetMedium("RICHgas_CO2_dis");
  TGeoMedium* medAlum = fMaterials.GetMedium("aluminium");

  double frameSizeHalf    = 5.;
  double frameSizeHalfGas = 4.3;
  double richHeightHalf   = fRichHeight / 2.;
  double richLengthHalf   = fRichLength / 2.;
  double richWidthHalf    = fRichWidth / 2.;
  double coverThickness   = fRichCoveringThickness;

  //double posY             = richHeightHalf - coverThickness - frameSizeHalf;
  double rotX     = 90 - fCameraFullRotX;
  double lenHalf2 = 23.2919390;
  double lenHalf3 = 20.;
  double lenHalf5 = richHeightHalf - Tan(rotX * pi / 180.0) * lenHalf3 * 2.0;
  double lenHalf6 = richLengthHalf - lenHalf2 - lenHalf3;
  double lenHalf8 = (richWidthHalf - 229.3 / 2) / 2;  // the 229.3 are the width of the shieldingbox

  double posY             = richHeightHalf - coverThickness - frameSizeHalf;
  double posY2            = 90;
  double posY3            = posY - Tan(rotX * pi / 180.0) * lenHalf3 * 2.0;
  double posZ1            = -147.779439;
  double posZ2            = posZ1 + lenHalf2 + 2 * lenHalf3;
  double posZ3            = posZ1 + 2 * lenHalf2 + 2 * lenHalf3;
  double posZ4            = posZ1 + fRichLength;
  double posX1            = 134.6;
  double posX2            = (posX1 + richWidthHalf) / 2;
  double posX3            = richWidthHalf - frameSizeHalf;

  TGeoBBox* crossingbox =
    new TGeoBBox("crossing", frameSizeHalf, frameSizeHalf,
                 frameSizeHalf);  // to cutoff parts of the frames where they would overlap each other

  TGeoVolumeAssembly* mainFrame = new TGeoVolumeAssembly("mainframe");

  // parts along x-axis
  TGeoBBox* pbGas1 =
    new TGeoBBox("mainframe1_gas_PB", richWidthHalf - frameSizeHalf * 2, frameSizeHalfGas, frameSizeHalfGas);
  TGeoBBox* pb1       = new TGeoBBox("mainframe1_PB", richWidthHalf - frameSizeHalf * 2, frameSizeHalf, frameSizeHalf);
  TGeoVolume* volGas1 = new TGeoVolume("mainframe1_gas", pbGas1, medGas);
  TGeoVolume* vol1    = new TGeoVolume("mainframe1", pb1, medAlum);
  //if (fSetVolumeColors) vol1->SetLineColor(kBlue);
  //if (fSetVolumeColors) volGas1->SetLineColor(kBlue);
  vol1->AddNode(volGas1, 1);
  mainFrame->AddNode(vol1, 1, MakeTrans(0, posY2, posZ1));
  mainFrame->AddNode(vol1, 2, MakeTrans(0, -posY2, posZ1));
  mainFrame->AddNode(vol1, 3, MakeTrans(0., posY, posZ4));
  mainFrame->AddNode(vol1, 4, MakeTrans(0., posY, posZ3));
  mainFrame->AddNode(vol1, 5, MakeTrans(0., -posY, posZ4));
  mainFrame->AddNode(vol1, 6, MakeTrans(0., -posY, posZ3));

  // first parts along z-axis (short)
  TGeoBBox* pbGas2    = new TGeoBBox("mainframe2_gas_PB", frameSizeHalfGas, frameSizeHalfGas, lenHalf2 - frameSizeHalf);
  TGeoBBox* pb2       = new TGeoBBox("mainframe2_PB", frameSizeHalf, frameSizeHalf, lenHalf2 - frameSizeHalf);
  TGeoVolume* volGas2 = new TGeoVolume("mainframe2_gas", pbGas2, medGas);
  TGeoVolume* vol2    = new TGeoVolume("mainframe2", pb2, medAlum);
  //if (fSetVolumeColors) vol2->SetLineColor(kBlue + 1);
  //if (fSetVolumeColors) volGas2->SetLineColor(kBlue + 1);
  vol2->AddNode(volGas2, 1);
  mainFrame->AddNode(vol2, 1, MakeTrans(-posX1, posY, posZ2 - frameSizeHalf / 2));
  mainFrame->AddNode(vol2, 2, MakeTrans(-posX1, -posY, posZ2 - frameSizeHalf / 2));
  mainFrame->AddNode(vol2, 3, MakeTrans(posX1, posY, posZ2 - frameSizeHalf / 2));
  mainFrame->AddNode(vol2, 4, MakeTrans(posX1, -posY, posZ2 - frameSizeHalf / 2));
  mainFrame->AddNode(vol2, 5, MakeTrans(posX3, posY, posZ2 - frameSizeHalf / 2));
  mainFrame->AddNode(vol2, 6, MakeTrans(posX3, -posY, posZ2 - frameSizeHalf / 2));
  mainFrame->AddNode(vol2, 7, MakeTrans(-posX3, posY, posZ2 - frameSizeHalf / 2));
  mainFrame->AddNode(vol2, 8, MakeTrans(-posX3, -posY, posZ2 - frameSizeHalf / 2));

  // diagonal parts (y-z)
  TGeoTrap* pbGas3 =
    new TGeoTrap("mainframe3_gas_PB", frameSizeHalfGas, 0, 0, lenHalf3 - frameSizeHalf - coverThickness, 6.3, 9.3, rotX,
                 lenHalf3 - frameSizeHalf - coverThickness, 6.3, 9.3, rotX);
  TGeoTrap* pb3       = new TGeoTrap("mainframe3_PB", frameSizeHalf, 0, 0, lenHalf3 - frameSizeHalf, 7, 10, rotX,
                               lenHalf3 - frameSizeHalf, 7, 10, rotX);
  TGeoVolume* volGas3 = new TGeoVolume("mainframe3_gas", pbGas3, medGas);
  TGeoVolume* vol3    = new TGeoVolume("mainframe3", pb3, medAlum);
  TGeoRotation* rotup = new TGeoRotation();
  rotup->RotateX(-90 + rotX * pi / 180);
  rotup->RotateZ(180);
  rotup->RotateY(90);
  vol3->AddNode(volGas3, 1);
  mainFrame->AddNode(vol3, 1,
                     new TGeoCombiTrans(*MakeTrans(-posX3, posY - Tan(rotX * pi / 180) * lenHalf3 + frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotup->Inverse()));
  mainFrame->AddNode(vol3, 2,
                     new TGeoCombiTrans(*MakeTrans(-posX1, posY - Tan(rotX * pi / 180) * lenHalf3 + frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotup->Inverse()));
  mainFrame->AddNode(vol3, 3,
                     new TGeoCombiTrans(*MakeTrans(posX1, posY - Tan(rotX * pi / 180) * lenHalf3 + frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotup->Inverse()));
  mainFrame->AddNode(vol3, 4,
                     new TGeoCombiTrans(*MakeTrans(posX3, posY - Tan(rotX * pi / 180) * lenHalf3 + frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotup->Inverse()));
  TGeoRotation* rotdown = new TGeoRotation();
  rotdown->RotateX(-90 + rotX * pi / 180);
  rotdown->RotateY(90);
  mainFrame->AddNode(vol3, 5,
                     new TGeoCombiTrans(*MakeTrans(-posX3, -posY + Tan(rotX * pi / 180) * lenHalf3 - frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotdown->Inverse()));
  mainFrame->AddNode(vol3, 6,
                     new TGeoCombiTrans(*MakeTrans(-posX1, -posY + Tan(rotX * pi / 180) * lenHalf3 - frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotdown->Inverse()));
  mainFrame->AddNode(vol3, 7,
                     new TGeoCombiTrans(*MakeTrans(posX1, -posY + Tan(rotX * pi / 180) * lenHalf3 - frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotdown->Inverse()));
  mainFrame->AddNode(vol3, 8,
                     new TGeoCombiTrans(*MakeTrans(posX3, -posY + Tan(rotX * pi / 180) * lenHalf3 - frameSizeHalf,
                                                   posZ1 + lenHalf3 + frameSizeHalf / 2),
                                        rotdown->Inverse()));


  // long vertical parts
  TGeoBBox* pbGas4 =
    new TGeoBBox("mainframe4_gas_PB", frameSizeHalfGas, richHeightHalf - coverThickness, frameSizeHalfGas);
  TGeoBBox* pb4       = new TGeoBBox("mainframe4_PB", frameSizeHalf, richHeightHalf - coverThickness, frameSizeHalf);
  TGeoVolume* volGas4 = new TGeoVolume("mainframe4_gas", pbGas4, medGas);
  TGeoVolume* vol4    = new TGeoVolume("mainframe4", pb4, medAlum);
  //if (fSetVolumeColors) vol4->SetLineColor(kBlue + 3);
  //if (fSetVolumeColors) volGas4->SetLineColor(kBlue + 3);
  vol4->AddNode(volGas4, 1);
  mainFrame->AddNode(vol4, 1, MakeTrans(-posX3, 0., posZ4));
  mainFrame->AddNode(vol4, 2, MakeTrans(posX3, 0., posZ4));
  mainFrame->AddNode(vol4, 3, MakeTrans(-posX3, 0, posZ3));
  mainFrame->AddNode(vol4, 4, MakeTrans(posX3, 0, posZ3));

  // small vertical parts
  TGeoTrap* pbGas5 = new TGeoTrap("mainframe5_gas_PB", frameSizeHalfGas, 0, 0, frameSizeHalfGas,
                                  lenHalf5 - 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf,
                                  lenHalf5 + 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf, 0, frameSizeHalfGas,
                                  lenHalf5 - 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf,
                                  lenHalf5 + 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf, 0);
  TGeoTrap* pb5    = new TGeoTrap("mainframe5_PB", frameSizeHalf, 0, 0, frameSizeHalf,
                               lenHalf5 - 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf,
                               lenHalf5 + 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf, 0, frameSizeHalf,
                               lenHalf5 - 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf,
                               lenHalf5 + 1 / Tan((90 - rotX) * pi / 180) * frameSizeHalf, 0);
  TGeoCompositeShape* sub5_1 =
    MakeUnion("subtraction_mainframe5", crossingbox, crossingbox, MakeTrans(posY2, 0, 0), MakeTrans(posY3, 0, 0));
  TGeoCompositeShape* sub5 =
    MakeUnion("subtraction_mainframe5", sub5_1, sub5_1, nullptr, MakeCombiTrans(0, 0, 0, 0, 180, 0));
  TGeoVolume* volGas5 =
    new TGeoVolume("mainframe5_gas", MakeSubtraction("mainframe5_gas_PB", pbGas5, sub5, nullptr, nullptr), medGas);
  TGeoVolume* vol5 =
    new TGeoVolume("mainframe5", MakeSubtraction("mainframe5_PB", pb5, sub5, nullptr, nullptr), medAlum);
  vol5->AddNode(volGas5, 1);
  mainFrame->AddNode(vol5, 1, MakeCombiTrans(-posX1, 0., posZ1, 90, 0, 90));
  mainFrame->AddNode(vol5, 2, MakeCombiTrans(posX1, 0., posZ1, 90, 0, 90));
  mainFrame->AddNode(vol5, 3, MakeCombiTrans(-posX3, 0., posZ1, 90, 0, 90));
  mainFrame->AddNode(vol5, 4, MakeCombiTrans(posX3, 0., posZ1, 90, 0, 90));

  // small horizontal parts
  TGeoBBox* pbGas6    = new TGeoBBox("mainframe6_gas_PB", frameSizeHalfGas, frameSizeHalfGas, lenHalf6 - frameSizeHalf);
  TGeoBBox* pb6       = new TGeoBBox("mainframe6_PB", frameSizeHalf, frameSizeHalf, lenHalf6 - frameSizeHalf);
  TGeoVolume* volGas6 = new TGeoVolume("mainframe6_gas", pbGas6, medGas);
  TGeoVolume* vol6    = new TGeoVolume("mainframe6", pb6, medAlum);
  //if (fSetVolumeColors) vol6->SetLineColor(kBlue - 2);
  //if (fSetVolumeColors) volGas6->SetLineColor(kBlue - 2);
  vol6->AddNode(volGas6, 1);
  mainFrame->AddNode(vol6, 1, MakeTrans(-posX2, posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 2, MakeTrans(-posX2, -posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 3, MakeTrans(posX2, posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 4, MakeTrans(posX2, -posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 5, MakeTrans(-posX1, posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 6, MakeTrans(-posX1, -posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 7, MakeTrans(posX1, posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 8, MakeTrans(posX1, -posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 9, MakeTrans(0., posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 10, MakeTrans(0., -posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 11, MakeTrans(posX3, posY2, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 12, MakeTrans(posX3, -posY2, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 13, MakeTrans(-posX3, posY2, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 14, MakeTrans(-posX3, -posY2, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 15, MakeTrans(-posX3, posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 16, MakeTrans(-posX3, -posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 17, MakeTrans(posX3, posY, posZ4 - lenHalf6));
  mainFrame->AddNode(vol6, 18, MakeTrans(posX3, -posY, posZ4 - lenHalf6));


  // first parts along z-axis (long)
  TGeoBBox* pbGas7    = new TGeoBBox("mainframe7_gas_PB", frameSizeHalfGas, frameSizeHalfGas, lenHalf2 + lenHalf3);
  TGeoBBox* pb7       = new TGeoBBox("mainframe7_PB", frameSizeHalf, frameSizeHalf, lenHalf2 + lenHalf3);
  TGeoVolume* volGas7 = new TGeoVolume("mainframe7_gas", pbGas7, medGas);
  TGeoVolume* vol7    = new TGeoVolume("mainframe7", pb7, medAlum);
  vol7->AddNode(volGas7, 1);
  mainFrame->AddNode(vol7, 1, MakeTrans(posX3, posY2, posZ2 - lenHalf3 - frameSizeHalf));
  mainFrame->AddNode(vol7, 2, MakeTrans(posX3, -posY2, posZ2 - lenHalf3 - frameSizeHalf));
  mainFrame->AddNode(vol7, 3, MakeTrans(-posX3, posY2, posZ2 - lenHalf3 - frameSizeHalf));
  mainFrame->AddNode(vol7, 4, MakeTrans(-posX3, -posY2, posZ2 - lenHalf3 - frameSizeHalf));

  // left & right of shieldingbox along x-axis
  TGeoBBox* pbGas8    = new TGeoBBox("mainframe8_gas_PB", lenHalf8, frameSizeHalfGas, frameSizeHalfGas);
  TGeoBBox* pb8       = new TGeoBBox("mainframe8_PB", lenHalf8, frameSizeHalf, frameSizeHalf);
  TGeoVolume* volGas8 = new TGeoVolume("mainframe8_gas", pbGas8, medGas);
  TGeoVolume* vol8    = new TGeoVolume("mainframe8", pb8, medAlum);
  vol8->AddNode(volGas8, 1);
  mainFrame->AddNode(vol8, 1, MakeTrans(posX3 - lenHalf8 + frameSizeHalf, posY3, posZ1));
  mainFrame->AddNode(vol8, 2, MakeTrans(-posX3 + lenHalf8 - frameSizeHalf, posY3, posZ1));
  mainFrame->AddNode(vol8, 3, MakeTrans(posX3 - lenHalf8 + frameSizeHalf, -posY3, posZ1));
  mainFrame->AddNode(vol8, 4, MakeTrans(-posX3 + lenHalf8 - frameSizeHalf, -posY3, posZ1));

  return mainFrame;
}

TGeoVolume* RichGeoCreator::CreateGas()
{
  double richLenHalf    = fRichLength / 2.;
  double richWidthHalf  = fRichWidth / 2.;
  double richHeightHalf = fRichHeight / 2.;

  double x1 = richWidthHalf - fRichCoveringThickness;
  double y1 = ((2. * richHeightHalf - fRichEntranceHeight) / 2. - 2. * fRichCoveringThickness) / 2.;
  double z1 = fRichAddZ / 2. - fRichCoveringThickness / 2. + fRichEntranceThickness / 2.;
  double y2 = (richHeightHalf + fRichEntranceHeight / 2.) / 2.;
  double z2 = -richLenHalf - fRichAddZ / 2. + fRichCoveringThickness;

  TGeoBBox* op0             = new TGeoBBox("rich_gas_PB_op0", x1, richHeightHalf - fRichCoveringThickness,
                               richLenHalf - fRichEntranceThickness / 2. - fRichExitThickness / 2.);
  TGeoBBox* op1             = new TGeoBBox("rich_gas_PB_op1", x1, y1, z1);
  TGeoCompositeShape* op01  = MakeUnion("rich_gas_PB_op01", op0, op1, nullptr, MakeTrans(0., y2, z2));
  TGeoBBox* op2             = new TGeoBBox("rich_gas_PB_op2", x1, y1, z1);
  TGeoCompositeShape* op012 = MakeUnion("rich_gas_PB_op012", op01, op2, nullptr, MakeTrans(0., -y2, z2));

  TGeoConeSeg* op3          = new TGeoConeSeg("rich_gas_PB_op3", fPipeLength / 2., 0., fPipeOuterRadiusEntrance, 0.,
                                     fPipeOuterRadiusExit, 0., 360.);
  TGeoTranslation* tr       = MakeTrans(0., 0., -(fRichEntranceThickness - fRichExitThickness) / 2.);
  TGeoCompositeShape* gasPB = MakeSubtraction("rich_gas_PB", op012, op3, nullptr, tr);
  TGeoVolume* gas           = new TGeoVolume("rich_gas", gasPB, fMaterials.GetMedium("RICHgas_CO2_dis"));
  //if (fSetVolumeColors) gas->SetLineColor(kYellow - 10);
  return gas;
}

TGeoVolume* RichGeoCreator::CreateRichContainer()
{
  double y1                = (fRichHeight / 2. + fRichEntranceHeight / 2.) / 2.;
  double z1                = -fRichLength / 2. - fRichAddZ / 2.;
  double y2                = (fRichHeight - fRichEntranceHeight) / 4.;
  TGeoBBox* op0            = new TGeoBBox("rich_cont_PB_op0", fRichWidth / 2., fRichHeight / 2., fRichLength / 2.);
  TGeoBBox* op1            = new TGeoBBox("rich_cont_PB_op1", fRichWidth / 2., y2, fRichAddZ / 2.);
  TGeoCompositeShape* op01 = MakeUnion("rich_cont_PB_op01", op0, op1, nullptr, MakeTrans(0., y1, z1));
  TGeoBBox* op2            = new TGeoBBox("rich_cont_PB_op2", fRichWidth / 2., y2, fRichAddZ / 2.);
  TGeoCompositeShape* pb   = MakeUnion("rich_cont_PB", op01, op2, nullptr, MakeTrans(0., -y1, z1));
  //double richLenHalf       = fRichLength / 2;
  TGeoConeSeg* pipePB =
    new TGeoConeSeg("rich_pipe_subtract", fPipeLength / 2., 0, fPipeOuterRadiusEntrance - fPipeThicknessEntrance, 0,
                    fPipeOuterRadiusExit - fPipeThicknessExit, 0., 360.);
  TGeoCompositeShape* pb2 = MakeSubtraction("rich_cont_PB2", pb, pipePB, nullptr, MakeTrans(0, 0, 0));
  TGeoVolume* richCont    = new TGeoVolume("rich_cont", pb2, fMaterials.GetMedium("aluminium"));
  //if (fSetVolumeColors) richCont->SetLineColor(kBlue - 10);
  return richCont;
}

TGeoVolume* RichGeoCreator::CreateSensitivePlane()
{
  double richLenHalf = fRichLength / 2.;
  TGeoBBox* op0      = new TGeoBBox("sens_plane_PB_op0", 170., 170., 0.05);
  TGeoTube* op1       = new TGeoTube(0, (fPipeOuterRadiusExit + fPipeOuterRadiusEntrance) / 2, richLenHalf + 0.5);
  TGeoTranslation* tr = MakeTrans(0., 0., (-fSensPlaneZ + (fRichExitThickness - fRichEntranceThickness) / 2.));
  TGeoCompositeShape* sensPlanePB = MakeSubtraction("sens_plane_PB", op0, op1, nullptr, tr);
  TGeoVolume* sensPlane           = new TGeoVolume("sens_plane", sensPlanePB, fMaterials.GetMedium("RICHgas_CO2_dis+"));
  //if (fSetVolumeColors) sensPlane->SetLineColor(kRed - 8);
  return sensPlane;
}

TGeoVolume* RichGeoCreator::CreateRichEntrance()
{
  double richLenHalf   = fRichLength / 2.;
  double richFlangeHalfThickness = 5.;

  TGeoBBox* op_flange = new TGeoBBox("rich_entrance_PB_flange_op0", fRichEntranceWidth / 2. + richFlangeHalfThickness,
                                     fRichEntranceHeight / 2. + richFlangeHalfThickness, 1.);
  TGeoBBox* op1_flange =
    new TGeoBBox("rich_entrance_PB_flange_op1", fRichEntranceWidth / 2., fRichEntranceHeight / 2., 2.);
  TGeoBBox* op0          = new TGeoBBox("rich_entrance_PB_op0", fRichEntranceWidth / 2., fRichEntranceHeight / 2.,
                               fRichEntranceThickness / 2.);
  TGeoTube* op1          = new TGeoTube(0, fPipeOuterRadiusEntrance, richLenHalf * 1.01);
  TGeoTranslation* tr    = MakeTrans(0., 0., richLenHalf - fRichEntranceThickness / 2.);
  TGeoCompositeShape* pb_flange = MakeSubtraction("rich_entrance_PB_flange", op_flange, op1_flange, nullptr, nullptr);
  TGeoCompositeShape* pb = MakeSubtraction("rich_entrance_PB", op0, op1, nullptr, tr);
  TGeoVolume* entrance          = new TGeoVolume("rich_entrance", pb_flange, fMaterials.GetMedium("aluminium"));
  TGeoVolume* entrance_kapton   = new TGeoVolume("rich_entrance_kapton", pb, fMaterials.GetMedium("kapton"));
  entrance->AddNode(entrance_kapton, 1);
  //if (fSetVolumeColors) entrance->SetLineColor(kGreen - 5);
  return entrance;
}

TGeoVolume* RichGeoCreator::CreateRichExit()
{
  double richLenHalf    = fRichLength / 2.;
  double richFlangeHalfThickness = 5.;

  TGeoBBox* op_flange  = new TGeoBBox("rich_exit_PB_flange_op0", fRichExitWidth / 2. + richFlangeHalfThickness,
                                     fRichExitHeight / 2. + richFlangeHalfThickness, 1.);
  TGeoBBox* op1_flange = new TGeoBBox("rich_exit_PB_flange_op1", fRichExitWidth / 2., fRichExitHeight / 2., 2.);
  TGeoBBox* op0 = new TGeoBBox("rich_exit_PB_op0", fRichExitWidth / 2., fRichExitHeight / 2., fRichExitThickness / 2.);
  TGeoTube* op1 = new TGeoTube(0, fPipeOuterRadiusExit, richLenHalf * 1.01);
  TGeoTranslation* tr           = MakeTrans(0., 0., -richLenHalf + fRichExitThickness / 2.);
  TGeoCompositeShape* pb_flange = MakeSubtraction("rich_exit_PB_flange", op_flange, op1_flange, nullptr, nullptr);
  TGeoCompositeShape* pb = MakeSubtraction("rich_exit_PB", op0, op1, nullptr, tr);
  TGeoVolume* exit              = new TGeoVolume("rich_exit", pb_flange, fMaterials.GetMedium("aluminium"));
  TGeoVolume* exit_innerpart    = new TGeoVolume("rich_exit_innerpart", pb, fMaterials.GetMedium("Polycarbonat"));
  exit->AddNode(exit_innerpart, 1);
  //if (fSetVolumeColors) exit->SetLineColor(kGreen - 5);
  return exit;
}

TGeoVolume* RichGeoCreator::CreatePipe()  // now used for the beampipe tunnel
{
  double pipeLenHalf  = fPipeLength / 2;
  TGeoConeSeg* pipePB = new TGeoConeSeg("rich_pipe_PB", pipeLenHalf, fPipeOuterRadiusEntrance - fPipeThicknessEntrance,
                                        fPipeOuterRadiusEntrance, fPipeOuterRadiusExit - fPipeThicknessExit,
                                        fPipeOuterRadiusExit, 0., 360.);
  TGeoVolume* pipe    = new TGeoVolume("rich_pipe", pipePB, fMaterials.GetMedium("carbon"));
  //if (fSetVolumeColors) pipe->SetLineColor(kSpring + 4);
  return pipe;
}

TGeoVolume* RichGeoCreator::CreatePipeCyl()
{
  TGeoTubeSeg* pipeCylVacPB = new TGeoTubeSeg(
    "rich_cyl_pipe_vac_PB", 0., fPipeOuterRadiusEntrance - fPipeThicknessEntrance, fPipeCylPartLen / 2., 0., 360.);
  TGeoTubeSeg* pipeCylPB =
    new TGeoTubeSeg("rich_cyl_pipe_PB", 0., fPipeOuterRadiusEntrance, fPipeCylPartLen / 2., 0., 360.);
  TGeoVolume* pipeCylVac = new TGeoVolume("rich_cyl_pipe_vac", pipeCylVacPB, fMaterials.GetMedium("vacuum"));
  TGeoVolume* pipeCyl    = new TGeoVolume("rich_cyl_pipe", pipeCylPB, fMaterials.GetMedium("aluminium"));
  //if (fSetVolumeColors) pipeCylVac->SetLineColor(kSpring + 5);
  //if (fSetVolumeColors) pipeCyl->SetLineColor(kSpring + 4);
  pipeCyl->AddNode(pipeCylVac, 1);
  return pipeCyl;
}

void RichGeoCreator::CreateRich()
{
  double richLenHalf = fRichLength / 2.;

  TGeoVolume* rich_gas = CreateGas();

  TGeoVolumeAssembly* camCont = CreateCameraContainer();
  double camY                 = fCameraOriginY + fCameraShiftY;
  double camZ                 = -fRichOrigZ - richLenHalf + fCameraOriginZ + fCameraShiftZ;
  rich_gas->AddNode(camCont, 1, MakeCombiTrans(0., camY, camZ, fCameraFullRotX, 0., 0.));
  rich_gas->AddNode(camCont, 2, MakeCombiTrans(0., -camY, camZ, -fCameraFullRotX, 0., 0.));

  double mirrorZ   = -fRichOrigZ - richLenHalf + fMirrorOriginZ;
  double beltsGapZ = 0.5;  // cm

  TGeoVolumeAssembly* mirrorHalf = CreateMirror();
  rich_gas->AddNode(mirrorHalf, 1, MakeCombiTrans(0., fMirrorOriginY + fMirrorGapY / 2., mirrorZ, 0., -90., -90.));
  rich_gas->AddNode(mirrorHalf, 2, MakeCombiTrans(0., -fMirrorOriginY - fMirrorGapY / 2., mirrorZ, 0., -90., 90.));

  TGeoVolumeAssembly* belts_half_container = CreateMirrorSupportBelts();

  rich_gas->AddNode(belts_half_container, 1,
                    MakeCombiTrans(0., fMirrorOriginY + fMirrorGapY, mirrorZ + beltsGapZ, 0., -90., -90.));
  rich_gas->AddNode(belts_half_container, 2,
                    MakeCombiTrans(0., -fMirrorOriginY - fMirrorGapY, mirrorZ + beltsGapZ, 0., -90., 90.));

  TGeoVolumeAssembly* pillars = CreateMirrorSupportPillars();
  rich_gas->AddNode(pillars, 1);

  TGeoVolume* sensPlane = CreateSensitivePlane();
  rich_gas->AddNode(sensPlane, 1, MakeTrans(0., 0., fSensPlaneZ));  //-30.));

  TGeoVolumeAssembly* mainFrame = CreateMainFrame();
  rich_gas->AddNode(mainFrame, 1, MakeTrans(0., 0., 38.541939));

  if (fAddShieldingBox) {
    TGeoVolumeAssembly* shieldingBox = CreateShieldingBox();
    double shieldingBoxZ             = -(fRichEntranceThickness - fRichExitThickness / 2. + fRichOrigZ + richLenHalf);
    rich_gas->AddNode(shieldingBox, 1, MakeTrans(0., 0., shieldingBoxZ));
    rich_gas->AddNode(shieldingBox, 2, MakeCombiTrans(0., 0., shieldingBoxZ, 0., 0., 180.));
  }

  rich_gas->AddNode(CreateRichEntrance(), 1, MakeTrans(0., 0., -richLenHalf + fRichExitThickness / 2.));
  rich_gas->AddNode(CreateRichExit(), 1, MakeTrans(0., 0., richLenHalf - fRichEntranceThickness / 2.));

  TGeoVolume* richCont = CreateRichContainer();
  richCont->AddNode(rich_gas, 1, MakeTrans(0., 0., (fRichEntranceThickness - fRichExitThickness) / 2.));
  //richCont->AddNode(CreateRichEntrance(), 1, MakeTrans(0., 0., -richLenHalf + fRichEntranceThickness / 2.));
  //richCont->AddNode(CreateRichExit(), 1, MakeTrans(0., 0., richLenHalf - fRichExitThickness / 2.));
  richCont->AddNode(CreatePipe(), 1);

  TGeoVolumeAssembly* rich = new TGeoVolumeAssembly(fGeoName.c_str());
  rich->AddNode(richCont, 1, MakeTrans(0., 0., -40. + fRichOrigZ + richLenHalf));
  // This pipe segment is removed to be compatible with new beam pipe.
  // See https://git.cbm.gsi.de/CbmSoft/cbmroot_geometry/-/merge_requests/184
  //rich->AddNode(CreatePipeCyl(), 1, MakeTrans(0., 0., -40. + fRichOrigZ - fPipeCylPartLen / 2.));

  TGeoVolume* top = new TGeoVolumeAssembly(fGeoName.c_str());

  gGeoManager->SetTopVolume(top);
  top->AddNode(rich, 1);
  gGeoManager->CloseGeometry();
  gGeoManager->CheckOverlaps(0.0001);
  gGeoManager->PrintOverlaps();
  gGeoManager->Test();

  string outputFile = fGeoName + ".geo.root";
  top->Export((outputFile + "?reproducible").c_str());
  LOG(info) << "Geometry file: " << outputFile;

  //top->Draw("ogl");
  gGeoManager->SetVisLevel(7);
}

TGeoVolumeAssembly* RichGeoCreator::CreateShieldingBox()
{
  // When converting GDML to root please look https://root.cern.ch/root/html520/src/TGDMLParse.cxx.htm
  // Shielding box is created using a number of boolean operations with shapes
  // All next shape is marked with increased number in name box1 to box 15

  TGeoEltu* box1           = new TGeoEltu("box1", 183.0, 183.0, 61.3);
  TGeoBBox* box2           = new TGeoBBox("box2", 229.3 / 2., 73.879 / 2., 113.3 / 2.);
  TGeoCompositeShape* box3 = MakeIntersection("box3", box1, box2, nullptr, MakeTrans(0., 146.068, -4.65));

  TGeoBBox* box4           = new TGeoBBox("box4", 227.3 / 2., 71.937 / 2., 102.0 / 2.);
  TGeoCompositeShape* box5 = MakeSubtraction("box5", box3, box4, nullptr, MakeTrans(0., 147.097, 0.));

  TGeoEltu* box6           = new TGeoEltu("box6", 172.0, 172.0, 4.65);
  TGeoCompositeShape* box7 = MakeSubtraction("box7", box5, box6, nullptr, MakeTrans(0., 0., -56.65));

  TGeoBBox* box8           = new TGeoBBox("box8", 75.0 / 2., 222.422 / 2., 104.0 / 2.);
  TGeoCompositeShape* box9 = MakeSubtraction("box9", box7, box8, nullptr, MakeTrans(0., -0.082, 0.));

  TGeoBBox* box10 = new TGeoBBox("box10", 145.0 / 2., 10.3 / 2., 11.0 / 2.);
  TGeoCompositeShape* box11 =
    MakeSubtraction("box11", box9, box10, nullptr, MakeCombiTrans(61.1525, 177.2736, -56.15, 0., 0., -31.26911));

  TGeoBBox* box12 = new TGeoBBox("box12", 145.0 / 2., 10.3 / 2., 11.0 / 2.);
  TGeoCompositeShape* box13 =
    MakeSubtraction("box13", box11, box12, nullptr, MakeCombiTrans(-61.152, 177.274, -56.15, 0., 0., 31.27));

  TGeoBBox* box14 = new TGeoBBox("box14", 145.0 / 2., 10.3 / 2., 16.0 / 2.);
  TGeoCompositeShape* box15 =
    MakeSubtraction("box15", box13, box14, nullptr, MakeCombiTrans(0., 181.2639, -59.75, -41.3565, 0., 0.));

  TGeoVolume* shBox = new TGeoVolume("shielding_box", box15, fMaterials.GetMedium("iron"));
  //if (fSetVolumeColors) shBox->SetLineColor(kBlue + 4);

  // Wing part
  TGeoBBox* wing1 = new TGeoBBox("wing1", 117.0 / 2., 8.9571 / 2., 13.7221 / 2.);
  TGeoBBox* wing2 = new TGeoBBox("wing2", 117.0 / 2., 8.9571 / 2., 13.7221 / 2.);
  TGeoCompositeShape* wing3 =
    MakeSubtraction("wing3", wing1, wing2, nullptr, MakeCombiTrans(49.9989, 0., 3.206, 0., 18.90578, 0.));

  TGeoBBox* wing4 = new TGeoBBox("wing4", 117.0 / 2., 8.9571 / 2., 13.7221 / 2.);
  TGeoCompositeShape* wing5 =
    MakeSubtraction("wing5", wing3, wing4, nullptr, MakeCombiTrans(-49.9989, 0., 3.206, 0., -18.90578, 0.));

  TGeoVolume* wing = new TGeoVolume("shieling_wing", wing5, fMaterials.GetMedium("iron"));
  //if (fSetVolumeColors) wing->SetLineColor(kBlue - 1);

  TGeoVolumeAssembly* shBoxFinal = new TGeoVolumeAssembly("shielding_box");
  shBoxFinal->AddNode(shBox, 1, MakeCombiTrans(0., 240.7246, 79.4162, 68.06551, 0., -180.0));
  shBoxFinal->AddNode(wing, 1, MakeCombiTrans(0., 118.3074, 225.1717, -26.70901, 0., 0.));

  return shBoxFinal;
}

TGeoCompositeShape* RichGeoCreator::MakeSubtraction(const string& name, TGeoShape* left, TGeoShape* right,
                                                    TGeoMatrix* lmat, TGeoMatrix* rmat)
{
  return new TGeoCompositeShape(name.c_str(), new TGeoSubtraction(left, right, lmat, rmat));
}

TGeoCompositeShape* RichGeoCreator::MakeUnion(const string& name, TGeoShape* left, TGeoShape* right, TGeoMatrix* lmat,
                                              TGeoMatrix* rmat)
{
  return new TGeoCompositeShape(name.c_str(), new TGeoUnion(left, right, lmat, rmat));
}

TGeoCompositeShape* RichGeoCreator::MakeIntersection(const string& name, TGeoShape* left, TGeoShape* right,
                                                     TGeoMatrix* lmat, TGeoMatrix* rmat)
{
  return new TGeoCompositeShape(name.c_str(), new TGeoIntersection(left, right, lmat, rmat));
}


TGeoCombiTrans* RichGeoCreator::MakeCombiTrans(double dx, double dy, double dz, double rotX, double rotY, double rotZ)
{
  // Note that rotX, rotY, rotZ should be provided as in GDML and in deg.
  // The rotation is done with oposite sign of rot(XYZ), first - Z, second - Y, third - X
  // Creation of TGeoCombiTrans is done with rot->Inverse()
  TGeoTranslation* tr = MakeTrans(dx, dy, dz);
  TGeoRotation* rot   = new TGeoRotation();
  if (rotZ != 0.) rot->RotateZ(-rotZ);
  if (rotY != 0.) rot->RotateY(-rotY);
  if (rotX != 0.) rot->RotateX(-rotX);
  return new TGeoCombiTrans(*tr, rot->Inverse());
}

TGeoTranslation* RichGeoCreator::MakeTrans(double dx, double dy, double dz) { return new TGeoTranslation(dx, dy, dz); }

TGeoRotation* RichGeoCreator::MakeRot(double rotX, double rotY, double rotZ)
{
  TGeoRotation* rot = new TGeoRotation();
  if (rotZ != 0.) rot->RotateZ(-rotZ);
  if (rotY != 0.) rot->RotateY(-rotY);
  if (rotX != 0.) rot->RotateX(-rotX);
  return rot;
}

ClassImp(RichGeoCreator)
