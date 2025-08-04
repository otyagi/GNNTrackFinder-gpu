# Check if cmake has the required version
# Since flesnet requires 3.14.0 we should be consistent while we
# build it in the CbmRoot context
cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
cmake_policy(VERSION 3.14...3.23)

Set(CTEST_SOURCE_DIRECTORY $ENV{SOURCEDIR})
Set(CTEST_BINARY_DIRECTORY $ENV{BUILDDIR})
Set(CTEST_SITE $ENV{SITE})
Set(CTEST_BUILD_NAME $ENV{LABEL})
Set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
Set(CTEST_PROJECT_NAME "CBMROOT")
Set(EXTRA_FLAGS $ENV{EXTRA_FLAGS})
Set(INSTALL_PROJECT $ENV{INSTALL_PROJECT})

if(NOT CBM_TEST_MODEL)
  set(CBM_TEST_MODEL NIGHTLY)
endif()

include(${CTEST_SOURCE_DIRECTORY}/CTestConfig.cmake)
Ctest_Read_Custom_Files("${CTEST_SOURCE_DIRECTORY}")

Set(CTEST_UPDATE_COMMAND "git")

Set(BUILD_COMMAND "make")
Set(CTEST_BUILD_COMMAND "${BUILD_COMMAND} -i -k -j$ENV{number_of_processors}")

# Extract the FairRoot version from fairroot-config
# The version info is of the form Major.Minor.Patch e.g. 15.11.1 and
# is stored in the variable FairRoot_VERSION
#Set(CMAKE_MODULE_PATH "${CTEST_SOURCE_DIRECTORY}/cmake/modules" ${CMAKE_MODULE_PATH})
#set(FAIRROOTPATH $ENV{FAIRROOTPATH})
#Include(CbmMacros)
#FairRootVersion()

message("Compiling with $ENV{number_of_processors} jobs in parallel.")
message("Testing with $ENV{number_of_processors_for_test} jobs in parallel.")

Set(CTEST_USE_LAUNCHERS 1)

If(${CBM_TEST_MODEL} MATCHES MergeRequest OR ${CBM_TEST_MODEL} MATCHES Continuous)
  Set(_BuildType NIGHTLY)
  Set(_CMakeModel Continuous)
elseIf(${CBM_TEST_MODEL} MATCHES Weekly OR ${CBM_TEST_MODEL} MATCHES Profile)
  Set(_BuildType PROFILE)
  Set(_CMakeModel Nightly)
Else()
  String(TOUPPER ${CBM_TEST_MODEL} _BuildType)
  set(_CMakeModel ${CBM_TEST_MODEL})
EndIf()

If(EXTRA_FLAGS AND INSTALL_PROJECT)
  Set(CTEST_CONFIGURE_COMMAND " \"${CMAKE_EXECUTABLE_NAME}\" \"-G${CTEST_CMAKE_GENERATOR}\" \"-DCBM_TEST_MODEL=${CBM_TEST_MODEL}\" \"-DCMAKE_BUILD_TYPE=${_BuildType}\" \"-DCTEST_USE_LAUNCHERS=${CTEST_USE_LAUNCHERS}\" \"${EXTRA_FLAGS}\" \"-DCMAKE_INSTALL_PREFIX=${CTEST_SOURCE_DIRECTORY}/install\" \"${CTEST_SOURCE_DIRECTORY}\" ")
ElseIf(EXTRA_FLAGS)
  Set(CTEST_CONFIGURE_COMMAND " \"${CMAKE_EXECUTABLE_NAME}\" \"-G${CTEST_CMAKE_GENERATOR}\" \"-DCBM_TEST_MODEL=${CBM_TEST_MODEL}\" \"-DCMAKE_BUILD_TYPE=${_BuildType}\" \"-DCTEST_USE_LAUNCHERS=${CTEST_USE_LAUNCHERS}\" \"${EXTRA_FLAGS}\" \"${CTEST_SOURCE_DIRECTORY}\" ")
