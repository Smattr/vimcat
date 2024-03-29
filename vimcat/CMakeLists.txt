add_executable(vimcat
  help.c
  main.c
  ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
)
target_link_libraries(vimcat PRIVATE libvimcat)

find_program(XXD xxd REQUIRED)
add_custom_command(
  OUTPUT manpage.c
  COMMAND ${XXD} -include vimcat.1 ${CMAKE_CURRENT_BINARY_DIR}/manpage.c
  MAIN_DEPENDENCY vimcat.1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

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

install(
  TARGETS vimcat
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/vimcat.1.gz
  DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)
