cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
cmake_policy(VERSION 3.14...3.23)

if (NOT DEFINED CBM_ONLINE_STANDALONE)
  if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CBM_ONLINE_STANDALONE ON)
    project(CbmOnline)
  else()
    set(CBM_ONLINE_STANDALONE OFF)
  endif()
endif()

if (CBM_ONLINE_STANDALONE)
  message(STATUS "Building online standalone")
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_EXTENSIONS OFF)

  Set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
  Set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
  Set(INCLUDE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/include")
  set(CMAKE_INSTALL_LIBDIR lib)


  # Set default build type
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
  endif()

  #Fairsoft Modules Path, SIMPATH must always be set
  if (NOT DEFINED SIMPATH)
    message(FATAL_ERROR "SIMPATH not set")
  endif()
  list(PREPEND CMAKE_PREFIX_PATH ${SIMPATH})

  find_package(OpenMP REQUIRED)
  find_package(Boost REQUIRED COMPONENTS serialization regex filesystem log log_setup container program_options thread iostreams)
  find_package(ROOT CONFIG REQUIRED GenVector)
  find_package(Vc 1.4.1 CONFIG REQUIRED)
  find_package(fmt CONFIG REQUIRED)

  include(ExternalProject)

  # cmake is symlinked to ../cmake by the install script in the container
  # We use the symlink because some cmake macros use relative path from the project root
  # to locate files (e.g. Gen_Exe_Script).
  # Thus we need the same path if the project root changes.
  set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
  list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
  include(CbmMacros) # for 'download_project_if_needed', 'Gen_Exe_Script' macro

  add_subdirectory(../external external)
endif()

add_subdirectory(kfp)
add_subdirectory(log)
add_subdirectory(data)
add_subdirectory(kf)
add_subdirectory(ca)
add_subdirectory(base)
#add_subdirectory(kfp) # For KFParticleOnline

# exclude unittests from being build inside the container
if (NOT CBM_ONLINE_STANDALONE)
  add_subdirectory(test)
endif()

set(DEVICE_SRCS
  base/gpu/DeviceImage.cxx
  base/gpu/Params.cxx
  detectors/sts/Hitfinder.cxx
)

set(SRCS
  ${DEVICE_SRCS}
  base/ChainContext.cxx
  base/DigiData.cxx
  base/Options.cxx
  base/MainConfig.cxx
  base/RecoParams.cxx
  base/System.cxx
  base/compat/Algorithm.cxx
  base/util/MemoryLogger.cxx
  base/util/StlUtils.cxx
  base/util/EnumDict.cxx
  base/util/TimingsFormat.cxx
  data/sts/HitfinderPars.cxx
  data/sts/LandauTable.cxx
  evbuild/Config.cxx
  evbuild/EventBuilder.cxx
  evbuild/EventBuilderConfig.cxx
  evbuild/EventbuildChain.cxx
  trigger/DigiTriggerConfig.cxx
  trigger/HitMultTrigger.cxx
  trigger/TimeClusterTrigger.cxx
  trigger/V0Trigger.cxx
  trigger/V0TriggerConfig.cxx
  evselector/DigiEventSelector.cxx
  evselector/DigiEventSelectorConfig.cxx
  unpack/CommonUnpacker.cxx
  detectors/bmon/Calibrate.cxx
  detectors/bmon/Clusterizer.cxx
  detectors/bmon/Hitfind.cxx
  detectors/bmon/ReadoutConfig.cxx
  detectors/bmon/Unpack.cxx
  detectors/bmon/UnpackMS.cxx
  detectors/sts/ChannelMaskSet.cxx
  detectors/sts/ReadoutConfig.cxx
  detectors/sts/HitfinderChain.cxx
  detectors/sts/TrackingInterface.cxx
  detectors/sts/Unpack.cxx
  detectors/sts/UnpackMS.cxx
  detectors/sts/WalkMap.cxx
  detectors/much/ReadoutConfig.cxx
  detectors/much/Unpack.cxx
  detectors/much/UnpackMS.cxx
  detectors/tof/HitFinder.cxx
  detectors/tof/Calibrate.cxx
  detectors/tof/Clusterizer.cxx
  detectors/tof/ReadoutConfig.cxx
  detectors/tof/Unpack.cxx
  detectors/tof/UnpackMS.cxx
  detectors/tof/Hitfind.cxx
  detectors/tof/TrackingInterface.cxx
  detectors/trd/Cluster.cxx
  detectors/trd/Clusterizer.cxx
  detectors/trd/Cluster2D.cxx
  detectors/trd/Clusterizer2D.cxx
  detectors/trd/DigiRec.cxx
  detectors/trd/Hit.cxx
  detectors/trd/Hitfind.cxx
  detectors/trd/HitFactory2D.cxx
  detectors/trd/HitFinder.cxx
  detectors/trd/HitFinder2D.cxx
  detectors/trd/HitMerger.cxx
  detectors/trd/HitMerger2D.cxx
  detectors/trd/ReadoutConfig.cxx
  detectors/trd/TrackingInterface.cxx
  detectors/trd/Unpack.cxx
  detectors/trd/UnpackMS.cxx
  detectors/trd2d/ReadoutConfig.cxx
  detectors/trd2d/Unpack.cxx
  detectors/trd2d/UnpackMS.cxx
  detectors/rich/ReadoutConfig.cxx
  detectors/rich/Unpack.cxx
  detectors/rich/UnpackMS.cxx
  global/ParFiles.cxx
  global/StorableRecoResults.cxx
  global/Reco.cxx
  global/RecoResultsInputArchive.cxx
  global/RecoResultsOutputArchive.cxx
  qa/DigiEventQa.cxx
  qa/Histo1D.cxx
  qa/HistogramContainer.cxx
  qa/CanvasConfig.cxx
  qa/PadConfig.cxx
  qa/QaData.cxx
  qa/RecoGeneralQa.cxx
  qa/QaManager.cxx
  qa/hitfind/BmonHitfindQa.cxx
  qa/hitfind/BmonHitfindQaParameters.cxx
  qa/trigger/V0TriggerQa.cxx
  qa/hitfind/TofHitfindQa.cxx
  qa/hitfind/TofHitfindQaParameters.cxx
  qa/unpack/StsDigiQa.cxx
  ca/TrackingSetup.cxx
  ca/TrackingChain.cxx
  ca/qa/CaQa.cxx
  kfp/KfpV0Finder.cxx
  kfp/KfpV0FinderChain.cxx
  kfp/KfpV0FinderConfig.cxx
  kfp/KfpV0FinderQa.cxx
)

set(BUILD_INFO_CXX ${CMAKE_CURRENT_BINARY_DIR}/base/BuildInfo.cxx)
set(SRCS ${SRCS} ${BUILD_INFO_CXX})

add_custom_target(generateBuildInfo
    COMMAND ${CMAKE_COMMAND}
        -DBUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DXPU_DEBUG=${XPU_DEBUG}
        -DINFILE=${CMAKE_CURRENT_SOURCE_DIR}/base/BuildInfo.cxx.in
        -DOUTFILE=${BUILD_INFO_CXX}
        -P ${CMAKE_SOURCE_DIR}/cmake/modules/GenerateBuildInfo.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/base/BuildInfo.cxx.in
            ${CMAKE_SOURCE_DIR}/cmake/modules/GenerateBuildInfo.cmake
    BYPRODUCTS ${BUILD_INFO_CXX}
    COMMENT "Generating build info"
)

add_library(Algo SHARED ${SRCS})

target_include_directories(Algo
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/data
         ${CMAKE_CURRENT_SOURCE_DIR}/base
         ${CMAKE_CURRENT_SOURCE_DIR}/evbuild
         ${CMAKE_CURRENT_SOURCE_DIR}/global
         ${CMAKE_CURRENT_SOURCE_DIR}/trigger
         ${CMAKE_CURRENT_SOURCE_DIR}/evselector
         ${CMAKE_CURRENT_SOURCE_DIR}/unpack
         ${CMAKE_CURRENT_SOURCE_DIR}/detectors
         ${CMAKE_CURRENT_SOURCE_DIR}/qa
         ${CMAKE_CURRENT_SOURCE_DIR}/qa/unpack
         ${CMAKE_CURRENT_SOURCE_DIR}/qa/hitfind
         ${CMAKE_CURRENT_SOURCE_DIR}/qa/trigger
         ${CMAKE_CURRENT_SOURCE_DIR}/kf
         ${CMAKE_CURRENT_SOURCE_DIR}/kf/core
         ${CMAKE_CURRENT_SOURCE_DIR}/kf/core/utils
         ${CMAKE_CURRENT_SOURCE_DIR}/kfp
         ${CMAKE_CURRENT_SOURCE_DIR}/ca
         ${CMAKE_CURRENT_SOURCE_DIR}/ca/qa
         ${CMAKE_CURRENT_SOURCE_DIR}/ca/core/data
         ${CMAKE_CURRENT_SOURCE_DIR}
         ${CMAKE_SOURCE_DIR}/core/data/global
)

target_link_libraries(Algo
  PUBLIC    OnlineData
            CbmYamlInterface
            KfCore
            CaCore
            ROOT::GenVector
            GSL
            fmt::fmt
            Boost::program_options
            Boost::filesystem
            Boost::iostreams
            Boost::headers
            xpu
            external::yaml-cpp
            external::fles_logging
            external::fles_ipc
            external::fles_monitoring
            cppzmq
            poolstl
  PRIVATE   CbmKFParticleOnlineInterface 
)
target_compile_definitions(Algo PUBLIC NO_ROOT)
xpu_attach(Algo ${DEVICE_SRCS})

# Link against OpenMP if available
if (OpenMP_CXX_FOUND)
  target_link_libraries(Algo PUBLIC OpenMP::OpenMP_CXX)
endif()

# Try to enable parallel execution in c++17 if TBB is available
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND CMAKE_PREFIX_PATH "/opt/intel/oneapi/tbb/latest/")
  find_package(TBB)

  if (TBB_FOUND)
    message(STATUS "Found TBB")
    target_compile_definitions(Algo PUBLIC -DHAVE_TBB)
    target_link_libraries(Algo PUBLIC TBB::tbb)
  else()
    message(STATUS "TBB not found")
  endif()

