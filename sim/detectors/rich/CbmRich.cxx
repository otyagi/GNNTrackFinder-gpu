/* Copyright (C) 2006-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: claudia Hoehne [committer], Andrey Lebedev, Florian Uhlig, Semen Lebedev, Martin Beyer */

#include "CbmRich.h"

#include "CbmGeometryUtils.h"
#include "CbmRichDigiMapManager.h"
#include "CbmRichPoint.h"
#include "CbmStack.h"

#include <FairGeoInterface.h>
#include <FairGeoLoader.h>
#include <FairRootManager.h>
#include <FairRun.h>
#include <FairVolume.h>

// #include <FairGeoBuilder.h>
#include <FairGeoMedia.h>

#include <TClonesArray.h>
#include <TFile.h>
#include <TGDMLParse.h>
#include <TGeoManager.h>
#include <TGeoMatrix.h>
#include <TGeoMedium.h>
#include <TGeoNode.h>
#include <TLorentzVector.h>
#include <TObjArray.h>
#include <TParticle.h>
#include <TParticlePDG.h>
#include <TVirtualMC.h>

std::map<TString, TGeoMedium*> CbmRich::fFixedMedia;

CbmRich::CbmRich()
  : FairDetector("RICH", kTRUE, ToIntegralType(ECbmModuleId::kRich))
  , fPosIndex(0)
  , fRegisterPhotonsOnSensitivePlane(true)
  , fRichPoints(new TClonesArray("CbmRichPoint"))
  , fRichRefPlanePoints(new TClonesArray("CbmRichPoint"))
  , fRichMirrorPoints(new TClonesArray("CbmRichPoint"))
  , fRotation(nullptr)
  , fPositionRotation(nullptr)
{
  fVerboseLevel = 1;
}

CbmRich::CbmRich(const char* name, Bool_t active, Double_t px, Double_t py, Double_t pz, Double_t rx, Double_t ry,
                 Double_t rz)
  : FairDetector(name, active, ToIntegralType(ECbmModuleId::kRich))
  , fPosIndex(0)
  , fRegisterPhotonsOnSensitivePlane(true)
  , fRichPoints(new TClonesArray("CbmRichPoint"))
  , fRichRefPlanePoints(new TClonesArray("CbmRichPoint"))
  , fRichMirrorPoints(new TClonesArray("CbmRichPoint"))
  , fRotation(new TGeoRotation("", rx, ry, rz))
  , fPositionRotation(new TGeoCombiTrans(px, py, pz, fRotation))
{
  fVerboseLevel = 1;
}

CbmRich::~CbmRich()
{
  if (fRichPoints) {
    fRichPoints->Delete();
    delete fRichPoints;
  }
  if (fRichRefPlanePoints) {
    fRichRefPlanePoints->Delete();
    delete fRichRefPlanePoints;
  }

  if (fRichMirrorPoints) {
    fRichMirrorPoints->Delete();
    delete fRichMirrorPoints;
  }
}

void CbmRich::Initialize()
{
  FairDetector::Initialize();
  CbmRichDigiMapManager::GetInstance();
}

Bool_t CbmRich::IsSensitive(const std::string& name)
{
  //return true;
  TString volName = name;
  if (volName.Contains("pmt_pixel") || volName.Contains("sens_plane")) return kTRUE;
  // mirrors
  if (volName.Contains("mirror_tile_type")) return kTRUE;
  return kFALSE;
}

Bool_t CbmRich::CheckIfSensitive(std::string name) { return IsSensitive(name); }


