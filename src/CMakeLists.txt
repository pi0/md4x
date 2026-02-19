
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")


# Build rules for MD4X parser library

configure_file(md4x.pc.in md4x.pc @ONLY)
add_library(md4x md4x.c md4x.h)
set_target_properties(md4x PROPERTIES
    COMPILE_FLAGS "-DMD4X_USE_UTF8"
    VERSION ${MD_VERSION}
    SOVERSION ${MD_VERSION_MAJOR}
    PUBLIC_HEADER md4x.h
)

# Build rules for HTML renderer library

configure_file(md4x-html.pc.in md4x-html.pc @ONLY)
add_library(md4x-html md4x-html.c md4x-html.h entity.c entity.h)
set_target_properties(md4x-html PROPERTIES
    VERSION ${MD_VERSION}
    SOVERSION ${MD_VERSION_MAJOR}
    PUBLIC_HEADER md4x-html.h
)
target_link_libraries(md4x-html md4x)

# Build rules for JSON renderer library

configure_file(md4x-json.pc.in md4x-json.pc @ONLY)
add_library(md4x-json md4x-json.c md4x-json.h)
set_target_properties(md4x-json PROPERTIES
    VERSION ${MD_VERSION}
    SOVERSION ${MD_VERSION_MAJOR}
    PUBLIC_HEADER md4x-json.h
)
target_link_libraries(md4x-json md4x)

# Build rules for ANSI renderer library

configure_file(md4x-ansi.pc.in md4x-ansi.pc @ONLY)
add_library(md4x-ansi md4x-ansi.c md4x-ansi.h entity.c entity.h)
set_target_properties(md4x-ansi PROPERTIES
    VERSION ${MD_VERSION}
    SOVERSION ${MD_VERSION_MAJOR}
    PUBLIC_HEADER md4x-ansi.h
)
target_link_libraries(md4x-ansi md4x)


# Install rules

install(
    TARGETS md4x
    EXPORT md4xConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_BINARY_DIR}/src/md4x.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(
    TARGETS md4x-html
    EXPORT md4xConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_BINARY_DIR}/src/md4x-html.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(
    TARGETS md4x-json
    EXPORT md4xConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_BINARY_DIR}/src/md4x-json.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(
    TARGETS md4x-ansi
    EXPORT md4xConfig
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_BINARY_DIR}/src/md4x-ansi.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(EXPORT md4xConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/md4x/ NAMESPACE md4x::)
