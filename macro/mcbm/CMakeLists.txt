# start of test CBM setups from geometry/setup
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/mcbm/mcbm_transport.C)
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/mcbm/mcbm_digi.C)
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/mcbm/mcbm_reco_event.C)
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/mcbm/mcbm_match_check.C)
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/mcbm/mcbm_hadron_analysis.C)
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/mcbm/mcbm_qa.C)
#GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/mcbm/mcbm_check_materials.C)

Set(MACRO_DIR ${CMAKE_CURRENT_BINARY_DIR})
# ============================================================================

# Put the .rootrc file into the directory from which root is executed.
# Otherwise the rootalias file is not loaded
File(COPY ${CBMROOT_SOURCE_DIR}/macro/include/.rootrc DESTINATION ${CBMROOT_BINARY_DIR}/macro/mcbm)

# Define the number of events to run
Set(NumEvents 10)
Set(BeamAngle 25)
Set(randomSeed 1)

if(APPLE)
  set(simulationEngine kGeant4)
  set(sim_timeout 1800)
  set(reco_timeout 500)
else()
  set(simulationEngine kGeant3)
  set(sim_timeout 600)
  set(reco_timeout 300)
endif()

# Define the different setups to be tested
# Default should always be tested in all case
# additional ones for current/future setups should also be added to the CI tests
# Old ones may be tested in addition for nightly/weekly to ensure backward compatibility
# Nightly if for now not different from CI so the "extras" are used only in Weekly
If(${CBM_TEST_MODEL} MATCHES Weekly)
  List(APPEND cbm_setup mcbm_beam_2021_07_surveyed) # Default
  List(APPEND cbm_setup mcbm_beam_2022_05_23_nickel mcbm_beam_2022_06_16_gold mcbm_beam_2022_06_18_gold mcbm_beam_2022_03_28_uranium) # Default mCBM 2022 setups
  List(APPEND cbm_setup mcbm_beam_2024_03_22_gold mcbm_beam_2024_05_08_nickel) # Default mCBM 2024 setups
Else()
  List(APPEND cbm_setup mcbm_beam_2021_07_surveyed) # Default
  List(APPEND cbm_setup mcbm_beam_2022_05_23_nickel ) # Default, representative & complete example of mCBM 2022 setups
# disable  List(APPEND cbm_setup mcbm_beam_2024_05_08_nickel) # mCBM Mai 2024
EndIf()

# =====   Cleanup the data directory   =======================================
add_test(mcbm_cleanup ${CMAKE_COMMAND}
        -P ${CMAKE_SOURCE_DIR}/cmake/scripts/cleanmacrodir.cmake)
set_tests_properties(mcbm_cleanup PROPERTIES
        TIMEOUT 10
        FIXTURES_SETUP mcbm_cleanup
)
# ============================================================================


