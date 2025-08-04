# Build parts of the flesnet software, which is maintained in the flesnet repository on github
#
# The included libraries provide the interface to the experiment data in timeslices
# both online and in timeslice archive (.tsa) files.

set(FLESNET_VERSION 0aefc28576151385390b621b30cfc1862b97baed) # 2024-06-07

set(FLESNET_SRC_URL "https://github.com/cbm-fles/flesnet")

set(FLESNET_DESTDIR "${CMAKE_CURRENT_BINARY_DIR}/flesnet-prefix")
set(FLESNET_BIN_PREFIX "${FLESNET_DESTDIR}/src/flesnet-build")

download_project_if_needed(
  PROJECT         flesnet
  GIT_REPOSITORY  ${FLESNET_SRC_URL}
  GIT_TAG         ${FLESNET_VERSION}
  GIT_STASH       TRUE
  SOURCE_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/flesnet
  TEST_FILE       CMakeLists.txt
)

include(FindBoostZstd)

If(ProjectUpdated)
  File(REMOVE_RECURSE ${FLESNET_DESTDIR})
  Message("flesnet source directory was changed so build directory was deleted")
EndIf()

if(APPLE)
  execute_process(COMMAND brew --prefix --installed openssl
                  RESULT_VARIABLE OPENSSL_FOUND
                  OUTPUT_VARIABLE OPENSSL_PATH
                  OUTPUT_STRIP_TRAILING_WHITESPACE
                 )
endif()

if ("${CMAKE_GENERATOR}" MATCHES "Make")
  set(FLESNET_BUILD_COMMAND "$(MAKE)")
else()
  set(FLESNET_BUILD_COMMAND "${CMAKE_COMMAND} --build . --target")
endif()

set(FLESNET_MODULES logging monitoring fles_ipc tsclient)
set(FLESNET_BYPRODUCTS
    ${FLESNET_DESTDIR}/src/flesnet-build/lib/fles_ipc/${CMAKE_STATIC_LIBRARY_PREFIX}fles_ipc${CMAKE_STATIC_LIBRARY_SUFFIX}
    ${FLESNET_BIN_PREFIX}/tsclient
   )

ExternalProject_Add(
  flesnet
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/flesnet
  CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env SIMPATH=${SIMPATH}
             ${CMAKE_COMMAND}
             -G ${CMAKE_GENERATOR}
             -DINCLUDE_ZMQ:BOOL=TRUE
             -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
             -DOPENSSL_ROOT_DIR:FILEPATH=${OPENSSL_PATH}
             ${CMAKE_CURRENT_SOURCE_DIR}/flesnet
  BUILD_IN_SOURCE 0
  LOG_DOWNLOAD 1 LOG_CONFIGURE 1 LOG_BUILD 1 LOG_INSTALL 1
  BUILD_COMMAND ${FLESNET_BUILD_COMMAND} ${FLESNET_MODULES}
  BUILD_BYPRODUCTS ${FLESNET_BYPRODUCTS}
  INSTALL_COMMAND ""
)

install(PROGRAMS ${FLESNET_BIN_PREFIX}/tsclient TYPE BIN)

add_library(external::zmq STATIC IMPORTED GLOBAL)
add_dependencies(external::zmq flesnet)
set_target_properties(external::zmq PROPERTIES
  IMPORTED_LOCATION ${FLESNET_DESTDIR}/src/flesnet-build/src/zeromq-build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}zmq${CMAKE_STATIC_LIBRARY_SUFFIX}
)
target_include_directories(external::zmq INTERFACE
  ${FLESNET_DESTDIR}/src/flesnet-build/src/zeromq/include
)
target_compile_definitions(external::zmq INTERFACE ZMQ_BUILD_DRAFT_API=1)

add_library(external::fles_logging STATIC IMPORTED GLOBAL)
add_dependencies(external::fles_logging flesnet)
set_target_properties(external::fles_logging PROPERTIES
  IMPORTED_LOCATION ${FLESNET_DESTDIR}/src/flesnet-build/lib/logging/${CMAKE_STATIC_LIBRARY_PREFIX}logging${CMAKE_STATIC_LIBRARY_SUFFIX}
)
target_include_directories(external::fles_logging INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/logging
  ${Boost_INCLUDE_DIRS}
)
target_compile_definitions(external::fles_logging
  INTERFACE BOOST_LOG_DYN_LINK
  INTERFACE BOOST_LOG_USE_NATIVE_SYSLOG
  INTERFACE BOOST_ERROR_CODE_HEADER_ONLY
)

add_library(external::fles_monitoring STATIC IMPORTED GLOBAL)
add_dependencies(external::fles_monitoring flesnet)
set_target_properties(external::fles_monitoring PROPERTIES
  IMPORTED_LOCATION ${FLESNET_DESTDIR}/src/flesnet-build/lib/monitoring/${CMAKE_STATIC_LIBRARY_PREFIX}monitoring${CMAKE_STATIC_LIBRARY_SUFFIX}
)
target_include_directories(external::fles_monitoring INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/monitoring
  ${Boost_INCLUDE_DIRS}
)
target_compile_definitions(external::fles_monitoring
  INTERFACE BOOST_LOG_DYN_LINK
  INTERFACE BOOST_LOG_USE_NATIVE_SYSLOG
  INTERFACE BOOST_ERROR_CODE_HEADER_ONLY
)