endif()

if (CBM_ONLINE_STANDALONE)
  # Add online binary
  add_subdirectory(../reco/app/cbmreco cbmreco)
endif()

##### Offline version without the NO_ROOT in order to get standard logger! #############################################
if (NOT CBM_ONLINE_STANDALONE)
  add_library(AlgoOffline SHARED ${SRCS})

  target_include_directories(AlgoOffline
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/data
           ${CMAKE_CURRENT_SOURCE_DIR}/base
           ${CMAKE_CURRENT_SOURCE_DIR}/evbuild
           ${CMAKE_CURRENT_SOURCE_DIR}/global
           ${CMAKE_CURRENT_SOURCE_DIR}/trigger
           ${CMAKE_CURRENT_SOURCE_DIR}/evselector
           ${CMAKE_CURRENT_SOURCE_DIR}/unpack
           ${CMAKE_CURRENT_SOURCE_DIR}/detectors
           ${CMAKE_CURRENT_SOURCE_DIR}/qa
           ${CMAKE_CURRENT_SOURCE_DIR}/qa/unpack
           ${CMAKE_CURRENT_SOURCE_DIR}/qa/hitfind
           ${CMAKE_CURRENT_SOURCE_DIR}/qa/trigger
           ${CMAKE_CURRENT_SOURCE_DIR}/kf
           ${CMAKE_CURRENT_SOURCE_DIR}/kf/core
           ${CMAKE_CURRENT_SOURCE_DIR}/kf/core/utils
           ${CMAKE_CURRENT_SOURCE_DIR}/kfp
           ${CMAKE_CURRENT_SOURCE_DIR}/ca
           ${CMAKE_CURRENT_SOURCE_DIR}/ca/qa
           ${CMAKE_CURRENT_SOURCE_DIR}/ca/core/data
           ${CMAKE_CURRENT_SOURCE_DIR}
           ${CMAKE_SOURCE_DIR}/core/data/global
  )

  target_link_libraries(AlgoOffline
    PUBLIC    CbmData
              CbmYamlInterface
              KfCoreOffline
              CaCoreOffline
              ROOT::GenVector
              GSL
              fmt::fmt
              Boost::program_options
              Boost::filesystem
              Boost::iostreams
              Boost::headers
              FairLogger::FairLogger
              xpu
              external::yaml-cpp
              external::fles_ipc
              external::fles_monitoring
              cppzmq
              poolstl
    PRIVATE   CbmKFParticleOnlineInterface 
  )
  xpu_attach(AlgoOffline ${DEVICE_SRCS})

  # Link against OpenMP if available
  if (OpenMP_CXX_FOUND)
    target_link_libraries(AlgoOffline PUBLIC OpenMP::OpenMP_CXX)
  endif()

  # Try to enable parallel execution in c++17 if TBB is available
  if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND CMAKE_PREFIX_PATH "/opt/intel/oneapi/tbb/latest/")
    find_package(TBB)

    if (TBB_FOUND)
      message(STATUS "AlgoOffline: Found TBB")
      target_compile_definitions(AlgoOffline PUBLIC -DHAVE_TBB)
      target_link_libraries(AlgoOffline PUBLIC TBB::tbb)
    else()
      message(STATUS "Algo Offline: TBB not found")
    endif()

  endif()
  install(TARGETS AlgoOffline DESTINATION lib)
