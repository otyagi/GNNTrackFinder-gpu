set(INCLUDE_DIRECTORIES
  ${INCLUDE_DIRECTORIES}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

set(SRCS
  Application.cxx
  main.cxx
  )

set(HEADERS
  Application.h
 )

set(INCLUDE_DIRECTORIES
  ${INCLUDE_DIRECTORIES}
  ${CBMROOT_SOURCE_DIR}
  )

add_executable(run_info ${SRCS} ${HEADERS})

target_link_libraries(run_info
  PRIVATE
    CbmBase
    Boost::program_options
  )

install(TARGETS run_info DESTINATION bin)
