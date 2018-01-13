cmake_minimum_required(VERSION 3.5)

project(yamlizer CXX)

if(NOT LibYAML_FOUND)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules/")
  find_package(LibYAML REQUIRED)
endif()
