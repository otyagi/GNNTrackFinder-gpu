set(GEOMETRY_VERSION 3d14de4d5c891a22275bcb9f9d1e101214a90af9)
set(GEOMETRY_SRC_URL "https://git.cbm.gsi.de/CbmSoft/cbmroot_geometry.git")

download_project_if_needed(PROJECT         Geometry_source
                           GIT_REPOSITORY  ${GEOMETRY_SRC_URL}
                           GIT_TAG         ${GEOMETRY_VERSION}
                           SOURCE_DIR      ${CMAKE_SOURCE_DIR}/geometry
                           TEST_FILE       media.geo
                          )
If(DEFINED ENV{CHECK_GEO_HASH_CHANGE} AND DEFINED ENV{CI_MERGE_REQUEST_PROJECT_URL})
  Message( STATUS "Checking geo setup changes against $ENV{CI_MERGE_REQUEST_PROJECT_URL} "
                                                     "$ENV{CI_MERGE_REQUEST_TARGET_BRANCH_NAME}")
  execute_process(COMMAND "${CBMROOT_SOURCE_DIR}/scripts/check-geo-hash-changes.sh"
                           $ENV{CI_MERGE_REQUEST_PROJECT_URL}
                           $ENV{CI_MERGE_REQUEST_TARGET_BRANCH_NAME}
                  WORKING_DIRECTORY ${CBMROOT_SOURCE_DIR}
                  OUTPUT_VARIABLE checks_std_out)

  ##### Strip trailing new line in output
  Strip_Trailing_Char(${checks_std_out} "\n")
  Set( checks_std_out ${_ret_val} )
  #####

  # Transform output lines into list items
  string(REPLACE "\n" ";" found_checks_exports ${checks_std_out})

  # Loop on lines, find those with the pattern "CMAKE_EXPORT XXXXX=YYYYY" and use to set CMAKE variables
  foreach(_LINE IN LISTS found_checks_exports)
    if (_LINE MATCHES "CMAKE_EXPORT (.*)$")
      string(REGEX MATCHALL "\ (.*)$" _PAIR ${_LINE})  # Cut out the token, lead space left -_-'
      string(SUBSTRING ${_PAIR} 1 -1 _PAIR)  # Remove leading space
      if (_PAIR MATCHES "^([^:]+)=(.*)$")
        #### Replace temporary : with ; to make it a CMAKE list (used to avoid CMAKE errors in previous commands)
        Strip_Trailing_Char(${CMAKE_MATCH_2} ":")
        Set( CMAKE_MATCH_2 ${_ret_val} )
        string(REPLACE ":" ";" CMAKE_MATCH_2 ${CMAKE_MATCH_2})
        ####
        # Message( STATUS "Setting: ${CMAKE_MATCH_1} to ${CMAKE_MATCH_2}")
        set("${CMAKE_MATCH_1}" ${CMAKE_MATCH_2} PARENT_SCOPE)  # Need parent scope to share it out of "add_subdirectory"
      else()
        message(FATAL_ERROR "Invalid export pair: ${_PAIR}")
      endif()
    endif()
  endforeach()
endIf()
