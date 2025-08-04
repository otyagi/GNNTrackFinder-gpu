/* Copyright (C) 2018-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig [committer] */

// $Id: TrdSimLinkDef.h $

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class CbmTrd + ;
#pragma link C++ class CbmDigitize < CbmTrdDigi> + ;
#pragma link C++ class CbmTrdDigitizer + ;
#pragma link C++ class CbmTrdModuleSim + ;
#pragma link C++ class CbmTrdModuleSimR + ;
#pragma link C++ class CbmTrdModuleSim2D + ;
#pragma link C++ class CbmTrdTrianglePRF + ;
#pragma link C++ class CbmTrdRawToDigiR + ;
#pragma link C++ class CbmTrdCheckUtil + ;

#pragma link C++ class CbmTrdDigitizerPRFQa + ;
#pragma link C++ class CbmTrdMCQa + ;
#pragma link C++ class CbmTrdHitRateFastQa + ;
#pragma link C++ class CbmTrdHitRateQa + ;
#endif