Bool_t CbmRich::ProcessHits(FairVolume* vol)
{
  Int_t pdgCode   = gMC->TrackPid();
  Int_t iVol      = vol->getMCid();
  TString volName = TString(vol->GetName());

  // Treat MAPMT pixels
  if (volName.Contains("pmt_pixel")) {
    if (gMC->IsTrackEntering()) {
      TParticle* part = gMC->GetStack()->GetCurrentTrack();
      Double_t charge = part->GetPDG()->Charge() / 3.;
      Int_t trackID   = gMC->GetStack()->GetCurrentTrackNumber();
      Double_t time   = gMC->TrackTime() * 1.0e09;
      Double_t length = gMC->TrackLength();
      Double_t eLoss  = gMC->Edep();

      // For pmt pixels use unique pixel address as detId instead of volume id
      const std::string path = std::string(gMC->CurrentVolPath());
      Int_t pixelAddress     = CbmRichDigiMapManager::GetInstance().GetPixelAddressByPath(path);
      if (pixelAddress == -1) {
        LOG(error) << "CbmRich::ProcessHits() Pixel address is not found for path " << path;
        return kFALSE;
      }

      TLorentzVector tPos, tMom;
      gMC->TrackPosition(tPos);
      gMC->TrackMomentum(tMom);

      if (pdgCode == 50000050) {  // Cherenkovs only
        AddHit(trackID, pixelAddress, TVector3(tPos.X(), tPos.Y(), tPos.Z()), TVector3(tMom.Px(), tMom.Py(), tMom.Pz()),
               time, length, eLoss);

        // Increment number of RichPoints for this track
        CbmStack* stack = static_cast<CbmStack*>(gMC->GetStack());
        stack->AddPoint(ECbmModuleId::kRich);
        return kTRUE;
      }
      else {
        if (charge == 0.) {
          return kFALSE;  // No neutrals
        }
        else {  // Charged particles
          AddHit(trackID, pixelAddress, TVector3(tPos.X(), tPos.Y(), tPos.Z()),
                 TVector3(tMom.Px(), tMom.Py(), tMom.Pz()), time, length, eLoss);

          // Increment number of RichPoints for this track
          CbmStack* stack = static_cast<CbmStack*>(gMC->GetStack());
          stack->AddPoint(ECbmModuleId::kRich);
          return kTRUE;
        }
      }
    }
  }

  // Treat imaginary plane in front of the mirrors
  // Two modes based on fRegisterPhotonsOnSensitivePlane:
  // 1. [true] register only cherenkov photons
  //    Use case: check ingoing cherenkov photons into the mirror and outgoing cherenkov photons from the mirror
  // 2. [false] register all charged particles
  //    Use case: compare track extrapolation with MC track
  if (volName.Contains("sens_plane")) {
    if (gMC->IsTrackEntering()) {
      TParticle* part = gMC->GetStack()->GetCurrentTrack();
      Double_t charge = part->GetPDG()->Charge() / 3.;

      if ((fRegisterPhotonsOnSensitivePlane && pdgCode == 50000050)
          || (!fRegisterPhotonsOnSensitivePlane && charge != 0.)) {
        Int_t trackID   = gMC->GetStack()->GetCurrentTrackNumber();
        Double_t time   = gMC->TrackTime() * 1.0e09;
        Double_t length = gMC->TrackLength();
        Double_t eLoss  = gMC->Edep();

        TLorentzVector tPos, tMom;
        gMC->TrackPosition(tPos);
        gMC->TrackMomentum(tMom);

        AddRefPlaneHit(trackID, iVol, TVector3(tPos.X(), tPos.Y(), tPos.Z()), TVector3(tMom.Px(), tMom.Py(), tMom.Pz()),
                       time, length, eLoss);

        // Increment number of RefPlanePoints for this track
        CbmStack* stack = static_cast<CbmStack*>(gMC->GetStack());
        stack->AddPoint(ECbmModuleId::kRef);
        return kTRUE;
      }
      else {
        return kFALSE;
      }
    }
  }

  // Treat mirror
  if (volName.Contains("mirror_tile_type")) {
    if (gMC->IsTrackEntering()) {
      TParticle* part = gMC->GetStack()->GetCurrentTrack();
      Double_t charge = part->GetPDG()->Charge() / 3.;
      if (charge != 0.) {
        Int_t trackID   = gMC->GetStack()->GetCurrentTrackNumber();
        Double_t time   = gMC->TrackTime() * 1.0e09;
        Double_t length = gMC->TrackLength();
        Double_t eLoss  = gMC->Edep();

        TLorentzVector tPos, tMom;
        gMC->TrackPosition(tPos);
        gMC->TrackMomentum(tMom);

        AddMirrorHit(trackID, iVol, TVector3(tPos.X(), tPos.Y(), tPos.Z()), TVector3(tMom.Px(), tMom.Py(), tMom.Pz()),
                     time, length, eLoss);
        return kTRUE;
      }
    }
  }

  return kFALSE;
}

