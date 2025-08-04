/* Copyright (C) 2022 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Eoin Clerkin, Volker Friese [committer] */

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;


#pragma link C++ class CbmBmon + ;
#pragma link C++ class CbmGeoBmon + ;

#pragma link C++ class CbmDigitize < CbmBmonDigi> + ;
#pragma link C++ class CbmBmonDigitize + ;

#endif
