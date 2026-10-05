cmake_minimum_required(VERSION 3.20)

project(BlackjackAI
    VERSION 1.1
    LANGUAGES CXX
)

option(BLACKJACK_BUILD_TESTS "Build the unit tests" ON)
option(BLACKJACK_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(BLACKJACK_SANITIZE "Build with AddressSanitizer and UBSan (GCC/Clang)" OFF)
option(BLACKJACK_BUILD_GUI "Build the graphical version (downloads raylib)" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler settings
add_library(blackjack_options INTERFACE)
target_compile_features(blackjack_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(blackjack_options INTERFACE /W4 /permissive-)
    if(BLACKJACK_WARNINGS_AS_ERRORS)
        target_compile_options(blackjack_options INTERFACE /WX)
    endif()
else()
    target_compile_options(blackjack_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
    )
    if(BLACKJACK_WARNINGS_AS_ERRORS)
        target_compile_options(blackjack_options INTERFACE -Werror)
    endif()
    if(BLACKJACK_SANITIZE)
        target_compile_options(blackjack_options INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(blackjack_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

# Core library
add_library(blackjack_core STATIC
    src/blackjack/Card.cpp
    src/blackjack/Deck.cpp
    src/blackjack/Hand.cpp
    src/blackjack/BlackjackGame.cpp
    src/agents/RandomAgent.cpp
    src/agents/BasicStrategyAgent.cpp
    src/agents/MonteCarloAgent.cpp
    src/agents/QLearningAgent.cpp
    src/agents/HumanAgent.cpp
    src/ui/ConsoleRenderer.cpp
)

target_include_directories(blackjack_core PUBLIC include)
target_link_libraries(blackjack_core PUBLIC blackjack_options)

# Console app
add_executable(blackjack src/main.cpp)
target_link_libraries(blackjack PRIVATE blackjack_core)

# GUI (optional, downloads raylib)
if(BLACKJACK_BUILD_GUI)
    if(CMAKE_VERSION VERSION_LESS 3.25)
        message(FATAL_ERROR "BLACKJACK_BUILD_GUI needs CMake 3.25 or newer.")
    endif()

    find_package(raylib 5.5 QUIET)

    if(NOT raylib_FOUND)
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
            GIT_SHALLOW TRUE
            SYSTEM
        )
        FetchContent_MakeAvailable(raylib)
    endif()

    add_executable(blackjack_gui
        gui/main.cpp
        gui/CardAtlas.cpp
        gui/TableView.cpp
    )
    target_link_libraries(blackjack_gui PRIVATE blackjack_core raylib)

    if(WIN32)
        # No console window in Release.
        set_target_properties(blackjack_gui PROPERTIES WIN32_EXECUTABLE $<CONFIG:Release>)
        if(MSVC)
            target_link_options(blackjack_gui PRIVATE $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
        endif()
    endif()
endif()

# Tests
if(BLACKJACK_BUILD_TESTS)
    enable_testing()

    foreach(test_name test_hand test_deck test_game test_agents)
        add_executable(${test_name} tests/${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE blackjack_core)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
