# CMakeList file for binary cbmreco_offline
# V. Friese,     13 May 2023

set(SRCS
  Application.cxx
  ProgramOptions.cxx
  main.cxx
  )
  
set(HEADERS
  Application.h
  ProgramOptions.h
 )
  

add_executable(cbmreco_offline ${SRCS} ${HEADERS})

target_link_libraries(cbmreco_offline
  PUBLIC 
    CbmRecoOfflineSteer
  PRIVATE 
    Boost::program_options
  )

install(TARGETS cbmreco_offline DESTINATION bin)
