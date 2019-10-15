# this file is just a helper to build examples and performance tests; if you
# want to use the library just drop the SDL_nmix.* files into your project.

cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(SDL_nmix C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_sound REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
include_directories(${SDL2_SOUND_INCLUDE_DIRS})
include_directories(deps/SDL_sound/src)

include_directories(..)
set(SDL_NMIX_SRCS ../SDL_nmix.c ../SDL_nmix_file.c)

# examples
add_executable(1_basic 1_basic.c ${SDL_NMIX_SRCS})
target_link_libraries(1_basic ${SDL2_LIBRARIES} ${SDL2_SOUND_LIBRARIES})

add_executable(2_file 2_file.c ${SDL_NMIX_SRCS})
target_link_libraries(2_file ${SDL2_LIBRARIES} ${SDL2_SOUND_LIBRARIES})

add_executable(3_stresstest 3_stresstest.c ${SDL_NMIX_SRCS})
target_link_libraries(3_stresstest ${SDL2_LIBRARIES} ${SDL2_SOUND_LIBRARIES})

add_executable(4_demo 4_demo.c ${SDL_NMIX_SRCS})
target_link_libraries(4_demo ${SDL2_LIBRARIES} ${SDL2_SOUND_LIBRARIES})

add_executable(5_stresstest_decoding 5_stresstest_decoding.c ${SDL_NMIX_SRCS})
target_link_libraries(5_stresstest_decoding ${SDL2_LIBRARIES} ${SDL2_SOUND_LIBRARIES})

# performance tests
add_executable(test_nmix test_nmix.c ${SDL_NMIX_SRCS})
target_link_libraries(test_nmix ${SDL2_LIBRARIES} ${SDL2_SOUND_LIBRARIES})

option(BUILD_SDLMIXER_TEST "Build SDL_mixer test program" ON)
message(STATUS "Build SDL_mixer test program: ${BUILD_SDLMIXER_TEST}")
if(BUILD_SDLMIXER_TEST)
    add_executable(test_sdlmixer test_sdlmixer.c)
    target_link_libraries(test_sdlmixer ${SDL2_LIBRARIES} ${SDL2_MIXER_LIBRARIES})
endif()

# link to libm on non-windows builds:
if(NOT WIN32)
    target_link_libraries(1_basic m)
    target_link_libraries(2_file m)
    target_link_libraries(3_stresstest m)
    target_link_libraries(4_demo m)
    target_link_libraries(5_stresstest_decoding m)
    target_link_libraries(test_nmix m)

    if(BUILD_SDLMIXER_TEST)
        target_link_libraries(test_sdlmixer m)
    endif()
endif()
