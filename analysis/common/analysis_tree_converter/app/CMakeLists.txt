# CMakeList file for binary cbmatc
# F.Linz,	27. October 2023

set(SRCS
  Application.cxx
  ProgramOptions.cxx
  main.cxx
  )

set(HEADERS
  Application.h
  ProgramOptions.h
  )


add_executable(cbm_atconverter ${SRCS} ${HEADERS})

target_link_libraries(cbm_atconverter
  PUBLIC
    CbmAtConverterSteer
  PRIVATE
    Boost::program_options
  )

install(TARGETS cbm_atconverter DESTINATION bin)
