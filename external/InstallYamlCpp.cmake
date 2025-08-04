set(YAMLCPP_VERSION 0579ae3d976091d7d664aa9d2527e0d0cff25763) # version 0.7.0

set(YAMLCPP_SRC_URL "https://github.com/jbeder/yaml-cpp")
set(YAMLCPP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/yaml-cpp")
set(YAMLCPP_INCLUDE_DIR "${YAMLCPP_SRC_DIR}/include")
set(YAMLCPP_DESTDIR "${CMAKE_CURRENT_BINARY_DIR}/yaml-cpp-prefix")
set(YAMLCPP_BYPRODUCT "${YAMLCPP_DESTDIR}/src/yaml-cpp-build/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

download_project_if_needed(PROJECT  yaml-cpp
        GIT_REPOSITORY  ${YAMLCPP_SRC_URL}
        GIT_TAG         ${YAMLCPP_VERSION}
        SOURCE_DIR      ${YAMLCPP_SRC_DIR}
        TEST_FILE       CMakeLists.txt
        )

If(ProjectUpdated)
    File(REMOVE_RECURSE ${YAMLCPP_DESTDIR})
    Message("yaml-cpp source directory was changed so build directory was deleted")
EndIf()

if ("${CMAKE_GENERATOR}" MATCHES "Make")
    set(YAMLCPP_BUILD_COMMAND "$(MAKE)")
else()
    set(YAMLCPP_BUILD_COMMAND "${CMAKE_COMMAND} --build . --target")
endif()

ExternalProject_Add(
  yaml-cpp
  SOURCE_DIR ${YAMLCPP_SRC_DIR}
  GIT_CONFIG advice.detachedHead=false
  BUILD_IN_SOURCE 0
  LOG_DOWNLOAD 1 LOG_CONFIGURE 1 LOG_BUILD 1 LOG_INSTALL 1
  CMAKE_ARGS -DYAML_CPP_BUILD_CONTRIB=OFF
             -DYAML_CPP_BUILD_TOOLS=OFF
             -DYAML_CPP_BUILD_TESTS=OFF
             -DYAML_BUILD_SHARED_LIBS=OFF
             -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  BUILD_COMMAND ${YAMLCPP_BUILD_COMMAND} yaml-cpp
  BUILD_BYPRODUCTS ${YAMLCPP_BYPRODUCT}
  INSTALL_COMMAND ""
)

# pre-create empty directory to make INTERFACE_INCLUDE_DIRECTORIES happy
file(MAKE_DIRECTORY ${YAMLCPP_INCLUDE_DIR})

add_library(external::yaml-cpp STATIC IMPORTED GLOBAL)
add_dependencies(external::yaml-cpp yaml-cpp)
set_target_properties(external::yaml-cpp PROPERTIES
  IMPORTED_LOCATION ${YAMLCPP_BYPRODUCT}
  INTERFACE_INCLUDE_DIRECTORIES ${YAMLCPP_INCLUDE_DIR}
)
