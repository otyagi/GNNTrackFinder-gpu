/* Copyright (C) 2018-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Eoin Clerkin */

#include "CbmGeometryUtils.h"

#include <FairGeoBuilder.h>    // for FairGeoBuilder
#include <FairGeoInterface.h>  // for FairGeoInterface
#include <FairGeoLoader.h>     // for FairGeoLoader
#include <FairGeoMedia.h>      // for FairGeoMedia
#include <FairModule.h>        // for FairModule
#include <Logger.h>            // for LOG, Logger, Severity, Severity::debug

#include <RtypesCore.h>    // for Bool_t, Int_t, kTRUE, kFALSE
#include <TCollection.h>   // for TIter
#include <TDirectory.h>    // for gDirectory, TDirectory (ptr only)
#include <TFile.h>         // for TFile, gFile
#include <TGeoManager.h>   // for TGeoManager, gGeoManager
#include <TGeoMaterial.h>  // for TGeoMaterial
#include <TGeoMatrix.h>    // for TGeoMatrix
#include <TGeoMedium.h>    // for TGeoMedium
#include <TGeoNode.h>      // for TGeoNode
#include <TGeoVolume.h>    // for TGeoVolume
#include <TKey.h>          // for TKey
#include <TList.h>         // for TList
#include <TObjArray.h>     // for TObjArray
#include <TObject.h>       // for TObject
#include <TString.h>       // for TString, operator<, operator<<
#include <TSystem.h>       // for gSystem

#include <climits>
#include <map>  // for map

#include <string.h>  // for strcmp

namespace Cbm
{
  namespace GeometryUtils
  {
    void PrintMedia()
    {
      TList* media = gGeoManager->GetListOfMedia();
      TIter next1(media);
      TGeoMedium* med;
      while ((med = static_cast<TGeoMedium*>(next1()))) {
        LOG(info) << "Medium " << med->GetName() << " with ID " << med->GetId();
      }
      LOG(info) << "****";
    }

    void PrintMaterials()
    {
      TList* material = gGeoManager->GetListOfMaterials();
      TIter next1(material);
      TGeoMaterial* mat;
      while ((mat = static_cast<TGeoMaterial*>(next1()))) {
        LOG(info) << "Material " << mat->GetName() << " with ID " << mat->GetIndex();
      }
      LOG(info) << "****";
    }


    void UniqueId()
    {
      /* Uniqueness is not strictly true but should be in practice.
         The names are converted to Int_t with 2 billion positives followed by same negative
	 after overflow. If each ascii character were uniquely converted guarantee uniqueness to the
         first 4 characters (256^4). I account for all characters in the name, allow overflow,
         uppercase and lowercase treated same. I treat all special characters the same.

	 Very unlikely names like "Rich_C02_gas+" would have same id as "rich-CO3-Gas-" which
	 could throw up errors in future.
      */

      TList* media = gGeoManager->GetListOfMedia();

      TGeoMedium* med;
      TGeoMaterial* mat;

      Int_t base = 27;  // 26 alphabet plus 1 for all special characters
      Int_t j    = 0;
      while (j < media->GetSize()) {  // cycle through med but mat name sets id

        med = (TGeoMedium*) media->At(j);
        mat = med->GetMaterial();

        Int_t UniqueID = 0;  // INT_MAX 2147483647 overflow to negative ID not so undesireable.

        for (size_t i = 0; i < strlen(mat->GetName()); ++i) {
          if (mat->GetName()[i] >= 'A' && mat->GetName()[i] <= 'Z') {
            UniqueID +=
              ((Int_t) pow((double) base, (double) i) % INT_MAX) * ((Int_t) mat->GetName()[i] - 'B');  // So A=1
          }
          else if (med->GetMaterial()->GetName()[i] >= 'a' && mat->GetName()[i] <= 'z') {
            UniqueID +=
              ((Int_t) pow((double) base, (double) i) % INT_MAX) * ((Int_t) mat->GetName()[i] - 'b');  // so a=1
          }
          else {
            UniqueID +=
              ((Int_t) pow((double) base, (double) i + 1) % INT_MAX) * ((Int_t) mat->GetName()[i]);  // i+1 equiv +base
          };
        }

        med->SetId((UniqueID > 0) ? UniqueID : -1 * UniqueID);
        mat->SetIndex((UniqueID > 0) ? UniqueID : -1 * UniqueID);  // ? negative number caused issue for mat but not med

        LOG(debug) << "Reset ID number of Medium " << med->GetName() << " to " << med->GetId();
        LOG(debug) << "Reset ID number of Material " << mat->GetName() << " to " << mat->GetIndex();

        j++;
      }
    }


