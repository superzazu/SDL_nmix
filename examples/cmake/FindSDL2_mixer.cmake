# FindSDL2_mixer.cmake
# Tested working on:
# - macOS 10.14: with both homebrew installation and frameworks
# - debian buster
# - Windows 10: with vspkg packages
# Documentation:
# https://gitlab.kitware.com/cmake/community/wikis/doc/tutorials/How-To-Find-Libraries
# https://stackoverflow.com/a/19302766

find_path(SDL2_MIXER_INCLUDE_DIR NAMES SDL_mixer.h PATH_SUFFIXES SDL2)
find_library(SDL2_MIXER_LIBRARY NAMES SDL2_mixer libsdl2-mixer)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL2_MIXER DEFAULT_MSG
                                  SDL2_MIXER_LIBRARY SDL2_MIXER_INCLUDE_DIR)

mark_as_advanced(SDL2_MIXER_INCLUDE_DIR SDL2_MIXER_LIBRARY)

set(SDL2_MIXER_LIBRARIES ${SDL2_MIXER_LIBRARY})
set(SDL2_MIXER_INCLUDE_DIRS ${SDL2_MIXER_INCLUDE_DIR})
