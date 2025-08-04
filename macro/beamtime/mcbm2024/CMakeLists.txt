
# ROOT macros, par files, documentation files, bash scripts
Install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION share/cbmroot/macro/beamtime
        FILES_MATCHING PATTERN "*.C"
                       PATTERN "*.par"
                       PATTERN "*.md"
                       PATTERN "*.sh"
       )
# # RICH calibration file, resolve symlink to get the full file
# get_filename_component(_resolvedRichFile ${CMAKE_CURRENT_SOURCE_DIR}/icd_offset_it_0.data REALPATH)
# Install(FILES ${_resolvedRichFile}
#         DESTINATION share/cbmroot/macro/beamtime/mcbm2022
#        )

# SLURM scripts, bash scripts
Install(DIRECTORY online
        DESTINATION share/cbmroot/macro/beamtime/mcbm2024
        FILES_MATCHING PATTERN "*.sbatch"
                       PATTERN "*.sh"
        )

# Just the empty folder for output
Install(DIRECTORY data
        DESTINATION share/cbmroot/macro/beamtime/mcbm2024
        PATTERN "*" EXCLUDE)


# =====   mCBM 2021 tests   ==================================================
# ====> only if test TSA files for "typical runs" present
If(DEFINED ENV{RAW_DATA_PATH} )
  SET( RAW_DATA_PATH $ENV{RAW_DATA_PATH} )
  Message( STATUS "Raw data path provided => Performing mCBM 2024 tests using tsa files in ${RAW_DATA_PATH}" )

  # =====   Cleanup the data directory   =======================================
  add_test(mcbm_2024_cleanup ${CMAKE_COMMAND}
           -P ${CMAKE_SOURCE_DIR}/cmake/scripts/cleanmacrodir.cmake)
  set_tests_properties(mcbm_2024_cleanup PROPERTIES
    TIMEOUT 200
    FIXTURES_SETUP mcbm_2024_cleanup
  )
  # ============================================================================

  # =====   Copy the .rootrc file into the directory from which root is executed
  # --- Otherwise the rootalias file is not loaded
  file(COPY ${CBMROOT_SOURCE_DIR}/macro/include/.rootrc
       DESTINATION ${CBMROOT_BINARY_DIR}/macro/beamtime/mcbm2024)
  # ============================================================================

  # =====   Create test script with all environment needed for the reco chain script
  GENERATE_TEST_SCRIPT(${CBMROOT_SOURCE_DIR}/macro/beamtime/mcbm2024/reco_mcbm.sh)
  # ============================================================================

  ## Run only if raw data files present: 2024
  # =====>   Set the timeouts and data range depending on test model (profiling = slow)
  if(${CBM_TEST_MODEL} MATCHES MergeRequest OR ${CBM_TEST_MODEL} MATCHES Continuous )
    Message( STATUS "MR or Continuous model detected, decreasing event nb for mCBM 2024 reco tests to single one." )
    SET( RECO_TS_NB 11)
    SET( RAW_DATA_UNPACK_TO 100) # MR mode on run4: 80s
    SET( DIGI_RECO_TO 95) # MR mode on run4: 60s
    SET( RECO_QA_TO 135) # MR mode on run4: 70
    List(APPEND mcbm_2024_runs 3105)  # Run CI only on 3105 for now
  elseif(${CBM_TEST_MODEL} MATCHES Weekly OR ${CBM_TEST_MODEL} MATCHES Profile )
    Message( STATUS "Profiling model detected, increasing timeout and event nb for mCBM 2024 tests with coverage." )
    SET( RECO_TS_NB 20)
    SET( RAW_DATA_UNPACK_TO 1200) # Weekly mode on virgo: ?? s
    SET( DIGI_RECO_TO 1400) # Weekly mode on virgo: ?? s
    SET( RECO_QA_TO 1500) # Weekly mode on virgo: ?? s
    List(APPEND mcbm_2024_runs 3105)  # Run CI only on 3105 for now
  else()
    SET( RECO_TS_NB 13)
    SET( RAW_DATA_UNPACK_TO 200) # MR mode on run4: 80s, spiil start at TS 10
    SET( DIGI_RECO_TO 180) # MR mode on run4: 60s, spiil start at TS 10
    SET( RECO_QA_TO 200) # MR mode on run4: 70, spiil start at TS 10
    List(APPEND mcbm_2024_runs 3105)  # Run CI only on 3105 for now
  endif()

  # ====================================================================
  ForEach(RUN IN LISTS mcbm_2024_runs)
    Message( STATUS "Checking if raw data file exists: ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa" )
    If(EXISTS ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa)
      Message( STATUS "Raw data file found, tests will be performed for mCBM run ${RUN}" )

      ### Complete reco chain script by S. Zharko
      Set(testname mcbm_2024_unp_${RUN})
      Set(fixture_unp_${RUN} fixture_done_${testname})
      Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/beamtime/mcbm2024/reco_mcbm.sh
            --tsa ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa
            --nts ${RECO_TS_NB}
            --setup --unpack
            )
      Set_Tests_Properties(${testname} PROPERTIES
            TIMEOUT ${RAW_DATA_UNPACK_TO}
            FAIL_REGULAR_EXPRESSION "segmentation violation"
            PASS_REGULAR_EXPRESSION "Reconstruction of ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa succeeded\n"
            FIXTURES_REQUIRED mcbm_2024_cleanup
            FIXTURES_SETUP ${fixture_unp_${RUN}}
            RESOURCE_LOCK tsa_file_${RUN} # Needed to avoid access collision with eventual online unpacker tests
            )

      Set(testname mcbm_2024_rec_${RUN})
      Set(fixture_rec_${RUN} fixture_done_${testname})
      Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/beamtime/mcbm2024/reco_mcbm.sh
            --tsa ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa
            --nts ${RECO_TS_NB}
            --reco
            )
      Set_Tests_Properties(${testname} PROPERTIES
            TIMEOUT ${DIGI_RECO_TO}
            FAIL_REGULAR_EXPRESSION "segmentation violation"
            PASS_REGULAR_EXPRESSION "Reconstruction of ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa succeeded\n"
            FIXTURES_REQUIRED ${fixture_unp_${RUN}}
            FIXTURES_SETUP ${fixture_rec_${RUN}}
            )

      Set(testname mcbm_2024_qa_${RUN})
      Set(fixture_qa_${RUN} fixture_done_${testname})
      Add_Test(${testname} ${CBMROOT_BINARY_DIR}/macro/beamtime/mcbm2024/reco_mcbm.sh
            --tsa ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa
            --nts ${RECO_TS_NB}
            --qa --qa-module
            )
      Set_Tests_Properties(${testname} PROPERTIES
            TIMEOUT ${RECO_QA_TO}
            FAIL_REGULAR_EXPRESSION "segmentation violation"
            PASS_REGULAR_EXPRESSION "Reconstruction of ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa succeeded\n"
            FIXTURES_REQUIRED ${fixture_rec_${RUN}}
            FIXTURES_SETUP ${fixture_qa_${RUN}}
            )

    EndIf() # If(EXISTS ${RAW_DATA_PATH}/${RUN}_first20Ts.tsa))
  EndForEach() # ForEach(RUN 2391 2488 )
EndIf() # If(DEFINED ENV{RAW_DATA_PATH} )
