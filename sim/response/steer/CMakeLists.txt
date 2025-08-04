# CMakeList file for library libCbmSimDigitizationSteer
# F. Linz,     09 January 2024

set(SRCS
  Config.cxx
  Run.cxx
  )

set(INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})

set(LIBRARY_NAME CbmSimDigitizationSteer)
set(LINKDEF RootLinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmData
  FairRoot::Base
  ROOT::Core
  external::yaml-cpp
  )

set(PRIVATE_DEPENDENCIES
  CbmBase
  FairLogger::FairLogger
  FairRoot::Online
  ROOT::Hist
  ROOT::RHTTP
  CbmSimSteer
  CbmSimResponse
  AlgoOffline
  )

generate_cbm_library()
