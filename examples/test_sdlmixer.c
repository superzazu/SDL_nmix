#include <stdio.h>
#include <SDL.h>

#include <SDL_mixer.h>

#define PREDECODED_SOURCE 0

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s filename", argv[0]);
        return 1;
    }
    const char* filename = argv[1];

    SDL_Init(SDL_INIT_AUDIO);
    int flags = MIX_INIT_OGG;
    if ((Mix_Init(flags) & flags) != flags) {
        printf("error initialising SDL_mixer: %s\n", Mix_GetError());
        return 1;
    }
    if (Mix_OpenAudioDevice(44100, AUDIO_F32SYS, 2, 4096, NULL, 0) == -1) {
        printf("error initialising SDL_mixer: %s\n", Mix_GetError());
        return 1;
    }
    int channels = 32;
    if (Mix_AllocateChannels(channels) != channels) {
        printf("error allocating %d channels\n", channels);
    }

    SDL_RWops* f = SDL_RWFromFile(filename, "rb");
    if (f == NULL) {
        fprintf(stderr, "Error: cannot open file %s\n", filename);
        return 1;
    }

    #if PREDECODED_SOURCE
    Mix_Chunk* source1 = Mix_LoadWAV_RW(f, 1);
    #else
    Mix_Music* source1 = Mix_LoadMUS_RW(f, 1);
    #endif

    if (source1 == NULL) {
        printf("Error: %s\n", Mix_GetError());
        return 1;
    }

    #if PREDECODED_SOURCE
    Mix_PlayChannel(-1, source1, -1);
    #else
    Mix_PlayMusic(source1, -1);
    #endif

    SDL_Delay(30 * 1000);

    #if PREDECODED_SOURCE
    Mix_FreeChunk(source1);
    #else
    Mix_FreeMusic(source1);
    #endif

    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();

    return 0;
}
