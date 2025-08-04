Function(AddBasicTest name)
  set(PVT_DEPS
    CbmData
    CbmStsBase
    Gtest
    GtestMain
    FairRoot::Base
    ROOT::Core
  )

  add_executable(${name} ${name}.cxx)
  target_link_libraries(${name} PRIVATE ${PVT_DEPS})
  Add_Test(
    NAME ${name}
    COMMAND ${name}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )
EndFunction()

set(INCLUDE_DIRECTORIES
  ${CMAKE_SOURCE_DIR}/core/detectors/sts
  )

AddBasicTest(_GTestCbmStsParAsic)
