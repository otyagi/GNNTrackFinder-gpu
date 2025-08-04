set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

set(SRCS
  CbmFsdMC.cxx
  CbmFsdDigitize.cxx
  )

set(LIBRARY_NAME CbmFsdSim)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmBase
  CbmData
  FairRoot::Base
  ROOT::Core
  ROOT::Physics
  )

set(PRIVATE_DEPENDENCIES
  CbmSimBase
  CbmFsdBase
  FairRoot::ParBase
  FairLogger::FairLogger
  ROOT::Geom
  ROOT::MathCore
  ${VMCLIB}
  )

generate_cbm_library()
