add_library(RTKitPrivate)

qt_add_dbus_adaptor(ADAPTOR_SRCS
    ${CMAKE_SOURCE_DIR}/data/org.freedesktop.RealtimeKit1.xml
    Daemon.h
    Daemon
    RealtimeKit1Adaptor
    RealtimeKit1Adaptor
)

target_sources(RTKitPrivate
    PRIVATE
        ${ADAPTOR_SRCS}
        AuthQueue.cpp
        Daemon.cpp
        DBusSavedContext.cpp
        Process.cpp
)

platform_target_sources(RTKitPrivate
    PRIVATE
        OSDep.cpp
)

target_include_directories(RTKitPrivate
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(RTKitPrivate
    PUBLIC
        Qt6::Core
        Qt6::DBus
        PolkitQt6-1::Core
        ${PLATFORM_LIBRARIES}
)
