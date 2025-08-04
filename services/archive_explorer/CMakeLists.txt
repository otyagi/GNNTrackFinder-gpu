#
# Application
#
set(APP cbm-archive-explorer)

set(APP_SRCS
  app/Application.cxx
  app/Histograms.cxx
  app/Options.cxx
  app/main.cxx
)

add_executable(${APP} ${APP_SRCS})

target_link_libraries(${APP}
  Algo
  Boost::program_options
  external::fles_logging
  fmt::fmt
)

#
# libCbmArchiveExplorer
#

set(LIBRARY_NAME CbmArchiveExplorer)
set(LINKDEF lib/LinkDef.h)

set(INCLUDE_DIRECTORIES
  lib
)

set(SRCS
  lib/HistogramCollection.cxx
  lib/Server.cxx
)

set(HEADERS
  lib/Server.h
)

set(PUBLIC_DEPENDENCIES
  Algo
  external::fles_logging
  fmt::fmt
  ROOT::Core
  ROOT::Gpad
  ROOT::Hist
  ROOT::RIO
  ROOT::RHTTP
)

generate_cbm_library()

target_link_libraries(${APP} CbmArchiveExplorer)
