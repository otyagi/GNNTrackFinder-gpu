# Create a library called "" which includes the source files given in
# the array .
# The extension is already found.  Any number of sources could be listed here.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
Set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_FAIRMCSTACK}
  ${Hal_INCLUDE_DIR}
  ${KF_HEADERS}
  ${CBMDATA_DIR}
  ${CBMDATA_DIR}/global
  ${CBMDATA_DIR}/sts
  ${CMAKE_SOURCE_DIR}/core/field
  ${HAL_CBM}/helpers
  ${HAL_CBM}/helpers/anatree
  ${AnalysisTree_INCLUDE_DIR}
)
Set(SYSTEM_INCLUDE_DIRECTORIES
  ${ROOT_INCLUDE_DIR} 
  ${BASE_INCLUDE_DIRECTORIES}
  ${CLHEP_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
)


Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})
 
Link_Directories(${LINK_DIRECTORIES})

# List of source files
set(SRCS
    HalCbmDetectorID.cxx
    HalCbmTaskManager.cxx
    HalCbmCompressionTask.cxx
    HalCbmAnalysisManager.cxx
    CbmHelix.cxx
    HalCbmField.cxx
    anatree/CbmAnaTreeContainer.cxx
    anatree/CbmAnaTreeSource.cxx
    anatree/HalCbmSource.cxx
)

Set(LINKDEF HalCbmHelpersLinkDef.h)
Set(LIBRARY_NAME HalCbmHelpers)
set(PUBLIC_DEPENDENCIES
  FairRoot::Base
  CbmField
  FairRoot::Base
  HalCut
  HalFair
  HalFeatures
  HalData
  KF
  KFParticle
  AnalysisTreeBase
  )
  
generate_cbm_library()

add_dependencies(G__HalCbmHelpers ANALYSISTREE HAL)


