# Create a library called "libL1" which includes the source files given in
# the array.
# The extension is already found.Any number of sources could be listed here.

# extra warnings to examine the code
# ADD_DEFINITIONS(-Wall -Wextra -Wshadow -Weffc++)

# L1 defines
ADD_DEFINITIONS(-DDO_TPCCATRACKER_EFF_PERFORMANCE -DNonhomogeneousField -DCBM -DUSE_TIMERS)

set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/L1Algo
  ${CMAKE_CURRENT_SOURCE_DIR}/OffLineInterface
  ${CMAKE_CURRENT_SOURCE_DIR}/qa
  ${CMAKE_CURRENT_SOURCE_DIR}/L1Algo/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/catools
  ${CMAKE_CURRENT_SOURCE_DIR}/utils

)

set(SRCS

#### Tracker interfaces (will be moved to core/detector/*) ################
  CbmTrackingDetectorInterfaceInit.cxx
###########################################################################
  CbmCaMCModule.cxx
  CbmCaTimeSliceReader.cxx
  CbmCaParametersHandler.cxx

  CbmL1.cxx
  OffLineInterface/CbmL1StsTrackFinder.cxx
  OffLineInterface/CbmL1GlobalTrackFinder.cxx
  OffLineInterface/CbmL1GlobalFindTracksEvents.cxx
  OffLineInterface/CbmGenerateMaterialMaps.cxx

  CbmL1Performance.cxx
  CbmL1MCTrack.cxx
  CbmL1Track.cxx

  L1Algo/utils/CaUvConverter.cxx
  L1Algo/utils/CaAlgoRandom.cxx

  catools/CaToolsHitRecord.cxx
  catools/CaToolsMCData.cxx
  catools/CaToolsMCPoint.cxx
  catools/CaToolsMCTrack.cxx
  catools/CaToolsDebugger.cxx
  catools/CaToolsWindowFinder.cxx
  catools/CaToolsWFExpression.cxx

  qa/CbmCaInputQaBase.cxx
  qa/CbmCaInputQaMvd.cxx
  qa/CbmCaInputQaSts.cxx
  qa/CbmCaInputQaMuch.cxx
  qa/CbmCaInputQaTrd.cxx
  qa/CbmCaInputQaTof.cxx
  qa/CbmCaInputQaSetup.cxx
  qa/CbmCaOutputQa.cxx
  qa/CbmCaTrackTypeQa.cxx
  qa/CbmCaTrackFitQa.cxx
  qa/CbmTofInteraction.cxx # Tests

  utils/CbmCaIdealHitProducer.cxx
)

set(NO_DICT_SRCS
)

set(HEADERS
  CbmL1Constants.h
  CbmL1DetectorID.h
  CbmL1MCPoint.h
  CbmL1Hit.h
  CbmL1Track.h
  CbmL1Vtx.h
  CbmCaTimeSliceReader.h
  CbmCaParametersHandler.h
  L1Algo/utils/CaUvConverter.h
  catools/CaToolsWindowFinder.h
  catools/CaToolsLinkKey.h
  catools/CaToolsHitRecord.h
  catools/CaToolsDef.h
  utils/CbmCaIdealHitProducer.h
  catools/CaToolsField.h
  qa/CbmCaInputQaBase.h
  qa/CbmCaHitQaData.h
)

If(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wno-parentheses) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.

  # -- - Check for compiler flags
  CHECK_CXX_COMPILER_FLAG("-Werror -Wno-pmf-conversions" HAS_PMF)

  If(HAS_PMF)
    ADD_DEFINITIONS(-Wno-pmf-conversions)
  EndIf()

  CHECK_CXX_COMPILER_FLAG("-Werror -Wstrict-null-sentinel" HAS_SENTINEL)

  If(HAS_SENTINEL)
    ADD_DEFINITIONS(-Wstrict-null-sentinel)
  EndIf()

  CHECK_CXX_COMPILER_FLAG("-Werror -Wno-non-template-friend" HAS_TEMPLATE_FRIEND)

  If(HAS_TEMPLATE_FRIEND)
    ADD_DEFINITIONS(-Wno-non-template-friend)
  EndIf()
Else()
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wno-pmf-conversions -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wstrict-null-sentinel -Wno-non-template-friend -Wno-parentheses) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.
EndIf()

Option(L1_NO_OPT "Compile L1 without optimization" OFF)

IF(SSE_FOUND)
  if(L1_NO_OPT)
    SET_SOURCE_FILES_PROPERTIES(${SRCS} PROPERTIES COMPILE_FLAGS
      "-msse")
    Message(STATUS "L1 will be compiled with SSE support but without optimization")
  else()
    SET_SOURCE_FILES_PROPERTIES(${SRCS} PROPERTIES COMPILE_FLAGS
      "-msse -O3")
    Message(STATUS "L1 will be compiled with SSE support and optimization")
  endif()
ELSE(SSE_FOUND)
  if(L1_NO_OPT)
    Message(STATUS "L1 will be compiled without SSE support and optimization")
  else()
    SET_SOURCE_FILES_PROPERTIES(${SRCS} PROPERTIES COMPILE_FLAGS
      "-O3")
    Message(STATUS "L1 will be compiled without SSE support but with optimization")
  endif()
ENDIF(SSE_FOUND)

set(LIBRARY_NAME L1)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h)
set(PUBLIC_DEPENDENCIES
  CbmMuchBase
  CbmMvdBase
  CbmStsBase
  CbmTofBase
  CbmTrdBase
  CaCoreOffline
  KfCbm
  CbmBase
  CbmData
  CbmQaBase
  FairLogger::FairLogger
  FairRoot::Base
  ROOT::Core
  ROOT::Gpad
  ROOT::Graf
  ROOT::Hist
  ROOT::Physics
  fmt::fmt
)

set(PRIVATE_DEPENDENCIES
  CbmRecoSts  # <==== !!!! was bringing dependency on the "online" version of the Algo library which brought the NO_ROOT flag !!!!!
  CbmSimSteer
  CbmRecoBase
  KFParticle
  external::yaml-cpp
  FairRoot::GeoBase
  FairRoot::ParBase
  ROOT::EG
  ROOT::Geom
  ROOT::Graf3d
  ROOT::MathCore
  ROOT::Matrix
  ROOT::Minuit
  ROOT::RIO
)

generate_cbm_library()

add_dependencies(G__L1 KFPARTICLE)

install(FILES CbmL1Counters.h
  utils/CbmCaIdealHitProducer.h
  utils/CbmCaIdealHitProducerDet.h
  L1Algo/utils/CaUvConverter.h
  qa/CbmCaInputQaBase.h
  DESTINATION include
)

