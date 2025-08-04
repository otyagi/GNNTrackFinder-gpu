/* Copyright (C) 2018-2023 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer], Eoin Clerkin */

#include "Rtypes.h"

class TGeoMatrix;
class TGeoVolume;
class TGeoManager;
class TGeoMedium;
class FairModule;
class TString;

namespace Cbm
{
  namespace GeometryUtils
  {
    void UniqueId();
    void PrintMedia();
    void PrintMaterials();
    void ReAssignMediaId();
    void RemoveDuplicateMaterials();
    void RemoveDuplicateMedia();

    void ImportRootGeometry(TString& filename, FairModule* mod, TGeoMatrix* mat = nullptr);

    bool IsNewGeometryFile(TString& filename);
    bool IsNewGeometryFile(TString& filename, TString& volumeName, TGeoMatrix** matrix);

    void AssignMediumAtImport(TGeoVolume* v);
    void ExpandNodes(TGeoVolume* volume, FairModule* mod);

    /// @brief  Convert the local X/Y covariance matrix to global coordinates
    /// @param m      the transformation matrix
    /// @param covXX covariance X,X
    /// @param covXY covariance X,Y
    /// @param covYY covariance Y,Y
    void LocalToMasterCovarianceMatrix(const TGeoMatrix& m, Double_t& covXX, Double_t& covXY, Double_t& covYY);

    /* Populate a GeoManager with the full media and materials defined to return */
    TGeoManager* pop_TGeoManager(const char* name);

    /* Open root file geometry and add it to parent */
    bool add_binary(const char rootFile[], TGeoVolume* top, TGeoMedium* med, Int_t inum, TGeoMatrix* mat);

    TGeoMatrix* cad_matrix(double XX, double XY, double XZ, double YX, double YY, double YZ, double ZX, double ZY,
                           double ZZ, double TX, double TY, double TZ);

  }  // namespace GeometryUtils
}  // namespace Cbm
