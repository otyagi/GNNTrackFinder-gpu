Function(AddBasicTest name)
  set(PVT_DEPS
    Algo
    Gtest
    GtestMain
  )

  add_executable(${name} ${name}.cxx)
  target_link_libraries(${name} PRIVATE ${PVT_DEPS})
  Add_Test(
    NAME ${name}
    COMMAND ${name}
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  )
EndFunction()

#AddBasicTest(_GTestKf)
AddBasicTest(_GTestTimeClusterTrigger)
AddBasicTest(_GTestEventBuilder)
AddBasicTest(_GTestDigiEventSelector)
AddBasicTest(_GTestYamlConfig)
AddBasicTest(_GTestPartitionedVector)
AddBasicTest(_GTestPartitionedSpan)
AddBasicTest(_GTestTrdClusterizer)
AddBasicTest(_GTestChannelMapping)

if (DEFINED ENV{RAW_DATA_PATH})
  set(RAW_DATA_PATH $ENV{RAW_DATA_PATH})
  set(RUN 2391)

  set(TEST_SCRIPT ${CMAKE_SOURCE_DIR}/algo/test/realdata_test.sh)
  set(RECO_BIN ${EXECUTABLE_OUTPUT_PATH}/cbmreco)
  set(PARAMS_DIR ${CMAKE_SOURCE_DIR}/parameters/online)
  set(TSA_FILE ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa)

  # FIXME: for now the wrapper/log checker script hard-codes the TS number to 1
  #        and would always succed if 1st TS passed !!!!!
  # FIXME: This test seems to need 45s during the MR CI but more than 600 during the Nightly/Weekly tests!!
  if(${CBM_TEST_MODEL} MATCHES MergeRequest OR ${CBM_TEST_MODEL} MATCHES Continuous )
    Message( STATUS "MR or Continuous model detected, decreasing TS nb for OnlineReco tests to single one." )
    SET( RECO_TS_NB 1)
    SET( RECO_THREADS_NB $ENV{number_of_processors_for_test})
    SET( ONLINE_RECO_TO 200) # MR mode on run4: 45-50s for 2391
  elseif(${CBM_TEST_MODEL} MATCHES Weekly OR ${CBM_TEST_MODEL} MATCHES Profile )
    Message( STATUS "Profiling model detected, increasing timeout and TS nb for OnlineReco tests with coverage." )
    SET( RECO_TS_NB 20)
    SET( RECO_THREADS_NB $ENV{number_of_processors_for_test})
    SET( ONLINE_RECO_TO 1600) # Coverage mode for run 2391: ???? on run4, ???? on virgo3
  else()
    SET( RECO_TS_NB 3)
    SET( RECO_THREADS_NB $ENV{number_of_processors_for_test})
    SET( ONLINE_RECO_TO  800) # Nightly mode on run4: ??? for 2391
  endif()

  Add_Test(
    NAME OnlineRecoCanProcessTimeslice
    COMMAND ${TEST_SCRIPT} ${RECO_BIN} ${PARAMS_DIR} ${TSA_FILE} ${RECO_TS_NB} ${RECO_THREADS_NB}
  )

  set_tests_properties(OnlineRecoCanProcessTimeslice PROPERTIES
    TIMEOUT ${ONLINE_RECO_TO}
    RESOURCE_LOCK tsa_file_${RUN} # Needed to avoid access collision with offline unpacker
    RUN_SERIAL TRUE # Do not run in parallel to other tests in order to allow usage of threads
  )

endif()
