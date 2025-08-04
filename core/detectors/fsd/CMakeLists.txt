set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

set(SRCS
  CbmFsdContFact.cxx
  CbmFsdGeoHandler.cxx
  CbmFsdDigiPar.cxx
  )

set(LIBRARY_NAME CbmFsdBase)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES 
  ROOT::Core
  FairRoot::ParBase
  )

set(PRIVATE_DEPENDENCIES  
  CbmData
  FairLogger::FairLogger
  ROOT::Geom
  ${VMCLIB}
  )

# Install file which has no corresponding source file
set(HEADERS
  CbmFsdDetectorSpecs.h
  )

generate_cbm_library()
