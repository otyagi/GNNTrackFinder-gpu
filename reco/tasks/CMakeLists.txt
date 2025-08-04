# CMakeList file for library libCbmRecoTasks
# V. Friese,     2 June 2021

set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CBMROOT_SOURCE_DIR}
  )

set(SRCS
  CbmReco.cxx
  CbmSourceTs.cxx
  CbmTaskBuildEvents.cxx
  CbmTaskDigiEventQa.cxx
  CbmTaskEventsCloneInToOut.cxx
  CbmTaskInspectDigiEvents.cxx
  CbmTaskInspectDigiTimeslice.cxx
  CbmTaskMakeRecoEvents.cxx
  CbmTaskTriggerDigi.cxx
  CbmTaskTofHitFinder.cxx
  CbmTaskTofClusterizer.cxx
  CbmTaskTofClusterizerParWrite.cxx
  CbmTaskTrdHitFinder.cxx
  CbmTaskTrdHitFinderParWrite.cxx
  CbmTaskTrdUnpackParWrite.cxx
  CbmTaskStsHitFinderParWrite.cxx
  CbmTaskUnpack.cxx
)


if(OpenMP_CXX_FOUND)
  list(APPEND DEPENDENCIES OpenMP::OpenMP_CXX)
endif()


set(LIBRARY_NAME CbmRecoTasks)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmData
  CbmStsBase
  CbmTofBase
  CbmRecoSts
  FairRoot::Base
  AlgoOffline
  ROOT::Core
  )

set(PRIVATE_DEPENDENCIES
  CbmBase
  CbmTrdBase
  FairLogger::FairLogger
  FairRoot::Online
  external::fles_monitoring
  external::fles_logging
  external::yaml-cpp
  ROOT::Hist
  ROOT::RHTTP
  )

set(INTERFACE_DEPENDENCIES
  external::fles_ipc
  )

generate_cbm_library()
