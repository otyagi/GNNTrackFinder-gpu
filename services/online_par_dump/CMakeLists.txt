# Copyright (C) 2024 Johann Wolfgang Goethe-Universitaet Frankfurt, Frankfurt am Main
# SPDX-License-Identifier: GPL-3.0-only
# Authors: Felix Weiglhofer [committer]

set(APP cbm-online-par-dump)

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
  PRIVATE
    CbmRecoTasks
    CbmRecoSteer
    CbmBase
    Boost::program_options
    external::fles_monitoring
    external::fles_logging
  )

install(TARGETS ${APP} DESTINATION bin)
