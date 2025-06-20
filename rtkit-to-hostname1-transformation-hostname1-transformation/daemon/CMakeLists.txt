add_executable(hostnamed
    main.cpp
)

target_link_libraries(hostnamed PUBLIC HostnamedPrivate)

install(TARGETS hostnamed
        RUNTIME DESTINATION libexec
)