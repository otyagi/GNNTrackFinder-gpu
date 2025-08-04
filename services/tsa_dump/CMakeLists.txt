# Copyright (C) 2024 GSI Helmholtzzentrum fuer Schwerionenforschung, Darmstadt
# SPDX-License-Identifier: GPL-3.0-only
# Authors: Pierre-Alain Loizeau [committer]

set(APP tsadump)

set(INCLUDE_DIRECTORIES
  ${INCLUDE_DIRECTORIES}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CBMROOT_SOURCE_DIR}
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

add_executable(${APP} ${SRCS} ${HEADERS})
target_link_libraries(${APP}
  PUBLIC
    CbmFlibFlesTools
    FairLogger::FairLogger
    external::fles_ipc
    libzmq
    cppzmq
  PRIVATE
    Boost::program_options
    external::fles_logging # Needed for TimesliceSource in CbmSourceTsArchive, not sure of conflicts with FairLogger
  )


install(TARGETS ${APP} DESTINATION bin)
