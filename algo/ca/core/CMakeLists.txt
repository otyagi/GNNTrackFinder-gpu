set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/utils
  ${CMAKE_CURRENT_SOURCE_DIR}/pars
  ${CMAKE_CURRENT_SOURCE_DIR}/qa
  ${CMAKE_CURRENT_SOURCE_DIR}/data
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking
  ${CMAKE_CURRENT_SOURCE_DIR}/experimental
  ${CMAKE_CURRENT_SOURCE_DIR}/experimental_Gnn
)

set(SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaDataManager.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaInputData.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaTrack.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaGrid.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaHit.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaTriplet.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaWindowData.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/data/CaTimesliceHeader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pars/CaConfigReader.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pars/CaInitManager.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pars/CaIteration.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pars/CaParameters.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pars/CaSearchWindow.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pars/CaStation.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/pars/CaStationInitializer.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/CaUtils.cxx  
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking/CaCloneMerger.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking/CaFramework.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking/CaTrackExtender.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking/CaTrackFinder.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking/CaTrackFinderWindow.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking/CaTrackFitter.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/tracking/CaTripletConstructor.cxx

  ${CMAKE_CURRENT_SOURCE_DIR}/experimental/CaGpuTrackFinderSetup.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/experimental/CaGpuTripletConstructor.cxx
  ${CMAKE_CURRENT_SOURCE_DIR}/experimental/CaDeviceImage.cxx

  # ${CMAKE_CURRENT_SOURCE_DIR}/experimental_Gnn/.cxx
)

SET_SOURCE_FILES_PROPERTIES(${SRCS} PROPERTIES COMPILE_FLAGS "-O3")


If(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wno-parentheses) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.
Else()
  ADD_DEFINITIONS(-Wall -Wextra -Wsign-promo -Wno-pmf-conversions -Wctor-dtor-privacy -Wreorder -Wno-deprecated -Wstrict-null-sentinel -Wno-non-template-friend -Wno-parentheses -Wmissing-field-initializers) # -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wold-style-cast : wait for other parts of cbmroot\root.
EndIf()

add_library(CaCore SHARED ${SRCS})

list(APPEND HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/utils/CaSimd.h)

target_include_directories(CaCore
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/data
         ${CMAKE_CURRENT_SOURCE_DIR}/utils
         ${CMAKE_CURRENT_SOURCE_DIR}/pars
         ${CMAKE_CURRENT_SOURCE_DIR}/qa
         ${CMAKE_CURRENT_SOURCE_DIR}/tracking
         ${CMAKE_CURRENT_SOURCE_DIR}/experimental
         ${CMAKE_CURRENT_SOURCE_DIR}/experimental_Gnn
         ${CMAKE_CURRENT_SOURCE_DIR}/
)

set(DEVICE_SRCS
  experimental/CaDeviceImage.cxx
  experimental/CaGpuTripletConstructor.cxx
  # experimental_Gnn/.cxx
)

target_compile_definitions(CaCore PUBLIC NO_ROOT)
target_link_libraries(CaCore
              PUBLIC  KfCore
                      Boost::serialization
                      OnlineDataLog          # needed for the logger
                      external::fles_logging # needed for the logger
                      external::fles_ipc     # needed for the logger
                      external::yaml-cpp
                      xpu
                     )
xpu_attach(CaCore ${DEVICE_SRCS})

##### Offline version without the NO_ROOT in order to get standard logger! #############################################
if (NOT CBM_ONLINE_STANDALONE)
  add_library(CaCoreOffline SHARED ${SRCS})

  target_include_directories(CaCoreOffline
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/data
           ${CMAKE_CURRENT_SOURCE_DIR}/utils
           ${CMAKE_CURRENT_SOURCE_DIR}/pars
           ${CMAKE_CURRENT_SOURCE_DIR}/qa
           ${CMAKE_CURRENT_SOURCE_DIR}/tracking
           ${CMAKE_CURRENT_SOURCE_DIR}/experimental
           ${CMAKE_CURRENT_SOURCE_DIR}/experimental_Gnn
           ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_link_libraries(CaCoreOffline
                 PUBLIC KfCoreOffline
                        Boost::serialization
                        external::yaml-cpp
                        xpu
                       )
  xpu_attach(CaCoreOffline ${DEVICE_SRCS})
  install(TARGETS CaCoreOffline DESTINATION lib)
endif()
########################################################################################################################

install(TARGETS CaCore DESTINATION lib)
install(DIRECTORY utils TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY data TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY tracking TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY pars TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY experimental TYPE INCLUDE FILES_MATCHING PATTERN "*.h")

install(
  FILES
    data/CaDataManager.h
    data/CaGridEntry.h
    data/CaHit.h
    data/CaInputData.h
    data/CaTrack.h
    data/CaWindowData.h
    pars/CaConfigReader.h
    data/CaGridEntry.h
    data/CaGrid.h
    data/CaGridArea.h
    data/CaTriplet.h
    data/CaBranch.h
    data/CaWindowData.h
    data/CaTimesliceHeader.h
    pars/CaDefs.h
    pars/CaInitManager.h
    pars/CaIteration.h
    pars/CaParameters.h
    pars/CaSearchWindow.h
    pars/CaStation.h
    pars/CaStationInitializer.h

    utils/CaTrackingMonitor.h
    utils/CaEnumArray.h
    utils/CaMonitor.h
    utils/CaMonitorData.h
    utils/CaObjectInitController.h
    utils/CaSimd.h
    utils/CaTimer.h
    utils/CaVector.h
    utils/CaDefines.h
    utils/CaUtils.h

    tracking/CaCloneMerger.h
    tracking/CaFramework.h
    tracking/CaTrackExtender.h
    tracking/CaTrackFinder.h
    tracking/CaTrackFinderWindow.h
    tracking/CaTrackFitter.h
    tracking/CaTripletConstructor.h

    experimental/CaDeviceImage.h
    experimental/CaGpuGrid.h
    experimental/CaGpuGridArea.h
    experimental/CaGpuParameters.h
    experimental/CaGpuStation.h
    experimental/CaGpuMaterialMap.h
    experimental/CaGpuField.h
    experimental/CaMeasurementXy.h
    experimental/CaGpuTrackFinderSetup.h
    experimental/CaGpuTripletConstructor.h
    experimental/CaDeviceImage.h
    experimental/CaGpuTimeMonitor.h

    experimental_Gnn/GnnGpuTimeMonitor.h

    DESTINATION
    include/
)

