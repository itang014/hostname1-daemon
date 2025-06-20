string(TOLOWER ${CMAKE_SYSTEM_NAME} _system_name)

set(DEFAULT_PLATFORM "${_system_name}" CACHE STRING "Implementations of platform-specific functions to be used by default")
set(FALLBACK_PLATFORM "unix" CACHE STRING "Implementations of platform-specific functions to be used in a fallback case")

function(setup_platform platform)
    if(${platform}_INCLUDED)
        return()
    endif()
    if(EXISTS ${CMAKE_SOURCE_DIR}/cmake/${platform}.cmake)
        include(${CMAKE_SOURCE_DIR}/cmake/${platform}.cmake)
        set(${platform}_INCLUDED TRUE PARENT_SCOPE)
    endif()
endfunction()

set(PLATFORM_LIBRARIES "" CACHE INTERNAL "Dependencies for platform-specific code" FORCE)

setup_platform(${DEFAULT_PLATFORM})
setup_platform(${FALLBACK_PLATFORM})

function(platform_target_sources target scope)
    foreach(src_with_ext ${ARGN})
        get_filename_component(src ${src_with_ext} NAME_WLE)
        string(TOUPPER ${src} SRC)
        if (DEFINED ${SRC}_PLATFORM)
            setup_platform(${${SRC}_PLATFORM})
            list(APPEND platform_srcs "${CMAKE_SOURCE_DIR}/lib/platform/${${SRC}_PLATFORM}/${src_with_ext}")
        elseif (EXISTS "${CMAKE_SOURCE_DIR}/lib/platform/${DEFAULT_PLATFORM}/${src_with_ext}")
            list(APPEND platform_srcs "${CMAKE_SOURCE_DIR}/lib/platform/${DEFAULT_PLATFORM}/${src_with_ext}")
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/lib/platform/${FALLBACK_PLATFORM}/${src_with_ext}")
            list(APPEND platform_srcs  "${CMAKE_SOURCE_DIR}/lib/platform/${FALLBACK_PLATFORM}/${src_with_ext}")
        else()
            message(FATAL_ERROR "No platform-specific implementation for ${src_with_ext}. \
                    Implement it for '${DEFAULT_PLATFORM}' or '${FALLBACK_PLATFORM}', or \
                    override with ${SRC}_PLATFORM")
        endif()
    endforeach()

    target_sources(${target}
        ${scope}
            ${platform_srcs}
    )
endfunction()
