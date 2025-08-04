# CMakeList file for binary cbmatc
# F.Linz,	09. January 2024

set(SRCS
  Application.cxx
  ProgramOptions.cxx
  main.cxx
  )

set(HEADERS
  Application.h
  ProgramOptions.h
  )


add_executable(cbmsim_digitization ${SRCS} ${HEADERS})

target_link_libraries(cbmsim_digitization
  PUBLIC
    CbmSimDigitizationSteer
    CbmSimResponse
  PRIVATE
    Boost::program_options
  )

install(TARGETS cbmsim_digitization DESTINATION bin)
