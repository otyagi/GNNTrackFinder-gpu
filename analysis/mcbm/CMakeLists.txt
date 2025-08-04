set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

set(SRCS
  CbmMcbm2024CheckBmonScvd.cxx
  CbmMcbm2024CheckEventsDt.cxx
  )


set(LIBRARY_NAME CbmMcbmAna)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmBase
  CbmData
  FairRoot::Base
  ROOT::Core
  )

set(PRIVATE_DEPENDENCIES
  FairRoot::ParBase
  ROOT::Geom
  ROOT::Hist
  ROOT::RIO
  )

generate_cbm_library()
