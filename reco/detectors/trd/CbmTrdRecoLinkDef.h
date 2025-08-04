/* Copyright (C) 2020 Institut fuer Kernphysik, Goethe-Universitaet Frankfurt, Frankfurt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Pascal Raisig [committer], Alexandru Bercuci */

// $Id: TrdRecoLinkDef.h $

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class CbmTrdTrackFinderIdeal + ;
#pragma link C++ class CbmTrdClusterFinder + ;
#pragma link C++ class CbmTrdHitProducer + ;
#pragma link C++ class CbmTrdModuleRec + ;
#pragma link C++ class CbmTrdModuleRecR + ;
#pragma link C++ class CbmTrdModuleRec2D + ;
#pragma link C++ class CbmTrdDigiRec + ;

#pragma link C++ class CbmTrdClusterizerFastQa + ;
#pragma link C++ class CbmTrdHitDensityQa + ;
#pragma link C++ class CbmTrdHitProducerClusterQa + ;
#pragma link C++ class CbmTrdHitProducerQa + ;
#pragma link C++ class CbmTrdOccupancyQa + ;
#pragma link C++ class CbmTrdQa + ;
#pragma link C++ class CbmTrdRecoQa + ;
#pragma link C++ class CbmTrdTracksPidQa + ;
#pragma link C++ class CbmTrdCalibTracker + ;
#pragma link C++ class CbmTrdHitMC + ;

#pragma link C++ class std::vector < std::pair < unsigned long, unsigned long>> + ;
#pragma link C++ class CbmTrdUnpackAlgoBaseR + ;
#pragma link C++ class CbmTrdUnpackAlgoR + ;
#pragma link C++ class CbmTrdUnpackAlgoLegacy2020R + ;
#pragma link C++ class CbmTrdUnpackConfig + ;
#pragma link C++ class CbmTrdUnpackMonitor + ;

#pragma link C++ class CbmTrdUnpackFaspAlgo + ;
#pragma link C++ class CbmTrdUnpackFaspConfig + ;
#pragma link C++ class CbmTrdUnpackFaspMonitor + ;

#pragma link C++ class CbmTrdElectronsTrainAnn + ;
#pragma link C++ class CbmTrdSetTracksPidWkn + ;
#pragma link C++ class CbmTrdSetTracksPidModWkn + ;
#pragma link C++ class CbmTrdSetTracksPidLike + ;
#pragma link C++ class CbmTrdSetTracksPidANN + ;

#pragma link C++ class CbmTrdRawToDigiBaseR + ;
#pragma link C++ class CbmTrdRawToDigiMaxAdcR + ;
#pragma link C++ class CbmTrdRawToDigiFitR + ;
#pragma link C++ class CbmTrdRawToDigiLookUpCorrR + ;

#endif
