# Create a library called "" which includes the source files given in
# the array .
# The extension is already found.  Any number of sources could be listed here.
Set(INCLUDE_DIRECTORIES
	${BASE_INCLUDE_DIRECTORIES}
	${CMAKE_SOURCE_FAIRMCSTACK}
	${KF_HEADERS}
	${CBMROOT_SOURCE_DIR}/sim/transport/generators/unigen
	${CBMROOT_SOURCE_DIR}/core/data
	${HAL_CBM}/helpers
	${HAL_CBM}/helpers/anatree
	${HAL_CBM_FORMAT}
	${AnalysisTree_INCLUDE_DIR}
    ${Hal_INCLUDE_DIR}
)
Set(SYSTEM_INCLUDE_DIRECTORIES
 ${ROOT_INCLUDE_DIR} 
 ${CLHEP_INCLUDE_DIR}
 ${Boost_INCLUDE_DIRS}
)

Include_Directories(${INCLUDE_DIRECTORIES})
Include_Directories(SYSTEM ${SYSTEM_INCLUDE_DIRECTORIES})

# List of source files
set(SRCS


HalCbmFormatTypes.cxx

combined/HalCbmFullEvent.cxx
combined/HalCbmHbtFullEvent.cxx

unigen/CbmUnigenSource.cxx
unigen/HalCbmUnigenEvent.cxx
unigen/HalCbmUnigenEventInterface.cxx
unigen/HalCbmUnigenTrack.cxx
unigen/HalCbmUnigenTrackInterface.cxx

anatree/mc/HalCbmMCEvent.cxx
anatree/mc/HalCbmMCEventInterface.cxx
anatree/mc/HalCbmMCTrack.cxx
anatree/mc/HalCbmMCTrackInterface.cxx
anatree/reco/HalCbmEvent.cxx
anatree/reco/HalCbmEventInterface.cxx
anatree/reco/HalCbmTrack.cxx
anatree/reco/HalCbmTrackInterface.cxx
anatree/femto/HalCbmHbtTrack.cxx
anatree/femto/HalCbmHbtEvent.cxx

)

Set(HEADERS )
Set(LINKDEF HalCbmFormatLinkDef.h)
Set(LIBRARY_NAME HalCbmFormat)

Set(PUBLIC_DEPENDENCIES
    ${_AnalysisTree_LIB}
    CbmData
    HalFeatures
    HalData
    HalCbmHelpers
    CbmSimGenerators
    ROOT::Tree
    )

generate_cbm_library()

add_dependencies(G__HalCbmFormat ANALYSISTREE HAL)
