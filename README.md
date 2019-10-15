# SDL_nmix

SDL_nmix is a lightweight audio mixer for the SDL (2.0.7+) that supports playback of both static and streaming sources. The code is written in C99 and available under the zlib license. It was made primarily for game development. Features:

- stereo audio mixer
- only two files to copy to your project, under 800 lines of code
- free and open source under zlib license
- cross-platform: tested on macOS, debian, Windows and web (thanks to emscripten)
- a binding to [SDL_sound](https://hg.icculus.org/icculus/SDL_sound/) is provided, to decode the most usual file formats (ogg/wav/flac/mp3/mod/xm/it/etc), with seamless looping. The files can be either preloaded into memory or streamed.
- automatic audio conversion on the fly
- linear panning + gain setting on each source
- a global gain setting

The library depends on the SDL (2.0.7+) which you can find [here](https://www.libsdl.org/). Just copy SDL_nmix.c/.h in your project, and you're good to go. If you want to use the binding to SDL_sound, copy `SDL_nmix_file.c` and `SDL_nmix_file.h` too.

To build the docs, just run `doxygen doxyfile` in the root folder. The documentation will be generated in `docs/html`.

Note that CMake is only needed to build the example programs: `mkdir build && cd build && cmake .. && make`.

The audio test files `music.ogg`, `sound.aif` and `sound.ogg` (in the folder `examples`) were created by me for debug purposes.