void CbmRich::EndOfEvent()
{
  if (fVerboseLevel) Print("");
  Reset();
}

void CbmRich::Register()
{
  FairRootManager::Instance()->Register("RichPoint", "Rich", fRichPoints, kTRUE);
  FairRootManager::Instance()->Register("RefPlanePoint", "RichRefPlane", fRichRefPlanePoints, kTRUE);
  FairRootManager::Instance()->Register("RichMirrorPoint", "RichMirror", fRichMirrorPoints, kFALSE);
}

TClonesArray* CbmRich::GetCollection(Int_t iColl) const
{
  if (iColl == 0) return fRichPoints;
  if (iColl == 1) return fRichRefPlanePoints;
  if (iColl == 2) return fRichMirrorPoints;
  return nullptr;
}

void CbmRich::Print(Option_t*) const
{
  Int_t nHits = fRichPoints->GetEntriesFast();
  LOG(info) << fName << ": " << nHits << " points registered in this event.";

  if (fVerboseLevel > 1)
    for (Int_t i = 0; i < nHits; i++)
      (*fRichPoints)[i]->Print();
}

void CbmRich::Reset()
{
  fRichPoints->Delete();
  fRichRefPlanePoints->Delete();
  fRichMirrorPoints->Delete();
  fPosIndex = 0;
}

void CbmRich::CopyClones(TClonesArray* cl1, TClonesArray* cl2, Int_t offset)
{
  Int_t nEntries = cl1->GetEntriesFast();
  LOG(info) << "CbmRich: " << nEntries << " entries to add.";
  TClonesArray& clref = *cl2;
  CbmRichPoint* oldpoint {nullptr};
  for (Int_t i = 0; i < nEntries; i++) {
    oldpoint    = static_cast<CbmRichPoint*>(cl1->At(i));
    Int_t index = oldpoint->GetTrackID() + offset;
    oldpoint->SetTrackID(index);
    new (clref[fPosIndex]) CbmRichPoint(*oldpoint);
    fPosIndex++;
  }
  LOG(info) << "CbmRich: " << cl2->GetEntriesFast() << " merged entries.";
}

void CbmRich::ConstructOpGeometry()
{
  LOG(info) << "CbmRich::ConstructOpGeometry()";
  SetRichGlassPropertiesForGeant4();
}

void CbmRich::ConstructGeometry()
{
  TString fileName = GetGeometryFileName();
  if (fileName.EndsWith(".root")) {
    if (Cbm::GeometryUtils::IsNewGeometryFile(fgeoName)) {
      LOG(info) << "Importing RICH geometry from ROOT file " << fgeoName.Data();
      TGeoCombiTrans* fCombiTrans {};  //! Transformation matrix for geometry positioning
      Cbm::GeometryUtils::ImportRootGeometry(fgeoName, this, fCombiTrans);
    }
    else {
      LOG(info) << "Constructing RICH geometry from ROOT file " << fgeoName.Data();
      FairModule::ConstructRootGeometry();
    }
  }
  else if (fileName.EndsWith(".gdml")) {
    LOG(info) << "Constructing RICH geometry from GDML  file " << fileName.Data();
    ConstructGdmlGeometry(fPositionRotation);
  }
  else {
    LOG(fatal) << "Geometry format of RICH file " << fileName.Data()
               << " not supported. Only ROOT and GDML formats are supported.";
  }
}

