/* Copyright (C) 2019-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese, Florian Uhlig [committer] */

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// --- transport/base
#pragma link C++ class CbmBeamGenerator + ;
#pragma link C++ class CbmPhsdGenerator + ;
#pragma link C++ class CbmPlutoGenerator + ;
#pragma link C++ class CbmShieldGenerator + ;
#pragma link C++ class CbmUnigenGenerator + ;

#pragma link C++ class URun + ;
#pragma link C++ class UEvent + ;
#pragma link C++ class UParticle + ;

#pragma link C++ class PDataBase;
#pragma link C++ class PMesh;
#pragma link C++ class PParticle;
#pragma link C++ class PStaticData;
#pragma link C++ class PStdData;
#pragma link C++ class PValues;


#endif /* __CINT__ */
