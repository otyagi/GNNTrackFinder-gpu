/* Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
   SPDX-License-Identifier: GPL-3.0-only
   Authors: Volker Friese [committer] */

// ===== LinkDef for reco/task =====

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// --- Classes
#pragma link C++ class CbmReco + ;
#pragma link C++ class CbmSourceTs + ;
#pragma link C++ class CbmTaskBuildEvents + ;
#pragma link C++ class CbmTaskDigiEventQa + ;
#pragma link C++ class CbmTaskEventsCloneInToOut + ;
#pragma link C++ class CbmTaskInspectDigiEvents + ;
#pragma link C++ class CbmTaskInspectDigiTimeslice + ;
#pragma link C++ class CbmTaskMakeRecoEvents + ;
#pragma link C++ class CbmTaskTofHitFinder + ;
#pragma link C++ class CbmTaskTofClusterizer + ;
#pragma link C++ class CbmTaskTofClusterizerParWrite + ;
#pragma link C++ class CbmTaskTrdHitFinderParWrite + ;
#pragma link C++ class CbmTaskTrdHitFinder + ;
#pragma link C++ class CbmTaskTriggerDigi + ;
#pragma link C++ class CbmTaskUnpack + ;
//#pragma link C++ class cbm::algo::MainConfig + ;


#endif /* __CINT__ */