endif()
########################################################################################################################

install(TARGETS Algo DESTINATION lib)
install(DIRECTORY base/compat TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
#install(DIRECTORY base/yaml TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY base/util TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY base/gpu TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY data/sts TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY detectors/bmon TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY detectors/much TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY detectors/sts TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY detectors/tof TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY detectors/trd TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY detectors/trd2d TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ca/qa TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY qa TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY qa/unpack TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ca TYPE INCLUDE FILES_MATCHING PATTERN "*.h")
install(DIRECTORY kfp TYPE INCLUDE FILES_MATCHING PATTERN "*.h")


install(
  FILES
    base/ChainContext.h
    base/Definitions.h
    base/HistogramSender.h
    base/Options.h
    base/RecoParams.h
    base/SubChain.h
    base/System.h
    base/PartitionedVector.h
    base/PartitionedSpan.h
    global/Reco.h
    global/RecoResults.h
    global/RecoResultsInputArchive.h
    global/RecoResultsOutputArchive.h
    global/StorableRecoResults.h
    qa/Histogram.h
    ca/TrackingChain.h
    ca/TrackingChainConfig.h
    # NOTE: SZh 20.11.2023:
    #       The ca/qa directory depends on the online qa classes, so for now it has to be a part of the Algo library.
    ca/qa/CaQa.h
  DESTINATION
    include/
)
