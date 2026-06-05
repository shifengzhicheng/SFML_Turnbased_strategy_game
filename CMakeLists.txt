cmake_minimum_required(VERSION 3.20)

project(SFML_Turnbased_Strategy_Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build SFML from the git submodule so macOS, Linux, and Windows use the same dependency path.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SFML/CMakeLists.txt")
    message(FATAL_ERROR "SFML submodule is missing. Run: git submodule update --init --recursive")
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build dependencies as static libraries" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "The game does not use SFML Audio" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "The game does not use SFML Network" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "Do not build SFML examples" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "Do not build SFML docs" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "Do not build SFML tests" FORCE)
set(SFML_INSTALL_PKGCONFIG_FILES OFF CACHE BOOL "Do not install SFML pkg-config files" FORCE)
add_subdirectory(third_party/SFML EXCLUDE_FROM_ALL)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    foreach(sfml_target sfml-system sfml-window sfml-graphics)
        if(TARGET ${sfml_target})
            target_compile_options(${sfml_target} PRIVATE -Wno-deprecated-declarations)
        endif()
    endforeach()
endif()

add_executable(sfml_tbs
    src/main.cpp
    src/Game.cpp
    src/AllUnit.cpp
    src/Action.cpp
    src/Astar.cpp
    src/ArtAssets.cpp
    src/Button.cpp
    src/Map.cpp
    src/Tile.cpp
)

target_include_directories(sfml_tbs PRIVATE src)
target_link_libraries(sfml_tbs PRIVATE sfml-graphics sfml-window sfml-system)

if(APPLE)
    target_compile_definitions(sfml_tbs PRIVATE GL_SILENCE_DEPRECATION)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sfml_tbs PRIVATE -Wno-deprecated-declarations)
endif()

# The game loads assets through relative paths such as data/..., so keep a copy
# beside the executable for CLI runs on every platform.
add_custom_command(TARGET sfml_tbs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:sfml_tbs>/data
    COMMENT "Copying game assets"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(sfml_tbs_tests
        tests/astar_tests.cpp
        src/Astar.cpp
    )
    target_include_directories(sfml_tbs_tests PRIVATE src)
    add_test(NAME astar_tests COMMAND sfml_tbs_tests)
endif()
