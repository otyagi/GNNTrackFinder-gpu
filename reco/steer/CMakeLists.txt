# CMakeList file for library CbmRecoSteer
# V. Friese,     2 June 2021

set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  )


set(SRCS
  CbmRecoUnpack.cxx
  CbmSourceDigiTimeslice.cxx
  CbmSourceDigiEvents.cxx
  CbmSourceTsArchive.cxx
  CbmOnlineParWrite.cxx
  )


set(LIBRARY_NAME CbmRecoSteer)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmData
  CbmSimSteer
  CbmRecoTasks
  FairRoot::Base
  ROOT::Core
  ROOT::Hist
  )

set(PRIVATE_DEPENDENCIES
  CbmTofReco
  CbmMuchReco
  CbmPsdReco
  CbmPsdBase
  CbmRichReco
  CbmRecoSts
  CbmTrdReco
  CbmBmonReco
  external::fles_monitoring
  external::fles_logging # Needed for TimesliceSource in CbmSourceTsArchive, not sure of conflicts with FairLogger
  )

set(INTERFACE_DEPENDENCIES
  CbmRecoBase
  FairRoot::ParBase
  external::fles_ipc
  )


generate_cbm_library()