void CbmRich::SetRichGlassPropertiesForGeant4()
{
  FairRun* run    = FairRun::Instance();
  Bool_t isGeant4 = std::string(run->GetName()) == "TGeant4";
  LOG(info) << "CbmRich::SetRichGlassPropertiesForGeant4() Transport engine:" << std::string(run->GetName());

  // for GEANT3 simulation no need to implement reflectivity
  if (!isGeant4) {
    LOG(info) << "CbmRich::SetRichGlassPropertiesForGeant4() fIsGeant4 is "
                 "false. No need to set RICH glass properties. Return.";
    return;
  }
  else {
    LOG(info) << "CbmRich::SetRichGlassPropertiesForGeant4() fIsGeant4 is "
                 "true. RICH glass properties will be set.";
  }

  std::vector<Double_t> energyAr = {
    1.5499, 1.5695, 1.5897, 1.6103, 1.6315, 1.6533, 1.6756, 1.6985, 1.7221, 1.7464, 1.7713, 1.7970, 1.8234,
    1.8507, 1.8787, 1.9076, 1.9374, 1.9682, 1.9999, 2.0327, 2.0666, 2.1016, 2.1378, 2.1753, 2.2142, 2.2544,
    2.2962, 2.3395, 2.3845, 2.4313, 2.4799, 2.5305, 2.5832, 2.6382, 2.6955, 2.7554, 2.8180, 2.8836, 2.9522,
    3.0242, 3.0998, 3.1793, 3.2630, 3.3512, 3.4443, 3.5427, 3.6469, 3.7574, 3.8748, 3.9998, 4.1331, 4.2757,
    4.4284, 4.5924, 4.7690, 4.9598, 5.1664, 5.3910, 5.6361, 5.9045, 6.1997};
  // Transform to GeV
  for (Double_t& energy : energyAr) {
    energy *= 1.e-9;
  }

  std::vector<Double_t> reflectivityAr = {
    0.15719, 0.15315, 0.15218, 0.14953, 0.14761, 0.14416, 0.13753, 0.13844, 0.13705, 0.13307, 0.13136, 0.12569, 0.12284,
    0.12089, 0.13101, 0.12494, 0.12199, 0.12150, 0.12257, 0.11783, 0.11909, 0.12021, 0.12080, 0.11712, 0.11935, 0.12015,
    0.11385, 0.11364, 0.11526, 0.11908, 0.12109, 0.12011, 0.12193, 0.12681, 0.12622, 0.12601, 0.12824, 0.13084, 0.13182,
    0.13287, 0.13463, 0.13513, 0.13665, 0.13826, 0.13960, 0.14072, 0.14205, 0.13977, 0.14090, 0.14073, 0.14031, 0.13951,
    0.13868, 0.13622, 0.13762, 0.13898, 0.13963, 0.14713, 0.15901, 0.18620, 0.22529};

  for (Double_t& reflectivity : reflectivityAr) {
    reflectivity = 1. - reflectivity;
  }

  gMC->DefineOpSurface("RICHglassSurf", kGlisur, kDielectric_metal, kPolished, 0.0);
  gMC->SetMaterialProperty("RICHglassSurf", "REFLECTIVITY", static_cast<Int_t>(energyAr.size()), energyAr.data(),
                           reflectivityAr.data());

  for (int i = 0; i < 10; i++) {
    if (gGeoManager->FindVolumeFast(("mirror_tile_type" + std::to_string(i)).c_str()) == nullptr) continue;
    gMC->SetSkinSurface(("RICHglassSkin" + std::to_string(i)).c_str(), ("mirror_tile_type" + std::to_string(i)).c_str(),
                        "RICHglassSurf");
  }
}

void CbmRich::ConstructGdmlGeometry(TGeoMatrix* geoMatrix)
{
  TFile* old = gFile;
  TGDMLParse parser;
  TGeoVolume* gdmlTop {nullptr};

  // Before importing GDML
  Int_t maxInd = gGeoManager->GetListOfMedia()->GetEntries() - 1;

  gdmlTop = parser.GDMLReadFile(GetGeometryFileName());

  // Cheating - reassigning media indices after GDML import (need to fix this in TGDMLParse class!!!)
  //   for (Int_t i=0; i<gGeoManager->GetListOfMedia()->GetEntries(); i++)
  //      gGeoManager->GetListOfMedia()->At(i)->Dump();
  // After importing GDML
  Int_t j = gGeoManager->GetListOfMedia()->GetEntries() - 1;
  Int_t curId {};
  TGeoMedium* m {nullptr};
  do {
    m     = static_cast<TGeoMedium*>(gGeoManager->GetListOfMedia()->At(j));
    curId = m->GetId();
    m->SetId(curId + maxInd);
    j--;
  } while (curId > 1);
  //   LOG(debug) << "====================================================================";
  //   for (Int_t i=0; i<gGeoManager->GetListOfMedia()->GetEntries(); i++)
  //      gGeoManager->GetListOfMedia()->At(i)->Dump();

  Int_t newMaxInd = gGeoManager->GetListOfMedia()->GetEntries() - 1;

  gGeoManager->GetTopVolume()->AddNode(gdmlTop, 1, geoMatrix);
  ExpandNodeForGdml(gGeoManager->GetTopVolume()->GetNode(gGeoManager->GetTopVolume()->GetNdaughters() - 1));

  for (Int_t k = maxInd + 1; k < newMaxInd + 1; k++) {
    TGeoMedium* medToDel = static_cast<TGeoMedium*>(gGeoManager->GetListOfMedia()->At(maxInd + 1));
    LOG(debug) << "    removing media " << medToDel->GetName() << " with id " << medToDel->GetId() << " (k=" << k
               << ")";
    gGeoManager->GetListOfMedia()->Remove(medToDel);
  }
  gGeoManager->SetAllIndex();

  gFile = old;
}

