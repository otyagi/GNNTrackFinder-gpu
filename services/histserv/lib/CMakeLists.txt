# CMakeList file for library associated to HistServ service, needed by CLING to allow commands in WebUI
# P.-A. Loizeau,     26 June 2023

set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

#set(SRCS
#  ui_callbacks.cxx
#  )

list(APPEND HEADERS ui_callbacks.h)

set(LIBRARY_NAME CbmServicesHistServ)
set(LINKDEF ${LIBRARY_NAME}LinkDef.h )

set(PUBLIC_DEPENDENCIES
  ROOT::Core
  )

generate_cbm_library()
