if(BBA_LOCAL_DIR)

  Add_Subdirectory(${BBA_LOCAL_DIR} bba_local)
  Set(BBA_INCLUDE_DIRECTORY ${BBA_LOCAL_DIR}/include PARENT_SCOPE)

else()

  download_project_if_needed(PROJECT           bba
                             GIT_REPOSITORY    "https://git.cbm.gsi.de/alignment/bba.git"
                             GIT_TAG           "e98ab50d4d1885a29abcf4af4e8bb8d722b02283"
                             SOURCE_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/bba
                             CONFIGURE_COMMAND ""
                             BUILD_COMMAND     ""
                             INSTALL_COMMAND   ""
  )
  Add_Subdirectory(bba)
  Set(BBA_INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bba/include PARENT_SCOPE)

endif()
