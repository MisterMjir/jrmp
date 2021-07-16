cmake_minimum_required(VERSION 3.10)

# Global settings
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

# Library
project(jrmp VERSION 0.0.0)

add_library(${PROJECT_NAME} SHARED
  ${CMAKE_CURRENT_LIST_DIR}/src/cleanup_stack.c
  ${CMAKE_CURRENT_LIST_DIR}/src/data.c
  ${CMAKE_CURRENT_LIST_DIR}/src/io.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PUBLIC m)

set_target_properties(${PROJECT_NAME} PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY lib
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION}
)

# Test
project(driver)

add_executable(${PROJECT_NAME}
  ${CMAKE_CURRENT_LIST_DIR}/driver/main.c
)

target_link_libraries(${PROJECT_NAME} PUBLIC jrmp)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY bin
)
