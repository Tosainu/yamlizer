cmake_minimum_required(VERSION 3.9)

project(yamlizer CXX)

option(YAMLIZER_BUILD_EXAMPLES   "Build example files" ON)
option(YAMLIZER_BUILD_UNIT_TESTS "Build unit tests"    ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LibYAML REQUIRED IMPORTED_TARGET yaml-0.1)

if(NOT Boost_FOUND)
  find_package(Boost 1.61.0 REQUIRED)
endif()

if(NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 17)
  set(CMAKE_CXX_STANDARD  17)
endif()
set(CMAKE_CXX_EXTENSIONS  OFF)
set(CXX_STANDARD_REQUIRED ON)

add_library(yaml++
  src/yamlizer/yaml++.cc
  src/yamlizer/yaml++.h
)
target_link_libraries(yaml++
  PRIVATE   PkgConfig::LibYAML
  INTERFACE PkgConfig::LibYAML
)
target_include_directories(yaml++
  PRIVATE ${PROJECT_SOURCE_DIR}/src
)
add_library(yamlizer::yaml++ ALIAS yaml++)

add_library(yamlizer INTERFACE)
target_link_libraries(yamlizer
  INTERFACE Boost::boost yaml++
)
target_include_directories(yamlizer SYSTEM
  INTERFACE ${PROJECT_SOURCE_DIR}/src
)
add_library(yamlizer::yamlizer ALIAS yamlizer)

if(YAMLIZER_BUILD_UNIT_TESTS)
  if(NOT Boost_UNIT_TEST_FRAMEWORK_FOUND)
    find_package(Boost 1.61.0 COMPONENTS unit_test_framework REQUIRED)
  endif()
  enable_testing()

  add_executable(yamlizer-test test/test.cc)
  target_link_libraries(yamlizer-test
    yamlizer::yamlizer
    Boost::unit_test_framework
  )
  add_test(yamlizer-test yamlizer-test)
endif()

if(YAMLIZER_BUILD_EXAMPLES)
  add_executable(example example/example.cc)
  target_link_libraries(example yamlizer::yamlizer)
endif()