    void RemoveDuplicateMaterials()
    {
      // Revove duplicate materials
      TList* materials = gGeoManager->GetListOfMaterials();
      TIter next(materials);
      // map for existing materials
      std::map<TString, Bool_t> mapMatName;
      TGeoMaterial* mat;
      while ((mat = static_cast<TGeoMaterial*>(next()))) {
        // If material exist - delete dublicated. If not - set the flag
        if (mapMatName[mat->GetName()]) {
          LOG(debug) << "Removing duplicate material " << mat->GetName();
          materials->Remove(mat);
        }
        else {
          mapMatName[mat->GetName()] = kTRUE;
        }
      }
    }

    void RemoveDuplicateMedia()
    {
      // Revove duplicate media
      TList* media = gGeoManager->GetListOfMedia();
      TIter next(media);
      // map for existing materials
      std::map<TString, Bool_t> mapMedName;
      TGeoMedium* med;
      while ((med = static_cast<TGeoMedium*>(next()))) {
        // If medium exist - delete duplicated. If not - set the flag
        if (mapMedName[med->GetName()]) {
          LOG(debug) << "Removing duplicate medium " << med->GetName();
          media->Remove(med);
        }
        else {
          mapMedName[med->GetName()] = kTRUE;
        }
      }
    }

    void ReAssignMediaId()
    {
      // Initialise pointer to GeoBuilder
      FairGeoBuilder* geoBuilder = FairGeoLoader::Instance()->getGeoBuilder();
      // Get list of TGeo media
      TList* media = gGeoManager->GetListOfMedia();

      gGeoManager->GetListOfMedia()->Print();

      // Loop over new media which are not in GeoBase and shift the ID
      TGeoMedium* med;
      for (Int_t i = geoBuilder->GetNMedia(); i < media->GetEntries(); i++) {
        med = static_cast<TGeoMedium*>(media->At(i));
        med->SetId(i + 1);
      }
      // Change GeoBase medium index
      geoBuilder->SetNMedia(media->GetEntries());

      Cbm::GeometryUtils::RemoveDuplicateMaterials();
      Cbm::GeometryUtils::RemoveDuplicateMedia();

      media = gGeoManager->GetListOfMedia();
      TIter next3(media);
      while ((med = static_cast<TGeoMedium*>(next3()))) {
        TGeoMaterial* mat = med->GetMaterial();
        if (mat) {
          //          mat->Print();
        }
        else {
          LOG(info) << "No Material found for medium " << med->GetName();
        }
      }
      gGeoManager->SetAllIndex();
    }

    Bool_t IsNewGeometryFile(TString& filename)
    {
      TString tempString {""};
      TGeoMatrix* tempMatrix {nullptr};
      return IsNewGeometryFile(filename, tempString, &tempMatrix);
    }

    void ImportRootGeometry(TString& filename, FairModule* mod, TGeoMatrix* mat)
    {

      TString fVolumeName {""};
      TGeoMatrix* tempMatrix {nullptr};

      IsNewGeometryFile(filename, fVolumeName, &tempMatrix);

      TGeoVolume* module1 = TGeoVolume::Import(filename, fVolumeName.Data());

      if (fair::Logger::Logging(fair::Severity::debug)) {
        LOG(debug) << "Information about imported volume:";
        module1->Print();
        LOG(debug);
        LOG(debug) << "Information about imported transformation matrix:";
        tempMatrix->Print();
        if (mat) {
          LOG(debug) << "There is a transformation matrix passed "
                     << "from the module class which overwrites "
                     << "the imported matrix.";
          LOG(debug);
          LOG(debug) << "Information about passed transformation matrix:";
          mat->Print();
        }
      }


      Cbm::GeometryUtils::RemoveDuplicateMaterials();
      Cbm::GeometryUtils::RemoveDuplicateMedia();
      Cbm::GeometryUtils::UniqueId();


      if (mat) { gGeoManager->GetTopVolume()->AddNode(module1, 0, mat); }
      else {
        gGeoManager->GetTopVolume()->AddNode(module1, 0, tempMatrix);
      }

      Cbm::GeometryUtils::ExpandNodes(module1, mod);
      gGeoManager->SetAllIndex();
    }

