/* Copyright (C) 2006-2020 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Florian Uhlig, Denis Bertini [committer], Semen Lebedev */

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class CbmRichTrainAnnSelect;
#pragma link C++ class CbmRichTrainAnnElectrons;

#pragma link C++ class CbmRichHitProducer + ;
#pragma link C++ class CbmRichReconstruction + ;

//mcbm
#pragma link C++ class CbmRichMCbmHitProducer;
#pragma link C++ class CbmRichMCbmQa;
#pragma link C++ class CbmRichMCbmQaReal;
#pragma link C++ class CbmRichMCbmQaRichOnly;
#pragma link C++ class CbmRichMCbmAerogelAna;
#pragma link C++ class CbmRichMCbmToTShifter;
#pragma link C++ class CbmRichMCbmSEDisplay;
#ifdef HAVE_ONNXRUNTIME
#pragma link C++ class CbmRichMCbmDenoiseCnn + ;
#endif
#pragma link C++ class CbmRichMCbmDenoiseQa + ;

//qa
#pragma link C++ class CbmRichGeoTest + ;
#pragma link C++ class CbmRichGeoTestOpt + ;
#pragma link C++ class CbmRichUrqmdTest + ;
#pragma link C++ class CbmRichGeoOpt;
#pragma link C++ class CbmRichRingFitterQa + ;
#pragma link C++ class CbmRichRecoQa + ;
#pragma link C++ class CbmRichRecoTbQa + ;
#pragma link C++ class CbmRichDigiQa + ;

//unpack
#pragma link C++ class CbmRichUnpackAlgo + ;
#pragma link C++ class CbmRichUnpackAlgo2022 + ;
#pragma link C++ class CbmRichUnpackAlgoBase + ;
#pragma link C++ class CbmRichUnpackMonitor + ;
#pragma link C++ class CbmRichUnpackConfig + ;


#endif
