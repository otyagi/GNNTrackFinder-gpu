# Create a library called "" which includes the source files given in
# the array .
# The extension is already found.  Any number of sources could be listed here.
Set(INCLUDE_DIRECTORIES
${BASE_INCLUDE_DIRECTORIES}
${CMAKE_SOURCE_FAIRMCSTACK}
${KF_HEADERS}
${HAL_CBM}/helpers
${HAL_CBM}/format
${HAL_CBM}/cuts
${HAL_CBM}/cuts/pair
${HAL_CBM}/cuts/track
${Hal_INCLUDE_DIR}
${HAL_CBM_FORMAT}
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
    pair/HalCbmBasicFemtoPairCut.cxx
    pair/HalCbmDeltaPhiDeltaThetaCut.cxx
    pair/HalCbmStsSepCut.cxx
    pair/HalCbmPairSeparationCut2D.cxx
    pair/HalCbmPairCut.cxx
    pair/HalCbmDeltaPhiDeltaThetaStarMinCut.cxx
    
    track/HalCbmBasicTrackCuts.cxx
    track/HalCbmNHitsCut.cxx
    track/HalCbmTofCut.cxx
    track/HalCbmTofM2PolyCut.cxx
)

Set(HEADERS )
Set(LINKDEF HalCbmCutsLinkDef.h)
Set(LIBRARY_NAME HalCbmCuts)
Set(PUBLIC_DEPENDENCIES
    HalFeatures
    HalData
    HalCut
    CbmData
    HalCbmFormat
)

generate_cbm_library()

add_dependencies(G__HalCbmCuts ANALYSISTREE HAL)
