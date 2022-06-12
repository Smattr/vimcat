add_library(libvimcat
  src/buffer.c
  src/debug.c
  src/get_environ.c
  src/read.c
  src/term.c
  ${CMAKE_CURRENT_BINARY_DIR}/version.c
  src/version_le.c)

if(APPLE)
  target_compile_options(libvimcat PRIVATE -fno-common)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
  OUTPUT version.c
  COMMAND src/make-version.py ${CMAKE_CURRENT_BINARY_DIR}/version.c
  MAIN_DEPENDENCY src/make-version.py
  DEPENDS always_run
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# dummy output to make sure we always re-evaluate the version step above
add_custom_command(
  OUTPUT always_run
  COMMAND /usr/bin/env true)

target_include_directories(libvimcat
  PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE
  src)

set_target_properties(libvimcat PROPERTIES PREFIX "")

install(TARGETS libvimcat EXPORT LibvimcatConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

export(TARGETS libvimcat FILE LibvimcatConfig.cmake)