void CbmRich::ExpandNodeForGdml(TGeoNode* node)
{
  LOG(debug) << "----------------------------------------- ExpandNodeForGdml for node " << node->GetName();

  TGeoVolume* curVol = node->GetVolume();

  LOG(debug) << "    volume: " << curVol->GetName();

  if (curVol->IsAssembly()) { LOG(debug) << "    skipping volume-assembly"; }
  else {

    TGeoMedium* curMed                    = curVol->GetMedium();
    TGeoMaterial* curMat                  = curVol->GetMaterial();
    TGeoMedium* curMedInGeoManager        = gGeoManager->GetMedium(curMed->GetName());
    TGeoMaterial* curMatOfMedInGeoManager = curMedInGeoManager->GetMaterial();
    TGeoMaterial* curMatInGeoManager      = gGeoManager->GetMaterial(curMat->GetName());

    // Current medium and material assigned to the volume from GDML
    LOG(debug2) << "    curMed\t\t\t\t" << curMed << "\t" << curMed->GetName() << "\t" << curMed->GetId();
    LOG(debug2) << "    curMat\t\t\t\t" << curMat << "\t" << curMat->GetName() << "\t" << curMat->GetIndex();

    // Medium and material found in the gGeoManager - either the pre-loaded one or one from GDML
    LOG(debug2) << "    curMedInGeoManager\t\t" << curMedInGeoManager << "\t" << curMedInGeoManager->GetName() << "\t"
                << curMedInGeoManager->GetId();
    LOG(debug2) << "    curMatOfMedInGeoManager\t\t" << curMatOfMedInGeoManager << "\t"
                << curMatOfMedInGeoManager->GetName() << "\t" << curMatOfMedInGeoManager->GetIndex();
    LOG(debug2) << "    curMatInGeoManager\t\t" << curMatInGeoManager << "\t" << curMatInGeoManager->GetName() << "\t"
                << curMatInGeoManager->GetIndex();

    TString matName = curMat->GetName();
    TString medName = curMed->GetName();

    if (curMed->GetId() != curMedInGeoManager->GetId()) {
      if (fFixedMedia.find(medName) == fFixedMedia.end()) {
        LOG(debug) << "    Medium needs to be fixed";
        fFixedMedia[medName] = curMedInGeoManager;
        Int_t ind            = curMat->GetIndex();
        gGeoManager->RemoveMaterial(ind);
        LOG(debug) << "    removing material " << curMat->GetName() << " with index " << ind;
        for (Int_t i = ind; i < gGeoManager->GetListOfMaterials()->GetEntries(); i++) {
          TGeoMaterial* m = static_cast<TGeoMaterial*>(gGeoManager->GetListOfMaterials()->At(i));
          m->SetIndex(m->GetIndex() - 1);
        }

        LOG(debug) << "    Medium fixed";
      }
      else {
        LOG(debug) << "    Already fixed medium found in the list    ";
      }
    }
    else {
      if (fFixedMedia.find(medName) == fFixedMedia.end()) {
        LOG(debug) << "    There is no correct medium in the memory yet";

        FairGeoLoader* geoLoad     = FairGeoLoader::Instance();
        FairGeoInterface* geoFace  = geoLoad->getGeoInterface();
        FairGeoMedia* geoMediaBase = geoFace->getMedia();
        // FairGeoBuilder* geobuild = geoLoad->getGeoBuilder();

        FairGeoMedium* curMedInGeo = geoMediaBase->getMedium(medName);
        if (curMedInGeo == nullptr) {
          LOG(fatal) << medName << " Media not found in Geo file.";
          //! This should not happen.
          //! This means that somebody uses material in GDML that is not in the media.geo file.
          //! Most probably this is the sign to the user to check materials' names in the CATIA model.
        }
        else {
          LOG(debug) << "    Found media in Geo file" << medName;
          // Int_t nmed = geobuild->createMedium(curMedInGeo);
          fFixedMedia[medName] = static_cast<TGeoMedium*>(gGeoManager->GetListOfMedia()->Last());
          gGeoManager->RemoveMaterial(curMatOfMedInGeoManager->GetIndex());
          LOG(debug) << "    removing material " << curMatOfMedInGeoManager->GetName() << " with index "
                     << curMatOfMedInGeoManager->GetIndex();
          for (Int_t i = curMatOfMedInGeoManager->GetIndex(); i < gGeoManager->GetListOfMaterials()->GetEntries();
               i++) {
            TGeoMaterial* m = static_cast<TGeoMaterial*>(gGeoManager->GetListOfMaterials()->At(i));
            m->SetIndex(m->GetIndex() - 1);
          }
        }

        if (curMedInGeo->getSensitivityFlag()) {
          LOG(debug) << "    Adding sensitive  " << curVol->GetName();
          AddSensitiveVolume(curVol);
        }
      }
      else {
        LOG(debug) << "    Already fixed medium found in the list";
        LOG(debug) << "!!! Sensitivity: " << fFixedMedia[medName]->GetParam(0);
        if (fFixedMedia[medName]->GetParam(0) == 1) {
          LOG(debug) << "    Adding sensitive  " << curVol->GetName();
          AddSensitiveVolume(curVol);
        }
      }
    }

    curVol->SetMedium(fFixedMedia[medName]);
    gGeoManager->SetAllIndex();

    // gGeoManager->GetListOfMaterials()->Print();
    // gGeoManager->GetListOfMedia()->Print();
  }

  //! Recursevly go down the tree of nodes
  if (curVol->GetNdaughters() != 0) {
    TObjArray* NodeChildList = curVol->GetNodes();
    TGeoNode* curNodeChild {nullptr};
    for (Int_t j = 0; j < NodeChildList->GetEntriesFast(); j++) {
      curNodeChild = static_cast<TGeoNode*>(NodeChildList->At(j));
      ExpandNodeForGdml(curNodeChild);
    }
  }
}

