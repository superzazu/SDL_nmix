// 4_demo.c: a demo program that shows all SDL_nmix features: press 's' to
// rewind music, 'd' to play a sound and 'f' to toggle sine wave; 'q' to exit

#include <stdio.h>
#include <SDL.h>

#include "../SDL_nmix.h"
#include "../SDL_nmix_file.h"

static void sine_callback(void* userdata, void* _stream, int len) {
    float* stream = (float*) _stream;
    double* x = (double*) userdata;

    float freq = 261.63; // C4
    for (size_t i = 0; i < len / sizeof(float); i++) {
        *x += (double) 1 / 44100;
        stream[i] = sinf(freq * (*x) * 2 * M_PI);
    }
}

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    if (NMIX_OpenAudio(NMIX_DEFAULT_DEVICE, NMIX_DEFAULT_FREQUENCY, NMIX_DEFAULT_SAMPLES) != 0) {
        fprintf(stderr, "NMIX Error: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    Sound_Init();

    NMIX_SetMasterGain(.7);

    // setup first source: streamed music
    SDL_RWops* f1 = SDL_RWFromFile("../music.ogg", "rb");
    NMIX_FileSource* source1 = NMIX_NewFileSource(f1, "ogg", 0);
    if (source1 == NULL) {
        fprintf(stderr, "Cannot open file: %s\n", SDL_GetError());
        return 1;
    }
    NMIX_SetLoop(source1, 1);
    NMIX_Play(source1->source);

    // setup second source: predecoded sound
    SDL_RWops* f2 = SDL_RWFromFile("../sound.ogg", "rb");
    NMIX_FileSource* source2 = NMIX_NewFileSource(f2, "ogg", 1);
    if (source2 == NULL) {
        fprintf(stderr, "Cannot open file: %s\n", SDL_GetError());
        return 1;
    }
    NMIX_SetGain(source2->source, .6);
    NMIX_SetPan(source2->source, -.2);

    // setup third source: sine wave
    double x = 0;
    NMIX_Source* source3 = NMIX_NewSource(
        AUDIO_F32SYS, 1, 44100, sine_callback, &x);
    if (source3 == NULL) {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        return 1;
    }
    NMIX_SetPan(source3, .2);
    NMIX_SetGain(source3, .1);

    // main loop
    int running = 1;
    SDL_Window* window = SDL_CreateWindow("SDL_nmix demo", 0, 0, 640, 480, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Log("press 's' to rewind music, 'd' to play a sound"
            " and 'f' to toggle sine wave. 'q' to exit");

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;
                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym) {
                        case SDLK_s:
                            NMIX_Rewind(source1);
                            break;
                        case SDLK_d:
                            NMIX_Rewind(source2);
                            NMIX_Play(source2->source);
                            break;
                        case SDLK_f:
                            if (NMIX_IsPlaying(source3)) {
                                NMIX_Pause(source3);
                            }
                            else {
                                NMIX_Play(source3);
                            }
                            break;
                        case SDLK_ESCAPE:
                        case SDLK_c:
                        case SDLK_q:
                            running = 0;
                            break;
                    }
                    break;
            }
        }

        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    if (source1) NMIX_FreeFileSource(source1);
    if (source2) NMIX_FreeFileSource(source2);
    if (source3) NMIX_FreeSource(source3);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    NMIX_CloseAudio();
    Sound_Quit();
    SDL_Quit();

    return 0;
}
