set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  )


set(SRCS
  CbmRecoQa.cxx
  CbmTrackingTrdQa.cxx
  CbmRecoQaTask.cxx
  #CbmRecoModuleQa.cxx
  )

set(LIBRARY_NAME CbmRecoQa)
set(LINKDEF  RecoQaLinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmSimSteer
  CbmBase
  CbmData
  FairRoot::Base
  ROOT::Hist
  ROOT::Core
  ROOT::RIO
  ROOT::Physics
  )

set(PRIVATE_DEPENDENCIES
  CbmTrdBase
  CbmQaBase
  ROOT::EG
  ROOT::Geom
  KF
  )

generate_cbm_library()
