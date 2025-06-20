include(FindPkgConfig)

pkg_check_modules(DBUS REQUIRED IMPORTED_TARGET dbus-1)

add_executable(rtkit-test
    rtkit.c
    rtkit-test.c
)

target_link_libraries(rtkit-test PkgConfig::DBUS)
