add_subdirectory(tools)

set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/CbmKfTarget.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/CbmKfUtil.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/CbmKfTrackingSetupBuilder.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/CbmKfTrackFitter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/CbmKfFitTracksTask.cxx
)

SET_SOURCE_FILES_PROPERTIES(${SRCS} PROPERTIES COMPILE_FLAGS "-O3")


If(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wno-parentheses) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.
Else()
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wno-pmf-conversions -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wstrict-null-sentinel -Wno-non-template-friend -Wno-parentheses -Wmissing-field-initializers) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.
EndIf()

add_library(KfCbm SHARED ${SRCS})

#list(APPEND HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/utils/KfSimd.h)

target_include_directories(KfCbm
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

#target_compile_definitions(KfCore PUBLIC NO_ROOT)

target_link_libraries(KfCbm
              PUBLIC  CbmMuchBase
                      CbmMvdBase
                      CbmStsBase
                      CbmTofBase
                      CbmTrdBase
                      CbmBase
                      #CbmQaBase
                      #FairRoot::Base
                      FairLogger::FairLogger
                      FairRoot::Base
                      ROOT::Core
                      ROOT::Gpad
                      ROOT::Graf
                      ROOT::Physics
                      fmt::fmt
                      KfTools
              PRIVATE CbmRecoSts
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

install(TARGETS KfCbm DESTINATION lib)
#install(DIRECTORY kf TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
#install(DIRECTORY kf/utils TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
#install(DIRECTORY kf/data TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
#install(DIRECTORY kf/geo TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
#install(DIRECTORY kf/algo TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
#install(DIRECTORY kf/pars TYPE INCLUDE FILES_MATCHING PATTERN "*.h")

install(
  FILES
    CbmKfOriginalField.h
    CbmKfTrackingSetupBuilder.h
    CbmKfUtil.h
    CbmKfTarget.h
  DESTINATION
    include/
)
