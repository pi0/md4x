
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")


# Build rules for md4c command line utility

include_directories("${PROJECT_SOURCE_DIR}/src")
add_executable(md4c-cli md4c-cli.c cmdline.c cmdline.h)
set_target_properties(md4c-cli PROPERTIES OUTPUT_NAME md4c)
target_link_libraries(md4c-cli md4c-html md4c-json md4c-ansi)


# Install rules

install(
    TARGETS md4c-cli
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "md4c.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
