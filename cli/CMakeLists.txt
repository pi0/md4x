
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")


# Build rules for md4x command line utility

include_directories("${PROJECT_SOURCE_DIR}/src")
add_executable(md4x-cli md4x-cli.c cmdline.c cmdline.h)
set_target_properties(md4x-cli PROPERTIES OUTPUT_NAME md4x)
target_link_libraries(md4x-cli md4x-html md4x-json md4x-ansi)


# Install rules

install(
    TARGETS md4x-cli
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "md4x.1" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