add_library(external::fles_ipc STATIC IMPORTED GLOBAL)
add_dependencies(external::fles_ipc flesnet external::fles_logging)
if (BOOST_IOS_HAS_ZSTD)
  target_compile_definitions(external::fles_ipc INTERFACE BOOST_IOS_HAS_ZSTD)
endif()

set(dir_to_link
    ${FLESNET_DESTDIR}/src/flesnet-build/src/zeromq-build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}zmq${CMAKE_STATIC_LIBRARY_SUFFIX}
   )
find_package("GnuTLS" 3.6.7)
if(GNUTLS_FOUND)
  list(APPEND dir_to_link
       ${GNUTLS_LIBRARIES}
      )
endif()
list(APPEND dir_to_link
     ${FLESNET_DESTDIR}/src/flesnet-build/lib/logging/${CMAKE_STATIC_LIBRARY_PREFIX}logging${CMAKE_STATIC_LIBRARY_SUFFIX}
     ${FLESNET_DESTDIR}/src/flesnet-build/lib/monitoring/${CMAKE_STATIC_LIBRARY_PREFIX}monitoring${CMAKE_STATIC_LIBRARY_SUFFIX}
     )
list(APPEND dir_to_link
     ${Boost_LOG_LIBRARY}
     ${Boost_FILESYSTEM_LIBRARY}
     ${Boost_REGEX_LIBRARY}
     ${Boost_SERIALIZATION_LIBRARY}
     ${Boost_SYSTEM_LIBRARY} # Needed for the monitoring library linking
     ${Boost_IOSTREAMS_LIBRARY} # needed for the fles_ipc library linking
     fmt::fmt
     )
if(NOT APPLE)
  list(APPEND dir_to_link ${Boost_THREAD_LIBRARY} -lrt)
endif()

# Hack such that for boost versions below 1.80.0 libzstd is found
if(APPLE AND Boost_VERSION LESS 1.80.0)
  set_target_properties(external::fles_ipc PROPERTIES
    IMPORTED_LOCATION ${FLESNET_DESTDIR}/src/flesnet-build/lib/fles_ipc/${CMAKE_STATIC_LIBRARY_PREFIX}fles_ipc${CMAKE_STATIC_LIBRARY_SUFFIX}
    IMPORTED_LINK_INTERFACE_LIBRARIES "${dir_to_link}"
    INTERFACE_LINK_DIRECTORIES "/usr/local/lib"
  )
else ()
  set_target_properties(external::fles_ipc PROPERTIES
    IMPORTED_LOCATION ${FLESNET_DESTDIR}/src/flesnet-build/lib/fles_ipc/${CMAKE_STATIC_LIBRARY_PREFIX}fles_ipc${CMAKE_STATIC_LIBRARY_SUFFIX}
    IMPORTED_LINK_INTERFACE_LIBRARIES "${dir_to_link}"
  )
endif()
target_include_directories(external::fles_ipc INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/fles_ipc
  ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/shm_ipc
)



Install(FILES ${FLESNET_DESTDIR}/src/flesnet-build/lib/fles_ipc/${CMAKE_STATIC_LIBRARY_PREFIX}fles_ipc${CMAKE_STATIC_LIBRARY_SUFFIX}
              ${FLESNET_DESTDIR}/src/flesnet-build/lib/shm_ipc/${CMAKE_STATIC_LIBRARY_PREFIX}shm_ipc${CMAKE_STATIC_LIBRARY_SUFFIX}
              ${FLESNET_DESTDIR}/src/flesnet-build/lib/logging/${CMAKE_STATIC_LIBRARY_PREFIX}logging${CMAKE_STATIC_LIBRARY_SUFFIX}
              ${FLESNET_DESTDIR}/src/flesnet-build/lib/monitoring/${CMAKE_STATIC_LIBRARY_PREFIX}monitoring${CMAKE_STATIC_LIBRARY_SUFFIX}
              ${FLESNET_DESTDIR}/src/flesnet-build/src/zeromq-build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}zmq${CMAKE_STATIC_LIBRARY_SUFFIX}
        DESTINATION lib
       )

file(GLOB IPC_LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/fles_ipc/*.hpp)
file(GLOB LOG_LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/logging/*.hpp)
file(GLOB MONITOR_LIB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/monitoring/*.hpp)
file(GLOB MONITOR_LIB_IPP ${CMAKE_CURRENT_SOURCE_DIR}/flesnet/lib/monitoring/*.ipp)

Install(FILES ${IPC_LIB_HEADERS}
              ${LOG_LIB_HEADERS}
              ${MONITOR_LIB_HEADERS}
              ${MONITOR_LIB_IPP}
        DESTINATION include
       )
