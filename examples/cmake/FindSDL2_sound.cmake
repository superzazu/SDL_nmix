# FindSDL2_mixer.cmake
# Tested working on:
# - macOS 10.14
# - debian buster
# - Windows 10: with vspkg packages
# Documentation:
# https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/How-To-Find-Libraries
# https://stackoverflow.com/a/19302766

find_path(SDL2_SOUND_INCLUDE_DIR NAMES SDL_sound.h PATH_SUFFIXES SDL2)
find_library(SDL2_SOUND_LIBRARY NAMES SDL2_sound libsdl2-sound)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2_SOUND DEFAULT_MSG
                                  SDL2_SOUND_LIBRARY SDL2_SOUND_INCLUDE_DIR)

mark_as_advanced(SDL2_SOUND_INCLUDE_DIR SDL2_SOUND_LIBRARY)

set(SDL2_SOUND_LIBRARIES ${SDL2_SOUND_LIBRARY})
set(SDL2_SOUND_INCLUDE_DIRS ${SDL2_SOUND_INCLUDE_DIR})
