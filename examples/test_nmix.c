#include <stdio.h>
#include <SDL.h>

#include "../SDL_nmix.h"
#include "../SDL_nmix_file.h"

#define PREDECODED_SOURCE 0

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s filename", argv[0]);
        return 1;
    }
    const char* filename = argv[1];

    if (NMIX_OpenAudio(NMIX_DEFAULT_DEVICE, NMIX_DEFAULT_FREQUENCY, NMIX_DEFAULT_SAMPLES) != 0) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
    }
    Sound_Init();

    SDL_RWops* f = SDL_RWFromFile(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: cannot open file %s\n", filename);
        return 1;
    }

    const char* ext = SDL_strrchr(filename, '.');
    if (ext != NULL) {
        ext += 1; // to remove the '.'
    }

    NMIX_FileSource* source1 = NMIX_NewFileSource(f, ext,
        PREDECODED_SOURCE);
    if (source1 == NULL) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
        return 1;
    }
    NMIX_SetLoop(source1, 1);
    NMIX_Play(source1->source);

    SDL_Delay(30 * 1000);

    NMIX_FreeFileSource(source1);
    NMIX_CloseAudio();
    Sound_Quit();
    SDL_Quit();

    return 0;
}
