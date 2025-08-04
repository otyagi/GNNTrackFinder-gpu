# CMakeList file for binary histserv_nofairmq
# P.-A. Loizeau,     14 July 2023

set(INCLUDE_DIRECTORIES
  ${INCLUDE_DIRECTORIES}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CBMROOT_SOURCE_DIR}/algo/qa/
  )

set(SRCS
  Application.cxx
  ProgramOptions.cxx
  main.cxx
  )

set(HEADERS
  Application.h
  ProgramOptions.h
 )

set(INCLUDE_DIRECTORIES
  ${INCLUDE_DIRECTORIES}
  ${CBMROOT_SOURCE_DIR}
  )

add_executable(histserv_nofairmq_tester ${SRCS} ${HEADERS})

target_link_libraries(histserv_nofairmq_tester
  PUBLIC
    Algo
    CbmFlibFlesTools
    CbmServicesHistServ
  PRIVATE
    Boost::program_options
    FairLogger::FairLogger
    ROOT::Core
    ROOT::Gpad
    ROOT::Hist
    ROOT::RIO
    ROOT::RHTTP
    libzmq
    cppzmq
  )

install(TARGETS histserv_nofairmq_tester DESTINATION bin)
