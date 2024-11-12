cmake_minimum_required(VERSION 3.14)
project(MaaDesktopIntegration C)

find_package(Fontconfig REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GDK_PIXBUF REQUIRED IMPORTED_TARGET gdk-pixbuf-2.0)
pkg_check_modules(LIBNOTIFY REQUIRED IMPORTED_TARGET libnotify)

add_library(MaaDesktopIntegration_SPEC IMPORTED STATIC)
set_target_properties(MaaDesktopIntegration_SPEC PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/MaaDesktopIntegration.spec")

add_library(MaaDesktopIntegration SHARED
    notification.c
    main_loop.c
    gobject.c
    gdk-pixbuf.c
    fontconfig.c
    MaaDesktopIntegration.spec
)

set_target_properties(MaaDesktopIntegration PROPERTIES
    NO_SONAME TRUE
    PREFIX ""
    OUTPUT_NAME "MaaDesktopIntegration"
    SUFFIX ".so"
)
target_link_libraries(MaaDesktopIntegration PkgConfig::GLIB PkgConfig::GDK_PIXBUF PkgConfig::LIBNOTIFY Fontconfig::Fontconfig MaaDesktopIntegration_SPEC)

install(TARGETS MaaDesktopIntegration LIBRARY DESTINATION ".")
