set(INCLUDE_DIRECTORIES
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(CbmYamlInterface INTERFACE)

target_include_directories(CbmYamlInterface
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CbmYamlInterface
  INTERFACE GSL
            xpu 
            fmt::fmt 
            external::yaml-cpp
)

install(
  FILES 
    BaseTypes.h
    Property.h
    Yaml.h
  DESTINATION
    include/yaml/
)
