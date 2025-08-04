set(POOLSTL_VERSION 5cf834a1625b4c0cb29785ec6e55280343e25436) # v0.3.5 - 2024-05-02
set(POOLSTL_REPO https://github.com/alugowski/poolSTL.git)
set(POOLSTL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/poolSTL)
set(POOLSTL_INCLUDE_DIRS ${POOLSTL_SRC_DIR}/include)

download_project_if_needed(PROJECT poolstl
  GIT_REPOSITORY ${POOLSTL_REPO}
  GIT_TAG ${POOLSTL_VERSION}
  SOURCE_DIR ${POOLSTL_SRC_DIR}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)

# Can't use add_subdirectory because older cmake versions don't set SYSTEM property to avoid warnings
# add_subdirectory(${POOLSTL_SRC_DIR} SYSTEM)
add_library(poolstl INTERFACE)
target_include_directories(poolstl SYSTEM INTERFACE
  $<BUILD_INTERFACE:${POOLSTL_INCLUDE_DIRS}>
  $<INSTALL_INTERFACE:include>
)

Install(DIRECTORY ${POOLSTL_INCLUDE_DIRS}/poolstl
        DESTINATION include
       )
