add_executable(vimcat main.c)
target_link_libraries(vimcat PRIVATE libvimcat)

find_program(GZIP gzip REQUIRED)
add_custom_target(man-vimcat
  ALL
  DEPENDS vimcat.1.gz
)
add_custom_command(
  OUTPUT vimcat.1.gz
  COMMAND ${GZIP} -9 --no-name --to-stdout ./vimcat.1
    >"${CMAKE_CURRENT_BINARY_DIR}/vimcat.1.gz"
  MAIN_DEPENDENCY vimcat.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
