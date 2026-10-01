cmake_minimum_required(VERSION 3.16)

project(libretro_battlehud VERSION 0.1.0 LANGUAGES C)

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

add_library(pokemon_gambatte_libretro SHARED
    src/proxy_core.c
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
    add_library(fake_backend SHARED tests/fake_backend.c)
    target_include_directories(fake_backend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set_target_properties(fake_backend PROPERTIES PREFIX "")

    add_executable(proxy_smoke_test tests/proxy_smoke_test.c)
    target_include_directories(proxy_smoke_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(proxy_smoke_test PRIVATE ${CMAKE_DL_LIBS})

    add_test(
        NAME proxy_smoke
        COMMAND proxy_smoke_test
            $<TARGET_FILE:pokemon_gambatte_libretro>
            $<TARGET_FILE:fake_backend>
    )
endif()