Else()
  Set(CTEST_CONFIGURE_COMMAND " \"${CMAKE_EXECUTABLE_NAME}\" \"-G${CTEST_CMAKE_GENERATOR}\" \"-DCBM_TEST_MODEL=${CBM_TEST_MODEL}\" \"-DCMAKE_BUILD_TYPE=${_BuildType}\" \"-DCTEST_USE_LAUNCHERS=${CTEST_USE_LAUNCHERS}\" \"${CTEST_SOURCE_DIRECTORY}\" ")
EndIf()

#If(${CBM_TEST_MODEL} MATCHES Nightly OR ${CBM_TEST_MODEL} MATCHES Weekly OR ${CBM_TEST_MODEL} MATCHES Profile)
If(NOT ${_BuildType} MATCHES EXPERIMENTAL)

  Find_Program(GCOV_COMMAND gcov)
  If(GCOV_COMMAND)
    Message("Found GCOV: ${GCOV_COMMAND}")
    Set(CTEST_COVERAGE_COMMAND ${GCOV_COMMAND})
  EndIf(GCOV_COMMAND)

  CTEST_EMPTY_BINARY_DIRECTORY(${CTEST_BINARY_DIRECTORY})

EndIf()

Ctest_Start(${_CMakeModel})

unset(repeat)
if(${_CMakeModel} MATCHES Continuous)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.17)
    set(repeat REPEAT UNTIL_PASS:2)
  endif()
EndIf()

# The stop time should be a date compatible string without day nor timezone
# Examples:
# => CTEST_END_TIME_LIMIT=`date -d "${END_TIME}CET -5minutes" +"%H:%M:%S"`
# => CTEST_END_TIME_LIMIT=`date -d "now +25minutes" +"%H:%M:%S"`
unset(stop_time)
If(DEFINED ENV{CTEST_END_TIME_LIMIT})
  set(stop_time STOP_TIME "$ENV{CTEST_END_TIME_LIMIT}")
  message(STATUS " End time for the ctest test runs set to ${stop_time}")
EndIf()

If(NOT ${_BuildType} MATCHES EXPERIMENTAL)
  Ctest_Update(SOURCE "${CTEST_SOURCE_DIRECTORY}")
EndIf()

Ctest_Configure(BUILD "${CTEST_BINARY_DIRECTORY}"
                RETURN_VALUE _RETVAL
)

If(NOT _RETVAL)
  Ctest_Build(BUILD "${CTEST_BINARY_DIRECTORY}"
              NUMBER_ERRORS _NUM_ERROR
             )

  If(${_CMakeModel} MATCHES Continuous)
    CTest_Submit(PARTS Update Configure Build
                 BUILD_ID cdash_build_id
                )
    if(${_NUM_ERROR} GREATER 0)
      message(STATUS " ")
      message(STATUS " You can find the produced results on the CDash server")
      message(STATUS " ")
      message(STATUS " CDash Build Summary ..: "
              "${CTEST_DROP_METHOD}://${CTEST_DROP_SITE}/buildSummary.php?buildid=${cdash_build_id}"
             )
      message(STATUS " CDash Test List ......: "
              "${CTEST_DROP_METHOD}://${CTEST_DROP_SITE}/viewTest.php?buildid=${cdash_build_id}"
             )
      message(STATUS " ")
      Message(STATUS "Build finished with ${_NUM_ERROR} errors")
      message(FATAL_ERROR "Compilation failure")
    endif()
  EndIf()

  If(DEFINED ENV{CI_TEST_STAGE_TOTAL_TIME_LIMIT})
    # Compute test stage timeout based on current time + limit from environment variable
    If(DEFINED ENV{CTEST_END_TIME_LIMIT})
      # If also full DASH run timeout defined, also compare the two and keep the earliest one
      execute_process (
        COMMAND scripts/find_citests_ctest_stop_time.sh $ENV{CTEST_END_TIME_LIMIT}
        OUTPUT_VARIABLE CI_TESTS_TOTAL_END_TIME_DATE
      )
    Else()
      # Else just get the CI jpb timeout
      execute_process (
        COMMAND scripts/find_citests_ctest_stop_time.sh
        OUTPUT_VARIABLE CI_TESTS_TOTAL_END_TIME_DATE
      )
    EndIf()
    set(stop_time STOP_TIME "${CI_TESTS_TOTAL_END_TIME_DATE}")
    message(STATUS "Due to CI mode, Set end time for the ctest test runs to ${stop_time}")
  EndIf()

  Ctest_Test(BUILD "${CTEST_BINARY_DIRECTORY}"
             PARALLEL_LEVEL $ENV{number_of_processors_for_test}
             ${repeat}
             ${stop_time}
             RETURN_VALUE _ctest_test_ret_val
            )

  If(${_CMakeModel} MATCHES Continuous)
    CTest_Submit(PARTS Test
                 BUILD_ID cdash_build_id
                )
  EndIf()

  If(GCOV_COMMAND)
    Ctest_Coverage(BUILD "${CTEST_BINARY_DIRECTORY}")
    If(${_CMakeModel} MATCHES Continuous)
      CTest_Submit(PARTS Coverage
                   BUILD_ID cdash_build_id
                  )
    EndIf()
  EndIf()

  Ctest_Upload(FILES ${CTEST_NOTES_FILES})
  If(NOT ${_CMakeModel} MATCHES Continuous)
    Ctest_Submit(BUILD_ID cdash_build_id)
  EndIf()

  If(EXTRA_FLAGS MATCHES "CBM_TEST_INSTALL" AND NOT _ctest_test_ret_val)
