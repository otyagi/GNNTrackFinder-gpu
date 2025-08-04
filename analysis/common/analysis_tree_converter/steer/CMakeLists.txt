# CMakeList file for library libCbmAtcOfflineSteer
# F. Linz,     27 October 2023

set(SRCS
  Config.cxx
  Run.cxx
  TaskFactory.cxx
  )

set(INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBRARY_NAME CbmAtConverterSteer)
set(LINKDEF RootLinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmData
  FairRoot::Base
  ROOT::Core
  external::yaml-cpp
  CbmAnalysisTreeInterface
  )

set(PRIVATE_DEPENDENCIES
  CbmBase
  CbmTrdBase
  CbmTrdReco
  FairLogger::FairLogger
  FairRoot::Online
  ROOT::Hist
  ROOT::RHTTP
  CbmSimSteer
  L1
  KF
  AlgoOffline
  )

generate_cbm_library()
