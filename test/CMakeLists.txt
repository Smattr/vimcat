add_executable(test_version_le test_version_le.c)
target_link_libraries(test_version_le PRIVATE libvimcat)

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_FOUND)
  return()
endif()

execute_process(
  COMMAND ${Python3_EXECUTABLE} -c "import pytest"
  RESULT_VARIABLE IMPORT_PYTEST_RET
  OUTPUT_QUIET
  ERROR_QUIET
)
if(NOT ${IMPORT_PYTEST_RET} EQUAL 0)
  return()
endif()

add_custom_target(check
  COMMAND env
    PATH=${CMAKE_BINARY_DIR}/vimcat:${CMAKE_BINARY_DIR}/test:$ENV{PATH}
    ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/tests.py
    --verbose)
add_dependencies(check test_version_le vimcat)
