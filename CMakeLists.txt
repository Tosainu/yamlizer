cmake_minimum_required(VERSION 3.5)

project(yamlizer CXX)

option(YAMLIZER_BUILD_EXAMPLES   "Build example files" ON)
option(YAMLIZER_BUILD_UNIT_TESTS "Build unit tests"    ON)

if(NOT LibYAML_FOUND)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules/")
  find_package(LibYAML REQUIRED)
endif()

set(CMAKE_CXX_EXTENSIONS  OFF)
set(CMAKE_CXX_STANDARD    14)
set(CXX_STANDARD_REQUIRED ON)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
  ${LibYAML_INCLUDE_DIRS}
)

file(GLOB_RECURSE SRCS src/*.cc)
add_library(yamlizer ${SRCS})

if(YAMLIZER_BUILD_UNIT_TESTS)
  if(NOT Boost_UNIT_TEST_FRAMEWORK_FOUND)
    find_package(Boost 1.59.0 COMPONENTS unit_test_framework REQUIRED)
  endif()
  enable_testing()

  add_executable(yamlizer-test test/test.cc)
  target_link_libraries(yamlizer-test
    yamlizer
    ${LibYAML_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  add_test(yamlizer-test yamlizer-test)
endif()

if(YAMLIZER_BUILD_EXAMPLES)
  add_executable(example example/example.cc)
  target_link_libraries(example
    yamlizer
    ${LibYAML_LIBRARY})
endif()
