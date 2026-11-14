cmake_minimum_required(VERSION 3.20)

project(space_invaders_console
        VERSION 0.1.0
        DESCRIPTION "Console Space Invaders (Win32/POSIX, no STL, 64KB budget)"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            Debug Release RelWithDebInfo MinSizeRel)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "32-bit builds are not supported. Configure with a 64-bit toolchain.")
endif()

option(BUILD_TESTS          "Build unit tests"                                OFF)
option(USE_NCURSES          "Use ncurses on POSIX (else raw termios)"         OFF)
option(USE_MINI_AUDIO       "Use miniaudio single-header for audio (all OS)"  OFF)
option(ENABLE_STRICT_WARNINGS "Enable -Werror / /WX"                          ON)
option(ENABLE_ASAN          "Enable AddressSanitizer (Debug only)"            OFF)
option(ENABLE_STL_CHECK     "Fail build if forbidden STL containers are used" ON)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

foreach(cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    string(TOUPPER ${cfg} CFG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_UPPER} ${CMAKE_BINARY_DIR}/bin)
endforeach()

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus /utf-8)
    add_compile_options($<$<CONFIG:Release>:/O2>)
    add_compile_options($<$<CONFIG:Debug>:/Od /RTC1>)
    add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN
            UNICODE
            _UNICODE
    )
    if(ENABLE_ASAN)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    endif()
    if(ENABLE_STRICT_WARNINGS)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(
            -Wall -Wextra -Wpedantic
            -Wshadow -Wnon-virtual-dtor -Wold-style-cast
            -Wcast-align -Wunused -Woverloaded-virtual
            -Wnull-dereference -Wdouble-promotion
            -Wformat=2 -Wimplicit-fallthrough
    )
    if(ENABLE_STRICT_WARNINGS)
        add_compile_options(-Werror)
    endif()
    if(ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        add_link_options(-fsanitize=address)
    endif()
endif()

# ============================================================================
# Common (platform-agnostic) sources
# ============================================================================
set(COMMON_SOURCES
        src/main.cpp
        #src/core/GameLoop.cpp
)

set(COMMON_HEADERS
        #include/core/GameLoop.h
)

set(PLATFORM_SOURCES   "")
set(PLATFORM_LIBRARIES "")
set(PLATFORM_INCLUDE_DIRS "")

if(WIN32)
    message(STATUS "Configuring for Windows (Win32 backend)")
    list(APPEND PLATFORM_SOURCES
            #src/platform/win32/InputWin32.cpp
            #src/platform/win32/RendererWin32.cpp
            #src/platform/win32/TimerWin32.cpp
            #src/platform/win32/RawMemoryWin32.cpp
            #src/platform/audio/AudioWin32.cpp
    )
    list(APPEND PLATFORM_LIBRARIES winmm)

elseif(APPLE)
    message(STATUS "Configuring for macOS (POSIX backend)")
    list(APPEND PLATFORM_SOURCES
            #src/platform/posix/InputPosix.cpp
            #src/platform/posix/RendererPosix.cpp
            #src/platform/posix/TimerPosix.cpp
            #src/platform/posix/RawMemoryPosix.cpp
            #src/platform/audio/AudioMacOS.cpp
    )
    find_library(CORE_AUDIO_LIB      CoreAudio      REQUIRED)
    find_library(AUDIO_TOOLBOX_LIB   AudioToolbox   REQUIRED)
    find_library(CORE_FOUNDATION_LIB CoreFoundation REQUIRED)
    list(APPEND PLATFORM_LIBRARIES
            ${CORE_AUDIO_LIB} ${AUDIO_TOOLBOX_LIB} ${CORE_FOUNDATION_LIB})

    if(USE_NCURSES)
        find_package(Curses REQUIRED)
        list(APPEND PLATFORM_INCLUDE_DIRS ${CURSES_INCLUDE_DIRS})
        list(APPEND PLATFORM_LIBRARIES    ${CURSES_LIBRARIES})
        add_compile_definitions(USE_NCURSES=1)
    endif()

elseif(UNIX)
    message(STATUS "Configuring for Linux (POSIX backend)")
    list(APPEND PLATFORM_SOURCES
            #src/platform/posix/InputPosix.cpp
            #src/platform/posix/RendererPosix.cpp
            #src/platform/posix/TimerPosix.cpp
            #src/platform/posix/RawMemoryPosix.cpp
            #src/platform/audio/AudioLinux.cpp
    )
    find_package(Threads REQUIRED)
    list(APPEND PLATFORM_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

    if(USE_NCURSES)
        find_package(Curses REQUIRED)
        list(APPEND PLATFORM_INCLUDE_DIRS ${CURSES_INCLUDE_DIRS})
        list(APPEND PLATFORM_LIBRARIES    ${CURSES_LIBRARIES})
        add_compile_definitions(USE_NCURSES=1)
    endif()

    find_package(ALSA QUIET)
    if(ALSA_FOUND)
        message(STATUS "ALSA found — using it for audio backend")
        list(APPEND PLATFORM_INCLUDE_DIRS ${ALSA_INCLUDE_DIRS})
        list(APPEND PLATFORM_LIBRARIES    ${ALSA_LIBRARIES})
        add_compile_definitions(USE_ALSA=1)
    else()
        message(WARNING "ALSA not found. Linux audio will use external `aplay` or be disabled.")
    endif()

else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()


if(USE_MINI_AUDIO)
    message(STATUS "Overriding audio backend with miniaudio (single-header)")
    list(REMOVE_ITEM PLATFORM_SOURCES
            #src/platform/win32/AudioWin32.cpp
    )
    list(APPEND PLATFORM_SOURCES src/platform/audio/AudioMiniaudio.cpp)
    add_compile_definitions(USE_MINI_AUDIO=1)
endif()

add_executable(space_invaders
        ${COMMON_SOURCES}
        ${COMMON_HEADERS}
        ${PLATFORM_SOURCES}
)

target_include_directories(space_invaders PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${PLATFORM_INCLUDE_DIRS}
)

if(PLATFORM_LIBRARIES)
    target_link_libraries(space_invaders PRIVATE ${PLATFORM_LIBRARIES})
endif()

add_custom_command(TARGET space_invaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:space_invaders>/assets
        COMMENT "Copying assets to output directory"
)


if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "")
message(STATUS "Space Invaders Console — configuration summary")
message(STATUS "  Build type            : ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard          : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Platform              : ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler              : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  USE_NCURSES           : ${USE_NCURSES}")
message(STATUS "  USE_MINI_AUDIO        : ${USE_MINI_AUDIO}")
message(STATUS "  ENABLE_STRICT_WARNINGS: ${ENABLE_STRICT_WARNINGS}")
message(STATUS "  ENABLE_ASAN           : ${ENABLE_ASAN}")
message(STATUS "  ENABLE_STL_CHECK      : ${ENABLE_STL_CHECK}")
message(STATUS "  BUILD_TESTS           : ${BUILD_TESTS}")
message(STATUS "")
