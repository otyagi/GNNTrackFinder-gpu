/* Copyright (C) 2006-2010 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Ivan Kisel, Sergey Gorbunov, Denis Bertini [committer], Igor Kulakov */

/*
 *====================================================================
 *
 *  CBM Level 1 Reconstruction 
 *  
 *  Authors: I.Kisel,  S.Gorbunov
 *
 *  e-mail : ikisel@kip.uni-heidelberg.de 
 *
 *====================================================================
 *
 *  L1 vertex class
 *
 *====================================================================
 */

#ifndef CbmL1Vtx_H
#define CbmL1Vtx_H


struct CbmL1Vtx {
  CbmL1Vtx()
    : MC_mass(0)
    , MC_q(0)
    , MC_p(0)
    , MC_x(0)
    , MC_y(0)
    , MC_z(0)
    , MC_px(0)
    , MC_py(0)
    , MC_pz(0)
    , MC_ID(0)
    , MC_pdg(0)
    , x(0)
    , y(0)
    , z(0)
    , chi2(0)
    , NDF(0)
    , mass(0)
    , mass_err(0){};

  double MC_mass, MC_q, MC_p, MC_x, MC_y, MC_z, MC_px, MC_py, MC_pz;
  int MC_ID, MC_pdg;

  double x, y, z, C[6], chi2;
  int NDF;

  double mass, mass_err;

  double& GetRefX() { return x; }
  double& GetRefY() { return y; }
  double& GetRefZ() { return z; }
  double* GetCovMatrix() { return C; }
  double& GetRefChi2() { return chi2; }
  int& GetRefNDF() { return NDF; }
  double& GetRefMass() { return mass; }
  double& GetRefMassError() { return mass_err; }
};

#endif