    Bool_t IsNewGeometryFile(TString& filename, TString& volumeName, TGeoMatrix** matrix)
    {
      // Save current gFile and gDirectory information
      TFile* oldFile           = gFile;
      TDirectory* oldDirectory = gDirectory;

      TFile* f      = new TFile(filename);
      TList* l      = f->GetListOfKeys();
      Int_t numKeys = l->GetSize();

      if (2 != numKeys) {
        LOG(debug) << "Not exactly two keys in the file. File is not of new type.";
        return kFALSE;
      }

      TKey* key;
      TIter next(l);

      Bool_t foundGeoVolume = kFALSE;
      Bool_t foundGeoMatrix = kFALSE;

      while ((key = (TKey*) next())) {
        if (key->ReadObj()->InheritsFrom("TGeoVolume")) {
          volumeName     = key->GetName();
          foundGeoVolume = kTRUE;
          LOG(debug) << "Found TGeoVolume with name" << volumeName;
          continue;
        }
        if (key->ReadObj()->InheritsFrom("TGeoMatrix")) {
          *matrix        = dynamic_cast<TGeoMatrix*>(key->ReadObj());
          foundGeoMatrix = kTRUE;
          LOG(debug) << "Found TGeoMatrix derrived object.";
          continue;
        }
      }

      // Restore previous gFile and gDirectory information
      f->Close();
      delete f;
      gFile      = oldFile;
      gDirectory = oldDirectory;

      if (foundGeoVolume && foundGeoMatrix) {
        LOG(debug) << "Geometry file is of new type.";
        return kTRUE;
      }
      else {
        if (!foundGeoVolume) { LOG(fatal) << "No TGeoVolume found in geometry file. File is of unknown type."; }
        if (!foundGeoMatrix) {
          LOG(fatal) << "No TGeoMatrix derived object found in geometry file. "
                        "File is of unknown type.";
        }
        return kFALSE;
      }
    }


    void AssignMediumAtImport(TGeoVolume* v)
    {
      /**
       * Assign medium to the the volume v, this has to be done in all cases:
       * case 1: For CAD converted volumes they have no mediums (only names)
       * case 2: TGeoVolumes, we need to be sure that the material is 
       *         defined in this session
       */
      FairGeoMedia* Media      = FairGeoLoader::Instance()->getGeoInterface()->getMedia();
      FairGeoBuilder* geobuild = FairGeoLoader::Instance()->getGeoBuilder();

      TGeoMedium* med1 = v->GetMedium();

      if (med1) {
        // In newer ROOT version also a TGeoVolumeAssembly has a material and medium.
        // This medium is called dummy and is automatically set when the geometry is constructed.
        // Since this material and medium is neither in the TGeoManager (at this point) nor in our
        // ASCII file we have to create it the same way it is done in TGeoVolume::CreateDummyMedium()
        // In the end the new medium and material has to be added to the TGeomanager, because this is
        // not done automatically when using the default constructor. For all other constructors the
        // newly created medium or material is added to the TGeomanger.
        // Create the medium and material only the first time.
        TString medName = static_cast<TString>(med1->GetName());
        if ((medName.EqualTo("dummy")) && (nullptr == gGeoManager->GetMedium(medName))) {
          TGeoMaterial* dummyMaterial = new TGeoMaterial();
          dummyMaterial->SetName("dummy");

          TGeoMedium* dummyMedium = new TGeoMedium();
          dummyMedium->SetName("dummy");
          dummyMedium->SetMaterial(dummyMaterial);

          gGeoManager->GetListOfMedia()->Add(dummyMedium);
          gGeoManager->AddMaterial(dummyMaterial);
        }

        TGeoMaterial* mat1   = v->GetMaterial();
        TGeoMaterial* newMat = gGeoManager->GetMaterial(mat1->GetName());
        if (nullptr == newMat) {
          /** The Material is not yet defined in the TGeoManager, 
           *  we try to create one if we have enough information about it
           */
          LOG(info) << "Create new material " << mat1->GetName();
          FairGeoMedium* FairMedium = Media->getMedium(mat1->GetName());
          if (!FairMedium) {
            LOG(fatal) << "Material " << mat1->GetName() << "is neither defined in ASCII file nor in Root file.";
          }
          else {
            Int_t nmed = geobuild->createMedium(FairMedium);
            v->SetMedium(gGeoManager->GetMedium(nmed));
            gGeoManager->SetAllIndex();
          }
        }
        else {
          /**Material is already available in the TGeoManager and we can set it */
          TGeoMedium* med2 = gGeoManager->GetMedium(mat1->GetName());
          v->SetMedium(med2);
        }
      }
      else {
        if (strcmp(v->ClassName(), "TGeoVolumeAssembly") != 0) {
          LOG(fatal) << "The volume " << v->GetName()
                     << "has no medium information and is not an Assembly so "
                        "we have to quit";
        }
      }
    }

    void ExpandNodes(TGeoVolume* vol, FairModule* mod)
    {

      Cbm::GeometryUtils::AssignMediumAtImport(vol);
      TObjArray* NodeList = vol->GetNodes();
      for (Int_t Nod = 0; Nod < NodeList->GetEntriesFast(); Nod++) {
        TGeoNode* fNode = (TGeoNode*) NodeList->At(Nod);

        TGeoVolume* v = fNode->GetVolume();
        if (fNode->GetNdaughters() > 0) { Cbm::GeometryUtils::ExpandNodes(v, mod); }
        Cbm::GeometryUtils::AssignMediumAtImport(v);

        if ((mod->InheritsFrom("FairDetector")) && mod->IsSensitive(v->GetName())) {
          LOG(debug) << "Module " << v->GetName() << " of detector " << mod->GetName() << " is sensitive";
          mod->AddSensitiveVolume(v);
        }
      }
    }


