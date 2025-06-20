cmake_minimum_required(VERSION 3.16)

project(freebsd-hostnamed)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)

find_package(ECM)
if (ECM_FOUND)
  include(${ECM_MODULE_DIR}/ECMEnableSanitizers.cmake)
endif()

find_package(Qt6Core REQUIRED)
find_package(Qt6DBus REQUIRED)
find_package(PolkitQt6-1 REQUIRED)

configure_file(
    data/org.freedesktop.hostname1.service.in
    data/org.freedesktop.hostname1.service
    @ONLY
)

configure_file(
    data/org.freedesktop.hostname1.conf.in
    data/org.freedesktop.hostname1.conf
    @ONLY
)

add_subdirectory(daemon)
add_subdirectory(lib)

install(FILES "${CMAKE_BINARY_DIR}/data/org.freedesktop.hostname1.conf"
    DESTINATION "share/dbus-1/system.d")
install(FILES "data/org.freedesktop.hostname1.policy"
    DESTINATION "share/polkit-1/actions")
install(FILES "${CMAKE_BINARY_DIR}/data/org.freedesktop.hostname1.service"
    DESTINATION "share/dbus-1/system-services")