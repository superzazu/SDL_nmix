cmake_minimum_required(VERSION 3.18)
include(FetchContent)

project(SDL_nmix
  LANGUAGES C
  VERSION 2.0.0
  DESCRIPTION "A lightweight stereo audio mixer for the SDL3"
  HOMEPAGE_URL "https://github.com/superzazu/SDL_nmix"
)

FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  EXCLUDE_FROM_ALL
  FIND_PACKAGE_ARGS
)
FetchContent_Declare(
  SDL3_sound
  GIT_REPOSITORY https://github.com/superzazu/SDL_sound.git
  GIT_TAG SDL3
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  EXCLUDE_FROM_ALL
  # FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(SDL3 SDL3_sound)

add_library(SDL_nmix STATIC SDL_nmix.c SDL_nmix_file.c)
target_link_libraries(SDL_nmix PUBLIC SDL3::SDL3 SDL3_sound::SDL3_sound-static)
set_target_properties(SDL_nmix PROPERTIES C_STANDARD 99)

if(MSVC)
  target_compile_options(SDL_nmix PRIVATE /W4)
else()
  target_compile_options(SDL_nmix PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(NMIX_BUILD_EXAMPLES "Build SDL_nmix example programs" ON)
# option(NMIX_BUILD_SDLMIXER_TEST "Build SDL_mixer test program" ON)

message(STATUS "Build SDL_nmix example programs: ${NMIX_BUILD_EXAMPLES}")
if(NMIX_BUILD_EXAMPLES)
  add_executable(1_basic ./examples/1_basic.c)
  target_link_libraries(1_basic SDL_nmix)

  add_executable(2_file ./examples/2_file.c)
  target_link_libraries(2_file SDL_nmix)

  add_executable(3_stresstest ./examples/3_stresstest.c)
  target_link_libraries(3_stresstest SDL_nmix)

  add_executable(4_demo ./examples/4_demo.c)
  target_link_libraries(4_demo SDL_nmix)

  add_executable(5_stresstest_decoding ./examples/5_stresstest_decoding.c)
  target_link_libraries(5_stresstest_decoding SDL_nmix)

  # performance tests
  add_executable(test_nmix ./examples/test_nmix.c ${SDL_NMIX_SRCS})
  target_link_libraries(test_nmix SDL_nmix)

  # if(NMIX_BUILD_SDLMIXER_TEST)
  #   add_executable(test_sdlmixer ./examples/test_sdlmixer.c)
  #   target_link_libraries(test_sdlmixer SDL3::SDL3 SDL3_mixer::SDL3_mixer)
  # endif()
endif()
