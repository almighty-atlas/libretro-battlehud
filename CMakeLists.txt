cmake_minimum_required(VERSION 3.16)

project(libretro_battlehud VERSION 0.1.0 LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_library(pokemon_gambatte_libretro SHARED
    src/proxy_core.c
    src/video_marker.c
    src/memory_view.c
    src/sha1.c
    src/game_profile.c
    src/battle_decoder.c
    src/training_stats.c
    src/move_effectiveness.c
    src/type_hud.c
)

target_include_directories(pokemon_gambatte_libretro
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pokemon_gambatte_libretro
    PRIVATE
        ${CMAKE_DL_LIBS}
)

target_compile_options(pokemon_gambatte_libretro
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

set_target_properties(pokemon_gambatte_libretro PROPERTIES
    PREFIX ""
    OUTPUT_NAME "pokemon_gambatte_libretro"
)

if(BUILD_TESTING)
    add_executable(training_stats_test tests/training_stats_test.c src/training_stats.c src/game_profile.c)
    target_include_directories(training_stats_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(training_stats_test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME training_stats COMMAND training_stats_test)
    set_tests_properties(training_stats PROPERTIES TIMEOUT 10)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME nextui_package COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/nextui_package_test.py)
    add_executable(type_hud_test tests/type_hud_test.c src/type_hud.c src/battle_decoder.c src/move_effectiveness.c src/training_stats.c)
    target_include_directories(type_hud_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_options(type_hud_test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME type_hud COMMAND type_hud_test)
    add_executable(battle_decoder_test tests/battle_decoder_test.c src/battle_decoder.c src/game_profile.c src/move_effectiveness.c src/training_stats.c)
    target_include_directories(battle_decoder_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(battle_decoder_test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME battle_decoder COMMAND battle_decoder_test)
    add_executable(sha1_test tests/sha1_test.c src/sha1.c)
    target_include_directories(sha1_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(sha1_test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME sha1 COMMAND sha1_test)
    add_executable(memory_view_test tests/memory_view_test.c src/memory_view.c)
    target_include_directories(memory_view_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(memory_view_test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME memory_view COMMAND memory_view_test)
    add_executable(video_marker_test tests/video_marker_test.c src/video_marker.c)
    target_include_directories(video_marker_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_compile_options(video_marker_test PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME video_marker COMMAND video_marker_test)
    add_library(fake_backend SHARED tests/fake_backend.c)
    target_include_directories(fake_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(fake_backend PROPERTIES
        PREFIX "" OUTPUT_NAME "gambatte_real_libretro")

    add_library(fake_bad_api SHARED tests/fake_backend.c)
    target_include_directories(fake_bad_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(fake_bad_api PRIVATE FAKE_BAD_API)
    set_target_properties(fake_bad_api PROPERTIES PREFIX "")

    add_library(fake_incomplete SHARED tests/fake_backend.c)
    target_include_directories(fake_incomplete PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(fake_incomplete PRIVATE FAKE_MISSING_RUN)
    set_target_properties(fake_incomplete PROPERTIES PREFIX "")

    add_executable(proxy_smoke_test tests/proxy_smoke_test.c)
    target_include_directories(proxy_smoke_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(proxy_smoke_test PRIVATE ${CMAKE_DL_LIBS})

    add_test(
        NAME proxy_smoke
        COMMAND proxy_smoke_test
            $<TARGET_FILE:pokemon_gambatte_libretro>
            $<TARGET_FILE:fake_backend>
    )
    foreach(mode adjacent recovery marker)
        add_test(NAME proxy_${mode} COMMAND proxy_smoke_test
            $<TARGET_FILE:pokemon_gambatte_libretro> $<TARGET_FILE:fake_backend> ${mode})
    endforeach()
    add_test(NAME proxy_missing_backend COMMAND proxy_smoke_test
        $<TARGET_FILE:pokemon_gambatte_libretro> /nonexistent/battlehud-backend.so failure)
    add_test(NAME proxy_self_backend COMMAND proxy_smoke_test
        $<TARGET_FILE:pokemon_gambatte_libretro> $<TARGET_FILE:pokemon_gambatte_libretro> failure)
    add_test(NAME proxy_bad_api COMMAND proxy_smoke_test
        $<TARGET_FILE:pokemon_gambatte_libretro> $<TARGET_FILE:fake_bad_api> failure)
    add_test(NAME proxy_missing_symbol COMMAND proxy_smoke_test
        $<TARGET_FILE:pokemon_gambatte_libretro> $<TARGET_FILE:fake_incomplete> failure)
    set_tests_properties(nextui_package type_hud sha1 battle_decoder memory_view video_marker proxy_marker proxy_smoke proxy_adjacent proxy_recovery proxy_missing_backend
        proxy_self_backend proxy_bad_api proxy_missing_symbol PROPERTIES TIMEOUT 10)
endif()
