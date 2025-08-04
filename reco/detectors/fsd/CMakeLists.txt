set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/unpack
  )

set(SRCS
  CbmFsdHitProducer.cxx
)


set(LIBRARY_NAME CbmFsdReco)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmBase
  CbmData
  CbmRecoBase
  FairRoot::Base
  ROOT::Core
  )

set(PRIVATE_DEPENDENCIES
  CbmFsdBase
  FairLogger::FairLogger
  ROOT::Physics
  )

set(INTERFACE_DEPENDENCIES
  external::fles_ipc
  )

generate_cbm_library()
