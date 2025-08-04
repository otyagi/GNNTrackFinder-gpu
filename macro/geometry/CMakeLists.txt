# =====   Generate the needed shell scripts   ================================
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/geometry/examine_materials.C)
GENERATE_ROOT_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/geometry/check_overlaps.C)

Set(MACRO_DIR ${CMAKE_CURRENT_BINARY_DIR})
# ============================================================================


# =====   Copy the .rootrc file into the directory from which root is executed
# --- Otherwise the rootalias file is not loaded
file(COPY ${CBMROOT_SOURCE_DIR}/macro/include/.rootrc
     DESTINATION ${CBMROOT_BINARY_DIR}/macro/run
    )
# ============================================================================

Set(timeOutTime 150)

# =====   Cleanup the data and execution directories   =======================
add_test(geo_cleanup ${CMAKE_COMMAND}
	-P ${CMAKE_SOURCE_DIR}/cmake/scripts/cleanmacrodir.cmake)
set_tests_properties(geo_cleanup PROPERTIES
	TIMEOUT ${timeOutTime}
	FIXTURES_SETUP geo_cleanup
)
# ============================================================================

# =====   Define tests for each setup   ======================================
file(GLOB setup_files
     LIST_DIRECTORIES false
     ${CBMROOT_SOURCE_DIR}/geometry/setup/setup_*.C
    )

SET( CHECK_SETUPS FALSE )
If(${CBM_TEST_MODEL} MATCHES Nightly OR ${CBM_TEST_MODEL} MATCHES Weekly)
  SET( CHECK_SETUPS TRUE )
  Message( STATUS "Enabling tests on geometry setups consistency (materials and overlaps) for Nightly/Weekly build")
elseIf(${GEO_SETUPS_CHANGE})
  SET( CHECK_SETUPS TRUE )
  # Use list of modified setups from script by default, but keep option for "full check" w/ only GEO_SETUPS_CHANGE=1
  If(DEFINED CHANGED_SETUPS)
    SET(setup_files ${CHANGED_SETUPS})
  endIf()
  Message( STATUS "Change in geo repo hash => Enabling tests on geometry setups consistency (materials and overlaps)")
  # Message( STATUS "Change in geo repo hash => Found following setups to test: ${setup_files}")
else()
  Message( STATUS "No checks booked over geo setups")
endIf()

If(${CHECK_SETUPS} AND setup_files)
    ForEach(setup IN LISTS setup_files)
        string(REPLACE setup/setup_ "" setup ${setup})
        string(REPLACE ${CBMROOT_SOURCE_DIR}/geometry/ "" setup ${setup})  # Needed only with "all setups list"
        string(REPLACE .C "" setup ${setup})
        Message("Change in geo repo hash => Found following setups to test: ${setup}")

        # --- Test setup materials
        # --- Check that materials in a loaded geometry (1 evt tra run) match corresponding single det geo files,
        #     using check_materials.C
        set(testname geo_check_mats_${setup})
        add_test(${testname} ${MACRO_DIR}/examine_materials.sh \"${setup}\")
        set_tests_properties(${testname} PROPERTIES
            TIMEOUT ${timeOutTime}
            FAIL_REGULAR_EXPRESSION "FAILED;issues with materials were detected"
            PASS_REGULAR_EXPRESSION "SUCCESS;Excluding air and dummy materials, no material errors were detected"
            FIXTURES_REQUIRED geo_cleanup
            FIXTURES_SETUP fixt_check_mats_${setup}
        )

        set(testname geo_setup_overlaps_${setup})
        add_test(${testname} ${MACRO_DIR}/check_overlaps.sh \"data/examine_${setup}\")
        set_tests_properties(${testname} PROPERTIES
            TIMEOUT 600
            FAIL_REGULAR_EXPRESSION "segmentation violation"
            PASS_REGULAR_EXPRESSION "Test Passed;All ok"
            FIXTURES_REQUIRED fixt_check_mats_${setup}
            FIXTURES_SETUP fixt_check_overlaps_${setup}
        )
    endForEach(setup IN LISTS setup_files)
endIf()
# ============================================================================

# Example of usage to check overlaps in aligned setup
if(${CBM_TEST_MODEL} MATCHES Nightly OR ${CBM_TEST_MODEL} MATCHES Weekly)
    set(setup mcbm_beam_2022_05_23_nickel)
    set(align_file ${CBMROOT_SOURCE_DIR}/parameters/mcbm/AlignmentMatrices_mcbm_beam_2022_05_23_nickel.root)
    set(testname geo_setup_overlaps_align_${setup})
    add_test(${testname} ${MACRO_DIR}/check_overlaps.sh \"data/examine_${setup}\" \"${align_file}\")
    set_tests_properties(${testname} PROPERTIES
        TIMEOUT 600
        FAIL_REGULAR_EXPRESSION "segmentation violation"
        PASS_REGULAR_EXPRESSION "Test Passed;All ok"
        FIXTURES_REQUIRED fixt_check_overlaps_${setup}
        FIXTURES_SETUP fixt_check_overlaps_align_${setup}
    )
endIf()
# ============================================================================

Install(FILES .rootrc examine_materials.C check_overlaps.C
        DESTINATION share/cbmroot/macro/geometry
       )
Install(CODE "FILE(MAKE_DIRECTORY \${CMAKE_INSTALL_PREFIX}/share/cbmroot/macro/geometry/data)")