ForEach(setup IN LISTS cbm_setup)
  Message( STATUS "Adding test for simulation chain of mCBM setup ${setup}" )

  Set(testname mcbm_transport_${setup})
  Set(fixture_mcbm_transport fixture_${testname})
  Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_transport.sh ${NumEvents} \"${setup}\" \"data/${setup}_test\" \"\" kTRUE ${randomSeed} ${simulationEngine})
  Set_Tests_Properties(${testname} PROPERTIES
          TIMEOUT "${sim_timeout}"
          PASS_REGULAR_EXPRESSION "Test Passed;All ok"
          FIXTURES_REQUIRED mcbm_cleanup
          FIXTURES_SETUP ${fixture_mcbm_transport}
  )


  Set(testname mcbm_digi_event_${setup})
  Set(fixture_mcbm_digi_event fixture_${testname})
  Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_digi.sh ${NumEvents} \"data/${setup}_test\" 0. 0. 1)
  Set_Tests_Properties(${testname} PROPERTIES
          TIMEOUT "300"
          PASS_REGULAR_EXPRESSION "Test Passed;All ok"
          FIXTURES_REQUIRED ${fixture_mcbm_transport}
          FIXTURES_SETUP ${fixture_mcbm_digi_event}
  )

  Set(testname mcbm_reco_event_${setup})
  Set(fixture_mcbm_reco_event fixture_${testname})
  Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_reco_event.sh ${NumEvents} \"data/${setup}_test\" \"${setup}\")
  Set_Tests_Properties(${testname} PROPERTIES
          TIMEOUT "300"
          PASS_REGULAR_EXPRESSION "Test Passed;All ok"
          FIXTURES_REQUIRED ${fixture_mcbm_digi_event}
          FIXTURES_SETUP ${fixture_mcbm_reco_event}
  )

  Set(testname mcbm_match_check_${setup})
  Set(fixture_mcbm_match_check fixture_${testname})
  Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_match_check.sh ${NumEvents} \"${setup}\" \"data/${setup}_test\")
  Set_Tests_Properties(${testname} PROPERTIES
          TIMEOUT "300"
          PASS_REGULAR_EXPRESSION "Test Passed;All ok"
          FIXTURES_REQUIRED ${fixture_mcbm_reco_event}
          FIXTURES_SETUP ${fixture_mcbm_match_check}
  )

  Set(testname mcbm_hadron_analysis_${setup})
  Set(fixture_mcbm_hadron_analysis fixture_${testname})
  Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_hadron_analysis.sh ${NumEvents} \"${setup}\" \"data/${setup}_test\")
  Set_Tests_Properties(${testname} PROPERTIES
          TIMEOUT "300"
          PASS_REGULAR_EXPRESSION "Test Passed;All ok"
          FIXTURES_REQUIRED ${fixture_mcbm_match_check}
          FIXTURES_SETUP ${fixture_mcbm_hadron_analysis}
  )

  Set(testname mcbm_digi_${setup})
  Set(fixture_mcbm_digi_tb fixture_${testname})
  Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_digi.sh ${NumEvents} \"data/${setup}_test\")
  Set_Tests_Properties(${testname} PROPERTIES
          TIMEOUT "300"
          PASS_REGULAR_EXPRESSION "Test Passed;All ok"
          FIXTURES_REQUIRED ${fixture_mcbm_hadron_analysis}
          FIXTURES_SETUP ${fixture_mcbm_digi_tb}
  )

  If(${CBM_TEST_MODEL} MATCHES Weekly)
    Set(qa_benchmark "")
  Else()
    Set(qa_benchmark ${CBMROOT_SOURCE_DIR}/qa_data/defaults/${setup}_test.qa.root)
  EndIf()

  Set(testname mcbm_qa_event_mc_${setup})
  Set(fixture_mcbm_qa_event_mc fixture_${testname})
  Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_qa.sh ${NumEvents} \"data/${setup}_test\" \"${setup}\" kTRUE \"\" \"${qa_benchmark}\" )
  Set_Tests_Properties(${testname} PROPERTIES
          TIMEOUT "300"
          PASS_REGULAR_EXPRESSION "QA checks passed;Test Passed;All ok"
          FAIL_REGULAR_EXPRESSION "ERROR"
          FAIL_REGULAR_EXPRESSION "FATAL"
          FAIL_REGULAR_EXPRESSION "QA checks failed"
          FIXTURES_REQUIRED ${fixture_mcbm_digi_tb}
          FIXTURES_SETUP ${fixture_mcbm_qa_event_mc}
  )

  #Set(testname mcbm_reco_${setup})
  #Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/mcbm/mcbm_reco.sh ${NumEvents} ${BeamAngle} \"${setup}\")
  #Set_Tests_Properties(${testname} PROPERTIES TIMEOUT "600")
  #Set_Tests_Properties(${testname} PROPERTIES PASS_REGULAR_EXPRESSION "Test Passed;All ok")
  #Set_Tests_Properties(${testname} PROPERTIES DEPENDS ${dep_mc})
  #Set(_FileName_Dependency data/mcbm_mc_${setup}_ok)
  #Set_Tests_Properties(${testname} PROPERTIES REQUIRED_FILES ${_FileName_Dependency})

EndForEach(setup IN LISTS cbm_setup)

Install(FILES .rootrc mcbm_transport.C mcbm_digi.C mcbm_reco_event.C mcbm_match_check.C mcbm_hadron_analysis.C mcbm_qa.C
        mcbm_transport_boxgen.C mcbm_hadron_kfp_ana.C
        DESTINATION share/cbmroot/macro/mcbm
       )
Install(DIRECTORY modules DESTINATION share/cbmroot/macro/mcbm)
Install(CODE "FILE(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/cbmroot/macro/mcbm/data)")
