set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/algo
  ${CMAKE_CURRENT_SOURCE_DIR}/data
  ${CMAKE_CURRENT_SOURCE_DIR}/geo
  ${CMAKE_CURRENT_SOURCE_DIR}/pars
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

set(SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/KfFramework.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/KfTrackKalmanFilter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/KfTrackParam.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/KfMeasurementU.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/KfMeasurementXy.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/KfMeasurementTime.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfMaterialMap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfMaterialMonitor.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfTarget.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfField.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfFieldValue.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfFieldSlice.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfFieldRegion.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfSetup.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfSetupBuilder.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/geo/KfModuleIndexMap.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/KfUtils.cxx
)

SET_SOURCE_FILES_PROPERTIES(${SRCS} PROPERTIES COMPILE_FLAGS "-O3")


If(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wno-parentheses) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.
Else()
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wno-pmf-conversions -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wstrict-null-sentinel -Wno-non-template-friend -Wno-parentheses -Wmissing-field-initializers) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.
EndIf()

add_library(KfCore SHARED ${SRCS})

list(APPEND HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/utils/KfSimd.h)

target_include_directories(KfCore
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/algo
         ${CMAKE_CURRENT_SOURCE_DIR}/data
         ${CMAKE_CURRENT_SOURCE_DIR}/geo
         ${CMAKE_CURRENT_SOURCE_DIR}/pars
         ${CMAKE_CURRENT_SOURCE_DIR}/utils
         ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(KfCore PUBLIC NO_ROOT)

target_link_libraries(KfCore
              PUBLIC  Vc::Vc     
                      OnlineDataLog          # needed for the logger
                      external::fles_logging # needed for the logger
                      external::fles_ipc     # needed for the logger
                      xpu
             PRIVATE  Boost::serialization
                      fmt::fmt
                      external::yaml-cpp
                     )

##### Offline version without the NO_ROOT in order to get standard logger! #############################################
if (NOT CBM_ONLINE_STANDALONE)
  add_library(KfCoreOffline SHARED ${SRCS})

  target_include_directories(KfCoreOffline
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/algo
           ${CMAKE_CURRENT_SOURCE_DIR}/data
           ${CMAKE_CURRENT_SOURCE_DIR}/geo
           ${CMAKE_CURRENT_SOURCE_DIR}/pars
           ${CMAKE_CURRENT_SOURCE_DIR}/utils
           ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_link_libraries(KfCoreOffline
                PUBLIC  Vc::Vc
                        OnlineDataLog          # needed for the logger?
                        FairLogger::FairLogger
                        xpu
               PRIVATE  Boost::serialization
                        fmt::fmt
                        external::yaml-cpp
                       )
  install(TARGETS KfCoreOffline DESTINATION lib)
endif()
########################################################################################################################

install(TARGETS KfCore DESTINATION lib)
install(DIRECTORY kf TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY kf/utils TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY kf/data TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY kf/geo TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY kf/algo TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY kf/pars TYPE INCLUDE FILES_MATCHING PATTERN "*.h")

install(
  FILES
    KfFramework.h
    KfDefs.h
    KfTrackKalmanFilter.h

    data/KfTrackParam.h
    data/KfMeasurementU.h
    data/KfMeasurementXy.h
    data/KfMeasurementTime.h

    geo/KfField.h
    geo/KfFieldRegion.h
    geo/KfFieldSlice.h
    geo/KfFieldValue.h
    geo/KfMaterialMap.h 
    geo/KfMaterialMonitor.h
    geo/KfModuleIndexMap.h 
    geo/KfSetup.h
    geo/KfSetupBuilder.h
    geo/KfIMaterialMapFactory.h
    geo/KfTarget.h
    geo/KfField.h

    pars/KfParticlePDG.h

    utils/KfVector.h
    utils/KfSimd.h
    utils/KfSimdVc.h 
    utils/KfSimdPseudo.h
    utils/KfUtils.h
    utils/KfMath.h

    DESTINATION
    include/
)
