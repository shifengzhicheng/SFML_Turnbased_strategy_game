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
            target_compile_options(${sfml_target} PRIVATE -Wno-deprecated-declarations -Wno-incompatible-pointer-types)
        endif()
    endforeach()
endif()

set(SFML_TBS_ART_SOURCES
    src/ArtAssets.cpp
    src/ArtInternal.cpp
)

set(SFML_TBS_GAME_SOURCES
    src/Game.cpp
    src/GameBalance.cpp
    src/GameBuildingPlacement.cpp
    src/GameCombat.cpp
    src/GameEconomy.cpp
    src/GameInput.cpp
    src/GameOperations.cpp
    src/GameMapState.cpp
    src/GamePathing.cpp
    src/GameRewards.cpp
    src/GameSelection.cpp
    src/GameUnitMastery.cpp
    src/GameLifecycle.cpp
    src/GameEffectsFacade.cpp
    src/GameHudRender.cpp
    src/GameOverlayRender.cpp
    src/GameProduction.cpp
    src/GameRender.cpp
    src/GameWorldRender.cpp
    src/GameUpdate.cpp
    src/GameWorkers.cpp
    src/GameSpatial.cpp
    src/Action.cpp
    src/AIController.cpp
    src/Astar.cpp
    src/AutoCombat.cpp
    src/Building.cpp
    src/BuildingDefinition.cpp
    src/Button.cpp
    src/Effects.cpp
    src/Map.cpp
    src/MoveableUnit.cpp
    src/PathfindingService.cpp
    src/PerkMechanics.cpp
    src/PolicyModel.cpp
    src/Tile.cpp
    src/UnitDefinition.cpp
    src/UnitFactory.cpp
    src/UnitStatsResolver.cpp
    src/UnitTypes.cpp
    src/UnitUpgradeDefinition.cpp
    src/UnitVisual.cpp
    src/Worker.cpp
)

add_library(sfml_tbs_art STATIC
    ${SFML_TBS_ART_SOURCES}
)
target_include_directories(sfml_tbs_art PUBLIC src)
target_link_libraries(sfml_tbs_art PUBLIC sfml-graphics sfml-window sfml-system)

add_library(sfml_tbs_game STATIC
    ${SFML_TBS_GAME_SOURCES}
)
target_include_directories(sfml_tbs_game PUBLIC src)
target_link_libraries(sfml_tbs_game PUBLIC sfml_tbs_art sfml-graphics sfml-window sfml-system)

if(APPLE)
    target_compile_definitions(sfml_tbs_art PUBLIC GL_SILENCE_DEPRECATION)
    target_compile_definitions(sfml_tbs_game PUBLIC GL_SILENCE_DEPRECATION)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sfml_tbs_art PRIVATE -Wno-deprecated-declarations)
    target_compile_options(sfml_tbs_game PRIVATE -Wno-deprecated-declarations)
endif()

add_executable(sfml_tbs
    src/main.cpp
    src/PolicyTrainer.cpp
    src/ScriptedPlan.cpp
    src/SimulationRunner.cpp
)
target_link_libraries(sfml_tbs PRIVATE sfml_tbs_game)

add_executable(sfml_tbs_art_preview EXCLUDE_FROM_ALL
    tools/art_preview.cpp
)
target_link_libraries(sfml_tbs_art_preview PRIVATE sfml_tbs_game)

add_executable(sfml_tbs_sidebar_preview EXCLUDE_FROM_ALL
    tools/sidebar_preview.cpp
)
target_link_libraries(sfml_tbs_sidebar_preview PRIVATE sfml_tbs_game)

add_executable(sfml_tbs_map_preview EXCLUDE_FROM_ALL
    tools/map_preview.cpp
)
target_link_libraries(sfml_tbs_map_preview PRIVATE sfml_tbs_game)

add_executable(sfml_tbs_balance_lab EXCLUDE_FROM_ALL
    tools/balance_lab.cpp
)
target_link_libraries(sfml_tbs_balance_lab PRIVATE sfml_tbs_game)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(sfml_tbs PRIVATE -Wno-deprecated-declarations)
    target_compile_options(sfml_tbs_art_preview PRIVATE -Wno-deprecated-declarations)
    target_compile_options(sfml_tbs_sidebar_preview PRIVATE -Wno-deprecated-declarations)
    target_compile_options(sfml_tbs_map_preview PRIVATE -Wno-deprecated-declarations)
    target_compile_options(sfml_tbs_balance_lab PRIVATE -Wno-deprecated-declarations)
endif()

# The game loads assets through relative paths such as data/..., so keep a copy
# beside the executable for CLI runs on every platform. This target runs even
# when the executable is already up to date.
add_custom_target(copy_sfml_tbs_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data
            $<TARGET_FILE_DIR:sfml_tbs>/data
    DEPENDS sfml_tbs
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

    add_executable(sfml_tbs_map_tests
        tests/map_tests.cpp
        src/Map.cpp
    )
    target_include_directories(sfml_tbs_map_tests PRIVATE src)
    add_test(NAME map_tests COMMAND sfml_tbs_map_tests)

    add_executable(sfml_tbs_realtime_config_tests
        tests/realtime_config_tests.cpp
    )
    target_include_directories(sfml_tbs_realtime_config_tests PRIVATE src)
    add_test(NAME realtime_config_tests COMMAND sfml_tbs_realtime_config_tests)

    add_executable(sfml_tbs_pathfinding_service_tests
        tests/pathfinding_service_tests.cpp
        src/Astar.cpp
        src/PathfindingService.cpp
    )
    target_include_directories(sfml_tbs_pathfinding_service_tests PRIVATE src)
    add_test(NAME pathfinding_service_tests COMMAND sfml_tbs_pathfinding_service_tests)

    add_executable(sfml_tbs_game_operation_tests
        tests/game_operation_tests.cpp
    )
    target_link_libraries(sfml_tbs_game_operation_tests PRIVATE sfml_tbs_game)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(sfml_tbs_game_operation_tests PRIVATE -Wno-deprecated-declarations)
    endif()
    add_test(NAME game_operation_tests COMMAND sfml_tbs_game_operation_tests)

    add_executable(sfml_tbs_unit_definition_tests
        tests/unit_definition_tests.cpp
    )
    target_link_libraries(sfml_tbs_unit_definition_tests PRIVATE sfml_tbs_game)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(sfml_tbs_unit_definition_tests PRIVATE -Wno-deprecated-declarations)
    endif()
    add_test(NAME unit_definition_tests COMMAND sfml_tbs_unit_definition_tests)
endif()