CbmRichPoint* CbmRich::AddHit(Int_t trackID, Int_t detID, TVector3 pos, TVector3 mom, Double_t time, Double_t length,
                              Double_t eLoss)
{
  TClonesArray& clref = *fRichPoints;
  Int_t size          = clref.GetEntriesFast();
  return new (clref[size]) CbmRichPoint(trackID, detID, pos, mom, time, length, eLoss);
}

CbmRichPoint* CbmRich::AddRefPlaneHit(Int_t trackID, Int_t detID, TVector3 pos, TVector3 mom, Double_t time,
                                      Double_t length, Double_t eLoss)
{
  TClonesArray& clref = *fRichRefPlanePoints;
  Int_t tsize         = clref.GetEntriesFast();
  return new (clref[tsize]) CbmRichPoint(trackID, detID, pos, mom, time, length, eLoss);
}

CbmRichPoint* CbmRich::AddMirrorHit(Int_t trackID, Int_t detID, TVector3 pos, TVector3 mom, Double_t time,
                                    Double_t length, Double_t eLoss)
{
  TClonesArray& clref = *fRichMirrorPoints;
  Int_t tsize         = clref.GetEntriesFast();
  return new (clref[tsize]) CbmRichPoint(trackID, detID, pos, mom, time, length, eLoss);
}

ClassImp(CbmRich)