#  If(EXTRA_FLAGS MATCHES "INSTALL_PREFIX" AND EXTRA_FLAGS MATCHES "CBM_TEST_INSTALL" AND NOT _ctest_test_ret_val)
    Message("Testing Installation")
    execute_process(COMMAND ${BUILD_COMMAND} install -j$ENV{number_of_processors} WORKING_DIRECTORY ${CTEST_BINARY_DIRECTORY}
                    RESULTS_VARIABLE _install_ret_value
                   )
    if (NOT _install_ret_value)
      execute_process(COMMAND ${CMAKE_EXECUTABLE_NAME} -E rm -R build MQ algo analysis core external fles mvd reco sim
                      WORKING_DIRECTORY ${CTEST_SOURCE_DIRECTORY}
                     )
      message("executing test suite in ${CTEST_BINARY_DIRECTORY}/install")
      execute_process(COMMAND ${CTEST_SOURCE_DIRECTORY}/cmake/scripts/execute_installation_test.sh ${CTEST_SOURCE_DIRECTORY}/install
                      RESULTS_VARIABLE _install_ret_value
                     )
    endif()
  Else()
     # if installation isn't tested the return value should be 0
     set(_install_ret_value false)
  EndIf()

  Message("_ctest_test_ret_val: ${_ctest_test_ret_val}")
  Message("_install_ret_value: ${_install_ret_value}")

  # Pipeline should fail also in case of failed tests
  if (_ctest_test_ret_val OR _install_ret_value)
    message(STATUS " ")
    message(STATUS " You can find the produced results on the CDash server")
    message(STATUS " ")
    message(STATUS " CDash Build Summary ..: "
            "${CTEST_DROP_METHOD}://${CTEST_DROP_SITE}/buildSummary.php?buildid=${cdash_build_id}"
           )
    message(STATUS " CDash Test List ......: "
            "${CTEST_DROP_METHOD}://${CTEST_DROP_SITE}/viewTest.php?buildid=${cdash_build_id}"
           )
    message(STATUS " ")
    Message(FATAL_ERROR "Some tests failed.")
  endif()

Else()
  CTest_Submit(BUILD_ID cdash_build_id)
EndIf()

message(STATUS " ")
message(STATUS " You can find the produced results on the CDash server")
message(STATUS " ")
message(STATUS " CDash Build Summary ..: "
        "${CTEST_DROP_METHOD}://${CTEST_DROP_SITE}/buildSummary.php?buildid=${cdash_build_id}"
       )
message(STATUS " CDash Test List ......: "
        "${CTEST_DROP_METHOD}://${CTEST_DROP_SITE}/viewTest.php?buildid=${cdash_build_id}"
       )
message(STATUS " ")
