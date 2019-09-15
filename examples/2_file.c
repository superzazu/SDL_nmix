// 2_file.c: demonstrates how to use SDL_nmix with SDL_sound to play
//           a given sound file

#include <stdio.h>
#include <SDL.h>

#include "../SDL_nmix.h"
#include "../SDL_nmix_file.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s sound_file\n", argv[0]);
        return 1;
    }

    if (NMIX_OpenAudio(NMIX_DEFAULT_DEVICE, NMIX_DEFAULT_FREQUENCY, NMIX_DEFAULT_SAMPLES) != 0) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
    }

    Sound_Init();

    SDL_RWops* f = SDL_RWFromFile(argv[1], "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: cannot open file %s\n", argv[1]);
        return 1;
    }

    const char* ext = SDL_strrchr(argv[1], '.');
    if (ext != NULL) {
        ext += 1; // to remove the '.'
    }

    NMIX_FileSource* source1 = NMIX_NewFileSource(f, ext, 0);
    if (source1 == NULL) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
        return 1;
    }
    NMIX_Play(source1->source);

    while (NMIX_IsPlaying(source1->source)) {
        SDL_Delay(1000);
    }

    NMIX_FreeFileSource(source1);
    NMIX_CloseAudio();
    Sound_Quit();
    SDL_Quit();

    return 0;
}