    void LocalToMasterCovarianceMatrix(const TGeoMatrix& m, Double_t& covXX, Double_t& covXY, Double_t& covYY)
    {
      // Fast calculation with skipping zeros

      // 3x3 covariance matrix. c01==c11, other elements are 0.
      double c00 = covXX;
      double c10 = covXY;
      double c11 = covYY;

      // 3x3 transformation matrix. other elements will not be needed
      double r00 = m.GetRotationMatrix()[0];
      double r01 = m.GetRotationMatrix()[1];
      double r10 = m.GetRotationMatrix()[3];
      double r11 = m.GetRotationMatrix()[4];

      // rc == r * c
      double rc00 = r00 * c00 + r01 * c10;
      double rc01 = r00 * c10 + r01 * c11;
      double rc10 = r10 * c00 + r11 * c10;
      double rc11 = r10 * c10 + r11 * c11;

      // transformed c = rc * r^t
      covXX = rc00 * r00 + rc01 * r01;
      covXY = rc10 * r00 + rc11 * r01;
      covYY = rc10 * r10 + rc11 * r11;
    }


    /* Populate a GeoManager with the full media and materials defined to return */
    TGeoManager* pop_TGeoManager(const char* name)
    {

      // Use the FairRoot geometry interface to load the media which are already defined
      FairGeoLoader* geoLoad    = new FairGeoLoader("TGeo", "FairGeoLoader");
      FairGeoInterface* geoFace = geoLoad->getGeoInterface();
      TString geoPath           = gSystem->Getenv("VMCWORKDIR");
      TString geoFile           = geoPath + "/geometry/media.geo";
      geoFace->setMediaFile(geoFile);
      geoFace->readMedia();

      // Read the required media and create them in the GeoManager
      FairGeoMedia* geoMedia   = geoFace->getMedia();
      FairGeoBuilder* geoBuild = geoLoad->getGeoBuilder();

      int num            = geoMedia->getListOfMedia()->GetSize();
      FairGeoMedium* med = new FairGeoMedium();

      for (int i = 0; i < num; i++) {
        med = geoMedia->getMedium(geoMedia->getListOfMedia()->At(i)->GetName());
        geoBuild->createMedium(med);
      };

      gGeoManager->SetTitle(name);

      return gGeoManager;
    }

    /* Open root file geometry and add it to parent */
    bool add_binary(const char rootFile[], TGeoVolume* top, TGeoMedium* med, Int_t inum, TGeoMatrix* mat)
    {

      TFile* file = TFile::Open(rootFile, "OPEN");
      if (file == NULL) {
        LOG(info) << "Error file " << rootFile << " not opened";
        return false;
      }
      else {
        ((TGeoVolume*) file->Get(file->GetListOfKeys()->First()->GetName()))->SetMedium(med);
        top->AddNode((TGeoVolume*) (file->Get(file->GetListOfKeys()->First()->GetName()))->Clone(), inum, mat);
      };

      (top->GetNode(top->GetNodes()->Last()->GetName()))->GetVolume()->SetMedium(med);
      file->Close();
      return true;
    }

    /* Take in the positional matrix and return it in TGeo format */
    TGeoMatrix* cad_matrix(double XX, double XY, double XZ, double YX, double YY, double YZ, double ZX, double ZY,
                           double ZZ, double TX, double TY, double TZ)
    {

      TGeoHMatrix* hmat              = new TGeoHMatrix();
      (hmat->GetRotationMatrix())[0] = XX;
      (hmat->GetRotationMatrix())[3] = XY;
      (hmat->GetRotationMatrix())[6] = XZ;

      (hmat->GetRotationMatrix())[1] = YX;
      (hmat->GetRotationMatrix())[4] = YY;
      (hmat->GetRotationMatrix())[7] = YZ;

      (hmat->GetRotationMatrix())[2] = ZX;
      (hmat->GetRotationMatrix())[5] = ZY;
      (hmat->GetRotationMatrix())[8] = ZZ;

      TGeoRotation* rot = new TGeoRotation();
      rot->SetRotation(*hmat);

      TGeoCombiTrans* mat = new TGeoCombiTrans(TX / 1, TY / 1, TZ / 1, rot);
      return mat;
    };


  }  // namespace GeometryUtils
}  // namespace Cbm
