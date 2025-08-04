
# ROOT macros, par files, documentation files, bash scripts
Install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DESTINATION share/cbmroot/macro/beamtime
        FILES_MATCHING PATTERN "*.C"
                       PATTERN "*.par"
                       PATTERN "*.sh"
       )

# Just the empty folder for output
Install(DIRECTORY data
        DESTINATION share/cbmroot/macro/beamtime/mcbm2021
        PATTERN "*" EXCLUDE)
